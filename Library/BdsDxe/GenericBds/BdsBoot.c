/** @file
  BDS Lib functions which relate with create or process the boot option.

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Guid/FileSystemInfo.h>
#include <Guid/LdrMemoryDescriptor.h>

#include "InternalBdsLib.h"
#include "BootMaintLib.h"
#include "Graphics.h"

#if   defined (MDE_CPU_IA32)
#define CLOVER_MEDIA_FILE_NAME L"\\EFI\\CLOVER\\CLOVERIA32.EFI"
#define SHELL_PATH             L"\\EFI\\TOOLS\\shell32.efi"
#elif defined (MDE_CPU_X64)
#define CLOVER_MEDIA_FILE_NAME L"\\EFI\\CLOVER\\CLOVERX64.EFI"
#define SHELL_PATH             L"\\EFI\\TOOLS\\shell64.efi"
#else
#error Unknown Processor Type
#endif

#define MACOSX_LOADER_PATH              L"\\System\\Library\\CoreServices\\boot.efi"
#define MACOSX_RECOVERY_LOADER_PATH     L"\\com.apple.recovery.boot\\boot.efi"
#define MACOSX_INSTALL_PATH             L"\\OS X Install Data\\boot.efi"
#define MACOSX_INSTALL_PATH2            L"\\.IABootFiles\\boot.efi"
#define MACOSX_INSTALL_PATH3            L"\\Mac OS X Install Data\\boot.efi"
#define MACOSX_INSTALL_PATH4            L"\\macOS Install Data\\boot.efi"

BOOLEAN                         WithKexts;
EFI_FILE_HANDLE                 gRootFHandle;
BOOLEAN                         gIsHibernation;

//extern EFI_EVENT OnReadyToBootEvent;
//extern EFI_EVENT ExitBootServiceEvent;

CHAR16* mLoaderPath[] = {
  CLOVER_MEDIA_FILE_NAME,
  EFI_REMOVABLE_MEDIA_FILE_NAME,
  MACOSX_LOADER_PATH,
  MACOSX_RECOVERY_LOADER_PATH,
  MACOSX_INSTALL_PATH,
  MACOSX_INSTALL_PATH2,
  MACOSX_INSTALL_PATH3,
  MACOSX_INSTALL_PATH4,
  L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi",
  L"\\EFI\\SuSE\\elilo.efi",
#if defined (MDE_CPU_X64)
  L"\\EFI\\opensuse\\grubx64.efi",
  L"\\EFI\\Ubuntu\\grubx64.efi",
#endif
  L"\\EFI\\RedHat\\grub.efi",
  SHELL_PATH,
  L"\\EFI\\MS\\Boot\\bootmgfw.efi"
};

#define MAX_LOADER_PATHS  (sizeof (mLoaderPath) / sizeof (CHAR16*))

VOID
DumpEfiMemoryMap (
  VOID
)
{
  EFI_PEI_HOB_POINTERS            GuidHob;
  VOID                            *Table;
  MEMORY_DESC_HOB                 MemoryDescHob;
  UINTN                           Index;

  GuidHob.Raw = GetFirstGuidHob (&gLdrMemoryDescriptorGuid);
  if (GuidHob.Raw == NULL) {
    return;
  }
  Table = GET_GUID_HOB_DATA (GuidHob.Guid);
  if (Table == NULL) {
    return;
  }
  MemoryDescHob.MemDescCount = *(UINTN *)Table;
  MemoryDescHob.MemDesc      = *(EFI_MEMORY_DESCRIPTOR **)((UINTN)Table + sizeof(UINTN));

  DBG ("Index  Type  Physical Start    Physical End      Number of Pages   Virtual Start     Attribute\n");
  for (Index = 0; Index < MemoryDescHob.MemDescCount; Index++) {
    DBG ("%02d     %02d    %016lx  %016lx  %016lx  %016lx  %016x\n",
         Index,
         MemoryDescHob.MemDesc[Index].Type,
         MemoryDescHob.MemDesc[Index].PhysicalStart,
         MemoryDescHob.MemDesc[Index].PhysicalStart + MultU64x32 (MemoryDescHob.MemDesc[Index].NumberOfPages, 4096) - 1,
         MemoryDescHob.MemDesc[Index].NumberOfPages,
         MemoryDescHob.MemDesc[Index].VirtualStart,
         MemoryDescHob.MemDesc[Index].Attribute);
  }
}

/**
  Process the boot option follow the UEFI specification and
  special treat the legacy boot option with BBS_DEVICE_PATH.

  @param  Option                 The boot option need to be processed
  @param  DevicePath             The device path which describe where to load the
                                 boot image or the legacy BBS device path to boot
                                 the legacy OS
  @param  ExitDataSize           The size of exit data.
  @param  ExitData               Data returned when Boot image failed.

  @retval EFI_SUCCESS            Boot from the input boot option successfully.
  @retval EFI_NOT_FOUND          If the Device Path is not found in the system

**/
EFI_STATUS
EFIAPI
BdsLibBootViaBootOption (
  IN  BDS_COMMON_OPTION             *Option,
  IN  CHAR16                        *LoadOptions,
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath,
  OUT UINTN                         *ExitDataSize,
  OUT CHAR16                        **ExitData OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_HANDLE                ImageHandle;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  EFI_LOADED_IMAGE_PROTOCOL *ImageInfo;
  UINT16                    buffer[255];
  UINT8                     Index;
  CHAR16                    *TmpLoadPath;
  EFI_FILE_HANDLE                 FHandle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;

  *ExitDataSize   = 0;
  *ExitData       = NULL;
  FHandle         = NULL;
  gIsHibernation  = FALSE;
  WithKexts       = FALSE;

  BdsSetMemoryTypeInformationVariable ();
  ASSERT (Option->DevicePath != NULL);
  EfiSignalEventReadyToBoot();

  Status = gBS->LoadImage (
                  TRUE,
                  gImageHandle,
                  DevicePath,
                  NULL,
                  0,
                  &ImageHandle
                );
  
  if (EFI_ERROR (Status)) {
    Handle = BdsLibGetBootableHandle(DevicePath);
    if (Handle == NULL) {
      goto Done;
    }
    FilePath = NULL;

    TmpLoadPath = NULL;

    if (StrCmp (LoadOptions, L"All") == 0) {
      for (Index =  0; Index < MAX_LOADER_PATHS; Index++) {
        FilePath = FileDevicePath (Handle, mLoaderPath[Index]);
        if (FilePath != NULL) {
          Status = gBS->LoadImage (
                                   TRUE,
                                   gImageHandle,
                                   FilePath,
                                   NULL,
                                   0,
                                   &ImageHandle
                                   );
          if (!EFI_ERROR (Status)) {
            TmpLoadPath = mLoaderPath[Index];
            break;
          }
        }
      }
    } else {
      FilePath = FileDevicePath (Handle, LoadOptions);
      if (FilePath != NULL) {
        Status = gBS->LoadImage (
                                 TRUE,
                                 gImageHandle,
                                 FilePath,
                                 NULL,
                                 0,
                                 &ImageHandle
                                 );
        if (!EFI_ERROR (Status)) {
          TmpLoadPath = LoadOptions;
        }
      }
    }

    if (TmpLoadPath != NULL) {
      if ((StrCmp (TmpLoadPath, MACOSX_LOADER_PATH) == 0) ||
          (StrCmp (TmpLoadPath, MACOSX_RECOVERY_LOADER_PATH) == 0) ||
          (StrCmp (TmpLoadPath, MACOSX_INSTALL_PATH) == 0) ||
          (StrCmp (TmpLoadPath, MACOSX_INSTALL_PATH2) == 0) ||
          (StrCmp (TmpLoadPath, MACOSX_INSTALL_PATH3) == 0) ||
          (StrCmp (TmpLoadPath, MACOSX_INSTALL_PATH4) == 0)) {
        goto MacOS;
      } else {
        goto Next;
      }
    }
  } else goto Next;
  goto Done;

MacOS:
  if (gFronPage) {
    if (gSettings.YoBlack) {
      ClearScreen (0x030000, NULL);
    } else {
      ClearScreen (0xBFBFBF, NULL);
    }
  }
  DBG ("%a: launching InitializeConsoleSim.\n",__FUNCTION__);
  InitializeConsoleSim (gImageHandle);
  
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID *) &Volume
                );
  if (!EFI_ERROR (Status)) {
    Status = Volume->OpenVolume (
                       Volume,
                       &FHandle
                     );
  }
  DBG ("%a: launching GetOSVersion.\n",__FUNCTION__);
  GetOSVersion (FHandle);
  DBG ("%a: launching GetCpuProps.\n",__FUNCTION__);
  GetCpuProps ();
  DBG ("%a: launching GetUserSettings.\n",__FUNCTION__);
  GetUserSettings ();
  if (gSettings.Hibernate) {
    DBG ("%a: launching PrepareHibernation.\n",__FUNCTION__);
    gIsHibernation = PrepareHibernation (DevicePath);
  }
  if (cDevProp == NULL) {
    DBG ("%a: launching SetDevices (no cDevProp).\n",__FUNCTION__);
    SetDevices ();
  }
  DBG ("%a: launching PatchSmbios.\n",__FUNCTION__);
  PatchSmbios ();
  DBG ("%a: launching PatchACPI.\n",__FUNCTION__);
  PatchACPI (gRootFHandle);
  DBG ("%a: launching SetVariablesForOSX.\n",__FUNCTION__);
  SetVariablesForOSX ();
  DBG ("%a: launching SetPrivateVarProto.\n",__FUNCTION__);
  SetPrivateVarProto ();
  DBG ("%a: launching SetupDataForOSX.\n",__FUNCTION__);
  SetupDataForOSX ();
  DBG ("%a: launching EventsInitialize.\n",__FUNCTION__);
  EventsInitialize ();
  if (gSettings.NvRam) {
    DBG ("%a: launching PutNvramPlistToRtVars.\n",__FUNCTION__);
    PutNvramPlistToRtVars ();
  }
