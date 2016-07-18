/*
 *  Hibernate.c
 *
 *  Created by dmazar, 01.2014.
 *
 *  Hibernate support.
 *
 */


#include "macosx.h"
#include <Protocol/BlockIo.h>

EFI_DEVICE_PATH_PROTOCOL  *mDiskDevicePath = NULL;
EFI_BLOCK_READ            OrigBlockIoRead = NULL;
UINT64                    gSleepImageOffset = 0;
UINT32                    gSleepTime = 0;


#pragma pack(push, 1)
//
// Just the first part of HFS+ volume header from where we can take modification time
//
typedef struct _HFSPlusVolumeHeaderMin {
  UINT16  signature;
  UINT16  version;
  UINT32  attributes;
  UINT32  lastMountedVersion;
  UINT32  journalInfoBlock;
  
  UINT32  createDate;
  UINT32  modifyDate;
  UINT32  backupDate;
  UINT32  checkedDate;
  
  UINT32  fileCount;
  UINT32  folderCount;
  
  UINT32  blockSize;
  UINT32  totalBlocks;
  UINT32  freeBlocks;
} HFSPlusVolumeHeaderMin;

// IOHibernateImageHeader.signature
enum
{
    kIOHibernateHeaderSignature        = 0x73696d65,
    kIOHibernateHeaderInvalidSignature = 0x7a7a7a7a
};

typedef struct _IOHibernateImageHeaderMin
{
  UINT64	imageSize;
  UINT64	image1Size;
  
  UINT32	restore1CodePhysPage;
  UINT32  reserved1;
  UINT64	restore1CodeVirt;
  UINT32	restore1PageCount;
  UINT32	restore1CodeOffset;
  UINT32	restore1StackOffset;
  
  UINT32	pageCount;
  UINT32	bitmapSize;
  
  UINT32	restore1Sum;
  UINT32	image1Sum;
  UINT32	image2Sum;
  
  UINT32	actualRestore1Sum;
  UINT32	actualImage1Sum;
  UINT32	actualImage2Sum;
  
  UINT32	actualUncompressedPages;
  UINT32	conflictCount;
  UINT32	nextFree;
  
  UINT32	signature;
  UINT32	processorFlags;
  UINT32  runtimePages;
  UINT32  runtimePageCount;
  UINT64  runtimeVirtualPages;

  UINT32  performanceDataStart;
  UINT32  performanceDataSize;

  UINT64	encryptStart;
  UINT64	machineSignature;

  UINT32  previewSize;
  UINT32  previewPageListSize;

  UINT32	diag[4];

  UINT32  handoffPages;
  UINT32  handoffPageCount;

  UINT32  systemTableOffset;

  UINT32	debugFlags;
  UINT32	options;
  UINT32	sleepTime;
  UINT32  compression;

} IOHibernateImageHeaderMin;

typedef struct _IOHibernateImageHeaderMinSnow
{
  UINT64	imageSize;
  UINT64	image1Size;

  UINT32	restore1CodePhysPage;
  UINT32	restore1PageCount;
  UINT32	restore1CodeOffset;
  UINT32	restore1StackOffset;

  UINT32	pageCount;
  UINT32	bitmapSize;

  UINT32	restore1Sum;
  UINT32	image1Sum;
  UINT32	image2Sum;

  UINT32	actualRestore1Sum;
  UINT32	actualImage1Sum;
  UINT32	actualImage2Sum;

  UINT32	actualUncompressedPages;
  UINT32	conflictCount;
  UINT32	nextFree;

  UINT32	signature;
  UINT32	processorFlags;
} IOHibernateImageHeaderMinSnow;

typedef struct _AppleRTCHibernateVars
{
  UINT8   signature[4];
  UINT32  revision;
  UINT8	  booterSignature[20];
  UINT8	  wiredCryptKey[16];
} AppleRTCHibernateVars;

#pragma pack(pop)

//
// Taken from VBoxFsDxe
//

//static fsw_u32 mac_to_posix(fsw_u32 mac_time)
INT32 mac_to_posix(UINT32 mac_time)
{
  /* Mac time is 1904 year based */
  return mac_time ?  mac_time - 2082844800 : 0;
}

