/*
Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

*/

#include "macosx.h"

EFI_EVENT   mVirtualAddressChangeEvent = NULL;
EFI_EVENT   OnReadyToBootEvent = NULL;
EFI_EVENT   ExitBootServiceEvent = NULL;


VOID
EFIAPI
OnExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *Context
)
{
  UINT8             *DTptr = (UINT8*) (UINTN) 0x100000;
  BootArgs1         *bootArgs;
  EFI_STATUS                      Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput = NULL;

  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **) &GraphicsOutput);

  if (EFI_ERROR (Status)) {
    Print (L"Cannot Find Graphics Output Protocol\n");
    gBS->Stall (2000000);
    return;
  }

  while (TRUE) {
    bootArgs = (BootArgs1*) DTptr;

    if ((bootArgs->Revision == 6 || bootArgs->Revision == 5 || bootArgs->Revision == 4) &&
    (bootArgs->Version == 1) && ((UINTN) bootArgs->efiMode == 32 || (UINTN) bootArgs->efiMode == 64)) {
      dtRoot = (CHAR8*) (UINTN) bootArgs->deviceTreeP;
      bootArgs->efiMode = archMode;
      break;
    }

    DTptr += 0x1000;

    if ((UINT32) (UINTN) DTptr > 0x3000000) {
      Print (L"bootArgs not found!\n");
      gBS->Stall (2000000);
      return;
    }
  }

#if 0
  USBOwnerFix();
  DisableUsbLegacySupport();
#endif
}

VOID
EFIAPI
OnReadyToBoot (
               IN      EFI_EVENT   Event,
               IN      VOID        *Context
               )
{
//
}

VOID
EFIAPI
VirtualAddressChangeEvent (
                           IN EFI_EVENT  Event,
                           IN VOID       *Context
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
  //
  // Register the event to reclaim variable for OS usage.
  //
#if 0
  EfiCreateEventReadyToBoot (&OnReadyToBootEvent);
  EfiCreateEventReadyToBootEx (
    TPL_NOTIFY, 
    OnReadyToBoot, 
    NULL, 
    &OnReadyToBootEvent
    );
#endif
  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         OnExitBootServices,
         NULL,
         &gEfiEventExitBootServicesGuid,
         &ExitBootServiceEvent
         ); 

  //
  // Register the event to convert the pointer for runtime.
  //
  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         VirtualAddressChangeEvent,
         NULL,
         &gEfiEventVirtualAddressChangeGuid,
         &mVirtualAddressChangeEvent
         );
  
  return EFI_SUCCESS;
}
