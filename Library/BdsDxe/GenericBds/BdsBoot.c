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
#include "InternalBdsLib.h"
#include "macosx/macosx.h"

#define CLOVER_MEDIA_FILE_NAME_IA32    L"\\EFI\\BOOT\\CLOVERIA32.EFI"
#define CLOVER_MEDIA_FILE_NAME_IA64    L"\\EFI\\BOOT\\CLOVERIA64.EFI"
#define CLOVER_MEDIA_FILE_NAME_X64     L"\\EFI\\BOOT\\CLOVERX64.EFI"
#define CLOVER_MEDIA_FILE_NAME_ARM     L"\\EFI\\BOOT\\CLOVERARM.EFI"

#if   defined (MDE_CPU_IA32)
#define CLOVER_MEDIA_FILE_NAME   CLOVER_MEDIA_FILE_NAME_IA32
#elif defined (MDE_CPU_IPF)
#define CLOVER_MEDIA_FILE_NAME   CLOVER_MEDIA_FILE_NAME_IA64
#elif defined (MDE_CPU_X64)
#define CLOVER_MEDIA_FILE_NAME   CLOVER_MEDIA_FILE_NAME_X64
#elif defined (MDE_CPU_EBC)
#elif defined (MDE_CPU_ARM)
#define CLOVER_MEDIA_FILE_NAME   CLOVER_MEDIA_FILE_NAME_ARM
#else
#error Unknown Processor Type
#endif

#define MACOSX_LOADER_PATH              L"\\System\\Library\\CoreServices\\boot.efi"
#define MACOSX_RECOVERY_LOADER_PATH     L"\\com.apple.recovery.boot\\boot.efi"
#define MACOSX_INSTALL_PATH             L"\\OS X Install Data\\boot.efi"
#define WINDOWS_LOADER_PATH             L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi"
#define SUSE_LOADER_PATH                L"\\EFI\\SuSE\\elilo.efi"
#define UBUNTU_LOADER_PATH              L"\\EFI\\Ubuntu\\grubx64.efi"
#define REDHAT_LOADER_PATH              L"\\EFI\\RedHat\\grub.efi"
#define SHELL_PATH                      L"\\EFI\\TOOLS\\shell64.efi"
#define ALT_WINDOWS_LOADER_PATH         L"\\EFI\\MS\\Boot\\bootmgfw.efi"

BOOLEAN mEnumBootDevice = FALSE;

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
  EFI_FILE_HANDLE                 FHandle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
  UINT8                     Index;

  *ExitDataSize = 0;
  *ExitData     = NULL;
  FHandle       = NULL;

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
      if ((StrCmp(LoadOptions, MACOSX_LOADER_PATH) == 0) ||
          (StrCmp(LoadOptions, MACOSX_RECOVERY_LOADER_PATH) == 0) ||
          (StrCmp(LoadOptions, MACOSX_INSTALL_PATH) == 0)) {
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
              if (!EFI_ERROR (Status)) goto MacOS;
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
          if (!EFI_ERROR (Status)) goto Next;
        }
      }