#if 0
  DBG ("gST->Hdr.Signature = 0x%x\n", gST->Hdr.Signature);
  DBG ("gST->Hdr.Revision = 0x%x\n", gST->Hdr.Revision);
  DBG ("gST->Hdr.HeaderSize = 0x%x\n", gST->Hdr.HeaderSize);
  DBG ("gST->FirmwareVendor = %s\n", gST->FirmwareVendor);
  DBG ("gST->FirmwareRevision = 0x%x\n", gST->FirmwareRevision);
  DBG ("gST->ConsoleInHandle = 0x%x size = 0x%x\n", gST->ConsoleInHandle, sizeof(gST->ConsoleInHandle));
  DBG ("gST->ConIn = 0x%x size = 0x%x\n", gST->ConIn, sizeof(gST->ConIn));
  DBG ("gST->ConsoleOutHandle = 0x%x size = 0x%x\n", gST->ConsoleOutHandle, sizeof(gST->ConsoleOutHandle));
  DBG ("gST->ConOut = 0x%x size = 0x%x\n", gST->ConOut, sizeof(gST->ConOut));
  DBG ("gST->StandardErrorHandle = 0x%x size = 0x%x\n", gST->StandardErrorHandle, sizeof(gST->StandardErrorHandle));
  DBG ("gST->StdErr = 0x%x size = 0x%x\n", gST->StdErr, sizeof(gST->StdErr));
  DBG ("gST->RuntimeServices = 0x%x size = 0x%x\n", gST->RuntimeServices, sizeof(gST->RuntimeServices));
  DBG ("gST->BootServices = 0x%x size = 0x%x\n", gST->BootServices, sizeof(gST->BootServices));
  DBG ("gST->NumberOfTableEntries = 0x%x\n", gST->NumberOfTableEntries);
  DBG ("gST->ConfigurationTable = 0x%x size = 0x%x\n", gST->StdErr, sizeof(gST->ConfigurationTable));
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    CHAR16  Buffer1[100];

    ZeroMem (Buffer1, sizeof (Buffer1));
    UnicodeSPrint (Buffer1, sizeof (Buffer1), L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                   gST->ConfigurationTable[Index].VendorGuid.Data1,
                   gST->ConfigurationTable[Index].VendorGuid.Data2,
                   gST->ConfigurationTable[Index].VendorGuid.Data3,
                   gST->ConfigurationTable[Index].VendorGuid.Data4[0],
                   gST->ConfigurationTable[Index].VendorGuid.Data4[1],
                   gST->ConfigurationTable[Index].VendorGuid.Data4[2],
                   gST->ConfigurationTable[Index].VendorGuid.Data4[3],
                   gST->ConfigurationTable[Index].VendorGuid.Data4[4],
                   gST->ConfigurationTable[Index].VendorGuid.Data4[5],
                   gST->ConfigurationTable[Index].VendorGuid.Data4[6],
                   gST->ConfigurationTable[Index].VendorGuid.Data4[7]
                   );
    DBG ("gST->ConfigurationTable[Index].VendorGuid = %s\n", &Buffer1);
    DBG ("gST->ConfigurationTable[Index].VendorTable = 0x%x size = 0x%x\n", gST->ConfigurationTable[Index].VendorTable, sizeof( gST->ConfigurationTable[Index].VendorTable));
  }
