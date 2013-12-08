/** @file

  This file contains the definination for host controller schedule routines.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_OHCI_SCHED_H_
#define _EFI_OHCI_SCHED_H_


/**
  Initialize the schedule data structure such as frame list.

  @param Ohc                    The OHCI device to init schedule data for.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to init schedule data.
  @retval EFI_SUCCESS           The schedule data is initialized.

**/
EFI_STATUS
OhcInitSched (
  IN USB2_HC_DEV          *Ohc
  );


/**
  Free the schedule data. It may be partially initialized.

  @param  Ohc            The OHCI device.

**/
VOID
OhcFreeSched (
  IN USB2_HC_DEV          *Ohc
  );


/**
  Link the queue head to the asynchronous schedule list.
  UEFI only supports one CTRL/BULK transfer at a time
  due to its interfaces. This simplifies the AsynList
  management: A reclamation header is always linked to
  the AsyncListAddr, the only active QH is appended to it.

  @param  Ohc            The OHCI device.
  @param  Qh             The queue head to link.

**/
VOID
OhcLinkQhToAsync (
  IN USB2_HC_DEV          *Ohc,
  IN OHC_QH               *Qh
  );


/**
  Unlink a queue head from the asynchronous schedule list.
  Need to synchronize with hardware.

  @param  Ohc            The OHCI device.
  @param  Qh             The queue head to unlink.

**/
VOID
OhcUnlinkQhFromAsync (
  IN USB2_HC_DEV          *Ohc,
  IN OHC_QH               *Qh
  );


/**
  Link a queue head for interrupt transfer to the periodic
  schedule frame list. This code is very much the same as
  that in UHCI.

  @param  Ohc            The OHCI device.
  @param  Qh             The queue head to link.

**/
VOID
OhcLinkQhToPeriod (
  IN USB2_HC_DEV          *Ohc,
  IN OHC_QH               *Qh
  );


/**
  Unlink an interrupt queue head from the periodic
  schedule frame list.

  @param  Ohc            The OHCI device.
  @param  Qh             The queue head to unlink.

**/
VOID
OhcUnlinkQhFromPeriod (
  IN USB2_HC_DEV          *Ohc,
  IN OHC_QH               *Qh
  );



/**
  Execute the transfer by polling the URB. This is a synchronous operation.

  @param  Ohc               The OHCI device.
  @param  Urb               The URB to execute.
  @param  TimeOut           The time to wait before abort, in millisecond.

  @retval EFI_DEVICE_ERROR  The transfer failed due to transfer error.
  @retval EFI_TIMEOUT       The transfer failed due to time out.
  @retval EFI_SUCCESS       The transfer finished OK.

**/
EFI_STATUS
OhcExecTransfer (
  IN  USB2_HC_DEV         *Ohc,
  IN  URB                 *Urb,
  IN  UINTN               TimeOut
  );


/**
  Delete a single asynchronous interrupt transfer for
  the device and endpoint.

  @param  Ohc            The OHCI device.
  @param  DevAddr        The address of the target device.
  @param  EpNum          The endpoint of the target.
  @param  DataToggle     Return the next data toggle to use.

  @retval EFI_SUCCESS    An asynchronous transfer is removed.
  @retval EFI_NOT_FOUND  No transfer for the device is found.

**/
EFI_STATUS
OhciDelAsyncIntTransfer (
  IN  USB2_HC_DEV         *Ohc,
  IN  UINT8               DevAddr,
  IN  UINT8               EpNum,
  OUT UINT8               *DataToggle
  );


/**
  Remove all the asynchronous interrutp transfers.

  @param  Ohc            The OHCI device.

**/
VOID
OhciDelAllAsyncIntTransfers (
  IN USB2_HC_DEV          *Ohc
  );


/**
  Interrupt transfer periodic check handler.

  @param  Event          Interrupt event.
  @param  Context        Pointer to USB2_HC_DEV.

**/
VOID
EFIAPI
OhcMonitorAsyncRequests (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  );

#endif
