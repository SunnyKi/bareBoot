/*++

Copyright (c) 2013, (to be filled later). All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  BdsUsbLegacy.c

Abstract:

  This file include USB Legacy handling code

--*/

#include "InternalBdsLib.h"

VOID
DisableEhciLegacy (
  EFI_PCI_IO_PROTOCOL       *PciIo,
  PCI_TYPE00                *Pci
  )
{
  EFI_STATUS Status;
  UINT64     pollResult;
  UINT32     HcCapParams;
  UINT32     ExtendCap;
  UINT8      ExtendCapPtr;

  DEBUG ((DEBUG_INFO, "%a: enter\n", __FUNCTION__));
  Status = PciIo->Mem.Read (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0,                   //EHC_BAR_INDEX
                       (UINT64) 0x08,       //EHC_HCCPARAMS_OFFSET
                       1,
                       &HcCapParams
                       );
  
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: bail out (reading hccparams: %r)\n", __FUNCTION__, Status));
    return;
  }
  /* Some controllers has debug port capabilities, so search for EHCI_EC_LEGSUP */
  for (ExtendCapPtr = (UINT8) (HcCapParams >> 8); ExtendCapPtr != 0; ExtendCapPtr = (UINT8) (ExtendCap >> 8)) {
    DEBUG ((DEBUG_INFO, "%a: extendcapptr 0x%08x\n", __FUNCTION__, (UINT32)ExtendCapPtr));
    Status = PciIo->Pci.Read (
                       PciIo,
                       EfiPciIoWidthUint32,
                       ExtendCapPtr,
                       1,
                       &ExtendCap
                       );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a: bail out (reading extendcap at 0x%x: %r)\n", __FUNCTION__, ExtendCapPtr, Status));
      return;
    }
    DEBUG ((DEBUG_INFO, "%a: extendcap 0x%08x\n", __FUNCTION__, ExtendCap));
    if ((UINT8) ExtendCap != 0x01) {
      /* Not EHCI_EC_LEGSUP */
      continue;
    }

#define BIOS_OWNED (1 << 16)
#define OS_OWNED (1 << 24)

    if ((ExtendCap & BIOS_OWNED) != 0) {
      ExtendCap = OS_OWNED;
      Status = PciIo->Pci.Write (
                       PciIo,
                       EfiPciIoWidthUint32,
                       ExtendCapPtr,
                       1,
                       &ExtendCap
                       );
      
      Status = PciIo->Flush (PciIo);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "%a: bail out (flush: %r)\n", __FUNCTION__, Status));
        return;
      }
      Status = PciIo->PollIo (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0,                     // EHCI_BAR_INDEX
                       (UINT64) ExtendCapPtr, // USBLEGSUP
                       (UINT64) (BIOS_OWNED | OS_OWNED),
		       (UINT64) OS_OWNED,
		       (UINT64) 4000000,  // wait that number of 100ns units
                       &pollResult
                       );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "%a: bail out (wating for ownership change: %r, result 0x%X)\n", __FUNCTION__, Status, pollResult));
      }
      return;
    }
#undef BIOS_OWNED
#undef OS_OWNED
  }
#if 0
  UINT32                    HcCapParams;
  UINT32                    ExtendCap;
  UINT32                    Value;
  UINT32                    TimeOut;
#ifdef USB_FIXUP
  UINT32                    SaveValue;
#endif
  Status = PciIo->Mem.Read (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0,                   //EHC_BAR_INDEX
                       (UINT64) 0x08,       //EHC_HCCPARAMS_OFFSET
                       1,
                       &HcCapParams
                       );
  
  ExtendCap = (HcCapParams >> 8) & 0xFF;
  //
  // Disable the SMI in USBLEGCTLSTS firstly
  //
#ifdef USB_FIXUP
  DEBUG ((DEBUG_INFO, "EHCI: EECP = 0x%X, USBLEGCTLSTS = 0x%X", ExtendCap, (ExtendCap + 4)));
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &SaveValue);
  Value = SaveValue & 0xFFFF0000;
  DEBUG ((DEBUG_INFO, ", Save val = 0x%X, Write val = 0x%X", SaveValue, Value));
