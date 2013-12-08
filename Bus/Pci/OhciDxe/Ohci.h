/** @file

  Provides some data struct used by OHCI controller driver.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_OHCI_H_
#define _EFI_OHCI_H_


#include <Uefi.h>

#include <Protocol/Usb2HostController.h>
#include <Protocol/PciIo.h>

#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <IndustryStandard/Pci.h>

typedef struct _USB2_HC_DEV  USB2_HC_DEV;

#include "UsbHcMem.h"
#include "OhciReg.h"
#include "OhciUrb.h"
#include "OhciSched.h"
#include "OhciDebug.h"
#include "ComponentName.h"

//
// OHC timeout experience values
//

#define OHC_1_MICROSECOND            1
#define OHC_1_MILLISECOND            (1000 * OHC_1_MICROSECOND)
#define OHC_1_SECOND                 (1000 * OHC_1_MILLISECOND)

//
// OHCI register operation timeout, set by experience
//
#define OHC_RESET_TIMEOUT            (1 * OHC_1_SECOND)
#define OHC_GENERIC_TIMEOUT          (10 * OHC_1_MILLISECOND)

//
// Wait for roothub port power stable, refers to Spec
//
#define OHC_ROOT_PORT_RECOVERY_STALL (20 * OHC_1_MILLISECOND)

//
// Sync and Async transfer polling interval, set by experience,
// and the unit of Async is 100us, means 50ms as interval.
//
#define OHC_SYNC_POLL_INTERVAL       (1 * OHC_1_MILLISECOND)
#define OHC_ASYNC_POLL_INTERVAL      (50 * 10000U)

//
// OHCI debug port control status register bit definition
//
#define USB_DEBUG_PORT_IN_USE        BIT10
#define USB_DEBUG_PORT_ENABLE        BIT28
#define USB_DEBUG_PORT_OWNER         BIT30

//
// OHC raises TPL to TPL_NOTIFY to serialize all its operations
// to protect shared data structures.
//
#define  OHC_TPL                     TPL_NOTIFY

//
//Iterate through the doule linked list. NOT delete safe
//
#define EFI_LIST_FOR_EACH(Entry, ListHead)    \
  for(Entry = (ListHead)->ForwardLink; Entry != (ListHead); Entry = Entry->ForwardLink)

//
//Iterate through the doule linked list. This is delete-safe.
//Don't touch NextEntry
//
#define EFI_LIST_FOR_EACH_SAFE(Entry, NextEntry, ListHead)            \
  for(Entry = (ListHead)->ForwardLink, NextEntry = Entry->ForwardLink;\
      Entry != (ListHead); Entry = NextEntry, NextEntry = Entry->ForwardLink)

#define EFI_LIST_CONTAINER(Entry, Type, Field) BASE_CR(Entry, Type, Field)


#define OHC_LOW_32BIT(Addr64)     ((UINT32)(((UINTN)(Addr64)) & 0XFFFFFFFF))
#define OHC_HIGH_32BIT(Addr64)    ((UINT32)(RShiftU64((UINTN)(Addr64), 32) & 0XFFFFFFFF))
#define OHC_BIT_IS_SET(Data, Bit) ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define OHC_REG_BIT_IS_SET(Ohc, Offset, Bit) \
          (OHC_BIT_IS_SET(OhcReadOpReg ((Ohc), (Offset)), (Bit)))

#define USB2_HC_DEV_SIGNATURE  SIGNATURE_32 ('o', 'h', 'c', 'i')

#define OHC_FROM_THIS(a)       CR(a, USB2_HC_DEV, Usb2Hc, USB2_HC_DEV_SIGNATURE)

struct _USB2_HC_DEV {
  UINTN                     Signature;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;

  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINT64                    OriginalPciAttributes;
  USBHC_MEM_POOL            *MemPool;

  //
  // Schedule data shared between asynchronous and periodic
  // transfers:
  // ShortReadStop, as its name indicates, is used to terminate
  // the short read except the control transfer. OHCI follows
  // the alternative next QTD point when a short read happens.
  // For control transfer, even the short read happens, try the
  // status stage.
  //
  OHC_QTD                  *ShortReadStop;
  EFI_EVENT                 PollTimer;

  //
  // ExitBootServicesEvent is used to stop the OHC DMA operation 
  // after exit boot service.
  //
  EFI_EVENT                 ExitBootServiceEvent;

  //
  // Asynchronous(bulk and control) transfer schedule data:
  // ReclaimHead is used as the head of the asynchronous transfer
  // list. It acts as the reclamation header.
  //
  OHC_QH                   *ReclaimHead;

  //
  // Peroidic (interrupt) transfer schedule data:
  //
  VOID                      *PeriodFrame;     // the buffer pointed by this pointer is used to store pci bus address of the QH descriptor.
  VOID                      *PeriodFrameHost; // the buffer pointed by this pointer is used to store host memory address of the QH descriptor.
  VOID                      *PeriodFrameMap;

  OHC_QH                    *PeriodOne;
  LIST_ENTRY                AsyncIntTransfers;

#if 1
  UINT32                    HcRhDescriptorA;
#else
  //
  // OHCI configuration data
  //
  UINT32                    HcStructParams; // Cache of HC structure parameter, OHC_HCSPARAMS_OFFSET
  UINT32                    HcCapParams;    // Cache of HC capability parameter, HCCPARAMS
  UINT32                    CapLen;         // Capability length
#endif

  //
  // Misc
  //
  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;
};


extern EFI_DRIVER_BINDING_PROTOCOL      gOhciDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL      gOhciComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL     gOhciComponentName2;

/**
  Test to see if this driver supports ControllerHandle. Any
  ControllerHandle that has Usb2HcProtocol installed will
  be supported.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
OhcDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );

/**
  Starting the Usb OHCI Driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          supports this device.
  @return EFI_UNSUPPORTED      do not support this device.
  @return EFI_DEVICE_ERROR     cannot be started due to device Error.
  @return EFI_OUT_OF_RESOURCES cannot allocate resources.

**/
EFI_STATUS
EFIAPI
OhcDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle. Support stoping any child handles
  created by this driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to stop driver on.
  @param  NumberOfChildren     Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer    List of handles for the children we need to stop.

  @return EFI_SUCCESS          Success.
  @return EFI_DEVICE_ERROR     Fail.

**/
EFI_STATUS
EFIAPI
OhcDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  );

#endif
