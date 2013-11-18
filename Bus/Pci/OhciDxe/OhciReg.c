/** @file

  The OHCI register operation routines.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ohci.h"

/**
  Read 1-byte width OHCI capability register.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the 1-byte width capability register.

  @return The register content read.
  @retval If err, return 0xFF.

**/
UINT8
OhcReadCapReg8 (
  IN  USB_OHCI_INSTANCE   *Ohc,
  IN  UINT32              Offset
  )
{
  UINT8                   Data;
  EFI_STATUS              Status;

  Status = Ohc->PciIo->Mem.Read (
                             Ohc->PciIo,
                             EfiPciIoWidthUint8,
                             OHC_BAR_INDEX,
                             (UINT64) Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcReadCapReg: Pci Io read error - %r at %d\n", Status, Offset));
    Data = 0xFF;
  }

  return Data;
}

/**
  Read 4-bytes width OHCI capability register.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the 4-bytes width capability register.

  @return The register content read.
  @retval If err, return 0xFFFFFFFF.

**/
UINT32
OhcReadCapReg (
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
                             (UINT64) Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcReadCapReg: Pci Io read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

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

  ASSERT (Ohc->CapLength != 0);

  Status = Ohc->PciIo->Mem.Read (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) (Ohc->CapLength + Offset),
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

  ASSERT (Ohc->CapLength != 0);

  Status = Ohc->PciIo->Mem.Write (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) (Ohc->CapLength + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcWriteOpReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Write the data to the 2-bytes width OHCI operational register.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the 2-bytes width operational register.
  @param  Data         The data to write.

**/
VOID
OhcWriteOpReg16 (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Offset,
  IN UINT16               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Ohc->CapLength != 0);

  Status = Ohc->PciIo->Mem.Write (
                             Ohc->PciIo,
                             EfiPciIoWidthUint16,
                             OHC_BAR_INDEX,
                             (UINT64) (Ohc->CapLength + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcWriteOpReg16: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Read OHCI door bell register.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the door bell register.

  @return The register content read

**/
UINT32
OhcReadDoorBellReg (
  IN  USB_OHCI_INSTANCE   *Ohc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  ASSERT (Ohc->DBOff != 0);

  Status = Ohc->PciIo->Mem.Read (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) (Ohc->DBOff + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcReadDoorBellReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Write the data to the OHCI door bell register.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the door bell register.
  @param  Data         The data to write.

**/
VOID
OhcWriteDoorBellReg (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Ohc->DBOff != 0);

  Status = Ohc->PciIo->Mem.Write (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) (Ohc->DBOff + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcWriteOpReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Read OHCI runtime register.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the runtime register.

  @return The register content read

**/
UINT32
OhcReadRuntimeReg (
  IN  USB_OHCI_INSTANCE   *Ohc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  ASSERT (Ohc->RTSOff != 0);

  Status = Ohc->PciIo->Mem.Read (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) (Ohc->RTSOff + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcReadRuntimeReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Write the data to the OHCI runtime register.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the runtime register.
  @param  Data         The data to write.

**/
VOID
OhcWriteRuntimeReg (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Ohc->RTSOff != 0);

  Status = Ohc->PciIo->Mem.Write (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) (Ohc->RTSOff + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcWriteRuntimeReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Read OHCI extended capability register.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the extended capability register.

  @return The register content read

**/
UINT32
OhcReadExtCapReg (
  IN  USB_OHCI_INSTANCE   *Ohc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  ASSERT (Ohc->ExtCapRegBase != 0);

  Status = Ohc->PciIo->Mem.Read (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) (Ohc->ExtCapRegBase + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcReadExtCapReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Write the data to the OHCI extended capability register.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the extended capability register.
  @param  Data         The data to write.

**/
VOID
OhcWriteExtCapReg (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Ohc->ExtCapRegBase != 0);

  Status = Ohc->PciIo->Mem.Write (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) (Ohc->ExtCapRegBase + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcWriteExtCapReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}


/**
  Set one bit of the runtime register while keeping other bits.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the runtime register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
OhcSetRuntimeRegBit (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = OhcReadRuntimeReg (Ohc, Offset);
  Data |= Bit;
  OhcWriteRuntimeReg (Ohc, Offset, Data);
}

/**
  Clear one bit of the runtime register while keeping other bits.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the runtime register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
OhcClearRuntimeRegBit (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT32               Offset,
  IN UINT32               Bit
  )
{
  UINT32                  Data;

  Data  = OhcReadRuntimeReg (Ohc, Offset);
  Data &= ~Bit;
  OhcWriteRuntimeReg (Ohc, Offset, Data);
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
  UINT32                    Buffer;

  if (Ohc->UsbLegSupOffset == 0xFFFFFFFF) {
    return;
  }

  DEBUG ((EFI_D_INFO, "OhcSetBiosOwnership: called to set BIOS ownership\n"));

  Buffer = OhcReadExtCapReg (Ohc, Ohc->UsbLegSupOffset);
  Buffer = ((Buffer & (~USBLEGSP_OS_SEMAPHORE)) | USBLEGSP_BIOS_SEMAPHORE);
  OhcWriteExtCapReg (Ohc, Ohc->UsbLegSupOffset, Buffer);
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
  UINT32                    Buffer;

  if (Ohc->UsbLegSupOffset == 0xFFFFFFFF) {
    return;
  }

  DEBUG ((EFI_D_INFO, "OhcClearBiosOwnership: called to clear BIOS ownership\n"));

  Buffer = OhcReadExtCapReg (Ohc, Ohc->UsbLegSupOffset);
  Buffer = ((Buffer & (~USBLEGSP_BIOS_SEMAPHORE)) | USBLEGSP_OS_SEMAPHORE);
  OhcWriteExtCapReg (Ohc, Ohc->UsbLegSupOffset, Buffer);
}

/**
  Calculate the offset of the OHCI capability.

  @param  Ohc     The OHCI Instance.
  @param  CapId   The OHCI Capability ID.

  @return The offset of OHCI legacy support capability register.

**/
UINT32
OhcGetCapabilityAddr (
  IN USB_OHCI_INSTANCE    *Ohc,
  IN UINT8                CapId
  )
{
  UINT32 ExtCapOffset;
  UINT8  NextExtCapReg;
  UINT32 Data;

  ExtCapOffset = 0;

  do {
    //
    // Check if the extended capability register's capability id is USB Legacy Support.
    //
    Data = OhcReadExtCapReg (Ohc, ExtCapOffset);
    if ((Data & 0xFF) == CapId) {
      return ExtCapOffset;
    }
    //
    // If not, then traverse all of the ext capability registers till finding out it.
    //
    NextExtCapReg = (UINT8)((Data >> 8) & 0xFF);
    ExtCapOffset += (NextExtCapReg << 2);
  } while (NextExtCapReg != 0);

  return 0xFFFFFFFF;
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

  if ((Ohc->DebugCapSupOffset == 0xFFFFFFFF) || ((OhcReadExtCapReg (Ohc, Ohc->DebugCapSupOffset) & 0xFF) != OHC_CAP_USB_DEBUG) ||
      ((OhcReadExtCapReg (Ohc, Ohc->DebugCapSupOffset + OHC_DC_DCCTRL) & BIT0) == 0)) {
    OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, OHC_USBCMD_RESET);
    Status = OhcWaitOpRegBit (Ohc, OHC_USBCMD_OFFSET, OHC_USBCMD_RESET, FALSE, Timeout);
  }

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