/** BlockIo->Read() override. */
EFI_STATUS
EFIAPI OurBlockIoRead (
  IN EFI_BLOCK_IO_PROTOCOL          *This,
  IN UINT32                         MediaId,
  IN EFI_LBA                        Lba,
  IN UINTN                          BufferSize,
  OUT VOID                          *Buffer
  )
{
  EFI_STATUS          Status;
  Status = OrigBlockIoRead (This, MediaId, Lba, BufferSize, Buffer);
  
  // Enter special processing only when gSleepImageOffset == 0, to avoid recursion when Boot/Log=true
  if (gSleepImageOffset == 0 && Status == EFI_SUCCESS && BufferSize >= sizeof (IOHibernateImageHeaderMin)) {

    IOHibernateImageHeaderMin *Header;
    IOHibernateImageHeaderMinSnow *Header2;
    UINT32 BlockSize = 0;
    
    // Mark that we are executing, to avoid entering above phrase again, and don't add DBGs outside this scope, to avoid recursion
    gSleepImageOffset = (UINT64) - 1;
    
    if (This->Media != NULL) {
      BlockSize = This->Media->BlockSize;
    } else {
      BlockSize = 512;
    }
    
    DBG("%a: Lba=%lx, Offset=%lx (BlockSize=%d)\n", __FUNCTION__, Lba, MultU64x32 (Lba, BlockSize), BlockSize);
    
    Header = (IOHibernateImageHeaderMin *) Buffer;
    Header2 = (IOHibernateImageHeaderMinSnow *) Buffer;
    DBG("%a: sig lion: %x\n", __FUNCTION__, Header->signature);
    DBG("%a: sig snow: %x\n", __FUNCTION__, Header2->signature);
    // DBG(" sig swap: %x\n", SwapBytes32(Header->signature));
    
    if (Header->signature == kIOHibernateHeaderSignature ||
        Header2->signature == kIOHibernateHeaderSignature) {
      gSleepImageOffset = MultU64x32 (Lba, BlockSize);
      DBG("%a: got sleep image offset\n", __FUNCTION__);
      //save sleep time as lvs1974 suggested
      if (Header->signature == kIOHibernateHeaderSignature) {
        gSleepTime = Header->sleepTime;
      } else
        gSleepTime = 0;
      // return invalid parameter in case of success in order to prevent driver from caching our buffer
      return EFI_INVALID_PARAMETER;
    } else {
      DBG("%a: no valid sleep image offset was found\n", __FUNCTION__);
      gSleepImageOffset = 0;
    }
  }
  
  return Status;
}

/** Returns byte offset of sleepimage on the whole disk or 0 if not found or error.
 *
 * To avoid messing with HFS+ format, we'll use the trick with overriding
 * BlockIo->Read() of the disk and then read first bytes of the sleepimage
 * through file system driver. And then we'll detect block delivered by BlockIo
 * and calculate position from there.
 * It's for hack after all :)
 */