#else
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);
  Value &= 0xFFFF0000;
#endif
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);

  //
  // Get EHCI Ownership from legacy bios
  //
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
  Value |= (0x1 << 24);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

  TimeOut = 40;
  while (TimeOut--) {
    gBS->Stall (500);

    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

    if ((Value & 0x01010000) == 0x01000000) {
      break;
    }
  }
#ifdef USB_FIXUP
#if 0
  SaveValue &= ~0x003F;
#endif
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);
  DEBUG ((DEBUG_INFO, ", New val = 0x%X\n", Value));
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &SaveValue);
#endif
#endif
  DEBUG ((DEBUG_INFO, "%a: leave\n", __FUNCTION__));
}

VOID
DisableOhciLegacy (
  EFI_PCI_IO_PROTOCOL       *PciIo,
  PCI_TYPE00                *Pci
  )
{
  EFI_STATUS Status;
  UINT64     pollResult;
  struct {
    UINT32     HcRevision;
    UINT32     HcControl;
    UINT32     HcCommandStatus;
  } OpRegs;

  DEBUG ((DEBUG_INFO, "%a: enter\n", __FUNCTION__));
  Status = PciIo->Mem.Read (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0,                // OHCI_BAR_INDEX
                       (UINT64) 0,       // OHCI_HC_OP_REGS
                       sizeof (OpRegs) / sizeof (UINT32),
                       &OpRegs
                       );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: bail out (reading opregs: %r)\n", __FUNCTION__, Status));
    return;
  }
  DEBUG ((DEBUG_INFO, "%a: revision 0x%08x\n", __FUNCTION__, OpRegs.HcRevision));
  DEBUG ((DEBUG_INFO, "%a: control 0x%08x\n", __FUNCTION__, OpRegs.HcControl));
  DEBUG ((DEBUG_INFO, "%a: cmd & status 0x%08x\n", __FUNCTION__, OpRegs.HcCommandStatus));
  if ((OpRegs.HcControl & 0x100) == 0) {
    /* No SMM driver active */
#if 0
    /* XXX: Let see the HostControllerFunctionalState for BIOS driver */
    switch (OpRegs.HcControl & 0xC0) {
    case 0x00: /* UsbReset */
      break;
    case 0x40: /* UsbResume */
      break;
    case 0x80: /* UsbOperational */
      break;
    case 0xC0: /* UsbSuspend */
      break;
    }
#endif
    return;
  }
  /* Time to do little dance with SMM driver */
  OpRegs.HcCommandStatus = 0x80;
  Status = PciIo->Mem.Write (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0,                // OHCI_BAR_INDEX
                       (UINT64) 8,       // OHCI_HC_COMMANDSTATUS
                       1,
                       &OpRegs.HcCommandStatus
                       );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: bail out (setting ownership change bit: %r)\n", __FUNCTION__, Status));
    return;
  }
  Status = PciIo->Flush (PciIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: bail out (flush: %r)\n", __FUNCTION__, Status));
    return;
  }
  Status = PciIo->PollMem (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0,                // OHCI_BAR_INDEX
                       (UINT64) 4,       // OHCI_HC_CONTROL
                       (UINT64) 0x100,   // check InterruptRouting bit
		       (UINT64) 0,       // we waiting for that
		       (UINT64) 4000000,  // wait that number of 100ns units
                       &pollResult
                       );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: bail out (wating for interruptrouting bit clear: %r, result 0x%X)\n", __FUNCTION__, Status, pollResult));
  }

  DEBUG ((DEBUG_INFO, "%a: leave\n", __FUNCTION__));
}

VOID
DisableUhciLegacy (
  EFI_PCI_IO_PROTOCOL       *PciIo,
  PCI_TYPE00                *Pci
  )
{
  DEBUG ((DEBUG_INFO, "%a: enter\n", __FUNCTION__));
#if 0
  Command = 0;
  Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0xC0, 1, &Command);
#endif
  DEBUG ((DEBUG_INFO, "%a: leave\n", __FUNCTION__));
}

