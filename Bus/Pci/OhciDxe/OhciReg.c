/** @file

  The OHCI register operation routines.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Ohci.h"

/**
  Read OHCI Operation register.

  @param  Ohc          The OHCI device.
  @param  Offset       The operation register offset.

  @return The register content read.
  @retval If err, return 0xffff.

**/
UINT32
OhcReadOpReg (
  IN  USB2_HC_DEV         *Ohc,
  IN  UINT32              Offset
  )
{
  UINT32                  Data;
  EFI_STATUS              Status;

  ASSERT (Ohc->CapLen != 0);

  Status = Ohc->PciIo->Mem.Read (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) (Ohc->CapLen + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcReadOpReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFF;
  }

  return Data;
}


/**
  Write  the data to the OHCI operation register.

  @param  Ohc          The OHCI device.
  @param  Offset       OHCI operation register offset.
  @param  Data         The data to write.

**/
VOID
OhcWriteOpReg (
  IN USB2_HC_DEV          *Ohc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  EFI_STATUS              Status;

  ASSERT (Ohc->CapLen != 0);

  Status = Ohc->PciIo->Mem.Write (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) (Ohc->CapLen + Offset),
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcWriteOpReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}


/**
  Set one bit of the operational register while keeping other bits.

  @param  Ohc          The OHCI device.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
OhcSetOpRegBit (
  IN USB2_HC_DEV          *Ohc,
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

  @param  Ohc          The OHCI device.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to clear.

**/
VOID
OhcClearOpRegBit (
  IN USB2_HC_DEV          *Ohc,
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

  @param  Ohc          The OHCI device.
  @param  Offset       The offset of the operation register.
  @param  Bit          The bit of the register to wait for.
  @param  WaitToSet    Wait the bit to set or clear.
  @param  Timeout      The time to wait before abort (in millisecond).

  @retval EFI_SUCCESS  The bit successfully changed by host controller.
  @retval EFI_TIMEOUT  The time out occurred.

**/
EFI_STATUS
OhcWaitOpRegBit (
  IN USB2_HC_DEV          *Ohc,
  IN UINT32               Offset,
  IN UINT32               Bit,
  IN BOOLEAN              WaitToSet,
  IN UINT32               Timeout
  )
{
  UINT32                  Index;

  for (Index = 0; Index < Timeout / OHC_SYNC_POLL_INTERVAL + 1; Index++) {
    if (OHC_REG_BIT_IS_SET (Ohc, Offset, Bit) == WaitToSet) {
      return EFI_SUCCESS;
    }

    gBS->Stall (OHC_SYNC_POLL_INTERVAL);
  }

  return EFI_TIMEOUT;
}


/**
  Add support for UEFI Over Legacy (UoL) feature, stop
  the legacy USB SMI support.

  @param  Ohc          The OHCI device.

**/
VOID
OhcClearLegacySupport (
  IN USB2_HC_DEV          *Ohc
  )
{
  UINT32                    ExtendCap;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT32                    Value;
  UINT32                    TimeOut;

  DEBUG ((EFI_D_INFO, "OhcClearLegacySupport: called to clear legacy support\n"));

  PciIo     = Ohc->PciIo;
  ExtendCap = (Ohc->HcCapParams >> 8) & 0xFF;

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
  Value |= (0x1 << 24);
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

  TimeOut = 40;
  while (TimeOut-- != 0) {
    gBS->Stall (500);

    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

    if ((Value & 0x01010000) == 0x01000000) {
      break;
    }
  }

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);
}



