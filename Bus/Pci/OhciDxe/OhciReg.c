/** @file

  The OHCI register operation routines.

Copyright (c) 2013, Nikolai Saoukh. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ohci.h"

/**
  Read 4-bytes width OHCI Operational register.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the 4-bytes width operational register.

  @return The register content read.
  @retval If err, return 0xFFFFFFFF.

**/
UINT32
OhcReadOpReg (
  IN  USB_OHCI_INSTANCE   *Ohc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  Status = Ohc->PciIo->Mem.Read (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             0xFEDC, /* XXX: nms */
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcReadOpReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Write the data to the 4-bytes width OHCI operational register.

  @param  Ohc      The OHCI Instance.
  @param  Offset   The offset of the 4-bytes width operational register.
  @param  Data     The data to write.

**/
VOID
OhcWriteOpReg (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  EFI_STATUS              Status;

  Status = Ohc->PciIo->Mem.Write (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             0xFEDC, /* XXX: nms */
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcWriteOpReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Set one bit of the operational register while keeping other bits.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
OhcSetOpRegBit (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = OhcReadOpReg (Ohc, Offset);
  Data |= Bit;
  OhcWriteOpReg (Ohc, Offset, Data);
}


/**
  Clear one bit of the operational register while keeping other bits.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to clear.

**/
VOID
OhcClearOpRegBit (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = OhcReadOpReg (Ohc, Offset);
  Data &= ~Bit;
  OhcWriteOpReg (Ohc, Offset, Data);
}

/**
  Wait the operation register's bit as specified by Bit
  to become set (or clear).

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the operation register.
  @param  Bit          The bit of the register to wait for.
  @param  WaitToSet    Wait the bit to set or clear.
  @param  Timeout      The time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The bit successfully changed by host controller.
  @retval EFI_TIMEOUT  The time out occurred.

**/
EFI_STATUS
OhcWaitOpRegBit (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Offset,
  IN UINT32               Bit,
  IN BOOLEAN              WaitToSet,
  IN UINT32               Timeout
  )
{
  UINT32                  Index;
  UINTN                   Loop;

  Loop   = (Timeout * OHC_1_MILLISECOND / OHC_POLL_DELAY) + 1;

  for (Index = 0; Index < Loop; Index++) {
    if (OHC_REG_BIT_IS_SET (Ohc, Offset, Bit) == WaitToSet) {
      return EFI_SUCCESS;
    }

    gBS->Stall (OHC_POLL_DELAY);
  }

  return EFI_TIMEOUT;
}

/**
  Set Bios Ownership

  @param  Ohc          The OHCI Instance.

**/
VOID
OhcSetBiosOwnership (
  IN USB_OHCI_INSTANCE    *Ohc
  )
{
  DEBUG ((EFI_D_INFO, "OhcSetBiosOwnership: called to set BIOS ownership\n"));
}

/**
  Clear Bios Ownership

  @param  Ohc       The OHCI Instance.

**/
VOID
OhcClearBiosOwnership (
  IN USB_OHCI_INSTANCE    *Ohc
  )
{
  DEBUG ((EFI_D_INFO, "OhcClearBiosOwnership: called to clear BIOS ownership\n"));
}

/**
  Whether the OHCI host controller is halted.

  @param  Ohc     The OHCI Instance.

  @retval TRUE    The controller is halted.
  @retval FALSE   It isn't halted.

**/
BOOLEAN
OhcIsHalt (
  IN USB_OHCI_INSTANCE    *Ohc
  )
{
  return OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, OHC_USBSTS_HALT);
}


/**
  Whether system error occurred.

  @param  Ohc      The OHCI Instance.

  @retval TRUE     System error happened.
  @retval FALSE    No system error.

**/
BOOLEAN
OhcIsSysError (
  IN USB_OHCI_INSTANCE    *Ohc
  )
{
  return OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, OHC_USBSTS_HSE);
}

/**
  Reset the OHCI host controller.

  @param  Ohc          The OHCI Instance.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The OHCI host controller is reset.
  @return Others       Failed to reset the OHCI before Timeout.

**/
EFI_STATUS
OhcResetHC (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  Status = EFI_SUCCESS;

  DEBUG ((EFI_D_INFO, "OhcResetHC!\n"));
  //
  // Host can only be reset when it is halt. If not so, halt it
  //
  if (!OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, OHC_USBSTS_HALT)) {
    Status = OhcHaltHC (Ohc, Timeout);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

#if 1
  OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, OHC_USBCMD_RESET);
  Status = OhcWaitOpRegBit (Ohc, OHC_USBCMD_OFFSET, OHC_USBCMD_RESET, FALSE, Timeout);
#else
  if ((Ohc->DebugCapSupOffset == 0xFFFFFFFF) || ((OhcReadExtCapReg (Ohc, Ohc->DebugCapSupOffset) & 0xFF) != OHC_CAP_USB_DEBUG) ||
      ((OhcReadExtCapReg (Ohc, Ohc->DebugCapSupOffset + OHC_DC_DCCTRL) & BIT0) == 0)) {
    OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, OHC_USBCMD_RESET);
    Status = OhcWaitOpRegBit (Ohc, OHC_USBCMD_OFFSET, OHC_USBCMD_RESET, FALSE, Timeout);
  }
#endif

  return Status;
}


/**
  Halt the OHCI host controller.

  @param  Ohc          The OHCI Instance.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @return EFI_SUCCESS  The OHCI host controller is halt.
  @return EFI_TIMEOUT  Failed to halt the OHCI before Timeout.

**/
EFI_STATUS
OhcHaltHC (
  IN USB_OHCI_INSTANCE   *Ohc,
  IN UINT32              Timeout
  )
{
  EFI_STATUS              Status;

  OhcClearOpRegBit (Ohc, OHC_USBCMD_OFFSET, OHC_USBCMD_RUN);
  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, OHC_USBSTS_HALT, TRUE, Timeout);
  return Status;
}


/**
  Set the OHCI host controller to run.

  @param  Ohc          The OHCI Instance.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @return EFI_SUCCESS  The OHCI host controller is running.
  @return EFI_TIMEOUT  Failed to set the OHCI to run before Timeout.

**/
EFI_STATUS
OhcRunHC (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, OHC_USBCMD_RUN);
  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, OHC_USBSTS_HALT, FALSE, Timeout);
  return Status;
}