EFI_STATUS
DisableUsbLegacySupport (
  VOID
  )
{
#if 0
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;
  
  /* Avoid double action, if already done in usb drivers */

  if (FeaturePcdGet (PcdTurnOffUsbLegacySupport)) {
    return EFI_SUCCESS;
  }

  //
  // Start to check all the PciIo to find all possible usb device(s)
  //
  HandleCount = 0;
  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID **)&PciIo);
    if (EFI_ERROR (Status)) {
      continue;
    }
    Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0, sizeof (Pci) / sizeof (UINT32), &Pci);
    if (EFI_ERROR (Status)) {
      continue;
    }
    if (IS_PCI_USB (&Pci)) {
      switch (Pci.Hdr.ClassCode[0]) {
      case PCI_IF_EHCI:
        DisableEhciLegacy (PciIo, &Pci);
        break;
      case PCI_IF_OHCI:
        DisableOhciLegacy (PciIo, &Pci);
        break;
      case PCI_IF_UHCI:
        DisableUhciLegacy (PciIo, &Pci);
        break;
      default:
        break;
      }
    }
  }
  gBS->FreePool (HandleBuffer);
#else
  EFI_STATUS                            Status;
  EFI_HANDLE                            *HandleArray;
  UINTN                                 HandleArrayCount;
  UINTN                                 Index;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  UINT8                                 Class[3];
  UINT16                                Command;
  UINT32                                HcCapParams;
  UINT32                                ExtendCap;
  UINT32                                Value;
  UINT32                                TimeOut;
#ifdef USB_FIXUP
  UINT32                                SaveValue;
#endif
  
  /* Avoid double action, if already done in usb drivers */

  if (FeaturePcdGet (PcdTurnOffUsbLegacySupport)) {
    return EFI_SUCCESS;
  }

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
        // Find the USB host controller controller
        //
        Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x09, 3, &Class);
        if (!EFI_ERROR (Status)) {
          if ((PCI_CLASS_SERIAL == Class[2]) &&
              (PCI_CLASS_SERIAL_USB == Class[1])) {
            if (PCI_IF_UHCI == Class[0]) {
              //
              // Found the UHCI, then disable the legacy support
              //
              Command = 0;
              Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0xC0, 1, &Command);
            } else if (PCI_IF_EHCI == Class[0]) {
              //
              // Found the EHCI, then disable the legacy support
              //
              Status = PciIo->Mem.Read (
                                   PciIo,
                                   EfiPciIoWidthUint32,
                                   0,                   //EHC_BAR_INDEX
                                   (UINT64) 0x08,       //EHC_HCCPARAMS_OFFSET
                                   1,
                                   &HcCapParams
                                   );
              
              ExtendCap = (HcCapParams >> 8) & 0xFF;
              //
              // Disable the SMI in USBLEGCTLSTS firstly
              //
#ifdef USB_FIXUP
              DEBUG ((DEBUG_INFO, "EHCI: EECP = 0x%X, USBLEGCTLSTS = 0x%X", ExtendCap, (ExtendCap + 4)));
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &SaveValue);
              Value = SaveValue & 0xFFFF0000;
              DEBUG ((DEBUG_INFO, ", Save val = 0x%X, Write val = 0x%X", SaveValue, Value));
#else
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);
              Value &= 0xFFFF0000;
#endif
              PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);

              //
              // Get EHCI Ownership from legacy bios
              //
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
              Value |= (0x1 << 24);
              PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

              TimeOut = 40;
              while (TimeOut--) {
                gBS->Stall (500);

                PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

                if ((Value & 0x01010000) == 0x01000000) {
                  break;
                }
              }
#ifdef USB_FIXUP
#if 0
              SaveValue &= ~0x003F;
#endif
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);
              DEBUG ((DEBUG_INFO, ", New val = 0x%X\n", Value));
              PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &SaveValue);
#endif
            }
          }
        }
      }
    }
  } else {
    return Status;
  }
  gBS->FreePool (HandleArray);
#endif
  return EFI_SUCCESS;
}