#endif
#ifdef MEMMAP_DEBUG
  DumpGcdMemoryMap ();
  DumpEfiMemoryMap ();
#endif
  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &ImageInfo);
  ASSERT_EFI_ERROR (Status);
  DBG ("%a: AddBootArgs = %a, gSettings.BootArgs = %a\n",__FUNCTION__, AddBootArgs, gSettings.BootArgs);
  AsciiStrToUnicodeStr (gSettings.BootArgs, buffer);
  
  if (StrLen(buffer) > 0) 
  {
    ImageInfo->LoadOptionsSize  = (UINT32) StrSize(buffer);
    ImageInfo->LoadOptions      = buffer;
  }

  if (AsciiStrStr(gSettings.BootArgs, "-v") == NULL) {
    gST->ConOut = NULL;
    if ((OSVersion != NULL) && (AsciiStrnCmp (OSVersion, "10.10", 5) == 0)) {
      if (gSettings.YoBlack) {
        ClearScreen (0x030000, PcdGetPtr (PcdAppleWhiteLogoFile));
      } else {
        ClearScreen (0xBFBFBF, PcdGetPtr (PcdAppleGrayLogoFile));
      }
    }
  }

  if (!gIsHibernation && gSettings.LoadExtraKexts) {
    WithKexts = LoadKexts ();
  }

