/*
Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

*/

#include <Library/IoLib.h>

#include "macosx.h"
#include "kernel_patcher.h"

EFI_EVENT   mVirtualAddressChangeEvent = NULL;
EFI_EVENT   OnReadyToBootEvent = NULL;
EFI_EVENT   ExitBootServiceEvent = NULL;

EFI_STATUS
USBOwnerFix(
  VOID
  )
/*++
 
 Routine Description:
 Disable the USB legacy Support in all Ehci and Uhci.
 This function assume all PciIo handles have been created in system.
 Slice - added also OHCI and more advanced algo. Better then known to Intel and Apple :)
 Arguments:
 None
 
 Returns:
 EFI_SUCCESS
 EFI_NOT_FOUND
 --*/
{
  EFI_STATUS          Status = EFI_SUCCESS;
  EFI_HANDLE          *HandleArray = NULL;
  UINTN             HandleArrayCount = 0;
  UINTN             Index;
  EFI_PCI_IO_PROTOCOL      *PciIo;
  PCI_TYPE00              Pci;
  UINT16            Command;
  UINT32            HcCapParams;
  UINT32            ExtendCap;
  UINT32            Value;
  UINT32            TimeOut;
  UINT32            Base;
  UINT32            PortBase;
  volatile UINT32        opaddr;
  UINT32            usbcmd, usbsts, usbintr;
  UINT32            usblegsup, usblegctlsts;
  UINTN             isOSowned;
  UINTN             isBIOSowned;
  BOOLEAN            isOwnershipConflict;
  
  //
  // Find the usb host controller
  //
  Status = gBS->LocateHandleBuffer (
            ByProtocol,
            &gEfiPciIoProtocolGuid,
            NULL,
            &HandleArrayCount,
            &HandleArray
            );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleArrayCount; Index++) {
      Status = gBS->HandleProtocol (
                HandleArray[Index],
                &gEfiPciIoProtocolGuid,
                (VOID **)&PciIo
                );
      if (!EFI_ERROR (Status)) {
        //
        // Find the USB host controller
        //
        Status = PciIo->Pci.Read (
                  PciIo,
                  EfiPciIoWidthUint32,
                  0,
                  sizeof (Pci) / sizeof (UINT32),
                  &Pci
                  );
        
        if (!EFI_ERROR (Status)) {
          if ((PCI_CLASS_SERIAL == Pci.Hdr.ClassCode[2]) &&
              (PCI_CLASS_SERIAL_USB == Pci.Hdr.ClassCode[1])) {
            switch (Pci.Hdr.ClassCode[0]) {
              case PCI_IF_UHCI:
                //
                // Found the UHCI, then disable the legacy support
                //
                Base = 0;
                Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, 0x20, 1, &Base);
                PortBase = (Base >> 5) & 0x07ff;
                Command = 0x8f00;
                Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0xC0, 1, &Command);
                if (PortBase) {
                  IoWrite16 (PortBase, 0x0002);
                  gBS->Stall (500);
                  IoWrite16 (PortBase+4, 0);
                  gBS->Stall (500);
                  IoWrite16 (PortBase, 0);
                }
                break;
              case PCI_IF_EHCI:
                //
                // Found the EHCI, then disable the legacy support
                //
                Value = 0x0002;
                PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0x04, 1, &Value);
                
                Base = 0;
                Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, 0x10, 1, &Base);
                if (*((UINT8*)(UINTN)Base) < 0x0C) {
                  break;
                }
                opaddr = Base + *((UINT8*)(UINTN)(Base));
                // eecp = EHCI Extended Capabilities offset = capaddr HCCPARAMS bits 15:8
                //UEFI
                Status = PciIo->Mem.Read (
                          PciIo,
                          EfiPciIoWidthUint32,
                          0,                   //EHC_BAR_INDEX
                          (UINT64) 0x08,       //EHC_HCCPARAMS_OFFSET
                          1,
                          &HcCapParams
                          );
                
                ExtendCap = (HcCapParams >> 8) & 0xFF;
                
                usbcmd = *((UINT32*)(UINTN)(opaddr));      // Command Register
                usbsts = *((UINT32*)(UINTN)(opaddr + 4));    // Status Register
                usbintr = *((UINT32*)(UINTN)(opaddr + 8));    // Interrupt Enable Register
                
                // read PCI Config 32bit USBLEGSUP (eecp+0)
                Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &usblegsup);
                
                // informational only
                isBIOSowned = !!((usblegsup) & (1 << (16)));
                isOSowned = !!((usblegsup) & (1 << (24)));
                
                // read PCI Config 32bit USBLEGCTLSTS (eecp+4)
                PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &usblegctlsts);
                //
                // Disable the SMI in USBLEGCTLSTS firstly
                //
                usblegctlsts &= 0xFFFF0000;
                PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &usblegctlsts);
                
                //double
                // if delay value is in milliseconds it doesn't appear to work.
                // setting value to anything up to 65535 does not add the expected delay here.
                gBS->Stall (500);
                usbcmd = *((UINT32*)(UINTN)(opaddr));      // Command Register
                usbsts = *((UINT32*)(UINTN)(opaddr + 4));    // Status Register
                usbintr = *((UINT32*)(UINTN)(opaddr + 8));    // Interrupt Enable Register
                
                // clear registers to default
                usbcmd = (usbcmd & 0xffffff00);
                *((UINT32*)(UINTN)(opaddr)) = usbcmd;
                *((UINT32*)(UINTN)(opaddr + 8)) = 0;      //usbintr - clear interrupt registers
                *((UINT32*)(UINTN)(opaddr + 4)) = 0x1000;    //usbsts - clear status registers
                Value = 1;
                PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
                
                // get the results
                usbcmd = *((UINT32*)(UINTN)(opaddr));      // Command Register
                usbsts = *((UINT32*)(UINTN)(opaddr + 4));    // Status Register
                usbintr = *((UINT32*)(UINTN)(opaddr + 8));    // Interrupt Enable Register
                
                // read 32bit USBLEGSUP (eecp+0)
                PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &usblegsup);
                
                // informational only
                isBIOSowned = !!((usblegsup) & (1 << (16)));
                isOSowned = !!((usblegsup) & (1 << (24)));
                
                // read 32bit USBLEGCTLSTS (eecp+4)
                PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &usblegctlsts);
                //
                // Get EHCI Ownership from legacy bios
                //
                PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &usblegsup);
                isOwnershipConflict = isBIOSowned && isOSowned;
                if (isOwnershipConflict) {
                  Value = 0;
                  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, ExtendCap + 3, 1, &Value);
                  TimeOut = 40;
                  while (TimeOut--) {
                    gBS->Stall (500);
                    
                    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
                    
                    if ((Value & 0x01000000) == 0x0) {
                      break;
                    }
                  }
                }
                
                PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
                Value |= (0x1 << 24);
                PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
                
                TimeOut = 40;
                while (TimeOut--) {
                  gBS->Stall (500);
                  
                  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
                  
                  if ((Value & 0x00010000) == 0x0) {
                    break;
                  }
                }
                isOwnershipConflict = ((Value & 0x00010000) != 0x0);
                if (isOwnershipConflict) {
                  // Soft reset has failed. Assume SMI being ignored
                  // Hard reset
                  Value = 0;
                  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, ExtendCap + 2, 1, &Value);
                  TimeOut = 40;
                  while (TimeOut--) {
                    gBS->Stall (500);
                    
                    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
                    
                    if ((Value & 0x00010000) == 0x0) {
                      break;
                    }
                  }
                  
                  // Disable further SMI events
                  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &usblegctlsts);
                  usblegctlsts &= 0xFFFF0000;
                  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &usblegctlsts);
                }                
                if (Value & 0x00010000) {          
                  Status = EFI_NOT_FOUND;
                  break;
                }
                
                break;
              default:
                break;
            }
          } 
        }
      }
    }
  } else {
    return Status;
  }
  gBS->FreePool (HandleArray);
  return Status;
}

VOID
EFIAPI
OnExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *Context
)
{
#if 0
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

  DisableUsbLegacySupport();
#endif
  gST->ConOut->OutputString (gST->ConOut, L"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	//
	// Patch kernel and kexts if needed
	//
	KernelAndKextsPatcherStart ();

  USBOwnerFix();
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
	EFI_STATUS			Status;
	VOID*           Registration = NULL;

	//
	// Register notify for exit boot services
	//
	Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  OnExitBootServices,
                  NULL,
                  &ExitBootServiceEvent
                );

	if(!EFI_ERROR(Status)) {
		Status = gBS->RegisterProtocolNotify (
                    &gEfiStatusCodeRuntimeProtocolGuid,
                    ExitBootServiceEvent,
                    &Registration
                  );
    }

#if 0
  //
  // Register the event to reclaim variable for OS usage.
  //
  EfiCreateEventReadyToBoot (&OnReadyToBootEvent);
  EfiCreateEventReadyToBootEx (
    TPL_NOTIFY, 
    OnReadyToBoot, 
    NULL, 
    &OnReadyToBootEvent
    );
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
#endif

  return EFI_SUCCESS;
}