//  ----=================================================----      
     if (StrCmp(LoadOptions, L"All") == 0) {
        FilePath = FileDevicePath (Handle, MACOSX_LOADER_PATH);
        if (FilePath != NULL) {
          Status = gBS->LoadImage (
                                   TRUE,
                                   gImageHandle,
                                   FilePath,
                                   NULL,
                                   0,
                                   &ImageHandle
                                   );      
          if (!EFI_ERROR (Status)) goto MacOS;
        }
        FilePath = FileDevicePath (Handle, MACOSX_RECOVERY_LOADER_PATH);
        if (FilePath != NULL) {
          Status = gBS->LoadImage (
                                   TRUE,
                                   gImageHandle,
                                   FilePath,
                                   NULL,
                                   0,
                                   &ImageHandle
                                   );      
          if (!EFI_ERROR (Status)) goto MacOS;
        }

        FilePath = FileDevicePath (Handle, MACOSX_INSTALL_PATH);
        if (FilePath != NULL) {
          Status = gBS->LoadImage (
                                   TRUE,
                                   gImageHandle,
                                   FilePath,
                                   NULL,
                                   0,
                                   &ImageHandle
                                   );
          if (!EFI_ERROR (Status)) goto MacOS;
        }

        FilePath = FileDevicePath (Handle, SUSE_LOADER_PATH);
        if (FilePath != NULL) {
          Status = gBS->LoadImage (
                                   TRUE,
                                   gImageHandle,
                                   FilePath,
                                   NULL,
                                   0,
                                   &ImageHandle
                                   );
          if (!EFI_ERROR (Status)) goto Next;
        }          
        FilePath = FileDevicePath (Handle, UBUNTU_LOADER_PATH);
        if (FilePath != NULL) {
          Status = gBS->LoadImage (
                                   TRUE,
                                   gImageHandle,
                                   FilePath,
                                   NULL,
                                   0,
                                   &ImageHandle
                                   );
          if (!EFI_ERROR (Status)) goto Next;
        }
        FilePath = FileDevicePath (Handle, REDHAT_LOADER_PATH);
        if (FilePath != NULL) {
          Status = gBS->LoadImage (
                                   TRUE,
                                   gImageHandle,
                                   FilePath,
                                   NULL,
                                   0,
                                   &ImageHandle
                                   );
          if (!EFI_ERROR (Status)) goto Next;
        }
        FilePath = FileDevicePath (Handle, EFI_REMOVABLE_MEDIA_FILE_NAME);
        if (FilePath != NULL) {
          Status = gBS->LoadImage (
                                   TRUE,
                                   gImageHandle,
                                   FilePath,
                                   NULL,
                                   0,
                                   &ImageHandle
                                   );
          if (!EFI_ERROR (Status)) goto Next;
        } 
        FilePath = FileDevicePath (Handle, WINDOWS_LOADER_PATH);
        if (FilePath != NULL) {
          Status = gBS->LoadImage (
                                   TRUE,
                                   gImageHandle,
                                   FilePath,
                                   NULL,
                                   0,
                                   &ImageHandle
                                   );
          if (!EFI_ERROR (Status)) goto Next;
        }
        FilePath = FileDevicePath (Handle, CLOVER_MEDIA_FILE_NAME);
        if (FilePath != NULL) {
          Status = gBS->LoadImage (
                                   TRUE,
                                   gImageHandle,
                                   FilePath,
                                   NULL,
                                   0,
                                   &ImageHandle
                                   );      
          if (EFI_ERROR (Status)) goto Done; else goto Next;
        }    
      } 
    } else goto Next;
    goto Done;

MacOS:
  DEBUG ((DEBUG_INFO, "BdsBoot: Starting InitializeConsoleSim\n"));
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
  DEBUG ((DEBUG_INFO, "BdsBoot: Starting GetOSVersion\n"));
  GetOSVersion (FHandle);
  DEBUG ((DEBUG_INFO, "BdsBoot: Starting GetCpuProps\n"));
  GetCpuProps ();
  DEBUG ((DEBUG_INFO, "BdsBoot: Starting GetUserSettings\n"));
  GetUserSettings (gRootFHandle);
  DEBUG ((DEBUG_INFO, "BdsBoot: Starting SetDevices\n"));
  SetDevices ();
  DEBUG ((DEBUG_INFO, "BdsBoot: Starting PatchSmbios\n"));
  PatchSmbios ();
  DEBUG ((DEBUG_INFO, "BdsBoot: Starting PatchACPI\n"));
  PatchACPI (gRootFHandle);
  DEBUG ((DEBUG_INFO, "BdsBoot: Starting SetVariablesForOSX\n"));
  SetVariablesForOSX ();
  DEBUG ((DEBUG_INFO, "BdsBoot: Starting SetPrivateVarProto\n"));
  SetPrivateVarProto ();
  DEBUG ((DEBUG_INFO, "BdsBoot: Starting SetupDataForOSX\n"));
  SetupDataForOSX ();
  DEBUG ((DEBUG_INFO, "BdsBoot: Starting EventsInitialize\n"));
  EventsInitialize ();

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
#ifdef BOOT_DEBUG
  SaveBooterLog (gRootFHandle, BOOT_LOG);
#endif

  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &ImageInfo);
  ASSERT_EFI_ERROR (Status);
  DEBUG ((DEBUG_INFO, "AddBootArgs = %a, gSettings.BootArgs = %a\n", AddBootArgs, gSettings.BootArgs));
  AsciiStrToUnicodeStr (gSettings.BootArgs, buffer);
  
  if (StrLen(buffer) > 0) 
  {
    ImageInfo->LoadOptionsSize  = (UINT32)StrSize(buffer);
    ImageInfo->LoadOptions      = buffer;
  }

  if (AsciiStrStr(gSettings.BootArgs, "-v") == 0) {
    gST->ConOut = NULL; 
  } 
#if 0
  gBS->CalculateCrc32 ((VOID *)gST, sizeof(EFI_SYSTEM_TABLE), &gST->Hdr.CRC32);
#endif
Next:
  if (ImageHandle == NULL) {
    goto Done;
  }
  Status = gBS->StartImage (ImageHandle, ExitDataSize, ExitData);

Done:
  gRT->SetVariable (
        L"BootCurrent",
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        0,
        &Option->BootCurrent
        );

  return Status;
}