#if 0
  gBS->CalculateCrc32 ((VOID *)gST, sizeof(EFI_SYSTEM_TABLE), &gST->Hdr.CRC32);
#endif

Next:
  if (ImageHandle == NULL) {
    goto Done;
  }
#ifdef BOOT_DEBUG
  DBG ("%a: launching StartImage.\n",__FUNCTION__);
  SaveBooterLog (gRootFHandle, BOOT_LOG);
#endif
  Status = gBS->StartImage (ImageHandle, ExitDataSize, ExitData);

Done:
#ifdef BOOT_DEBUG
  DBG ("%a: something wrong. we can not launch StartImage.\n",__FUNCTION__);
  SaveBooterLog (gRootFHandle, BOOT_LOG);
#endif
  gRT->SetVariable (
        L"BootCurrent",
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        0,
        &Option->BootCurrent
        );

  return Status;
}

CHAR16*
EFIAPI
BdsLibGetVolumeName (
  IN EFI_FILE_HANDLE FHandle,
  IN UINTN Index
  )
{
  EFI_FILE_SYSTEM_INFO* fsi;
  CHAR16                wbuff[64];

  fsi = EfiLibFileSystemInfo (FHandle);
  if (fsi != NULL) {
    if (StrLen(fsi->VolumeLabel) > 0) {
      UnicodeSPrint (wbuff, sizeof (wbuff), L"%s", fsi->VolumeLabel);
    } else {
      UnicodeSPrint (wbuff, sizeof (wbuff), L"%s %d", L"Unnamed Volume", Index);
    }
    FreePool (fsi);
  } else {
    UnicodeSPrint (wbuff, sizeof (wbuff), L"%s %d", L"No File System Info", Index);
  }
  return EfiStrDuplicate (wbuff);
}

VOID
EFIAPI
BdsLibBuildOneOptionFromHandle (
  IN EFI_FILE_HANDLE RFHandle,
  IN OUT EFI_FILE_HANDLE FHandle,
  IN OUT CHAR16* Path,
  IN CHAR16* OSName,
  IN CHAR16* VolName,
  IN OUT LIST_ENTRY          *BdsBootOptionList,
  IN OUT BOOLEAN Flag
  )
{
  CHAR16 Buffer[255];

  if (FileExists(RFHandle, Path)) {
    if (OSName != NULL) {
      UnicodeSPrint (Buffer, sizeof (Buffer), L"%s (%s)", VolName, OSName);
    } else {
      UnicodeSPrint (Buffer, sizeof (Buffer), L"%s", VolName);
    }
    BdsLibBuildOptionFromHandle (FHandle, Path, BdsBootOptionList, Buffer, TRUE);
  }
}

