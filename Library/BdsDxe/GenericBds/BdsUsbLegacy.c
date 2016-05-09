/*++

Copyright (c) 2013-2016, (to be filled later). All rights reserved.
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
  UINT32     ExtendCap;
  UINT32     HcCapParams;
  UINT32     TimeOut;
  UINT32     UsbLegCtlSts;
  UINT32     UsbLegSup;
  UINT8      ExtendCapPtr;

  DEBUG ((DEBUG_INFO, "%a: enter for %04x&%04x\n", __FUNCTION__, Pci->Hdr.VendorId, Pci->Hdr.DeviceId));
  Status = PciIo->Mem.Read (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0,                   //EHCI_BAR_INDEX
                       (UINT64) 0x08,       //EHCI_HCCPARAMS_OFFSET
                       1,
                       &HcCapParams
                       );
  
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: bail out (reading hccparams: %r)\n", __FUNCTION__, Status));
    return;
  }
  /* Some controllers has debug port capabilities also, so search for EHCI_EC_LEGSUP */
  for (ExtendCapPtr = (UINT8) (HcCapParams >> 8); ExtendCapPtr != 0; ExtendCapPtr = (UINT8) (ExtendCap >> 8)) {
    Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCapPtr, 1, &ExtendCap);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a: bail out (reading extendcap at 0x%x: %r)\n", __FUNCTION__, ExtendCapPtr, Status));
      return;
    }
    if ((UINT8) ExtendCap != 0x01) {
      /* Not EHCI_EC_LEGSUP */
      continue;
    }

    UsbLegSup = ExtendCap;
    /* Read CtlSts to dismiss possible pending interrupts */
    (void) PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCapPtr + 4, 1, &UsbLegCtlSts);

#define BIOS_OWNED BIT16
#define OS_OWNED   BIT24

    DEBUG ((DEBUG_INFO, "%a: usblegsup before 0x%08x\n", __FUNCTION__, UsbLegSup));
    if ((UsbLegSup & BIOS_OWNED) != 0) {
      DEBUG ((DEBUG_INFO, "%a: usblegctlsts before 0x%08x\n", __FUNCTION__, UsbLegCtlSts));
      UsbLegSup |= OS_OWNED;
      (void) PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCapPtr, 1, &UsbLegSup);
      Status = PciIo->Flush (PciIo);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "%a: bail out (flush: %r)\n", __FUNCTION__, Status));
        return;
      }
      TimeOut = 40;
      while (TimeOut-- != 0) {
        gBS->Stall (500);
    
        (void) PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCapPtr, 1, &UsbLegSup);
    
        if ((UsbLegSup & (BIOS_OWNED | OS_OWNED)) == OS_OWNED) {
          (void) PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCapPtr + 4, 1, &UsbLegCtlSts);
          DEBUG ((DEBUG_INFO, "%a: usblegctlsts in middle 0x%08x\n", __FUNCTION__, UsbLegCtlSts));
          (void) PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCapPtr + 4, 1, &UsbLegCtlSts);
          break;
        }
      }
      /* Read back registers to dismiss pending interrupts */
      (void) PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCapPtr + 4, 1, &UsbLegCtlSts);
      DEBUG ((DEBUG_INFO, "%a: usblegsup after 0x%08x\n", __FUNCTION__, UsbLegSup));
      DEBUG ((DEBUG_INFO, "%a: usblegctlsts after 0x%08x\n", __FUNCTION__, UsbLegCtlSts));
    } else {
      DEBUG ((DEBUG_INFO, "%a: no legacy on device\n", __FUNCTION__));
    }
    break;