/**
  Delete all invalid EFI boot options.

  @retval EFI_SUCCESS            Delete all invalid boot option success
  @retval EFI_NOT_FOUND          Variable "BootOrder" is not found
  @retval EFI_OUT_OF_RESOURCES   Lack of memory resource
  @retval Other                  Error return value from SetVariable()

**/
EFI_STATUS
BdsDeleteAllInvalidEfiBootOption (
  VOID
  )
{
  UINT16                    *BootOrder;
  UINT8                     *BootOptionVar;
  UINTN                     BootOrderSize;
  UINTN                     BootOptionSize;
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     Index2;
  UINT16                    BootOption[BOOT_OPTION_MAX_CHAR];
  EFI_DEVICE_PATH_PROTOCOL  *OptionDevicePath;
  UINT8                     *TempPtr;
  CHAR16                    *Description;

  Status        = EFI_SUCCESS;
  BootOrder     = NULL;
  BootOrderSize = 0;

  //
  // Check "BootOrder" variable firstly, this variable hold the number of boot options
  //
  BootOrder = BdsLibGetVariableAndSize (
                L"BootOrder",
                &gEfiGlobalVariableGuid,
                &BootOrderSize
                );
  if (NULL == BootOrder) {
    return EFI_NOT_FOUND;
  }

  Index = 0;
  while (Index < BootOrderSize / sizeof (UINT16)) {
    UnicodeSPrint (BootOption, sizeof (BootOption), L"Boot%04x", BootOrder[Index]);
    BootOptionVar = BdsLibGetVariableAndSize (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      &BootOptionSize
                      );
    if (NULL == BootOptionVar) {
      FreePool (BootOrder);
      return EFI_OUT_OF_RESOURCES;
    }

    TempPtr = BootOptionVar;
    TempPtr += sizeof (UINT32) + sizeof (UINT16);
    Description = (CHAR16 *) TempPtr;
    TempPtr += StrSize ((CHAR16 *) TempPtr);
    OptionDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;

    //
    // Skip legacy boot option (BBS boot device)
    //
    if ((DevicePathType (OptionDevicePath) == BBS_DEVICE_PATH) &&
        (DevicePathSubType (OptionDevicePath) == BBS_BBS_DP)) {
      FreePool (BootOptionVar);
      Index++;
      continue;
    }

    if (!BdsLibIsValidEFIBootOptDevicePathExt (OptionDevicePath, FALSE, Description)) {
      //
      // Delete this invalid boot option "Boot####"
      //
      Status = gRT->SetVariable (
                      BootOption,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      0,
                      NULL
                      );
      //
      // Mark this boot option in boot order as deleted
      //
      BootOrder[Index] = 0xffff;
    }

    FreePool (BootOptionVar);
    Index++;
  }

  //
  // Adjust boot order array
  //
  Index2 = 0;
  for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
    if (BootOrder[Index] != 0xffff) {
      BootOrder[Index2] = BootOrder[Index];
      Index2 ++;
    }
  }
  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  Index2 * sizeof (UINT16),
                  BootOrder
                  );

  FreePool (BootOrder);

  return Status;
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
#if 0
  UINT16                        FloppyNumber;
  UINT16                        NonBlockNumber;
  BOOLEAN                       Removable[2];
  CHAR8                         *PlatLang;
  CHAR8                         *LastLang;
#endif
  BOOLEAN                       ConfigNotFound;
  UINTN                         Index;
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandleBuffer;
  EFI_FV_FILETYPE               Type;
  UINTN                         Size;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINT32                        AuthenticationStatus;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  CHAR16                        Buffer[255];
  EFI_HANDLE                    *FileSystemHandles;
  UINTN                         NumberFileSystemHandles;
  EFI_FILE_HANDLE                 FHandle;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
  EFI_FILE_SYSTEM_INFO            *FileSystemInfo;
  UINT32                          BufferSizeVolume;
  UINTN                         NumberBlockIoHandles;
  EFI_HANDLE                    *BlockIoHandles;
  EFI_BLOCK_IO_PROTOCOL         *BlkIo;
  UINTN                         DevicePathType;
  UINT16                        CdromNumber;
  BOOLEAN                       Removable[2];
  UINTN                         RemovableIndex;

  gRootFHandle    = NULL;
  FileSystemInfo  = NULL;
  FileSystemHandles = NULL;
  FHandle         = NULL;
  ConfigNotFound  = TRUE;

  NumberFileSystemHandles = 0;

  ZeroMem (Buffer, sizeof (Buffer));
  CdromNumber     = 0;
  DevicePathType  = 0;
  BlkIo           = NULL;
  Removable[0]    = FALSE;
  Removable[1]    = TRUE;

