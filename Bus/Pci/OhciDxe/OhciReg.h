/** @file

  This file contains the definination for host controller register operation routines.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_OHCI_REG_H_
#define _EFI_OHCI_REG_H_

//
// Operational register offset
//
#define OHC_REVISION_OFFSET              0x0000

#define OHC_CONTROL_OFFSET               0x0004
#define   HCCTL_PLE                        0x00000004  // Periodic List Enable
#define   HCFS_MASK                        0x000000c0  /* HostControllerFunctionalState */
#define   HCFS_RESET                       0x00000000
#define   HCFS_RESUME                      0x00000040
#define   HCFS_OPERATIONAL                 0x00000080
#define   HCFS_SUSPEND                     0x000000c0
#define   HCCTL_IR                         0x00000100  // Interrupt Routing

#define OHC_COMMANDSTATUS_OFFSET         0x0008
#define   HCCTS_OCR                        0x00000008  // Ownership Change Request

#define OHC_INTERRUPTSTATUS_OFFSET       0x000C
#define OHC_INTERRUPTENABLE_OFFSET       0x0010
#define OHC_INTERRUPTDISABLE_OFFSET      0x0014
#define   HCINT_RHSC                       BIT6  // Root Hub Status Change
#define   HCINT_MIE                        0x80000000 // Master Interrupt Enable|Disable
#define   HCINT_ALLINTS                    (HCINT_RHSC)

#define OHC_HCHCCA_OFFSET                0x0018

#define OHC_RHDESCRITORA_OFFSET          0x0048
#define   HCRHA_NPORTS                     0x000000FF // Number of root hub port
#define   HCRHA_PSM                        BIT8 // Power Switching Mode

#define OHC_RHPORTSTATUS_OFFSET          0x0054 /* XXX: 0x50? */

#define OHC_FRAME_LEN           1024

//
// Register bit definition
//

#define USBCMD_RUN              0x01   // Run/stop
#define USBCMD_RESET            0x02   // Start the host controller reset
#define USBCMD_ENABLE_PERIOD    0x10   // Enable periodic schedule
#define USBCMD_ENABLE_ASYNC     0x20   // Enable asynchronous schedule
#define USBCMD_IAAD             0x40   // Interrupt on async advance doorbell

#define USBSTS_IAA              0x20   // Interrupt on async advance
#define USBSTS_PERIOD_ENABLED   0x4000 // Periodic schedule status
#define USBSTS_ASYNC_ENABLED    0x8000 // Asynchronous schedule status
#define USBSTS_HALT             0x1000 // Host controller halted
#define USBSTS_SYS_ERROR        0x10   // Host system error
#define USBSTS_INTACK_MASK      0x003F // Mask for the interrupt ACK, the WC
                                       // (write clean) bits in USBSTS register

#define PORTSC_CONN             0x01   // Current Connect Status
#define PORTSC_CONN_CHANGE      0x02   // Connect Status Change
#define PORTSC_ENABLED          0x04   // Port Enable / Disable
#define PORTSC_ENABLE_CHANGE    0x08   // Port Enable / Disable Change
#define PORTSC_OVERCUR          0x10   // Over current Active
#define PORTSC_OVERCUR_CHANGE   0x20   // Over current Change
#define PORSTSC_RESUME          0x40   // Force Port Resume
#define PORTSC_SUSPEND          0x80   // Port Suspend State
#define PORTSC_RESET            0x100  // Port Reset
#define PORTSC_LINESTATE_K      0x400  // Line Status K-state
#define PORTSC_LINESTATE_J      0x800  // Line Status J-state
#define PORTSC_POWER            0x1000 // Port Power
#define PORTSC_OWNER            0x2000 // Port Owner
#define PORTSC_CHANGE_MASK      0x2A   // Mask of the port change bits,
                                       // they are WC (write clean)
//
// PCI Configuration Registers
//
#define OHC_BAR_INDEX           0      // how many bytes away from USB_BASE to 0x10

#define OHC_LINK_TERMINATED(Link) (((Link) & 0x01) != 0)

#define OHC_ADDR(High, QhHw32)   \
        ((VOID *) (UINTN) (LShiftU64 ((High), 32) | ((QhHw32) & 0xFFFFFFF0)))

#define OHCI_IS_DATAIN(EndpointAddr) OHC_BIT_IS_SET((EndpointAddr), 0x80)

//
// Structure to map the hardware port states to the
// UEFI's port states.
//
typedef struct {
  UINT16                  HwState;
  UINT16                  UefiState;
} USB_PORT_STATE_MAP;

//
// Ohci Data and Ctrl Structures
//
#pragma pack(1)
typedef struct {
  UINT8                   ProgInterface;
  UINT8                   SubClassCode;
  UINT8                   BaseCode;
} USB_CLASSC;
#pragma pack()

/**
  Read OHCI Operation register.

  @param  Ohc      The OHCI device.
  @param  Offset   The operation register offset.

  @return The register content.

**/
UINT32
OhcReadOpReg (
  IN  USB2_HC_DEV         *Ohc,
  IN  UINT32              Offset
  );


/**
  Write  the data to the OHCI operation register.

  @param  Ohc      The OHCI device.
  @param  Offset   OHCI operation register offset.
  @param  Data     The data to write.

**/
VOID
OhcWriteOpReg (
  IN USB2_HC_DEV          *Ohc,
  IN UINT32               Offset,
  IN UINT32               Data
  );

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
  );

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
  );

/**
  Add support for UEFI Over Legacy (UoL) feature, stop
  the legacy USB SMI support.

  @param  Ohc      The OHCI device.

**/
VOID
OhcClearLegacySupport (
  IN USB2_HC_DEV          *Ohc
  );



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
  IN  UINT32               Timeout
  );


/**
  Clear all the interrutp status bits, these bits are Write-Clean.

  @param  Ohc      The OHCI device.

**/
VOID
OhcAckAllInterrupt (
  IN  USB2_HC_DEV         *Ohc
  );



/**
  Whether Ohc is halted.

  @param  Ohc     The OHCI device.

  @retval TRUE    The controller is halted.
  @retval FALSE   It isn't halted.

**/
BOOLEAN
OhcIsHalt (
  IN USB2_HC_DEV          *Ohc
  );


/**
  Whether system error occurred.

  @param  Ohc      The OHCI device.

  @retval TRUE     System error happened.
  @retval FALSE    No system error.

**/
BOOLEAN
OhcIsSysError (
  IN USB2_HC_DEV          *Ohc
  );


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
  );


/**
  Halt the host controller.

  @param  Ohc          The OHCI device.
  @param  Timeout      Time to wait before abort.

  @return EFI_SUCCESS  The OHCI is halt.
  @return EFI_TIMEOUT  Failed to halt the controller before Timeout.

**/
EFI_STATUS
OhcHaltHC (
  IN USB2_HC_DEV         *Ohc,
  IN UINT32              Timeout
  );


/**
  Set the OHCI to run.

  @param  Ohc          The OHCI device.
  @param  Timeout      Time to wait before abort.

  @return EFI_SUCCESS  The OHCI is running.
  @return Others       Failed to set the OHCI to run.

**/
EFI_STATUS
OhcRunHC (
  IN USB2_HC_DEV          *Ohc,
  IN UINT32               Timeout
  );

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
  );

#endif