#undef BIOS_OWNED
#undef OS_OWNED
  }
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
    UINT32     HcControl;
    UINT32     HcCommandStatus;
  } OpRegs;

  DEBUG ((DEBUG_INFO, "%a: enter for %04x&%04x\n", __FUNCTION__, Pci->Hdr.VendorId, Pci->Hdr.DeviceId));
  Status = PciIo->Mem.Read (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0,                // OHCI_BAR_INDEX
                       (UINT64) 4,       // OHCI_HC_CONTROL_OFFSET
                       sizeof (OpRegs) / sizeof (UINT32),
                       &OpRegs
                       );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: bail out (reading opregs: %r)\n", __FUNCTION__, Status));
    return;
  }

  if ((OpRegs.HcControl & 0x100) == 0) {
    /* No SMM driver active */
    DEBUG ((DEBUG_INFO, "%a: no SMM (legacy) on device\n", __FUNCTION__));
    /* XXX: Let see the HostControllerFunctionalState for BIOS driver */
    switch (OpRegs.HcControl & 0xC0) {
    case 0x00: /* UsbReset */
      DEBUG ((DEBUG_INFO, "%a: BIOS driver check: device is in UsbReset state\n", __FUNCTION__));
      break;
    case 0x40: /* UsbResume */
      DEBUG ((DEBUG_INFO, "%a: BIOS driver check: device is in UsbResume state\n", __FUNCTION__));
      break;
    case 0x80: /* UsbOperational */
      DEBUG ((DEBUG_INFO, "%a: BIOS driver check: device is in UsbOperational state\n", __FUNCTION__));
      break;
    case 0xC0: /* UsbSuspend */
      DEBUG ((DEBUG_INFO, "%a: BIOS driver check: device is in UsbSuspend state\n", __FUNCTION__));
      break;
    }
    return;
  }

  DEBUG ((DEBUG_INFO, "%a: SMM (legacy) on device\n", __FUNCTION__));

  /* Time to do little dance with SMM driver */
  OpRegs.HcCommandStatus = 0x08; // Ownership Change Request
  Status = PciIo->Mem.Write (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0,                // OHCI_BAR_INDEX
                       (UINT64) 8,       // OHCI_HC_COMMANDSTATUS
                       1,
                       &OpRegs.HcCommandStatus
                       );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: bail out (setting ownership change request bit: %r)\n", __FUNCTION__, Status));
    return;
  }

  MemoryFence (); // Just to be sure

  Status = PciIo->PollMem (
                       PciIo,
                       EfiPciIoWidthUint32,
                       0,                // OHCI_BAR_INDEX
                       (UINT64) 4,       // OHCI_HC_CONTROL
                       (UINT64) 0x100,   // check InterruptRouting bit
                       (UINT64) 0,       // we waiting for that
                       (UINT64) 10000,  // wait that number of 100ns units
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
  EFI_STATUS Status;
  UINT16     Command;

  DEBUG ((DEBUG_INFO, "%a: enter for %04x&%04x\n", __FUNCTION__, Pci->Hdr.VendorId, Pci->Hdr.DeviceId));

  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 0xC0, 1, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: bail out (read usb_legsup: %r)\n", __FUNCTION__, Status));
    return;
  }
  DEBUG ((DEBUG_INFO, "%a: legsup 0x%04x\n", __FUNCTION__, Command));

  /*
   * XXX: Is the device in legacy mode?
   * (need to read more specs and other implementations)
   * Let go blind
   */

  Command = 0;  /* No need in PIRQD, as efi runs in polling mode */
  Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0xC0, 1, &Command);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: bail out (write usb_legsup: %r)\n", __FUNCTION__, Status));
    return;
  }

  DEBUG ((DEBUG_INFO, "%a: leave\n", __FUNCTION__));
}

VOID
DisableXhciLegacy (
  EFI_PCI_IO_PROTOCOL       *PciIo,
  PCI_TYPE00                *Pci
  )
{
  DEBUG ((DEBUG_INFO, "%a: enter for %04x&%04x\n", __FUNCTION__, Pci->Hdr.VendorId, Pci->Hdr.DeviceId));
  DEBUG ((DEBUG_INFO, "%a: leave\n", __FUNCTION__));
}

EFI_STATUS
DisableUsbLegacySupport (
  VOID
  )
{
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
      DBG ("%a: usb device %04x&%04x (0x%02x)\n", __FUNCTION__, Pci.Hdr.VendorId, Pci.Hdr.DeviceId, Pci.Hdr.ClassCode[0]);
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
      case PCI_IF_XHCI:
        DisableXhciLegacy (PciIo, &Pci);
        break;
      default:
        break;
      }
    }
  }
  gBS->FreePool (HandleBuffer);
  return EFI_SUCCESS;
}