#if 0
  PlatLang        = NULL;
  LastLang        = NULL;
  FloppyNumber    = 0;
  //
  // If the boot device enumerate happened, just get the boot
  // device from the boot order variable
  //
  if (mEnumBootDevice) {
    LastLang = GetVariable (LAST_ENUM_LANGUAGE_VARIABLE_NAME, &gLastEnumLangGuid);
    PlatLang = GetEfiGlobalVariable (L"PlatformLang");
    ASSERT (PlatLang != NULL);
    if ((LastLang != NULL) && (AsciiStrCmp (LastLang, PlatLang) == 0)) {
      Status = BdsLibBuildOptionFromVar (BdsBootOptionList, L"BootOrder");
      FreePool (LastLang);
      FreePool (PlatLang);
      return Status;
    } else {
      Status = gRT->SetVariable (
        LAST_ENUM_LANGUAGE_VARIABLE_NAME,
        &gLastEnumLangGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
        AsciiStrSize (PlatLang),
        PlatLang
        );
      ASSERT_EFI_ERROR (Status);

      if (LastLang != NULL) {
        FreePool (LastLang);
      }
      FreePool (PlatLang);
    }
  }

  //
  // Notes: this dirty code is to get the legacy boot option from the
  // BBS table and create to variable as the EFI boot option, it should
  // be removed after the CSM can provide legacy boot option directly
  //
//  REFRESH_LEGACY_BOOT_OPTIONS;

  //
  // Delete invalid boot option
  //
  BdsDeleteAllInvalidEfiBootOption ();

  //
  // Parse removable media followed by fixed media.
  // The Removable[] array is used by the for-loop below to create removable media boot options 
  // at first, and then to create fixed media boot options.
  //

  if (DevicePathType != BDS_EFI_ACPI_FLOPPY_BOOT) {
    NonBlockNumber = 0;
#endif

    BdsDeleteAllInvalidEfiBootOption ();

    gBS->LocateHandleBuffer (
                             ByProtocol,
                             &gEfiBlockIoProtocolGuid,
                             NULL,
                             &NumberBlockIoHandles,
                             &BlockIoHandles
                             );

    for (RemovableIndex = 0; RemovableIndex < 2; RemovableIndex++) {
      for (Index = 0; Index < NumberBlockIoHandles; Index++) {

        Status = gBS->HandleProtocol (
                        BlockIoHandles[Index],
                        &gEfiBlockIoProtocolGuid,
                        (VOID **) &BlkIo
                      );

        if (EFI_ERROR (Status) || (BlkIo->Media->RemovableMedia == Removable[RemovableIndex])) {
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
      }

      if ((FileExists (FHandle, L"\\EFI\\bareboot\\config.plist")) && (ConfigNotFound)) {
        gRootFHandle = FHandle;
        ConfigNotFound  = FALSE;
      }
        
      BufferSizeVolume = SIZE_OF_EFI_FILE_SYSTEM_INFO + 255;
      FileSystemInfo = AllocateZeroPool(BufferSizeVolume);      
      Status = FHandle->GetInfo(FHandle, &gEfiFileSystemInfoGuid,(UINTN*)&BufferSizeVolume, FileSystemInfo);
      DBG ("BdsLibEnumerateAllBootOption: FileSystemInfo->VolumeLabel -> |%s|\n", FileSystemInfo->VolumeLabel);
      if (!EFI_ERROR(Status)) {
        if ((FileSystemInfo->VolumeLabel != NULL) &&
            (StrLen(FileSystemInfo->VolumeLabel) > 0)) {
          UnicodeSPrint (Buffer, sizeof (Buffer), L"%s", FileSystemInfo->VolumeLabel);
        } else {
          UnicodeSPrint (Buffer, sizeof (Buffer), L"%s %d", L"Unnamed Volume ", Index);
        }
      } else {
        UnicodeSPrint (Buffer, sizeof (Buffer), L"%s %d", L"Unnamed Volume ", Index);
      }

      BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], MACOSX_LOADER_PATH, NULL, Buffer, BdsBootOptionList, TRUE);
      BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], MACOSX_RECOVERY_LOADER_PATH, NULL, Buffer, BdsBootOptionList, TRUE);
      BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], MACOSX_INSTALL_PATH, L"OS X Install Data", Buffer, BdsBootOptionList, TRUE);
      BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], SUSE_LOADER_PATH, L"OpenSuSE EFI Loader", Buffer, BdsBootOptionList, TRUE);
      BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], UBUNTU_LOADER_PATH, L"Ubuntu EFI Loader", Buffer, BdsBootOptionList, TRUE);
      BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], REDHAT_LOADER_PATH, L"RedHat EFI Loader", Buffer, BdsBootOptionList, TRUE);
      BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], EFI_REMOVABLE_MEDIA_FILE_NAME, L"EFI boot Loader", Buffer, BdsBootOptionList, TRUE);
      BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], WINDOWS_LOADER_PATH, L"Windows EFI Loader", Buffer, BdsBootOptionList, TRUE);
      BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], ALT_WINDOWS_LOADER_PATH, L"Alt Windows EFI Loader", Buffer, BdsBootOptionList, TRUE);
      BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], CLOVER_MEDIA_FILE_NAME, L"Clover EFI", Buffer, BdsBootOptionList, TRUE);
      BdsLibBuildOneOptionFromHandle (FHandle, FileSystemHandles[Index], SHELL_PATH, L"[EFI SHell]", Buffer, BdsBootOptionList, TRUE);
    }
    GetBootDefault (gRootFHandle);
