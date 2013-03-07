/** @file
  This module produce main entry for BDS phase - BdsEntry.
  When this module was dispatched by DxeCore, gEfiBdsArchProtocolGuid will be installed
  which contains interface of BdsEntry.
  After DxeCore finish DXE phase, gEfiBdsArchProtocolGuid->BdsEntry will be invoked
  to enter BDS phase.

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Bds.h"
#include "FrontPage.h"
//#include "HwErrRecSupport.h"
#include "GenericBds/macosx/macosx.h"

//#include "Hotkey.h"

///
/// BDS arch protocol instance initial value.
///
/// Note: Current BDS not directly get the BootMode, DefaultBoot,
/// TimeoutDefault, MemoryTestLevel value from the BDS arch protocol.
/// Please refer to the library useage of BdsLibGetBootMode, BdsLibGetTimeout
/// and PlatformBdsDiagnostics in BdsPlatform.c
///
EFI_HANDLE  gBdsHandle = NULL;

EFI_BDS_ARCH_PROTOCOL  gBds = {
  BdsEntry
};

UINT16                          *mBootNext = NULL;

/**

  Install Boot Device Selection Protocol

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  BDS has finished initializing.
                        Return the dispatcher and recall BDS.Entry
  @retval  Other        Return status from AllocatePool() or gBS->InstallProtocolInterface

**/
EFI_STATUS
EFIAPI
BdsInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS    Status;
  
  //gImageHandle  = ImageHandle;
  gRS           = SystemTable->RuntimeServices;

  //
  // Install protocol interface
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gBdsHandle,
                  &gEfiBdsArchProtocolGuid, &gBds,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**

  This function attempts to boot for the boot order specified
  by platform policy.

**/
VOID
BdsBootDeviceSelect (
  VOID
  )
{
  EFI_STATUS        Status;
  LIST_ENTRY        *Link;
  BDS_COMMON_OPTION *BootOption;
  UINTN             ExitDataSize;
  CHAR16            *ExitData;
  UINT16            Timeout;
  LIST_ENTRY        BootLists;
  CHAR16            Buffer[20];
  BOOLEAN           BootNextExist;
  LIST_ENTRY        *LinkBootNext;

  BootNextExist = FALSE;
  LinkBootNext  = NULL;
  InitializeListHead (&BootLists);
  ZeroMem (Buffer, sizeof (Buffer));
  if (mBootNext != NULL) {
    BootNextExist = TRUE;
    gRT->SetVariable (
          L"BootNext",
          &gEfiGlobalVariableGuid,
          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
          0,
          mBootNext
          );
    UnicodeSPrint (Buffer, sizeof (Buffer), L"Boot%04x", *mBootNext);
    BootOption = BdsLibVariableToOption (&BootLists, Buffer);
    if (BootOption == NULL) {
      return;
    }
    BootOption->BootCurrent = *mBootNext;
  }
  BdsLibBuildOptionFromVar (&BootLists, L"BootOrder");
  if (IsListEmpty (&BootLists)) {
    BdsLibEnumerateAllBootOption (&BootLists);
  }
  Link = BootLists.ForwardLink;
  if (Link == NULL) {
    return ;
  }
  for (;;) {
    if (Link == &BootLists) {
      Timeout = 0xffff;
      PlatformBdsEnterFrontPage (Timeout, FALSE);
      InitializeListHead (&BootLists);
      BdsLibBuildOptionFromVar (&BootLists, L"BootOrder");
      Link = BootLists.ForwardLink;
      continue;
    }
    //
    // Get the boot option from the link list
    //
    BootOption = CR (Link, BDS_COMMON_OPTION, Link, BDS_LOAD_OPTION_SIGNATURE);
    if (!IS_LOAD_OPTION_TYPE (BootOption->Attribute, LOAD_OPTION_ACTIVE)) {
      Link = Link->ForwardLink;
      continue;
    }
    if (DevicePathType (BootOption->DevicePath) != BBS_DEVICE_PATH) {
      BdsLibConnectDevicePath (BootOption->DevicePath);
    }
    if (StrCmp(gSettings.DefaultBoot, BootOption->Description) == 0) 
      Status = BdsLibBootViaBootOption (BootOption, BootOption->LoadOptions, BootOption->DevicePath, &ExitDataSize, &ExitData);
      else Status = EFI_NOT_FOUND;    
    if (Status != EFI_SUCCESS) {
      Link = Link->ForwardLink;
    } else {
      BootOption->StatusString = GetStringById (STRING_TOKEN (STR_BOOT_SUCCEEDED));
      PlatformBdsBootSuccess (BootOption);
      Timeout = 0xffff;
      PlatformBdsEnterFrontPage (Timeout, FALSE);
      if (BootNextExist) {
        LinkBootNext = BootLists.ForwardLink;
      }
      InitializeListHead (&BootLists);
      if (LinkBootNext != NULL) {
        InsertTailList (&BootLists, LinkBootNext);
      }
      BdsLibBuildOptionFromVar (&BootLists, L"BootOrder");
      Link = BootLists.ForwardLink;
    }
  }

}

/**

  Service routine for BdsInstance->Entry(). Devices are connected, the
  consoles are initialized, and the boot options are tried.

  @param This             Protocol Instance structure.

**/
VOID
EFIAPI
BdsEntry (
  IN EFI_BDS_ARCH_PROTOCOL  *This
  )
{
  LIST_ENTRY                      BootOptionList;
  UINTN                           BootNextSize;
  CHAR16                          *FirmwareVendor;

  InitializeListHead (&BootOptionList);
//  InitializeHotkeyService ();
  FirmwareVendor = (CHAR16 *)PcdGetPtr (PcdFirmwareVendor);
  gST->FirmwareVendor = AllocateRuntimeCopyPool (StrSize (FirmwareVendor), FirmwareVendor);
  ASSERT (gST->FirmwareVendor != NULL);
  gST->FirmwareRevision = PcdGet32 (PcdFirmwareRevision);
  gBS->CalculateCrc32 ((VOID *)gST, sizeof(EFI_SYSTEM_TABLE), &gST->Hdr.CRC32);
  PlatformBdsInit ();
  InitializeStringSupport ();
//  InitializeFrontPage ();
  InitializeBootManager ();
  mBootNext = BdsLibGetVariableAndSize (
                L"BootNext",
                &gEfiGlobalVariableGuid,
                &BootNextSize
                );
  PlatformBdsPolicyBehavior (&BootOptionList, BdsMemoryTest);
  BdsBootDeviceSelect ();
  ASSERT (FALSE);
  return ;
}