/**
  Set door bell and wait it to be ACKed by host controller.
  This function is used to synchronize with the hardware.

  @param  Ohc          The OHCI device.
  @param  Timeout      The time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  Synchronized with the hardware.
  @retval EFI_TIMEOUT  Time out happened while waiting door bell to set.

**/
EFI_STATUS
OhcSetAndWaitDoorBell (
  IN  USB2_HC_DEV         *Ohc,
  IN  UINT32              Timeout
  )
{
  EFI_STATUS              Status;
  UINT32                  Data;

  OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_IAAD);

  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_IAA, TRUE, Timeout);

  //
  // ACK the IAA bit in USBSTS register. Make sure other
  // interrupt bits are not ACKed. These bits are WC (Write Clean).
  //
  Data  = OhcReadOpReg (Ohc, OHC_USBSTS_OFFSET);
  Data &= ~USBSTS_INTACK_MASK;
  Data |= USBSTS_IAA;

  OhcWriteOpReg (Ohc, OHC_USBSTS_OFFSET, Data);

  return Status;
}


/**
  Clear all the interrutp status bits, these bits
  are Write-Clean.

  @param  Ohc          The OHCI device.

**/
VOID
OhcAckAllInterrupt (
  IN  USB2_HC_DEV         *Ohc
  )
{
  OhcWriteOpReg (Ohc, OHC_USBSTS_OFFSET, USBSTS_INTACK_MASK);
}


/**
  Enable the periodic schedule then wait OHC to
  actually enable it.

  @param  Ohc          The OHCI device.
  @param  Timeout      The time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The periodical schedule is enabled.
  @retval EFI_TIMEOUT  Time out happened while enabling periodic schedule.

**/
EFI_STATUS
OhcEnablePeriodSchd (
  IN USB2_HC_DEV          *Ohc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_ENABLE_PERIOD);

  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_PERIOD_ENABLED, TRUE, Timeout);
  return Status;
}


/**
  Disable periodic schedule.

  @param  Ohc               The OHCI device.
  @param  Timeout           Time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS       Periodic schedule is disabled.
  @retval EFI_DEVICE_ERROR  Fail to disable periodic schedule.

**/
EFI_STATUS
OhcDisablePeriodSchd (
  IN USB2_HC_DEV          *Ohc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  OhcClearOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_ENABLE_PERIOD);

  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_PERIOD_ENABLED, FALSE, Timeout);
  return Status;
}



/**
  Enable asynchrounous schedule.

  @param  Ohc          The OHCI device.
  @param  Timeout      Time to wait before abort.

  @retval EFI_SUCCESS  The OHCI asynchronous schedule is enabled.
  @return Others       Failed to enable the asynchronous scheudle.

**/
EFI_STATUS
OhcEnableAsyncSchd (
  IN USB2_HC_DEV          *Ohc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_ENABLE_ASYNC);

  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_ASYNC_ENABLED, TRUE, Timeout);
  return Status;
}



/**
  Disable asynchrounous schedule.

  @param  Ohc          The OHCI device.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The asynchronous schedule is disabled.
  @return Others       Failed to disable the asynchronous schedule.

**/
EFI_STATUS
OhcDisableAsyncSchd (
  IN USB2_HC_DEV          *Ohc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS  Status;

  OhcClearOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_ENABLE_ASYNC);

  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_ASYNC_ENABLED, FALSE, Timeout);
  return Status;
}



/**
  Whether Ohc is halted.

  @param  Ohc          The OHCI device.

  @retval TRUE         The controller is halted.
  @retval FALSE        It isn't halted.

**/
BOOLEAN
OhcIsHalt (
  IN USB2_HC_DEV          *Ohc
  )
{
  return OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, USBSTS_HALT);
}


/**
  Whether system error occurred.

  @param  Ohc          The OHCI device.

  @return TRUE         System error happened.
  @return FALSE        No system error.

**/
BOOLEAN
OhcIsSysError (
  IN USB2_HC_DEV          *Ohc
  )
{
  return OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, USBSTS_SYS_ERROR);
}


/**
  Reset the host controller.

  @param  Ohc          The OHCI device.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The host controller is reset.
  @return Others       Failed to reset the host.

**/
EFI_STATUS
OhcResetHC (
  IN USB2_HC_DEV          *Ohc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  //
  // Host can only be reset when it is halt. If not so, halt it
  //
  if (!OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, USBSTS_HALT)) {
    Status = OhcHaltHC (Ohc, Timeout);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_RESET);
  Status = OhcWaitOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_RESET, FALSE, Timeout);
  return Status;
}