#if 0
  }
#endif
  gBS->LocateHandleBuffer (
        ByProtocol,
        &gEfiFirmwareVolume2ProtocolGuid,
        NULL,
        &FvHandleCount,
        &FvHandleBuffer
        );
  for (Index = 0; Index < FvHandleCount; Index++) {
    gBS->HandleProtocol (
          FvHandleBuffer[Index],
          &gEfiFirmwareVolume2ProtocolGuid,
          (VOID **) &Fv
          );

    Status = Fv->ReadFile (
                  Fv,
                  PcdGetPtr(PcdShellFile),
                  NULL,
                  &Size,
                  &Type,
                  &Attributes,
                  &AuthenticationStatus
                  );
    if (EFI_ERROR (Status)) {
      //
      // Skip if no shell file in the FV
      //
      continue;
    }
    BdsLibBuildOptionFromShell (FvHandleBuffer[Index], BdsBootOptionList);
  }

  if (FvHandleCount != 0) {
    FreePool (FvHandleBuffer);
  }
  Status = BdsLibBuildOptionFromVar (BdsBootOptionList, L"BootOrder");
  mEnumBootDevice = TRUE;
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
  Build the on flash shell boot option with the handle parsed in.

  @param  Handle                 The handle which present the device path to create
                                 on flash shell boot option
  @param  BdsBootOptionList      The header of the link list which indexed all
                                 current boot options

**/
VOID
EFIAPI
BdsLibBuildOptionFromShell (
  IN EFI_HANDLE                  Handle,
  IN OUT LIST_ENTRY              *BdsBootOptionList
  )
{
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH ShellNode;

  DevicePath = DevicePathFromHandle (Handle);
  EfiInitializeFwVolDevicepathNode (&ShellNode, PcdGetPtr(PcdShellFile));
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *) &ShellNode);
  BdsLibRegisterNewOption (BdsBootOptionList, NULL, DevicePath, L"EFI Internal Shell", L"BootOrder", FALSE);
}

