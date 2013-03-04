/** @file
  Decode an Apple formatted partition table

  Copyright (c) 2009-2010, Oracle Corporation
**/


#include "Partition.h"

#define DPISTRLEN       32

#pragma pack(1)

typedef struct APPLE_PT_HEADER {
    UINT16   sbSig; /* must be BE 0x4552 */
    UINT16   sbBlkSize; /* block size of device */
    UINT32   sbBlkCount; /* number of blocks on device */
    UINT16   sbDevType; /* device type */
    UINT16   sbDevId; /* device id */
    UINT32   sbData; /* not used */
    UINT16   sbDrvrCount; /* driver descriptor count */
    UINT16   sbMap[247]; /* descriptor map */
} APPLE_PT_HEADER;

typedef struct APPLE_PT_ENTRY  {
    UINT16       signature          ; /* must be BE 0x504D for new style PT */
    UINT16       reserved_1         ;
    UINT32       map_entries        ; /* how many PT entries are there */
    UINT32       pblock_start       ; /* first physical block */
    UINT32       pblocks            ; /* number of physical blocks */
    char         name[DPISTRLEN]    ; /* name of partition */
    char         type[DPISTRLEN]    ; /* type of partition */
    /* Some more data we don't really need */
} APPLE_PT_ENTRY;

#pragma pack()

/**
  Install child handles if the Handle supports Apple partition table format.

  @param[in]  This        Calling context.
  @param[in]  Handle      Parent Handle
  @param[in]  DiskIo      Parent DiskIo interface
  @param[in]  BlockIo     Parent BlockIo interface
  @param[in]  DevicePath  Parent Device Path


  @retval EFI_SUCCESS         Child handle(s) was added
  @retval EFI_MEDIA_CHANGED   Media changed Detected
  @retval other               no child handle was added

**/
EFI_STATUS
PartitionInstallAppleChildHandles (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DISK_IO_PROTOCOL         *DiskIo,
  IN  EFI_BLOCK_IO_PROTOCOL        *BlockIo,
  IN  EFI_BLOCK_IO2_PROTOCOL       *BlockIo2,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  EFI_STATUS                Status;
  UINT32                    Lba;
  EFI_BLOCK_IO_MEDIA       *Media;
#if 0
  VOID                     *Block;
  UINTN                   MaxIndex;
#endif
  /** @todo: wrong, as this PT can be on both HDD or CD */
  CDROM_DEVICE_PATH         CdDev;
#if 0
  EFI_DEVICE_PATH_PROTOCOL  Dev;
#endif
  UINT32                    Partition;
  UINT32                    PartitionEntries;
  UINT32                    VolSpaceSize;
  UINT32                    SubBlockSize;
  UINT32                    BlkPerSec;
  UINT32                    MediaId;
  UINT32                    BlockSize;
  EFI_LBA                   LastBlock;
  APPLE_PT_HEADER           *AppleHeader;
  APPLE_PT_ENTRY            *AppleEntry;
  UINT32                    StartLba;
  UINT32                    SizeLbs;

  AppleEntry = NULL;

  Media         = BlockIo->Media;
  BlockSize     = BlockIo->Media->BlockSize;
  LastBlock     = BlockIo->Media->LastBlock;
  MediaId       = BlockIo->Media->MediaId;
  VolSpaceSize  = 0;

  AppleHeader = AllocatePool (BlockSize);

  if (AppleHeader == NULL) {
    return EFI_NOT_FOUND;
  }

  do {
      Lba = 0;
      Status = DiskIo->ReadDisk (
                       DiskIo,
                       MediaId,
                       0,
                       BlockSize,
                       AppleHeader
                       );
      if (EFI_ERROR (Status)) {
        goto done;
      }
#if 0
      Header = (APPLE_PT_HEADER *)Block;
#endif

      if (SwapBytes16(AppleHeader->sbSig) != 0x4552) {
        Status = EFI_NOT_FOUND;
        goto done; 
      }
      SubBlockSize = SwapBytes16(AppleHeader->sbBlkSize);
      BlkPerSec    = BlockSize / SubBlockSize;
      if (BlockSize != SubBlockSize * BlkPerSec) {
        Status = EFI_NOT_FOUND;
        goto done; 
      }
      AppleEntry = AllocatePool (SubBlockSize);
      PartitionEntries = 1;
      for (Partition = 1; Partition <= PartitionEntries; Partition++)
      {
          Status = DiskIo->ReadDisk (
                       DiskIo,
                       MediaId,
                       MultU64x32 (Partition, SubBlockSize),
                       SubBlockSize,
                       AppleEntry
                       );

          if (EFI_ERROR (Status)) {
              Status = EFI_NOT_FOUND;
              goto done; /* would break, but ... */
          }

#if 0
          Entry = (APPLE_PT_ENTRY *)Block;
#endif

          if (SwapBytes16(AppleEntry->signature) != 0x504D) continue;
          if (Partition == 1) PartitionEntries  = SwapBytes32(AppleEntry->map_entries);
          StartLba = SwapBytes32(AppleEntry->pblock_start);
          SizeLbs  = SwapBytes32(AppleEntry->pblocks);
          ZeroMem (&CdDev, sizeof (CdDev));
          CdDev.Header.Type     = MEDIA_DEVICE_PATH;
          CdDev.Header.SubType  = MEDIA_CDROM_DP;
          SetDevicePathNodeLength (&CdDev.Header, sizeof (CdDev));
          CdDev.BootEntry = 0;
          CdDev.PartitionStart = StartLba / BlkPerSec;  /* start, LBA */
          CdDev.PartitionSize  = SizeLbs / BlkPerSec;   /* size,  LBs */

          Status = PartitionInstallChildHandle (
              This,
              Handle,
              DiskIo,
              BlockIo,
              BlockIo2,
              DevicePath,
              (EFI_DEVICE_PATH_PROTOCOL *) &CdDev,
              CdDev.PartitionStart,
              CdDev.PartitionStart + CdDev.PartitionSize - 1,
              SubBlockSize,
              FALSE );
      }

  } while (0);

 done:
  
  FreePool (AppleHeader);
  FreePool (AppleEntry);

  return Status;
}