/**
  For EFI boot option, BDS separate them as six types:
  1. Network - The boot option points to the SimpleNetworkProtocol device.
               Bds will try to automatically create this type boot option when enumerate.
  2. Shell   - The boot option points to internal flash shell.
               Bds will try to automatically create this type boot option when enumerate.
  3. Removable BlockIo      - The boot option only points to the removable media
                              device, like USB flash disk, DVD, Floppy etc.
                              These device should contain a *removable* blockIo
                              protocol in their device handle.
                              Bds will try to automatically create this type boot option
                              when enumerate.
  4. Fixed BlockIo          - The boot option only points to a Fixed blockIo device,
                              like HardDisk.
                              These device should contain a *fixed* blockIo
                              protocol in their device handle.
                              BDS will skip fixed blockIo devices, and NOT
                              automatically create boot option for them. But BDS
                              will help to delete those fixed blockIo boot option,
                              whose description rule conflict with other auto-created
                              boot options.
  5. Non-BlockIo Simplefile - The boot option points to a device whose handle
                              has SimpleFileSystem Protocol, but has no blockio
                              protocol. These devices do not offer blockIo
                              protocol, but BDS still can get the
                              \EFI\BOOT\boot{machinename}.EFI by SimpleFileSystem
                              Protocol.
  6. File    - The boot option points to a file. These boot options are usually
               created by user manually or OS loader. BDS will not delete or modify
               these boot options.

  This function will enumerate all possible boot device in the system, and
  automatically create boot options for Network, Shell, Removable BlockIo,
  and Non-BlockIo Simplefile devices.
  It will only execute once of every boot.

  @param  BdsBootOptionList      The header of the link list which indexed all
                                 current boot options

  @retval EFI_SUCCESS            Finished all the boot device enumerate and create
                                 the boot option base on that boot device

  @retval EFI_OUT_OF_RESOURCES   Failed to enumerate the boot device and create the boot option list
**/
EFI_STATUS
EFIAPI
BdsLibEnumerateAllBootOption (
  IN OUT LIST_ENTRY          *BdsBootOptionList
  )
{
  EFI_STATUS                    Status;
  BOOLEAN                       ConfigNotFound;
  UINTN                         Index, Index2;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  CHAR16                        Buffer[255];
  EFI_HANDLE                    *FileSystemHandles;
  UINTN                         NumberFileSystemHandles;
  EFI_FILE_HANDLE               FHandle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
  CHAR16                        *VolumeName;
  UINTN                         NumberBlockIoHandles;
  EFI_HANDLE                    *BlockIoHandles;
  EFI_BLOCK_IO_PROTOCOL         *BlkIo;
  UINTN                         DevicePathType;
  UINT16                        CdromNumber;
  UINT16                        *PNConfigPlist2;
  
  gRootFHandle    = NULL;
  FileSystemHandles = NULL;
  FHandle         = NULL;
  ConfigNotFound  = TRUE;

  NumberFileSystemHandles = 0;

  ZeroMem (Buffer, sizeof (Buffer));
  CdromNumber     = 0;
  DevicePathType  = 0;
  BlkIo           = NULL;

  if (gProductNameDir != NULL) {
    gPNConfigPlist = AllocateZeroPool (StrSize (gProductNameDir) + StrSize (L"config.plist"));
    StrCpy (gPNConfigPlist, gProductNameDir);
    StrCat (gPNConfigPlist, L"config.plist");
  }

  PNConfigPlist2 = NULL;
  if (gProductNameDir2 != NULL) {
    PNConfigPlist2 = AllocateZeroPool (StrSize (gProductNameDir2) + StrSize (L"config.plist"));
    StrCpy (PNConfigPlist2, gProductNameDir2);
    StrCat (PNConfigPlist2, L"config.plist");
  }
  
  gBS->LocateHandleBuffer (
                           ByProtocol,
                           &gEfiBlockIoProtocolGuid,
                           NULL,
                           &NumberBlockIoHandles,
                           &BlockIoHandles
                           );

  for (Index = 0; Index < NumberBlockIoHandles; Index++) {

    Status = gBS->HandleProtocol (
                    BlockIoHandles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                  );

    if (EFI_ERROR (Status) || (BlkIo->Media->RemovableMedia == FALSE)) {
      continue;
    }
    
    DevicePath  = DevicePathFromHandle (BlockIoHandles[Index]);
    DevicePathType = BdsGetBootTypeFromDevicePath (DevicePath);

    switch (DevicePathType) {
      case BDS_EFI_MESSAGE_ATAPI_BOOT:
      case BDS_EFI_MESSAGE_SATA_BOOT:
        if (BlkIo->Media->RemovableMedia) {
          if (CdromNumber != 0) {
            UnicodeSPrint (Buffer, sizeof (Buffer), L"%s %d", L"DVD/CDROM", CdromNumber);
          } else {
            UnicodeSPrint (Buffer, sizeof (Buffer), L"%s", L"DVD/CDROM");
          }
          CdromNumber++;
        } else {
          break;
        }
        BdsLibBuildOptionFromHandle (BlockIoHandles[Index], L"All", BdsBootOptionList, Buffer, FALSE);
        break;
        
      default:
        break;
    }
  }

  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL,
         &NumberFileSystemHandles,
         &FileSystemHandles
       );
  
  for (Index = 0; Index < NumberFileSystemHandles; Index++) {
    DevicePath  = DevicePathFromHandle (FileSystemHandles[Index]);
    Status = gBS->HandleProtocol (
                    FileSystemHandles[Index],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID *) &Volume
                  );
    if (!EFI_ERROR (Status)) {
      Status = Volume->OpenVolume (
                         Volume,
                         &FHandle
                       );
    }
    
    if ((gPNConfigPlist != NULL) && (FileExists (FHandle, gPNConfigPlist)) && (ConfigNotFound)) {
      gPNDirExists = TRUE;
      gRootFHandle = FHandle;
      ConfigNotFound  = FALSE;
      DBG ("BdsBoot: config's dir: %s\n", gProductNameDir);
    }

    if ((PNConfigPlist2 != NULL) && (FileExists (FHandle, PNConfigPlist2)) && (ConfigNotFound)) {
      FreePool (gPNConfigPlist);
      gPNConfigPlist = PNConfigPlist2;
      FreePool (gProductNameDir);
      gProductNameDir = gProductNameDir2;

      gPNDirExists = TRUE;
      gRootFHandle = FHandle;
      ConfigNotFound  = FALSE;
      DBG ("BdsBoot: config's dir: %s\n", gProductNameDir);
    }
    
    if ((FileExists (FHandle, L"\\EFI\\bareboot\\config.plist")) && (ConfigNotFound)) {
      gRootFHandle = FHandle;
      ConfigNotFound  = FALSE;
      DBG ("BdsBoot: config's dir: \\EFI\\bareboot\\ \n");
    }

    VolumeName = BdsLibGetVolumeName (FHandle, Index);
    DBG ("BdsBoot: %d VolumeName: %s\n", Index, VolumeName);

    for (Index2 =  0; Index2 < MAX_LOADER_PATHS; Index2++) {
      if ((StrCmp (mLoaderPath[Index2], MACOSX_LOADER_PATH) == 0)||
         ((StrCmp (mLoaderPath[Index2], MACOSX_RECOVERY_LOADER_PATH) == 0) &&
          (StrCmp (VolumeName, L"Recovery HD") == 0))) {
        BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], mLoaderPath[Index2], NULL, VolumeName, BdsBootOptionList, TRUE);
      } else if (StrCmp (mLoaderPath[Index2], MACOSX_RECOVERY_LOADER_PATH) == 0) {
        BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], mLoaderPath[Index2], L"Recovery HD", VolumeName, BdsBootOptionList, TRUE);
      } else {
        BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], mLoaderPath[Index2], mLoaderPath[Index2], VolumeName, BdsBootOptionList, TRUE);
      }
    }

    FreePool (VolumeName);
  }

  if (gPNDirExists) {
    gPNAcpiDir = AllocateZeroPool (StrSize (gProductNameDir) + StrSize (L"acpi\\"));
    StrCpy (gPNAcpiDir, gProductNameDir);
    StrCat (gPNAcpiDir, L"acpi\\");
    DBG ("BdsBoot: acpi dir: %s\n", gPNAcpiDir);
  }

  GetBootDefault (gRootFHandle);
  
  Status = BdsLibBuildOptionFromVar (BdsBootOptionList, L"BootOrder");

  return Status;
}