/**
  Boot from the UEFI spec defined "BootNext" variable.

**/
VOID
EFIAPI
BdsLibBootNext (
  VOID
  )
{
  UINT16            *BootNext;
  UINTN             BootNextSize;
  CHAR16            Buffer[20];
  BDS_COMMON_OPTION *BootOption;
  LIST_ENTRY        TempList;
  UINTN             ExitDataSize;
  CHAR16            *ExitData;

  //
  // Init the boot option name buffer and temp link list
  //
  InitializeListHead (&TempList);
  ZeroMem (Buffer, sizeof (Buffer));

  BootNext = BdsLibGetVariableAndSize (
              L"BootNext",
              &gEfiGlobalVariableGuid,
              &BootNextSize
              );

  //
  // Clear the boot next variable first
  //
  if (BootNext != NULL) {
    gRT->SetVariable (
          L"BootNext",
          &gEfiGlobalVariableGuid,
          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
          0,
          BootNext
          );

    //
    // Start to build the boot option and try to boot
    //
    UnicodeSPrint (Buffer, sizeof (Buffer), L"Boot%04x", *BootNext);
    BootOption = BdsLibVariableToOption (&TempList, Buffer);
    ASSERT (BootOption != NULL);
    BdsLibConnectDevicePath (BootOption->DevicePath);
    BdsLibBootViaBootOption (BootOption, BootOption->LoadOptions, BootOption->DevicePath, &ExitDataSize, &ExitData);
  }

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
  UINTN                           Index;
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
      if ((FileExists(FHandle, MACOSX_LOADER_PATH)) ||
          (FileExists(FHandle, MACOSX_RECOVERY_LOADER_PATH)) ||
          (FileExists(FHandle, MACOSX_INSTALL_PATH)) ||
          (FileExists(FHandle, WINDOWS_LOADER_PATH)) ||
          (FileExists(FHandle, ALT_WINDOWS_LOADER_PATH)) ||
          (FileExists(FHandle, SUSE_LOADER_PATH)) ||
          (FileExists(FHandle, UBUNTU_LOADER_PATH)) ||
          (FileExists(FHandle, REDHAT_LOADER_PATH)) ||
          (FileExists(FHandle, CLOVER_MEDIA_FILE_NAME)) ||
          (FileExists(FHandle, SHELL_PATH)) ||
          (FileExists(FHandle, EFI_REMOVABLE_MEDIA_FILE_NAME)))
      {
        ReturnHandle = SimpleFileSystemHandles[Index];
        break;
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
  Check to see if the network cable is plugged in. If the DevicePath is not
  connected it will be connected.

  @param  DevicePath             Device Path to check

  @retval TRUE                   DevicePath points to an Network that is connected
  @retval FALSE                  DevicePath does not point to a bootable network

**/
BOOLEAN
BdsLibNetworkBootWithMediaPresent (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
{
  EFI_STATUS                      Status;
  EFI_DEVICE_PATH_PROTOCOL        *UpdatedDevicePath;
  EFI_HANDLE                      Handle;
  EFI_SIMPLE_NETWORK_PROTOCOL     *Snp;
  BOOLEAN                         MediaPresent;
  UINT32                          InterruptStatus;

  MediaPresent = FALSE;

  UpdatedDevicePath = DevicePath;
  //
  // Locate Load File Protocol for PXE boot option first
  //
  Status = gBS->LocateDevicePath (&gEfiLoadFileProtocolGuid, &UpdatedDevicePath, &Handle);
  if (EFI_ERROR (Status)) {
    //
    // Device not present so see if we need to connect it
    //
    Status = BdsLibConnectDevicePath (DevicePath);
    if (!EFI_ERROR (Status)) {
      //
      // This one should work after we did the connect
      //
      Status = gBS->LocateDevicePath (&gEfiLoadFileProtocolGuid, &UpdatedDevicePath, &Handle);
    }
  }

  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (Handle, &gEfiSimpleNetworkProtocolGuid, (VOID **)&Snp);
    if (EFI_ERROR (Status)) {
      //
      // Failed to open SNP from this handle, try to get SNP from parent handle
      //
      UpdatedDevicePath = DevicePathFromHandle (Handle);
      if (UpdatedDevicePath != NULL) {
        Status = gBS->LocateDevicePath (&gEfiSimpleNetworkProtocolGuid, &UpdatedDevicePath, &Handle);
        if (!EFI_ERROR (Status)) {
          //
          // SNP handle found, get SNP from it
          //
          Status = gBS->HandleProtocol (Handle, &gEfiSimpleNetworkProtocolGuid, (VOID **) &Snp);
        }
      }
    }

    if (!EFI_ERROR (Status)) {
      if (Snp->Mode->MediaPresentSupported) {
        if (Snp->Mode->State == EfiSimpleNetworkInitialized) {
          //
          // Invoke Snp->GetStatus() to refresh the media status
          //
          Snp->GetStatus (Snp, &InterruptStatus, NULL);

          //
          // In case some one else is using the SNP check to see if it's connected
          //
          MediaPresent = Snp->Mode->MediaPresent;
        } else {
          //
          // No one is using SNP so we need to Start and Initialize so
          // MediaPresent will be valid.
          //
          Status = Snp->Start (Snp);
          if (!EFI_ERROR (Status)) {
            Status = Snp->Initialize (Snp, 0, 0);
            if (!EFI_ERROR (Status)) {
              MediaPresent = Snp->Mode->MediaPresent;
              Snp->Shutdown (Snp);
            }
            Snp->Stop (Snp);
          }
        }
      } else {
        MediaPresent = TRUE;
      }
    }
  }

  return MediaPresent;
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

/**
  Check whether the Device path in a boot option point to a valid bootable device,
  And if CheckMedia is true, check the device is ready to boot now.

  @param  DevPath     the Device path in a boot option
  @param  CheckMedia  if true, check the device is ready to boot now.

  @retval TRUE        the Device path  is valid
  @retval FALSE       the Device path  is invalid .

**/
BOOLEAN
EFIAPI
BdsLibIsValidEFIBootOptDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath,
  IN BOOLEAN                      CheckMedia
  )
{
  return BdsLibIsValidEFIBootOptDevicePathExt (DevPath, CheckMedia, NULL);
}

/**
  Check whether the Device path in a boot option point to a valid bootable device,
  And if CheckMedia is true, check the device is ready to boot now.
  If Description is not NULL and the device path point to a fixed BlockIo
  device, check the description whether conflict with other auto-created
  boot options.

  @param  DevPath     the Device path in a boot option
  @param  CheckMedia  if true, check the device is ready to boot now.
  @param  Description the description in a boot option

  @retval TRUE        the Device path  is valid
  @retval FALSE       the Device path  is invalid .

**/
BOOLEAN
EFIAPI
BdsLibIsValidEFIBootOptDevicePathExt (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath,
  IN BOOLEAN                      CheckMedia,
  IN CHAR16                       *Description
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *LastDeviceNode;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;

  TempDevicePath = DevPath;
  LastDeviceNode = DevPath;

  //
  // Check if it's a valid boot option for network boot device.
  // Check if there is EfiLoadFileProtocol installed. 
  // If yes, that means there is a boot option for network.
  //
  Status = gBS->LocateDevicePath (
                  &gEfiLoadFileProtocolGuid,
                  &TempDevicePath,
                  &Handle
                  );
  if (EFI_ERROR (Status)) {
    //
    // Device not present so see if we need to connect it
    //
    TempDevicePath = DevPath;
    BdsLibConnectDevicePath (TempDevicePath);
    Status = gBS->LocateDevicePath (
                    &gEfiLoadFileProtocolGuid,
                    &TempDevicePath,
                    &Handle
                    );
  }

  if (!EFI_ERROR (Status)) {
    if (!IsDevicePathEnd (TempDevicePath)) {
      //
      // LoadFile protocol is not installed on handle with exactly the same DevPath
      //
      return FALSE;
    }

    if (CheckMedia) {
      //
      // Test if it is ready to boot now
      //
      if (BdsLibNetworkBootWithMediaPresent(DevPath)) {
        return TRUE;
      }
    } else {
      return TRUE;
    }    
  }

  //
  // If the boot option point to a file, it is a valid EFI boot option,
  // and assume it is ready to boot now
  //
  while (!IsDevicePathEnd (TempDevicePath)) {
    //
    // If there is USB Class or USB WWID device path node, treat it as valid EFI
    // Boot Option. BdsExpandUsbShortFormDevicePath () will be used to expand it
    // to full device path.
    //
    if ((DevicePathType (TempDevicePath) == MESSAGING_DEVICE_PATH) &&
        ((DevicePathSubType (TempDevicePath) == MSG_USB_CLASS_DP) ||
         (DevicePathSubType (TempDevicePath) == MSG_USB_WWID_DP))) {
      return TRUE;
    }

    LastDeviceNode = TempDevicePath;
    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }
  if ((DevicePathType (LastDeviceNode) == MEDIA_DEVICE_PATH) &&
    (DevicePathSubType (LastDeviceNode) == MEDIA_FILEPATH_DP)) {
    return TRUE;
  }

  //
  // Check if it's a valid boot option for internal FV application
  //
  if (EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) LastDeviceNode) != NULL) {
    //
    // If the boot option point to internal FV application, make sure it is valid
    //
    TempDevicePath = DevPath;
    Status = BdsLibUpdateFvFileDevicePath (
               &TempDevicePath,
               EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) LastDeviceNode)
               );
    if (Status == EFI_ALREADY_STARTED) {
      return TRUE;
    } else {
      if (Status == EFI_SUCCESS) {
        FreePool (TempDevicePath);
      }
      return FALSE;
    }
  }

  //
  // If the boot option point to a blockIO device:
  //    if it is a removable blockIo device, it is valid.
  //    if it is a fixed blockIo device, check its description confliction.
  //
  TempDevicePath = DevPath;
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &TempDevicePath, &Handle);
  if (EFI_ERROR (Status)) {
    //
    // Device not present so see if we need to connect it
    //
    Status = BdsLibConnectDevicePath (DevPath);
    if (!EFI_ERROR (Status)) {
      //
      // Try again to get the Block Io protocol after we did the connect
      //
      TempDevicePath = DevPath;
      Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &TempDevicePath, &Handle);
    }
  }

  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
    if (!EFI_ERROR (Status)) {
      if (CheckMedia) {
        //
        // Test if it is ready to boot now
        //
        if (BdsLibGetBootableHandle (DevPath) != NULL) {
          return TRUE;
        }
      } else {
        return TRUE;
      }
    }
  } else {
    //
    // if the boot option point to a simple file protocol which does not consume block Io protocol, it is also a valid EFI boot option,
    //
    Status = gBS->LocateDevicePath (&gEfiSimpleFileSystemProtocolGuid, &TempDevicePath, &Handle);
    if (!EFI_ERROR (Status)) {
      if (CheckMedia) {
        //
        // Test if it is ready to boot now
        //
        if (BdsLibGetBootableHandle (DevPath) != NULL) {
          return TRUE;
        }
      } else {
        return TRUE;
      }
    }
  }

  return FALSE;
}