UINT64
GetSleepImagePosition (
  IN  EFI_HANDLE                VolHandle,
  IN  EFI_HANDLE                DiskHandle
  )
{
  EFI_STATUS                      Status;
  EFI_FILE                        *File;
  VOID                            *Buffer;
  UINTN                           BufferSize;
  CHAR16                          *ImageName;
  EFI_BLOCK_IO                    *DiskBlockIO;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
  EFI_FILE_HANDLE                 FHandle;

  if (gSleepImageOffset != 0) {
    DBG("%a: returning previously calculated offset: %lx\n", __FUNCTION__, gSleepImageOffset);
    return gSleepImageOffset;
  }

  Status = gBS->HandleProtocol (
                  DiskHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &DiskBlockIO
                );
  Status = gBS->HandleProtocol (
                  VolHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID *) &Volume
                );

  FHandle = NULL;

  if (!EFI_ERROR (Status)) {
    Status = Volume->OpenVolume (
                       Volume,
                       &FHandle
                     );
  }

  // Open sleepimage
  // expect path of sleepimage is default.
  // since 10.12 /Library/Preferences/com.apple.PowerManagement.plist file is a binary format,
  // so is easier to set a path using pmset (from macOS), than read binary plist from the loader
  ImageName = AllocateZeroPool (StrSize (L"\\private\\var\\vm\\sleepimage") + 2);
  StrCpy (ImageName, L"\\private\\var\\vm\\sleepimage");
  Status = FHandle->Open (FHandle, &File, ImageName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) {
    DBG("%a: sleepimage not found -> %r\n", __FUNCTION__, Status);
    return 0;
  }

  // We want to read the first 512 bytes from sleepimage
  BufferSize = 512;
  Buffer = AllocatePool (BufferSize);
  if (Buffer == NULL) {
    DBG("%a: could not allocate buffer for sleepimage\n", __FUNCTION__);
    return 0;
  }

  DBG("%a: reading first %d bytes of sleepimage ...\n", __FUNCTION__, BufferSize);

  OrigBlockIoRead = DiskBlockIO->ReadBlocks;
  DiskBlockIO->ReadBlocks = OurBlockIoRead;
  gSleepImageOffset = 0;
  Status = File->Read (File, &BufferSize, Buffer);
  DiskBlockIO->ReadBlocks = OrigBlockIoRead;
  // OurBlockIoRead always returns invalid parameter in order to avoid driver caching, so that is a good value
  if (Status == EFI_INVALID_PARAMETER) {
    Status = EFI_SUCCESS;
  }
  DBG("%a: reading completed: %r\n", __FUNCTION__, Status);

  // Close sleepimage
  File->Close (File);
  
  // We don't use the buffer, as actual signature checking is being done by OurBlockIoRead
  if (Buffer) {
    FreePool (Buffer);
  }

  if (EFI_ERROR (Status)) {
    DBG("%a: can not read sleepimage: %r\n", __FUNCTION__, Status);
    return 0;
  } 

  return gSleepImageOffset;
}

/** Returns TRUE if /private/var/vm/sleepimage exists
 *  and it's modification time is close to volume modification time).
 */