/**
  Halt the host controller.

  @param  Ohc          The OHCI device.
  @param  Timeout      Time to wait before abort.

  @retval EFI_SUCCESS  The OHCI is halt.
  @retval EFI_TIMEOUT  Failed to halt the controller before Timeout.

**/
EFI_STATUS
OhcHaltHC (
  IN USB2_HC_DEV         *Ohc,
  IN UINT32              Timeout
  )
{
  EFI_STATUS              Status;

  OhcClearOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_RUN);
  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_HALT, TRUE, Timeout);
  return Status;
}


/**
  Set the OHCI to run.

  @param  Ohc          The OHCI device.
  @param  Timeout      Time to wait before abort.

  @retval EFI_SUCCESS  The OHCI is running.
  @return Others       Failed to set the OHCI to run.

**/
EFI_STATUS
OhcRunHC (
  IN USB2_HC_DEV          *Ohc,
  IN UINT32               Timeout
  )
{
  EFI_STATUS              Status;

  OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_RUN);
  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_HALT, FALSE, Timeout);
  return Status;
}


/**
  Initialize the HC hardware.
  OHCI spec lists the five things to do to initialize the hardware:
  1. Program CTRLDSSEGMENT
  2. Set USBINTR to enable interrupts
  3. Set periodic list base
  4. Set USBCMD, interrupt threshold, frame list size etc
  5. Write 1 to CONFIGFLAG to route all ports to OHCI

  @param  Ohc          The OHCI device.

  @return EFI_SUCCESS  The OHCI has come out of halt state.
  @return EFI_TIMEOUT  Time out happened.

**/
EFI_STATUS
OhcInitHC (
  IN USB2_HC_DEV          *Ohc
  )
{
  EFI_STATUS              Status;
  UINT32                  Index;

  // This ASSERT crashes the BeagleBoard. There is some issue in the USB stack.
  // This ASSERT needs to be removed so the BeagleBoard will boot. When we fix
  // the USB stack we can put this ASSERT back in
  // ASSERT (OhcIsHalt (Ohc));

  //
  // Allocate the periodic frame and associated memeory
  // management facilities if not already done.
  //
  if (Ohc->PeriodFrame != NULL) {
    OhcFreeSched (Ohc);
  }

  Status = OhcInitSched (Ohc);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // 1. Clear USBINTR to disable all the interrupt. UEFI works by polling
  //
  OhcWriteOpReg (Ohc, OHC_USBINTR_OFFSET, 0);

  //
  // 2. Start the Host Controller
  //
  OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_RUN);

  //
  // 3. Power up all ports if OHCI has Port Power Control (PPC) support
  //
  if (Ohc->HcStructParams & HCSP_PPC) {
    for (Index = 0; Index < (UINT8) (Ohc->HcStructParams & HCSP_NPORTS); Index++) {
      OhcSetOpRegBit (Ohc, (UINT32) (OHC_PORT_STAT_OFFSET + (4 * Index)), PORTSC_POWER);
    }
  }

  //
  // Wait roothub port power stable
  //
  gBS->Stall (OHC_ROOT_PORT_RECOVERY_STALL);

  //
  // 4. Set all ports routing to OHC
  //
  OhcSetOpRegBit (Ohc, OHC_CONFIG_FLAG_OFFSET, CONFIGFLAG_ROUTE_OHC);

  Status = OhcEnablePeriodSchd (Ohc, OHC_GENERIC_TIMEOUT);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcInitHC: failed to enable period schedule\n"));
    return Status;
  }

  Status = OhcEnableAsyncSchd (Ohc, OHC_GENERIC_TIMEOUT);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcInitHC: failed to enable async schedule\n"));
    return Status;
  }

  return EFI_SUCCESS;
}
