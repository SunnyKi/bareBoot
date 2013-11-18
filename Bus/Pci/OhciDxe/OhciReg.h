/** @file

  This file contains the register definition of OHCI host controller.

Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
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
// PCI Configuration Registers
//
#define OHC_BAR_INDEX               0x00

#define OHC_PCI_BAR_OFFSET          0x10       // Memory Bar Register Offset
#define OHC_PCI_BAR_MASK            0xFFFF     // Memory Base Address Mask

#define USB_HUB_CLASS_CODE          0x09
#define USB_HUB_SUBCLASS_CODE       0x00

#define OHC_CAP_USB_LEGACY          0x01
#define OHC_CAP_USB_DEBUG           0x0A

//============================================//
//           OHCI register offset             //
//============================================//

//
// Capability registers offset
//
#define OHC_CAPLENGTH_OFFSET               0x00 // Capability register length offset
#define OHC_HCIVERSION_OFFSET              0x02 // Interface Version Number 02-03h
#define OHC_HCSPARAMS1_OFFSET              0x04 // Structural Parameters 1
#define OHC_HCSPARAMS2_OFFSET              0x08 // Structural Parameters 2
#define OHC_HCSPARAMS3_OFFSET              0x0c // Structural Parameters 3
#define OHC_HCCPARAMS_OFFSET               0x10 // Capability Parameters
#define OHC_DBOFF_OFFSET                   0x14 // Doorbell Offset
#define OHC_RTSOFF_OFFSET                  0x18 // Runtime Register Space Offset

//
// Operational registers offset
//
#define OHC_USBCMD_OFFSET                  0x0000 // USB Command Register Offset
#define OHC_USBSTS_OFFSET                  0x0004 // USB Status Register Offset
#define OHC_PAGESIZE_OFFSET                0x0008 // USB Page Size Register Offset
#define OHC_DNCTRL_OFFSET                  0x0014 // Device Notification Control Register Offset
#define OHC_CRCR_OFFSET                    0x0018 // Command Ring Control Register Offset
#define OHC_DCBAAP_OFFSET                  0x0030 // Device Context Base Address Array Pointer Register Offset
#define OHC_CONFIG_OFFSET                  0x0038 // Configure Register Offset
#define OHC_PORTSC_OFFSET                  0x0400 // Port Status and Control Register Offset

//
// Runtime registers offset
//
#define OHC_MFINDEX_OFFSET                 0x00 // Microframe Index Register Offset
#define OHC_IMAN_OFFSET                    0x20 // Interrupter X Management Register Offset
#define OHC_IMOD_OFFSET                    0x24 // Interrupter X Moderation Register Offset
#define OHC_ERSTSZ_OFFSET                  0x28 // Event Ring Segment Table Size Register Offset
#define OHC_ERSTBA_OFFSET                  0x30 // Event Ring Segment Table Base Address Register Offset
#define OHC_ERDP_OFFSET                    0x38 // Event Ring Dequeue Pointer Register Offset

//
// Debug registers offset
//
#define OHC_DC_DCCTRL                      0x20

#define USBLEGSP_BIOS_SEMAPHORE            BIT16 // HC BIOS Owned Semaphore
#define USBLEGSP_OS_SEMAPHORE              BIT24 // HC OS Owned Semaphore

#pragma pack (1)
typedef struct {
  UINT8                   MaxSlots;       // Number of Device Slots
  UINT16                  MaxIntrs:11;    // Number of Interrupters
  UINT16                  Rsvd:5;
  UINT8                   MaxPorts;       // Number of Ports
} HCSPARAMS1;

//
// Structural Parameters 1 Register Bitmap Definition
//
typedef union {
  UINT32                  Dword;
  HCSPARAMS1              Data;
} OHC_HCSPARAMS1;

typedef struct {
  UINT32                  Ist:4;          // Isochronous Scheduling Threshold
  UINT32                  Erst:4;         // Event Ring Segment Table Max
  UINT32                  Rsvd:13;
  UINT32                  ScratchBufHi:5; // Max Scratchpad Buffers Hi
  UINT32                  Spr:1;          // Scratchpad Restore
  UINT32                  ScratchBufLo:5; // Max Scratchpad Buffers Lo
} HCSPARAMS2;

//
// Structural Parameters 2 Register Bitmap Definition
//
typedef union {
  UINT32                  Dword;
  HCSPARAMS2              Data;
} OHC_HCSPARAMS2;

typedef struct {
  UINT16                  Ac64:1;        // 64-bit Addressing Capability
  UINT16                  Bnc:1;         // BW Negotiation Capability
  UINT16                  Csz:1;         // Context Size
  UINT16                  Ppc:1;         // Port Power Control
  UINT16                  Pind:1;        // Port Indicators
  UINT16                  Lhrc:1;        // Light HC Reset Capability
  UINT16                  Ltc:1;         // Latency Tolerance Messaging Capability
  UINT16                  Nss:1;         // No Secondary SID Support
  UINT16                  Pae:1;         // Parse All Event Data
  UINT16                  Rsvd:3;
  UINT16                  MaxPsaSize:4;  // Maximum Primary Stream Array Size
  UINT16                  ExtCapReg;     // xHCI Extended Capabilities Pointer
} HCCPARAMS;

//
// Capability Parameters Register Bitmap Definition
//
typedef union {
  UINT32                  Dword;
  HCCPARAMS               Data;
} OHC_HCCPARAMS;

#pragma pack ()

//
// Register Bit Definition
//
#define OHC_USBCMD_RUN                     BIT0  // Run/Stop
#define OHC_USBCMD_RESET                   BIT1  // Host Controller Reset
#define OHC_USBCMD_INTE                    BIT2  // Interrupter Enable
#define OHC_USBCMD_HSEE                    BIT3  // Host System Error Enable

#define OHC_USBSTS_HALT                    BIT0  // Host Controller Halted
#define OHC_USBSTS_HSE                     BIT2  // Host System Error
#define OHC_USBSTS_EINT                    BIT3  // Event Interrupt
#define OHC_USBSTS_PCD                     BIT4  // Port Change Detect
#define OHC_USBSTS_SSS                     BIT8  // Save State Status
#define OHC_USBSTS_RSS                     BIT9  // Restore State Status
#define OHC_USBSTS_SRE                     BIT10 // Save/Restore Error
#define OHC_USBSTS_CNR                     BIT11 // Host Controller Not Ready
#define OHC_USBSTS_HCE                     BIT12 // Host Controller Error

#define OHC_PAGESIZE_MASK                  0xFFFF // Page Size

#define OHC_CRCR_RCS                       BIT0  // Ring Cycle State
#define OHC_CRCR_CS                        BIT1  // Command Stop
#define OHC_CRCR_CA                        BIT2  // Command Abort
#define OHC_CRCR_CRR                       BIT3  // Command Ring Running

#define OHC_CONFIG_MASK                    0xFF  // Command Ring Running

#define OHC_PORTSC_CCS                     BIT0  // Current Connect Status
#define OHC_PORTSC_PED                     BIT1  // Port Enabled/Disabled
#define OHC_PORTSC_OCA                     BIT3  // Over-current Active
#define OHC_PORTSC_RESET                   BIT4  // Port Reset
#define OHC_PORTSC_PLS                     (BIT5|BIT6|BIT7|BIT8)     // Port Link State
#define OHC_PORTSC_PP                      BIT9  // Port Power
#define OHC_PORTSC_PS                      (BIT10|BIT11|BIT12|BIT13) // Port Speed
#define OHC_PORTSC_LWS                     BIT16 // Port Link State Write Strobe
#define OHC_PORTSC_CSC                     BIT17 // Connect Status Change
#define OHC_PORTSC_PEC                     BIT18 // Port Enabled/Disabled Change
#define OHC_PORTSC_WRC                     BIT19 // Warm Port Reset Change
#define OHC_PORTSC_OCC                     BIT20 // Over-Current Change
#define OHC_PORTSC_PRC                     BIT21 // Port Reset Change
#define OHC_PORTSC_PLC                     BIT22 // Port Link State Change
#define OHC_PORTSC_CEC                     BIT23 // Port Config Error Change
#define OHC_PORTSC_CAS                     BIT24 // Cold Attach Status

#define OHC_HUB_PORTSC_CCS                 BIT0  // Hub's Current Connect Status
#define OHC_HUB_PORTSC_PED                 BIT1  // Hub's Port Enabled/Disabled
#define OHC_HUB_PORTSC_OCA                 BIT3  // Hub's Over-current Active
#define OHC_HUB_PORTSC_RESET               BIT4  // Hub's Port Reset
#define OHC_HUB_PORTSC_PP                  BIT9  // Hub's Port Power
#define OHC_HUB_PORTSC_CSC                 BIT16 // Hub's Connect Status Change
#define OHC_HUB_PORTSC_PEC                 BIT17 // Hub's Port Enabled/Disabled Change
#define OHC_HUB_PORTSC_OCC                 BIT19 // Hub's Over-Current Change
#define OHC_HUB_PORTSC_PRC                 BIT20 // Hub's Port Reset Change
#define OHC_IMAN_IP                        BIT0  // Interrupt Pending
#define OHC_IMAN_IE                        BIT1  // Interrupt Enable

#define OHC_IMODI_MASK                     0x0000FFFF  // Interrupt Moderation Interval
#define OHC_IMODC_MASK                     0xFFFF0000  // Interrupt Moderation Counter

//
// Structure to map the hardware port states to the
// UEFI's port states.
//
typedef struct {
  UINT32                  HwState;
  UINT16                  UefiState;
} USB_PORT_STATE_MAP;

/**
  Read 1-byte width OHCI capability register.

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the 1-byte width capability register.

  @return The register content read.
  @retval If err, return 0xFFFF.

**/
UINT8
OhcReadCapReg8 (
  IN  USB_OHCI_INSTANCE   *Ohc,
  IN  UINT32              Offset
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Wait the operation register's bit as specified by Bit
  to be set (or clear).

  @param  Ohc          The OHCI Instance.
  @param  Offset       The offset of the operational register.
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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Whether the OHCI host controller is halted.

  @param  Ohc     The OHCI Instance.

  @retval TRUE    The controller is halted.
  @retval FALSE   It isn't halted.

**/
BOOLEAN
OhcIsHalt (
  IN USB_OHCI_INSTANCE    *Ohc
  );

/**
  Whether system error occurred.

  @param  Ohc      The OHCI Instance.

  @retval TRUE     System error happened.
  @retval FALSE    No system error.

**/
BOOLEAN
OhcIsSysError (
  IN USB_OHCI_INSTANCE    *Ohc
  );

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
  );

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
  );

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
  );

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
  );

#endif