BOOLEAN
IsSleepImageValidBySleepTime (
  IN  EFI_HANDLE                VolHandle
  )
{
  EFI_STATUS              Status;
  VOID                    *Buffer;
  EFI_BLOCK_IO_PROTOCOL   *BlockIo;
  HFSPlusVolumeHeaderMin  *HFSHeader;
  UINT32                  HFSVolumeModifyDate;
  INTN                    TimeDiff;
  INTN                    Pages; // = 1;

  DBG("%a: gSleepTime: %d\n", __FUNCTION__, gSleepTime);

  //
  // Get HFS+ volume nodification time
  //
  // use 4KB aligned page to avoid possible issues with BlockIo buffer alignment
  Status = gBS->HandleProtocol (
                  VolHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **) &BlockIo
                );
  Pages = EFI_SIZE_TO_PAGES (BlockIo->Media->BlockSize);
  Buffer = AllocatePages (Pages);
  if (Buffer == NULL) {
    return FALSE;
  }

  Status = BlockIo->ReadBlocks (BlockIo, BlockIo->Media->MediaId, 2, BlockIo->Media->BlockSize, Buffer);
  if (EFI_ERROR(Status)) {
    DBG("%a: can not read HFS+ header -> %r\n", __FUNCTION__, Status);
    FreePages(Buffer, Pages);
    return FALSE;
  }

  HFSHeader = (HFSPlusVolumeHeaderMin *) Buffer;
  HFSVolumeModifyDate = SwapBytes32 (HFSHeader->modifyDate);
  HFSVolumeModifyDate = mac_to_posix (HFSVolumeModifyDate);
  DBG("%a: HFS+ volume modifyDate: %d\n", __FUNCTION__, HFSVolumeModifyDate);

  FreePages(Buffer, Pages);

  //
  // Check that sleepimage is not more then 5 secs older then volume modification date
  // Idea is from Chameleon
  //
  TimeDiff = HFSVolumeModifyDate - (INTN) gSleepTime;
  DBG("%a: image older then volume: %d sec\n", __FUNCTION__, TimeDiff);
  if (TimeDiff > 5) {
    DBG("%a: image too old\n", __FUNCTION__);
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
PrepareHibernation (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS                Status;
  CHAR16                    OffsetHexStr[17];
  EFI_DEVICE_PATH_PROTOCOL  *Node, *BootImageDevPath;
  UINTN                     Size;
  VOID                      *Value;
  AppleRTCHibernateVars     RtcVars;
  EFI_HANDLE                DiskHandle;
  EFI_HANDLE                VolHandle;
//  UINT8                     *VarData = NULL;

  Size = 0;
  Node = DuplicateDevicePath (DevicePath);
  mDiskDevicePath =  Node;

  while ((Node->Type != MEDIA_DEVICE_PATH || Node->SubType != MEDIA_HARDDRIVE_DP) &&
          IsDevicePathEnd (Node) != TRUE) {
    Node = NextDevicePathNode (Node);
  }
  if (IsDevicePathEnd (Node) == TRUE) {
    DBG ("%a: no HD node in device path (%s)\n", __FUNCTION__,
          ConvertDevicePathToText (DevicePath, FALSE, FALSE)
        );
  } else {
    SetDevicePathEndNode (Node);
    DBG ("%a: cut off HD node in device path = %s\n", __FUNCTION__,
          ConvertDevicePathToText (mDiskDevicePath, FALSE, FALSE)
        );
  }
  Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &DevicePath, &VolHandle);
  Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &mDiskDevicePath, &DiskHandle);

  gSleepImageOffset = GetSleepImagePosition (VolHandle, DiskHandle);

  if (gSleepImageOffset !=0 &&
      IsDevicePathValid (mDiskDevicePath, 0) &&
      IsSleepImageValidBySleepTime (VolHandle)) {

    UnicodeSPrint(OffsetHexStr, sizeof(OffsetHexStr), L"%lx", gSleepImageOffset);
    BootImageDevPath = FileDevicePath (DiskHandle, OffsetHexStr);
    Size = GetDevicePathSize(BootImageDevPath);
    DBG("%a: boot-image = %s\n", __FUNCTION__, ConvertDevicePathToText (BootImageDevPath, FALSE, FALSE));

  //  VarData = (UINT8*)BootImageDevPath;
  //  PrintBytes(VarData, Size);
  //   VarData[6] = 8;
  //  VarData[24] = 0xFF;
  //  VarData[25] = 0xFF;
  //  DBG(" boot-image corrected = %s\n", ConvertDevicePathToText (BootImageDevPath, FALSE, FALSE));

    if (mDiskDevicePath != NULL) {
      FreePool (mDiskDevicePath);
    }

    Status = gRT->SetVariable (L"boot-image",
                                &gEfiAppleBootGuid,
                                EFI_VARIABLE_NON_VOLATILE |
                                EFI_VARIABLE_BOOTSERVICE_ACCESS |
                                EFI_VARIABLE_RUNTIME_ACCESS,
                                Size,
                                BootImageDevPath
                              );
    if (EFI_ERROR(Status)) {
      DBG("%a: can not write boot-image -> %r\n", __FUNCTION__, Status);
      return FALSE;
    }

    DBG("%a: setting dummy boot-switch-vars\n", __FUNCTION__);

    Size = sizeof(RtcVars);
    Value = &RtcVars;
    SetMem (&RtcVars, Size, 0);

    RtcVars.signature[0] = 'A';
    RtcVars.signature[1] = 'A';
    RtcVars.signature[2] = 'P';
    RtcVars.signature[3] = 'L';
    RtcVars.revision     = 1;

    Status = gRT->SetVariable (L"boot-switch-vars",
                                &gEfiAppleBootGuid,
                                EFI_VARIABLE_NON_VOLATILE |
                                EFI_VARIABLE_BOOTSERVICE_ACCESS |
                                EFI_VARIABLE_RUNTIME_ACCESS,
                                Size,
                                Value
                              );
    if (EFI_ERROR(Status)) {
      DBG("%a: can not write boot-switch-vars -> %r\n", __FUNCTION__, Status);
      return FALSE;
    }

    return TRUE;
  } else {
    return FALSE;
  }
}