/**
  According to a file guild, check a Fv file device path is valid. If it is invalid,
  try to return the valid device path.
  FV address maybe changes for memory layout adjust from time to time, use this function
  could promise the Fv file device path is right.

  @param  DevicePath             on input, the Fv file device path need to check on
                                 output, the updated valid Fv file device path
  @param  FileGuid               the Fv file guild

  @retval EFI_INVALID_PARAMETER  the input DevicePath or FileGuid is invalid
                                 parameter
  @retval EFI_UNSUPPORTED        the input DevicePath does not contain Fv file
                                 guild at all
  @retval EFI_ALREADY_STARTED    the input DevicePath has pointed to Fv file, it is
                                 valid
  @retval EFI_SUCCESS            has successfully updated the invalid DevicePath,
                                 and return the updated device path in DevicePath

**/
EFI_STATUS
EFIAPI
BdsLibUpdateFvFileDevicePath (
  IN  OUT EFI_DEVICE_PATH_PROTOCOL      ** DevicePath,
  IN  EFI_GUID                          *FileGuid
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL      *LastDeviceNode;
  EFI_STATUS                    Status;
  EFI_GUID                      *GuidPoint;
  UINTN                         Index;
  UINTN                         FvHandleCount;
  EFI_HANDLE                    *FvHandleBuffer;
  EFI_FV_FILETYPE               Type;
  UINTN                         Size;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINT32                        AuthenticationStatus;
  BOOLEAN                       FindFvFile;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH FvFileNode;
  EFI_HANDLE                    FoundFvHandle;
  EFI_DEVICE_PATH_PROTOCOL      *NewDevicePath;

  if ((DevicePath == NULL) || (*DevicePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if (FileGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether the device path point to the default the input Fv file
  //
  TempDevicePath = *DevicePath;
  LastDeviceNode = TempDevicePath;
  while (!IsDevicePathEnd (TempDevicePath)) {
     LastDeviceNode = TempDevicePath;
     TempDevicePath = NextDevicePathNode (TempDevicePath);
  }
  GuidPoint = EfiGetNameGuidFromFwVolDevicePathNode (
                (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) LastDeviceNode
                );
  if (GuidPoint == NULL) {
    //
    // if this option does not points to a Fv file, just return EFI_UNSUPPORTED
    //
    return EFI_UNSUPPORTED;
  }
  if (!CompareGuid (GuidPoint, FileGuid)) {
    //
    // If the Fv file is not the input file guid, just return EFI_UNSUPPORTED
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Check whether the input Fv file device path is valid
  //
  TempDevicePath = *DevicePath;
  FoundFvHandle = NULL;
  Status = gBS->LocateDevicePath (
                  &gEfiFirmwareVolume2ProtocolGuid,
                  &TempDevicePath,
                  &FoundFvHandle
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (
                    FoundFvHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &Fv
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Set FV ReadFile Buffer as NULL, only need to check whether input Fv file exist there
      //
      Status = Fv->ReadFile (
                    Fv,
                    FileGuid,
                    NULL,
                    &Size,
                    &Type,
                    &Attributes,
                    &AuthenticationStatus
                    );
      if (!EFI_ERROR (Status)) {
        return EFI_ALREADY_STARTED;
      }
    }
  }

  //
  // Look for the input wanted FV file in current FV
  // First, try to look for in Bds own FV. Bds and input wanted FV file usually are in the same FV
  //
  FindFvFile = FALSE;
  FoundFvHandle = NULL;
  Status = gBS->HandleProtocol (
             gImageHandle,
             &gEfiLoadedImageProtocolGuid,
             (VOID **) &LoadedImage
             );
  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (
                    LoadedImage->DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &Fv
                    );
    if (!EFI_ERROR (Status)) {
      Status = Fv->ReadFile (
                    Fv,
                    FileGuid,
                    NULL,
                    &Size,
                    &Type,
                    &Attributes,
                    &AuthenticationStatus
                    );
      if (!EFI_ERROR (Status)) {
        FindFvFile = TRUE;
        FoundFvHandle = LoadedImage->DeviceHandle;
      }
    }
  }
  //
  // Second, if fail to find, try to enumerate all FV
  //
  if (!FindFvFile) {
    FvHandleBuffer = NULL;
    gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiFirmwareVolume2ProtocolGuid,
          NULL,
          &FvHandleCount,
          &FvHandleBuffer
          );
    for (Index = 0; Index < FvHandleCount; Index++) {
      gBS->HandleProtocol (
            FvHandleBuffer[Index],
            &gEfiFirmwareVolume2ProtocolGuid,
            (VOID **) &Fv
            );

      Status = Fv->ReadFile (
                    Fv,
                    FileGuid,
                    NULL,
                    &Size,
                    &Type,
                    &Attributes,
                    &AuthenticationStatus
                    );
      if (EFI_ERROR (Status)) {
        //
        // Skip if input Fv file not in the FV
        //
        continue;
      }
      FindFvFile = TRUE;
      FoundFvHandle = FvHandleBuffer[Index];
      break;
    }

    if (FvHandleBuffer != NULL) {
      FreePool (FvHandleBuffer);
    }
  }

  if (FindFvFile) {
    //
    // Build the shell device path
    //
    NewDevicePath = DevicePathFromHandle (FoundFvHandle);
    EfiInitializeFwVolDevicepathNode (&FvFileNode, FileGuid);
    NewDevicePath = AppendDevicePathNode (NewDevicePath, (EFI_DEVICE_PATH_PROTOCOL *) &FvFileNode);
    *DevicePath = NewDevicePath;
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}