/**
  Build the boot option with the handle parsed in

  @param  Handle                 The handle which present the device path to create
                                 boot option
  @param  BdsBootOptionList      The header of the link list which indexed all
                                 current boot options
  @param  String                 The description of the boot option.

**/
VOID
EFIAPI
BdsLibBuildOptionFromHandle (
  IN  EFI_HANDLE                 Handle,
  IN  CHAR16                     *LoadOptions,
  IN  LIST_ENTRY                 *BdsBootOptionList,
  IN  CHAR16                     *String,
  IN BOOLEAN                     Extra
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = DevicePathFromHandle (Handle);
  BdsLibRegisterNewOption (BdsBootOptionList, LoadOptions, DevicePath, String, L"BootOrder", Extra);
}

/**
  Return the bootable media handle.
  First, check the device is connected
  Second, check whether the device path point to a device which support SimpleFileSystemProtocol,
  Third, detect the the default boot file in the Media, and return the removable Media handle.

  @param  DevicePath  Device Path to a  bootable device

  @return  The bootable media handle. If the media on the DevicePath is not bootable, NULL will return.

**/
EFI_HANDLE
EFIAPI
BdsLibGetBootableHandle (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
{
  EFI_STATUS                      Status;
  EFI_TPL                         OldTpl;
  EFI_DEVICE_PATH_PROTOCOL        *UpdatedDevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *DupDevicePath;
  EFI_HANDLE                      Handle;
  EFI_BLOCK_IO_PROTOCOL           *BlockIo;
  VOID                            *Buffer;
  EFI_DEVICE_PATH_PROTOCOL        *TempDevicePath;
  UINTN                           Size;
  UINTN                           TempSize;
  EFI_HANDLE                      ReturnHandle;
  EFI_HANDLE                      *SimpleFileSystemHandles;

  UINTN                           NumberSimpleFileSystemHandles;
  UINTN                           Index, Index2;
  EFI_FILE_HANDLE                 FHandle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;

  FHandle = NULL;

  UpdatedDevicePath = DevicePath;

  //
  // Enter to critical section to protect the acquired BlockIo instance 
  // from getting released due to the USB mass storage hotplug event
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Check whether the device is connected
  //
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &UpdatedDevicePath, &Handle);
  if (EFI_ERROR (Status)) {
    //
    // Skip the case that the boot option point to a simple file protocol which does not consume block Io protocol,
    //
    Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &UpdatedDevicePath, &Handle);
    if (EFI_ERROR (Status)) {
      //
      // Fail to find the proper BlockIo and simple file protocol, maybe because device not present,  we need to connect it firstly
      //
      UpdatedDevicePath = DevicePath;
      Status            = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &UpdatedDevicePath, &Handle);
      gBS->ConnectController (Handle, NULL, NULL, TRUE);
    }
  } else {
    //
    // For removable device boot option, its contained device path only point to the removable device handle, 
    // should make sure all its children handles (its child partion or media handles) are created and connected. 
    //
    gBS->ConnectController (Handle, NULL, NULL, TRUE);
    DBG ("    BdsBoot: BdsLibGetBootableHandle->ConnectController BlockIo\n");
    //
    // Get BlockIo protocol and check removable attribute
    //
    Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
    ASSERT_EFI_ERROR (Status);

    //
    // Issue a dummy read to the device to check for media change.
    // When the removable media is changed, any Block IO read/write will
    // cause the BlockIo protocol be reinstalled and EFI_MEDIA_CHANGED is
    // returned. After the Block IO protocol is reinstalled, subsequent
    // Block IO read/write will success.
    //
    Buffer = AllocatePool (BlockIo->Media->BlockSize);
    if (Buffer != NULL) {
      BlockIo->ReadBlocks (
               BlockIo,
               BlockIo->Media->MediaId,
               0,
               BlockIo->Media->BlockSize,
               Buffer
               );
      FreePool(Buffer);
    }
  }

  //
  // Detect the the default boot file from removable Media
  //

  //
  // If fail to get bootable handle specified by a USB boot option, the BDS should try to find other bootable device in the same USB bus
  // Try to locate the USB node device path first, if fail then use its previous PCI node to search
  //
  DupDevicePath = DuplicateDevicePath (DevicePath);
  ASSERT (DupDevicePath != NULL);

  UpdatedDevicePath = DupDevicePath;
  Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &UpdatedDevicePath, &Handle);
  //
  // if the resulting device path point to a usb node, and the usb node is a dummy node, should only let device path only point to the previous Pci node
  // Acpi()/Pci()/Usb() --> Acpi()/Pci()
  //
  if ((DevicePathType (UpdatedDevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (UpdatedDevicePath) == MSG_USB_DP)) {
    //
    // Remove the usb node, let the device path only point to PCI node
    //
    SetDevicePathEndNode (UpdatedDevicePath);
    UpdatedDevicePath = DupDevicePath;
  } else {
    UpdatedDevicePath = DevicePath;
  }

  //
  // Get the device path size of boot option
  //
  Size = GetDevicePathSize(UpdatedDevicePath) - sizeof (EFI_DEVICE_PATH_PROTOCOL); // minus the end node
  ReturnHandle = NULL;
  gBS->LocateHandleBuffer (
      ByProtocol,
      &gEfiSimpleFileSystemProtocolGuid,
      NULL,
      &NumberSimpleFileSystemHandles,
      &SimpleFileSystemHandles
      );
  for (Index = 0; Index < NumberSimpleFileSystemHandles; Index++) {
    //
    // Get the device path size of SimpleFileSystem handle
    //
    TempDevicePath = DevicePathFromHandle (SimpleFileSystemHandles[Index]);
    TempSize = GetDevicePathSize (TempDevicePath)- sizeof (EFI_DEVICE_PATH_PROTOCOL); // minus the end node
    //
    // Check whether the device path of boot option is part of the  SimpleFileSystem handle's device path
    //
    if (Size <= TempSize && CompareMem (TempDevicePath, UpdatedDevicePath, Size)==0) {
      Status = gBS->HandleProtocol (
                                    SimpleFileSystemHandles[Index],
                                    &gEfiSimpleFileSystemProtocolGuid,
                                    (VOID *) &Volume
                                    );
      if (!EFI_ERROR (Status)) {
        Status = Volume->OpenVolume (
                                     Volume,
                                     &FHandle
                                     );
      }

      for (Index2 =  0; Index2 < MAX_LOADER_PATHS; Index2++) {
        if (FileExists(FHandle, mLoaderPath[Index2])) {
          ReturnHandle = SimpleFileSystemHandles[Index];
          break;
        }
      }
    }
  }
  DBG ("    BdsBoot: Get the device path size of SimpleFileSystem handle finished\n");
  FreePool(DupDevicePath);
  if (SimpleFileSystemHandles != NULL) {
    FreePool(SimpleFileSystemHandles);
  }
  gBS->RestoreTPL (OldTpl);
  return ReturnHandle;
}

