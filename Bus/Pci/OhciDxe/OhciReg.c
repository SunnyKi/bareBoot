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

  Status = Ohc->PciIo->Mem.Read (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": Pci Io Read error - %r at %d\n", Status, Offset));
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

  Status = Ohc->PciIo->Mem.Write (
                             Ohc->PciIo,
                             EfiPciIoWidthUint32,
                             OHC_BAR_INDEX,
                             (UINT64) Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": Pci Io Write error: %r at %d\n", Status, Offset));
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
  EFI_STATUS Status;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  if (!OHC_REG_BIT_IS_SET (Ohc, OHC_CONTROL_OFFSET, HCCTL_IR)) {
    /* No SMM driver active */
    DEBUG ((EFI_D_INFO, __FUNCTION__ ": no SMM (legacy) on device\n"));
    /* XXX: Let see the HostControllerFunctionalState for BIOS driver */
    switch (OhcReadOpReg (Ohc, OHC_CONTROL_OFFSET) & HCFS_MASK) {
    case HCFS_RESET:
      DEBUG ((EFI_D_INFO, __FUNCTION__ ": BIOS driver check: device is in UsbReset state\n"));
      break;
    case HCFS_RESUME:
      DEBUG ((EFI_D_INFO, __FUNCTION__ ": BIOS driver check: device is in UsbResume state\n"));
      break;
    case HCFS_OPERATIONAL:
      DEBUG ((EFI_D_INFO, __FUNCTION__ ": BIOS driver check: device is in UsbOperational state\n"));
      break;
    case HCFS_SUSPEND:
      DEBUG ((EFI_D_INFO, __FUNCTION__ ": BIOS driver check: device is in UsbSuspend state\n"));
      break;
    }
    return;
  }

  DEBUG ((EFI_D_INFO,  __FUNCTION__ ": SMM (legacy) on device\n"));

  /* Time to do little dance with SMM driver */
  OhcSetOpRegBit (Ohc, OHC_COMMANDSTATUS_OFFSET, HCCTS_OCR);

  MemoryFence (); // Just to be sure

  Status = OhcWaitOpRegBit (Ohc, OHC_CONTROL_OFFSET, HCCTL_IR, FALSE, 10000);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": bail out (wating for interruptrouting bit clear (%r))\n", Status));
  }
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
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

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));
#if 1
  Status = EFI_SUCCESS;
#else
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
#endif

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%r)\n", Status));
  return Status;
}


/**
  Clear all the interrupt status bits, these bits
  are Write-Clean.

  @param  Ohc          The OHCI device.

**/
VOID
OhcAckAllInterrupt (
  IN  USB2_HC_DEV         *Ohc
  )
{
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));
  OhcWriteOpReg (Ohc, OHC_INTERRUPTSTATUS_OFFSET, HCINT_ALLINTS);
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
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

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Status = EFI_SUCCESS;

  OhcSetOpRegBit (Ohc, OHC_CONTROL_OFFSET, HCCTL_PLE);

#if 0
  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_PERIOD_ENABLED, TRUE, Timeout);
#endif
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%r)\n", Status));
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

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Status = EFI_SUCCESS;

  OhcClearOpRegBit (Ohc, OHC_CONTROL_OFFSET, HCCTL_PLE);

#if 0
  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_PERIOD_ENABLED, FALSE, Timeout);
#endif
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%r)\n", Status));
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

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Status = EFI_SUCCESS;
#if 0
  OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_ENABLE_ASYNC);

  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_ASYNC_ENABLED, TRUE, Timeout);
#endif
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%r)\n", Status));
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

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Status = EFI_SUCCESS;
#if 0
  OhcClearOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_ENABLE_ASYNC);

  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_ASYNC_ENABLED, FALSE, Timeout);
#endif
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%r)\n", Status));
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
  BOOLEAN halted;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  halted = FALSE;
#if 0
  halted = OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, USBSTS_HALT);
#endif
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%d)\n", halted));
  return halted;
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
  BOOLEAN syserred;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  syserred = FALSE;
#if 0
  syserred = OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, USBSTS_SYS_ERROR);
#endif
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%d)\n", syserred));
  return syserred;
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

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  OhcSetOpRegBit (Ohc, OHC_COMMANDSTATUS_OFFSET, HCCTS_HCR);
  Status = OhcWaitOpRegBit (Ohc, OHC_COMMANDSTATUS_OFFSET, HCCTS_HCR, FALSE, Timeout);

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%r)\n", Status));
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

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Status = EFI_SUCCESS;
#if 0
  OhcClearOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_RUN);
  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_HALT, TRUE, Timeout);
#endif
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%r)\n", Status));
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

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Status = EFI_SUCCESS;
#if 0
  OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_RUN);
  Status = OhcWaitOpRegBit (Ohc, OHC_USBSTS_OFFSET, USBSTS_HALT, FALSE, Timeout);
#endif
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%r)\n", Status));
  return Status;
}


/**
  Initialize the HC hardware.

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

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  //
  // Allocate the periodic frame and associated memory
  // management facilities if not already done.
  //
  if (Ohc->PeriodFrame != NULL) {
    OhcFreeSched (Ohc);
  }

  Status = OhcInitSched (Ohc);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to init schedule (%r)\n", Status));
    return Status;
  }

  //
  // 1. Disable all the interrupts. UEFI works by polling
  //
  OhcWriteOpReg (Ohc, OHC_INTERRUPTDISABLE_OFFSET, HCINT_MIE);

#if 0
  //
  // 2. Start the Host Controller
  //
  OhcSetOpRegBit (Ohc, OHC_USBCMD_OFFSET, USBCMD_RUN);
#endif

  //
  // 3. Power up all ports if OHCI has Power Switching Mode (PSM) support
  //
  if (OHC_REG_BIT_IS_SET (Ohc, OHC_RHDESCRIPTORA_OFFSET, HCRHA_PSM)) {
    for (Index = 0; Index < (UINT8) (OhcReadOpReg (Ohc, OHC_RHDESCRIPTORA_OFFSET) & HCRHA_NPORTS_MASK); Index++) {
      OhcSetOpRegBit (Ohc, (UINT32) (OHC_RHPORTSTATUS_OFFSET + (4 * Index)), HCRPS_SPP);
    }
  }

  //
  // Wait roothub port power stable
  //
  gBS->Stall (OHC_ROOT_PORT_RECOVERY_STALL);

  Status = OhcEnablePeriodSchd (Ohc, OHC_GENERIC_TIMEOUT);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to enable period schedule (%r)\n", Status));
    return Status;
  }

  Status = OhcEnableAsyncSchd (Ohc, OHC_GENERIC_TIMEOUT);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to enable async schedule (%r)\n", Status));
    return Status;
  }

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave with (%r)\n", Status));
  return EFI_SUCCESS;
}
