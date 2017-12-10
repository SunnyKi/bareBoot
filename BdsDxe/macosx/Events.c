/*
Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

*/

#include <macosx.h>
#include <Library/IoLib.h>

#include "kernel_patcher.h"

EFI_EVENT mVirtualAddressChangeEvent = NULL;
EFI_EVENT OnReadyToBootEvent = NULL;
EFI_EVENT ExitBootServiceEvent = NULL;

VOID
  EFIAPI
OnExitBootServices (
  IN EFI_EVENT Event,
  IN VOID *Context
)
{
  if (!gIsHibernation) {
    //
    // Patch kernel and kexts if needed
    //
    KernelAndKextsPatcherStart ();
  }
#ifdef BOOT_DEBUG
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  UINTN                           NumberFileSystemHandles;
  EFI_HANDLE                      *FileSystemHandles;
  UINTN                           Index;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
  EFI_FILE_PROTOCOL               *FHandle;
  EFI_STATUS                      Status;
  
  DEBUG ((DEBUG_INFO, "%a: an attempt to save ext_boot.log\n", __FUNCTION__));
  FHandle = NULL;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL,
         &NumberFileSystemHandles,
         &FileSystemHandles
         );
  
  for (Index = 0; Index < NumberFileSystemHandles; Index++) {
    DevicePath  = DevicePathFromHandle (FileSystemHandles[Index]);
#if 0
    Print (L"%a: Device = %s\n",__FUNCTION__, ConvertDevicePathToText (DevicePath, FALSE, FALSE));
    Print (L"%a: DevicePath->Type = 0x%x, DevicePath->SubType = 0x%x\n",
           __FUNCTION__, DevicePath->Type, DevicePath->SubType);
#endif
    // at this stage attempt to write a file on the USB device ends with error 'Device Error'
    // so we will write to device that contains bareboot's config file and non USB
    // most likely it will first ESP
    // temporary workaround
 
    if (StrStr (ConvertDevicePathToText (DevicePath, FALSE, FALSE), L"USB") != NULL) {
      continue;
    }
    Status = gBS->HandleProtocol (
                    FileSystemHandles[Index],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID *) &Volume
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }
    Status = Volume->OpenVolume (Volume, &FHandle);
    if (EFI_ERROR (Status)) {
      continue;
    }
    if (gPNConfigPlist != NULL && FileExists (FHandle, gPNConfigPlist)) {
      break;
    }
  }
  
  DBG ("%a: Finished.\n", __FUNCTION__);
  if (FHandle != NULL) {
    Status = SaveBooterLog (FHandle, BB_HOME_DIR L"ext_boot.log");
    FHandle->Close (FHandle);
  }
#endif

#if defined(KERNEL_PATCH_DEBUG) || defined(KEXT_INJECT_DEBUG) || defined(KEXT_PATCH_DEBUG)
    gBS->Stall (5000000);
#endif
}

VOID
  EFIAPI
OnReadyToBoot (
  IN EFI_EVENT Event,
  IN VOID *Context
)
{
  //
}

VOID
  EFIAPI
VirtualAddressChangeEvent (
  IN EFI_EVENT Event,
  IN VOID *Context
)
{
#if 0
  EfiConvertPointer (0x0, (VOID **) &mProperty);
  EfiConvertPointer (0x0, (VOID **) &mSmmCommunication);
#endif
}

EFI_STATUS
  EFIAPI
EventsInitialize (
  VOID
)
{
  EFI_STATUS Status;
  VOID *Registration = NULL;

  //
  // Register notify for exit boot services
  //
  Status =
    gBS->CreateEvent (EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_CALLBACK,
                      OnExitBootServices, NULL, &ExitBootServiceEvent);

  if (!EFI_ERROR (Status)) {
    Status =
      gBS->RegisterProtocolNotify (&gEfiStatusCodeRuntimeProtocolGuid,
                                   ExitBootServiceEvent, &Registration);
  }

#if 0
  //
  // Register the event to reclaim variable for OS usage.
  //
  EfiCreateEventReadyToBoot (&OnReadyToBootEvent);
  EfiCreateEventReadyToBootEx (TPL_NOTIFY, OnReadyToBoot, NULL,
                               &OnReadyToBootEvent);
  gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, OnExitBootServices, NULL,
                      &gEfiEventExitBootServicesGuid, &ExitBootServiceEvent);
  //
  // Register the event to convert the pointer for runtime.
  //
  gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, VirtualAddressChangeEvent,
                      NULL, &gEfiEventVirtualAddressChangeGuid,
                      &mVirtualAddressChangeEvent);
#endif

  return EFI_SUCCESS;
}