/**
  For a bootable Device path, return its boot type.

  @param  DevicePath                      The bootable device Path to check

  @retval BDS_EFI_MEDIA_HD_BOOT           If given device path contains MEDIA_DEVICE_PATH type device path node
                                          which subtype is MEDIA_HARDDRIVE_DP
  @retval BDS_EFI_MEDIA_CDROM_BOOT        If given device path contains MEDIA_DEVICE_PATH type device path node
                                          which subtype is MEDIA_CDROM_DP
  @retval BDS_EFI_ACPI_FLOPPY_BOOT        If given device path contains ACPI_DEVICE_PATH type device path node
                                          which HID is floppy device.
  @retval BDS_EFI_MESSAGE_ATAPI_BOOT      If given device path contains MESSAGING_DEVICE_PATH type device path node
                                          and its last device path node's subtype is MSG_ATAPI_DP.
  @retval BDS_EFI_MESSAGE_SCSI_BOOT       If given device path contains MESSAGING_DEVICE_PATH type device path node
                                          and its last device path node's subtype is MSG_SCSI_DP.
  @retval BDS_EFI_MESSAGE_USB_DEVICE_BOOT If given device path contains MESSAGING_DEVICE_PATH type device path node
                                          and its last device path node's subtype is MSG_USB_DP.
  @retval BDS_EFI_MESSAGE_MISC_BOOT       If the device path not contains any media device path node,  and
                                          its last device path node point to a message device path node.
  @retval BDS_LEGACY_BBS_BOOT             If given device path contains BBS_DEVICE_PATH type device path node.
  @retval BDS_EFI_UNSUPPORT               An EFI Removable BlockIO device path not point to a media and message device,

**/
UINT32
EFIAPI
BdsGetBootTypeFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  ACPI_HID_DEVICE_PATH          *Acpi;
  EFI_DEVICE_PATH_PROTOCOL      *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *LastDeviceNode;
  UINT32                        BootType;

  if (NULL == DevicePath) {
    return BDS_EFI_UNSUPPORT;
  }

  TempDevicePath = DevicePath;

  while (!IsDevicePathEndType (TempDevicePath)) {
    switch (DevicePathType (TempDevicePath)) {
      case BBS_DEVICE_PATH:
         return BDS_LEGACY_BBS_BOOT;
      case MEDIA_DEVICE_PATH:
        if (DevicePathSubType (TempDevicePath) == MEDIA_HARDDRIVE_DP) {
          return BDS_EFI_MEDIA_HD_BOOT;
        } else if (DevicePathSubType (TempDevicePath) == MEDIA_CDROM_DP) {
          return BDS_EFI_MEDIA_CDROM_BOOT;
        }
        break;
      case ACPI_DEVICE_PATH:
        Acpi = (ACPI_HID_DEVICE_PATH *) TempDevicePath;
        if (EISA_ID_TO_NUM (Acpi->HID) == 0x0604) {
          return BDS_EFI_ACPI_FLOPPY_BOOT;
        }
        break;
      case MESSAGING_DEVICE_PATH:
        //
        // Get the last device path node
        //
        LastDeviceNode = NextDevicePathNode (TempDevicePath);
        if (DevicePathSubType(LastDeviceNode) == MSG_DEVICE_LOGICAL_UNIT_DP) {
          //
          // if the next node type is Device Logical Unit, which specify the Logical Unit Number (LUN),
          // skip it
          //
          LastDeviceNode = NextDevicePathNode (LastDeviceNode);
        }
        //
        // if the device path not only point to driver device, it is not a messaging device path,
        //
        if (!IsDevicePathEndType (LastDeviceNode)) {
          break;
        }

        switch (DevicePathSubType (TempDevicePath)) {
        case MSG_ATAPI_DP:
          BootType = BDS_EFI_MESSAGE_ATAPI_BOOT;
          break;

        case MSG_USB_DP:
          BootType = BDS_EFI_MESSAGE_USB_DEVICE_BOOT;
          break;

        case MSG_SCSI_DP:
          BootType = BDS_EFI_MESSAGE_SCSI_BOOT;
          break;

        case MSG_SATA_DP:
          BootType = BDS_EFI_MESSAGE_SATA_BOOT;
          break;

        case MSG_MAC_ADDR_DP:
        case MSG_VLAN_DP:
        case MSG_IPv4_DP:
        case MSG_IPv6_DP:
          BootType = BDS_EFI_MESSAGE_MAC_BOOT;
          break;

        default:
          BootType = BDS_EFI_MESSAGE_MISC_BOOT;
          break;
        }
        return BootType;

      default:
        break;
    }
    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }

  return BDS_EFI_UNSUPPORT;
}
