/** @file  
  The Ohci controller driver.

  OhciDxe driver is responsible for managing the behavior of OHCI controller. 
  It implements the interfaces of monitoring the status of all ports and transferring 
  Control, Bulk, Interrupt and Isochronous requests to Usb1.1 device.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Ohci.h"

//
// Two arrays used to translate the OHCI port state (change)
// to the UEFI protocol's port state (change).
//
USB_PORT_STATE_MAP  mUsbPortStateMap[] = {
  {PORTSC_CONN,     USB_PORT_STAT_CONNECTION},
  {PORTSC_ENABLED,  USB_PORT_STAT_ENABLE},
  {PORTSC_SUSPEND,  USB_PORT_STAT_SUSPEND},
  {PORTSC_OVERCUR,  USB_PORT_STAT_OVERCURRENT},
  {PORTSC_RESET,    USB_PORT_STAT_RESET},
  {PORTSC_POWER,    USB_PORT_STAT_POWER},
  {PORTSC_OWNER,    USB_PORT_STAT_OWNER}
};

USB_PORT_STATE_MAP  mUsbPortChangeMap[] = {
  {PORTSC_CONN_CHANGE,    USB_PORT_STAT_C_CONNECTION},
  {PORTSC_ENABLE_CHANGE,  USB_PORT_STAT_C_ENABLE},
  {PORTSC_OVERCUR_CHANGE, USB_PORT_STAT_C_OVERCURRENT}
};

EFI_DRIVER_BINDING_PROTOCOL
gOhciDriverBinding = {
  OhcDriverBindingSupported,
  OhcDriverBindingStart,
  OhcDriverBindingStop,
  0x30,
  NULL,
  NULL
};

/**
  Retrieves the capability of root hub ports.

  @param  This                  This EFI_USB_HC_PROTOCOL instance.
  @param  MaxSpeed              Max speed supported by the controller.
  @param  PortNumber            Number of the root hub ports.
  @param  Is64BitCapable        Whether the controller supports 64-bit memory
                                addressing.

  @retval EFI_SUCCESS           Host controller capability were retrieved successfully.
  @retval EFI_INVALID_PARAMETER Either of the three capability pointer is NULL.

**/
EFI_STATUS
EFIAPI
OhcGetCapability (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT UINT8                 *MaxSpeed,
  OUT UINT8                 *PortNumber,
  OUT UINT8                 *Is64BitCapable
  )
{
  USB2_HC_DEV             *Ohc;
  EFI_TPL                 OldTpl;

  if ((MaxSpeed == NULL) || (PortNumber == NULL) || (Is64BitCapable == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl          = gBS->RaiseTPL (OHC_TPL);
  Ohc             = OHC_FROM_THIS (This);

  *MaxSpeed       = EFI_USB_SPEED_HIGH;
  *PortNumber     = (UINT8) (Ohc->HcRhDescriptorA & HCRHA_NPORTS);
#if 1
  *Is64BitCapable = 0; /* XXX: check AMD SB??? */
#else
  *Is64BitCapable = (UINT8) (Ohc->HcCapParams & HCCP_64BIT);
#endif

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": %d ports, 64 bit %d\n", *PortNumber, *Is64BitCapable));

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}


/**
  Provides software reset for the USB host controller.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  Attributes            A bit mask of the reset operation to perform.

  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_INVALID_PARAMETER Attributes is not valid.
  @retval EFI_UNSUPPOURTED      The type of reset specified by Attributes is
                                not currently supported by the host controller.
  @retval EFI_DEVICE_ERROR      Host controller isn't halted to reset.

**/
EFI_STATUS
EFIAPI
OhcReset (
  IN EFI_USB2_HC_PROTOCOL *This,
  IN UINT16               Attributes
  )
{
  USB2_HC_DEV             *Ohc;
  EFI_TPL                 OldTpl;
  EFI_STATUS              Status;
#if 0
  UINT32                  DbgCtrlStatus;
#endif

  Ohc = OHC_FROM_THIS (This);

  if (Ohc->DevicePath != NULL) {
    //
    // Report Status Code to indicate reset happens
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      (EFI_IO_BUS_USB | EFI_IOB_PC_RESET),
      Ohc->DevicePath
      );
  }

  OldTpl  = gBS->RaiseTPL (OHC_TPL);

  switch (Attributes) {
  case EFI_USB_HC_RESET_GLOBAL:
  //
  // Flow through, same behavior as Host Controller Reset
  //
  case EFI_USB_HC_RESET_HOST_CONTROLLER:
    //
    // Host Controller must be Halt when Reset it
    //
#if 0
    if (Ohc->DebugPortNum != 0) {
      DbgCtrlStatus = OhcReadDbgRegister(Ohc, 0);
      if ((DbgCtrlStatus & (USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_OWNER)) == (USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_OWNER)) {
        Status = EFI_SUCCESS;
        goto ON_EXIT;
      }
    }
#endif

    if (!OhcIsHalt (Ohc)) {
      Status = OhcHaltHC (Ohc, OHC_GENERIC_TIMEOUT);

      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto ON_EXIT;
      }
    }

    //
    // Clean up the asynchronous transfers, currently only
    // interrupt supports asynchronous operation.
    //
    OhciDelAllAsyncIntTransfers (Ohc);
    OhcAckAllInterrupt (Ohc);
    OhcFreeSched (Ohc);

    Status = OhcResetHC (Ohc, OHC_RESET_TIMEOUT);

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    Status = OhcInitHC (Ohc);
    break;

  case EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG:
  case EFI_USB_HC_RESET_HOST_WITH_DEBUG:
    Status = EFI_UNSUPPORTED;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": exit status %r\n", Status));
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Retrieve the current state of the USB host controller.

  @param  This                   This EFI_USB2_HC_PROTOCOL instance.
  @param  State                  Variable to return the current host controller
                                 state.

  @retval EFI_SUCCESS            Host controller state was returned in State.
  @retval EFI_INVALID_PARAMETER  State is NULL.
  @retval EFI_DEVICE_ERROR       An error was encountered while attempting to
                                 retrieve the host controller's current state.

**/
EFI_STATUS
EFIAPI
OhcGetState (
  IN   EFI_USB2_HC_PROTOCOL  *This,
  OUT  EFI_USB_HC_STATE      *State
  )
{
  EFI_TPL                 OldTpl;
  USB2_HC_DEV             *Ohc;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl  = gBS->RaiseTPL (OHC_TPL);
  Ohc     = OHC_FROM_THIS (This);

#if 0
  if (OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, USBSTS_HALT)) {
    *State = EfiUsbHcStateHalt;
  } else {
    *State = EfiUsbHcStateOperational;
  }
#endif

  gBS->RestoreTPL (OldTpl);

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": current state %d\n", *State));
  return EFI_SUCCESS;
}


/**
  Sets the USB host controller to a specific state.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  State                 The state of the host controller that will be set.

  @retval EFI_SUCCESS           The USB host controller was successfully placed
                                in the state specified by State.
  @retval EFI_INVALID_PARAMETER State is invalid.
  @retval EFI_DEVICE_ERROR      Failed to set the state due to device error.

**/
EFI_STATUS
EFIAPI
OhcSetState (
  IN EFI_USB2_HC_PROTOCOL *This,
  IN EFI_USB_HC_STATE     State
  )
{
  USB2_HC_DEV             *Ohc;
  EFI_TPL                 OldTpl;
  EFI_STATUS              Status;
  EFI_USB_HC_STATE        CurState;

  Status = OhcGetState (This, &CurState);

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (CurState == State) {
    return EFI_SUCCESS;
  }

  OldTpl  = gBS->RaiseTPL (OHC_TPL);
  Ohc     = OHC_FROM_THIS (This);

  switch (State) {
  case EfiUsbHcStateHalt:
    Status = OhcHaltHC (Ohc, OHC_GENERIC_TIMEOUT);
    break;

  case EfiUsbHcStateOperational:
#if 0
    if (OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, USBSTS_SYS_ERROR)) {
      Status = EFI_DEVICE_ERROR;
      break;
    }

    //
    // Software must not write a one to this field unless the host controller
    // is in the Halted state. Doing so will yield undefined results.
    // refers to Spec[OHCI1.0-2.3.1]
    //
    if (!OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, USBSTS_HALT)) {
      Status = EFI_DEVICE_ERROR;
      break;
    }
#endif
    Status = OhcRunHC (Ohc, OHC_GENERIC_TIMEOUT);
    break;

  case EfiUsbHcStateSuspend:
    Status = EFI_UNSUPPORTED;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": exit status %r\n", Status));
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Retrieves the current status of a USB root hub port.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            The root hub port to retrieve the state from.
                                This value is zero-based.
  @param  PortStatus            Variable to receive the port state.

  @retval EFI_SUCCESS           The status of the USB root hub port specified.
                                by PortNumber was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid.
  @retval EFI_DEVICE_ERROR      Can't read register.

**/
EFI_STATUS
EFIAPI
OhcGetRootHubPortStatus (
  IN   EFI_USB2_HC_PROTOCOL  *This,
  IN   UINT8                 PortNumber,
  OUT  EFI_USB_PORT_STATUS   *PortStatus
  )
{
  USB2_HC_DEV             *Ohc;
  EFI_TPL                 OldTpl;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  UINTN                   Index;
  UINTN                   MapSize;
  EFI_STATUS              Status;
#if 0
  UINT32                  DbgCtrlStatus;
#endif

  if (PortStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl    = gBS->RaiseTPL (OHC_TPL);

  Ohc       = OHC_FROM_THIS (This);
  Status    = EFI_SUCCESS;

  TotalPort = (Ohc->HcRhDescriptorA & HCRHA_NPORTS);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset                        = (UINT32) (OHC_RHPORTSTATUS_OFFSET + (4 * PortNumber));
  PortStatus->PortStatus        = 0;
  PortStatus->PortChangeStatus  = 0;

#if 0
  if ((Ohc->DebugPortNum != 0) && (PortNumber == (Ohc->DebugPortNum - 1))) {
    DbgCtrlStatus = OhcReadDbgRegister(Ohc, 0);
    if ((DbgCtrlStatus & (USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_OWNER)) == (USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_OWNER)) {
      goto ON_EXIT;
    }
  }
#endif

  State                         = OhcReadOpReg (Ohc, Offset);

  //
  // Identify device speed. If in K state, it is low speed.
  // If the port is enabled after reset, the device is of
  // high speed. The USB bus driver should retrieve the actual
  // port speed after reset.
  //
  if (OHC_BIT_IS_SET (State, PORTSC_LINESTATE_K)) {
    PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;

  } else if (OHC_BIT_IS_SET (State, PORTSC_ENABLED)) {
    PortStatus->PortStatus |= USB_PORT_STAT_HIGH_SPEED;
  }

  //
  // Convert the OHCI port/port change state to UEFI status
  //
  MapSize = sizeof (mUsbPortStateMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (OHC_BIT_IS_SET (State, mUsbPortStateMap[Index].HwState)) {
      PortStatus->PortStatus = (UINT16) (PortStatus->PortStatus | mUsbPortStateMap[Index].UefiState);
    }
  }

  MapSize = sizeof (mUsbPortChangeMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (OHC_BIT_IS_SET (State, mUsbPortChangeMap[Index].HwState)) {
      PortStatus->PortChangeStatus = (UINT16) (PortStatus->PortChangeStatus | mUsbPortChangeMap[Index].UefiState);
    }
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Sets a feature for the specified root hub port.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            Root hub port to set.
  @param  PortFeature           Feature to set.

  @retval EFI_SUCCESS           The feature specified by PortFeature was set.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
  @retval EFI_DEVICE_ERROR      Can't read register.

**/
EFI_STATUS
EFIAPI
OhcSetRootHubPortFeature (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  IN  UINT8                 PortNumber,
  IN  EFI_USB_PORT_FEATURE  PortFeature
  )
{
  USB2_HC_DEV             *Ohc;
  EFI_TPL                 OldTpl;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  EFI_STATUS              Status;

  OldTpl    = gBS->RaiseTPL (OHC_TPL);
  Ohc       = OHC_FROM_THIS (This);
  Status    = EFI_SUCCESS;

  TotalPort = (Ohc->HcRhDescriptorA & HCRHA_NPORTS);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset  = (UINT32) (OHC_RHPORTSTATUS_OFFSET + (4 * PortNumber));
  State   = OhcReadOpReg (Ohc, Offset);

  //
  // Mask off the port status change bits, these bits are
  // write clean bit
  //
  State &= ~PORTSC_CHANGE_MASK;

  switch (PortFeature) {
  case EfiUsbPortEnable:
    //
    // Sofeware can't set this bit, Port can only be enable by
    // OHCI as a part of the reset and enable
    //
    State |= PORTSC_ENABLED;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortSuspend:
    State |= PORTSC_SUSPEND;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortReset:
    //
    // Make sure Host Controller not halt before reset it
    //
    if (OhcIsHalt (Ohc)) {
      Status = OhcRunHC (Ohc, OHC_GENERIC_TIMEOUT);

      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_INFO, __FUNCTION__ ": failed to start HC - %r\n", Status));
        break;
      }
    }

    //
    // Set one to PortReset bit must also set zero to PortEnable bit
    //
    State |= PORTSC_RESET;
    State &= ~PORTSC_ENABLED;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortPower:
    //
    // Set port power bit when PSM is 1
    //
    if ((Ohc->HcRhDescriptorA & HCRHA_PSM) == HCRHA_PSM) {
      State |= PORTSC_POWER;
      OhcWriteOpReg (Ohc, Offset, State);
    }
    break;

  case EfiUsbPortOwner:
    State |= PORTSC_OWNER;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": exit status %r\n", Status));

  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Clears a feature for the specified root hub port.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            Specifies the root hub port whose feature is
                                requested to be cleared.
  @param  PortFeature           Indicates the feature selector associated with the
                                feature clear request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was cleared
                                for the USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
  @retval EFI_DEVICE_ERROR      Can't read register.

**/
EFI_STATUS
EFIAPI
OhcClearRootHubPortFeature (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  IN  UINT8                 PortNumber,
  IN  EFI_USB_PORT_FEATURE  PortFeature
  )
{
  USB2_HC_DEV             *Ohc;
  EFI_TPL                 OldTpl;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  EFI_STATUS              Status;

  OldTpl    = gBS->RaiseTPL (OHC_TPL);
  Ohc       = OHC_FROM_THIS (This);
  Status    = EFI_SUCCESS;

  TotalPort = (Ohc->HcRhDescriptorA & HCRHA_NPORTS);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset  = OHC_RHPORTSTATUS_OFFSET + (4 * PortNumber);
  State   = OhcReadOpReg (Ohc, Offset);
  State &= ~PORTSC_CHANGE_MASK;

  switch (PortFeature) {
  case EfiUsbPortEnable:
    //
    // Clear PORT_ENABLE feature means disable port.
    //
    State &= ~PORTSC_ENABLED;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortSuspend:
    //
    // A write of zero to this bit is ignored by the host
    // controller. The host controller will unconditionally
    // set this bit to a zero when:
    //   1. software sets the Forct Port Resume bit to a zero from a one.
    //   2. software sets the Port Reset bit to a one frome a zero.
    //
    State &= ~PORSTSC_RESUME;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortReset:
    //
    // Clear PORT_RESET means clear the reset signal.
    //
    State &= ~PORTSC_RESET;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortOwner:
    //
    // Clear port owner means this port owned by OHC
    //
    State &= ~PORTSC_OWNER;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortConnectChange:
    //
    // Clear connect status change
    //
    State |= PORTSC_CONN_CHANGE;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortEnableChange:
    //
    // Clear enable status change
    //
    State |= PORTSC_ENABLE_CHANGE;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortOverCurrentChange:
    //
    // Clear PortOverCurrent change
    //
    State |= PORTSC_OVERCUR_CHANGE;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortPower:
    //
    // Clear port power bit when PSM is 1
    //
    if ((Ohc->HcRhDescriptorA & HCRHA_PSM) == HCRHA_PSM) {
      State &= ~PORTSC_POWER;
      OhcWriteOpReg (Ohc, Offset, State);
    }
    break;
  case EfiUsbPortSuspendChange:
  case EfiUsbPortResetChange:
    //
    // Not supported or not related operation
    //
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

ON_EXIT:
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": exit status %r\n", Status));
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Submits control transfer to a target USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         The target device address.
  @param  DeviceSpeed           Target device speed.
  @param  MaximumPacketLength   Maximum packet size the default control transfer
                                endpoint is capable of sending or receiving.
  @param  Request               USB device request to send.
  @param  TransferDirection     Specifies the data direction for the data stage
  @param  Data                  Data buffer to be transmitted or received from USB
                                device.
  @param  DataLength            The size (in bytes) of the data buffer.
  @param  TimeOut               Indicates the maximum timeout, in millisecond.
  @param  Translator            Transaction translator to be used by this device.
  @param  TransferResult        Return the result of this control transfer.

  @retval EFI_SUCCESS           Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
OhcControlTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  EFI_USB_DEVICE_REQUEST              *Request,
  IN  EFI_USB_DATA_DIRECTION              TransferDirection,
  IN  OUT VOID                            *Data,
  IN  OUT UINTN                           *DataLength,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
{
  USB2_HC_DEV             *Ohc;
  URB                     *Urb;
  EFI_TPL                 OldTpl;
  UINT8                   Endpoint;
  EFI_STATUS              Status;

  //
  // Validate parameters
  //
  if ((Request == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbDataIn) &&
      (TransferDirection != EfiUsbDataOut) &&
      (TransferDirection != EfiUsbNoData)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection == EfiUsbNoData) &&
      ((Data != NULL) || (*DataLength != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbNoData) &&
     ((Data == NULL) || (*DataLength == 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MaximumPacketLength != 8)  && (MaximumPacketLength != 16) &&
      (MaximumPacketLength != 32) && (MaximumPacketLength != 64)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) && (MaximumPacketLength != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl          = gBS->RaiseTPL (OHC_TPL);
  Ohc             = OHC_FROM_THIS (This);

  Status          = EFI_DEVICE_ERROR;
  *TransferResult = EFI_USB_ERR_SYSTEM;

  if (OhcIsHalt (Ohc) || OhcIsSysError (Ohc)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": HC halted at entrance\n"));

    OhcAckAllInterrupt (Ohc);
    goto ON_EXIT;
  }

  OhcAckAllInterrupt (Ohc);

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  //
  //
  // Encode the direction in address, although default control
  // endpoint is bidirectional. OhcCreateUrb expects this
  // combination of Ep addr and its direction.
  //
  Endpoint = (UINT8) (0 | ((TransferDirection == EfiUsbDataIn) ? 0x80 : 0));
  Urb = OhcCreateUrb (
          Ohc,
          DeviceAddress,
          Endpoint,
          DeviceSpeed,
          0,
          MaximumPacketLength,
          Translator,
          OHC_CTRL_TRANSFER,
          Request,
          Data,
          *DataLength,
          NULL,
          NULL,
          1
          );

  if (Urb == NULL) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to create URB"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  OhcLinkQhToAsync (Ohc, Urb->Qh);
  Status = OhcExecTransfer (Ohc, Urb, TimeOut);
  OhcUnlinkQhFromAsync (Ohc, Urb->Qh);

  //
  // Get the status from URB. The result is updated in OhcCheckUrbResult
  // which is called by OhcExecTransfer
  //
  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  }

  OhcAckAllInterrupt (Ohc);
  OhcFreeUrb (Ohc, Urb);

ON_EXIT:
  Ohc->PciIo->Flush (Ohc->PciIo);
  gBS->RestoreTPL (OldTpl);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": error - %r, transfer - %x\n", Status, *TransferResult));
  }

  return Status;
}


/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction in bit 7.
  @param  DeviceSpeed           Device speed, Low speed device doesn't support bulk
                                transfer.
  @param  MaximumPacketLength   Maximum packet size the endpoint is capable of
                                sending or receiving.
  @param  DataBuffersNumber     Number of data buffers prepared for the transfer.
  @param  Data                  Array of pointers to the buffers of data to transmit
                                from or receive into.
  @param  DataLength            The lenght of the data buffer.
  @param  DataToggle            On input, the initial data toggle for the transfer;
                                On output, it is updated to to next data toggle to
                                use of the subsequent bulk transfer.
  @param  TimeOut               Indicates the maximum time, in millisecond, which
                                the transfer is allowed to complete.
  @param  Translator            A pointr to the transaction translator data.
  @param  TransferResult        A pointer to the detailed result information of the
                                bulk transfer.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
OhcBulkTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN  OUT VOID                            *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
{
  USB2_HC_DEV             *Ohc;
  URB                     *Urb;
  EFI_TPL                 OldTpl;
  EFI_STATUS              Status;

  //
  // Validate the parameters
  //
  if ((DataLength == NULL) || (*DataLength == 0) ||
      (Data == NULL) || (Data[0] == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 0) && (*DataToggle != 1)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) ||
      ((DeviceSpeed == EFI_USB_SPEED_FULL) && (MaximumPacketLength > 64)) ||
      ((EFI_USB_SPEED_HIGH == DeviceSpeed) && (MaximumPacketLength > 512))) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl          = gBS->RaiseTPL (OHC_TPL);
  Ohc             = OHC_FROM_THIS (This);

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;

  if (OhcIsHalt (Ohc) || OhcIsSysError (Ohc)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": HC is halted\n"));

    OhcAckAllInterrupt (Ohc);
    goto ON_EXIT;
  }

  OhcAckAllInterrupt (Ohc);

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  //
  Urb = OhcCreateUrb (
          Ohc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          *DataToggle,
          MaximumPacketLength,
          Translator,
          OHC_BULK_TRANSFER,
          NULL,
          Data[0],
          *DataLength,
          NULL,
          NULL,
          1
          );

  if (Urb == NULL) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to create URB\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  OhcLinkQhToAsync (Ohc, Urb->Qh);
  Status = OhcExecTransfer (Ohc, Urb, TimeOut);
  OhcUnlinkQhFromAsync (Ohc, Urb->Qh);

  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;
  *DataToggle     = Urb->DataToggle;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  }

  OhcAckAllInterrupt (Ohc);
  OhcFreeUrb (Ohc, Urb);

ON_EXIT:
  Ohc->PciIo->Flush (Ohc->PciIo);
  gBS->RestoreTPL (OldTpl);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": error - %r, transfer - %x\n", Status, *TransferResult));
  }

  return Status;
}


/**
  Submits an asynchronous interrupt transfer to an
  interrupt endpoint of a USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction encoded in bit 7
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Maximum packet size the target endpoint is capable
  @param  IsNewTransfer         If TRUE, to submit an new asynchronous interrupt
                                transfer If FALSE, to remove the specified
                                asynchronous interrupt.
  @param  DataToggle            On input, the initial data toggle to use; on output,
                                it is updated to indicate the next data toggle.
  @param  PollingInterval       The he interval, in milliseconds, that the transfer
                                is polled.
  @param  DataLength            The length of data to receive at the rate specified
                                by  PollingInterval.
  @param  Translator            Transaction translator to use.
  @param  CallBackFunction      Function to call at the rate specified by
                                PollingInterval.
  @param  Context               Context to CallBackFunction.

  @retval EFI_SUCCESS           The request has been successfully submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request failed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
OhcAsyncInterruptTransfer (
  IN  EFI_USB2_HC_PROTOCOL                  * This,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN  BOOLEAN                               IsNewTransfer,
  IN  OUT UINT8                             *DataToggle,
  IN  UINTN                                 PollingInterval,
  IN  UINTN                                 DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR    * Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction,
  IN  VOID                                  *Context OPTIONAL
  )
{
  USB2_HC_DEV             *Ohc;
  URB                     *Urb;
  EFI_TPL                 OldTpl;
  EFI_STATUS              Status;
  UINT8                   *Data;

  //
  // Validate parameters
  //
  if (!OHCI_IS_DATAIN (EndPointAddress)) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsNewTransfer) {
    if (DataLength == 0) {
      return EFI_INVALID_PARAMETER;
    }

    if ((*DataToggle != 1) && (*DataToggle != 0)) {
      return EFI_INVALID_PARAMETER;
    }

    if ((PollingInterval > 255) || (PollingInterval < 1)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  OldTpl  = gBS->RaiseTPL (OHC_TPL);
  Ohc     = OHC_FROM_THIS (This);

  //
  // Delete Async interrupt transfer request. DataToggle will return
  // the next data toggle to use.
  //
  if (!IsNewTransfer) {
    Status = OhciDelAsyncIntTransfer (Ohc, DeviceAddress, EndPointAddress, DataToggle);

    DEBUG ((EFI_D_INFO, __FUNCTION__ ": remove old transfer - %r\n", Status));
    goto ON_EXIT;
  }

  Status = EFI_SUCCESS;

  if (OhcIsHalt (Ohc) || OhcIsSysError (Ohc)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": HC is halt\n"));
    OhcAckAllInterrupt (Ohc);

    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  OhcAckAllInterrupt (Ohc);

  Data = AllocatePool (DataLength);

  if (Data == NULL) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to allocate buffer\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Urb = OhcCreateUrb (
          Ohc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          *DataToggle,
          MaximumPacketLength,
          Translator,
          OHC_INT_TRANSFER_ASYNC,
          NULL,
          Data,
          DataLength,
          CallBackFunction,
          Context,
          PollingInterval
          );

  if (Urb == NULL) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to create URB\n"));

    gBS->FreePool (Data);
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // New asynchronous transfer must inserted to the head.
  // Check the comments in OhcMoniteAsyncRequests
  //
  OhcLinkQhToPeriod (Ohc, Urb->Qh);
  InsertHeadList (&Ohc->AsyncIntTransfers, &Urb->UrbList);

ON_EXIT:
  Ohc->PciIo->Flush (Ohc->PciIo);
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Submits synchronous interrupt transfer to an interrupt endpoint
  of a USB device.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction encoded in bit 7
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Maximum packet size the target endpoint is capable
                                of sending or receiving.
  @param  Data                  Buffer of data that will be transmitted to  USB
                                device or received from USB device.
  @param  DataLength            On input, the size, in bytes, of the data buffer; On
                                output, the number of bytes transferred.
  @param  DataToggle            On input, the initial data toggle to use; on output,
                                it is updated to indicate the next data toggle.
  @param  TimeOut               Maximum time, in second, to complete.
  @param  Translator            Transaction translator to use.
  @param  TransferResult        Variable to receive the transfer result.

  @return EFI_SUCCESS           The transfer was completed successfully.
  @return EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @return EFI_INVALID_PARAMETER Some parameters are invalid.
  @return EFI_TIMEOUT           The transfer failed due to timeout.
  @return EFI_DEVICE_ERROR      The failed due to host controller or device error

**/
EFI_STATUS
EFIAPI
OhcSyncInterruptTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  OUT VOID                            *Data,
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
{
  USB2_HC_DEV             *Ohc;
  EFI_TPL                 OldTpl;
  URB                     *Urb;
  EFI_STATUS              Status;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  //
  // Validates parameters
  //
  if ((DataLength == NULL) || (*DataLength == 0) ||
      (Data == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!OHCI_IS_DATAIN (EndPointAddress)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 1) && (*DataToggle != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (((DeviceSpeed == EFI_USB_SPEED_LOW) && (MaximumPacketLength != 8))  ||
      ((DeviceSpeed == EFI_USB_SPEED_FULL) && (MaximumPacketLength > 64)) ||
      ((DeviceSpeed == EFI_USB_SPEED_HIGH) && (MaximumPacketLength > 3072))) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": params validated\n"));

  OldTpl          = gBS->RaiseTPL (OHC_TPL);
  Ohc             = OHC_FROM_THIS (This);

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;

  if (OhcIsHalt (Ohc) || OhcIsSysError (Ohc)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": HC is halt\n"));

    OhcAckAllInterrupt (Ohc);
    goto ON_EXIT;
  }

  OhcAckAllInterrupt (Ohc);

  Urb = OhcCreateUrb (
          Ohc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          *DataToggle,
          MaximumPacketLength,
          Translator,
          OHC_INT_TRANSFER_SYNC,
          NULL,
          Data,
          *DataLength,
          NULL,
          NULL,
          1
          );

  if (Urb == NULL) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to create URB\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  OhcLinkQhToPeriod (Ohc, Urb->Qh);
  Status = OhcExecTransfer (Ohc, Urb, TimeOut);
  OhcUnlinkQhFromPeriod (Ohc, Urb->Qh);

  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;
  *DataToggle     = Urb->DataToggle;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  }

ON_EXIT:
  Ohc->PciIo->Flush (Ohc->PciIo);
  gBS->RestoreTPL (OldTpl);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": error - %r, transfer - %x\n", Status, *TransferResult));
  }

  return Status;
}


/**
  Submits isochronous transfer to a target USB device.

  @param  This                 This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress        Target device address.
  @param  EndPointAddress      End point address with its direction.
  @param  DeviceSpeed          Device speed, Low speed device doesn't support this
                               type.
  @param  MaximumPacketLength  Maximum packet size that the endpoint is capable of
                               sending or receiving.
  @param  DataBuffersNumber    Number of data buffers prepared for the transfer.
  @param  Data                 Array of pointers to the buffers of data that will
                               be transmitted to USB device or received from USB
                               device.
  @param  DataLength           The size, in bytes, of the data buffer.
  @param  Translator           Transaction translator to use.
  @param  TransferResult       Variable to receive the transfer result.

  @return EFI_UNSUPPORTED      Isochronous transfer is unsupported.

**/
EFI_STATUS
EFIAPI
OhcIsochronousTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
{
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter/leave\n"));
  return EFI_UNSUPPORTED;
}


/**
  Submits Async isochronous transfer to a target USB device.

  @param  This                 This EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress        Target device address.
  @param  EndPointAddress      End point address with its direction.
  @param  DeviceSpeed          Device speed, Low speed device doesn't support this
                               type.
  @param  MaximumPacketLength  Maximum packet size that the endpoint is capable of
                               sending or receiving.
  @param  DataBuffersNumber    Number of data buffers prepared for the transfer.
  @param  Data                 Array of pointers to the buffers of data that will
                               be transmitted to USB device or received from USB
                               device.
  @param  DataLength           The size, in bytes, of the data buffer.
  @param  Translator           Transaction translator to use.
  @param  IsochronousCallBack  Function to be called when the transfer complete.
  @param  Context              Context passed to the call back function as
                               parameter.

  @return EFI_UNSUPPORTED      Isochronous transfer isn't supported.

**/
EFI_STATUS
EFIAPI
OhcAsyncIsochronousTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN  VOID                                *Context
  )
{
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter/leave\n"));
  return EFI_UNSUPPORTED;
}

/**
  Entry point for EFI drivers.

  @param  ImageHandle       EFI_HANDLE.
  @param  SystemTable       EFI_SYSTEM_TABLE.

  @return EFI_SUCCESS       Success.
          EFI_DEVICE_ERROR  Fail.

**/
EFI_STATUS
EFIAPI
OhcDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter/leave\n"));
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gOhciDriverBinding,
           ImageHandle,
           &gOhciComponentName,
           &gOhciComponentName2
           );
}


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
  )
{
  EFI_STATUS              Status;
  EFI_PCI_IO_PROTOCOL     *PciIo;
  USB_CLASSC              UsbClassCReg;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter for %p\n", Controller));

  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": first bail out for %p with %r\n", Controller, Status));
    return EFI_UNSUPPORTED;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (USB_CLASSC) / sizeof (UINT8),
                        &UsbClassCReg
                        );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": second bail out for %p with %r\n", Controller, Status));
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  //
  // Test whether the controller belongs to OHCI type
  //
  if ((UsbClassCReg.BaseCode != PCI_CLASS_SERIAL) ||
      (UsbClassCReg.SubClassCode != PCI_CLASS_SERIAL_USB) ||
      (UsbClassCReg.ProgInterface != PCI_IF_OHCI)
      ) {

    Status = EFI_UNSUPPORTED;
  }

ON_EXIT:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave for %p with %r\n", Controller, Status));
  return Status;
}

/**
  Create and initialize a USB2_HC_DEV.

  @param  PciIo                  The PciIo on this device.
  @param  DevicePath             The device path of host controller.
  @param  OriginalPciAttributes  Original PCI attributes.

  @return  The allocated and initialized USB2_HC_DEV structure if created,
           otherwise NULL.

**/
USB2_HC_DEV *
OhcCreateUsb2Hc (
  IN EFI_PCI_IO_PROTOCOL       *PciIo,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN UINT64                    OriginalPciAttributes
  )
{
  USB2_HC_DEV             *Ohc;
  EFI_STATUS              Status;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Ohc = AllocateZeroPool (sizeof (USB2_HC_DEV));

  if (Ohc == NULL) {
    return NULL;
  }

  //
  // Init EFI_USB2_HC_PROTOCOL interface and private data structure
  //
  Ohc->Signature                        = USB2_HC_DEV_SIGNATURE;

  Ohc->Usb2Hc.GetCapability             = OhcGetCapability;
  Ohc->Usb2Hc.Reset                     = OhcReset;
  Ohc->Usb2Hc.GetState                  = OhcGetState;
  Ohc->Usb2Hc.SetState                  = OhcSetState;
  Ohc->Usb2Hc.ControlTransfer           = OhcControlTransfer;
  Ohc->Usb2Hc.BulkTransfer              = OhcBulkTransfer;
  Ohc->Usb2Hc.AsyncInterruptTransfer    = OhcAsyncInterruptTransfer;
  Ohc->Usb2Hc.SyncInterruptTransfer     = OhcSyncInterruptTransfer;
  Ohc->Usb2Hc.IsochronousTransfer       = OhcIsochronousTransfer;
  Ohc->Usb2Hc.AsyncIsochronousTransfer  = OhcAsyncIsochronousTransfer;
  Ohc->Usb2Hc.GetRootHubPortStatus      = OhcGetRootHubPortStatus;
  Ohc->Usb2Hc.SetRootHubPortFeature     = OhcSetRootHubPortFeature;
  Ohc->Usb2Hc.ClearRootHubPortFeature   = OhcClearRootHubPortFeature;
  Ohc->Usb2Hc.MajorRevision             = 0x1;
  Ohc->Usb2Hc.MinorRevision             = 0x1;

  Ohc->PciIo                 = PciIo;
  Ohc->DevicePath            = DevicePath;
  Ohc->OriginalPciAttributes = OriginalPciAttributes;

  InitializeListHead (&Ohc->AsyncIntTransfers);

  //
  // Create AsyncRequest Polling Timer
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OhcMonitorAsyncRequests,
                  Ohc,
                  &Ohc->PollTimer
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": bail out with %r\n", Status));
    gBS->FreePool (Ohc);
    return NULL;
  }

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
  return Ohc;
}

/**
  One notified function to stop the Host Controller when gBS->ExitBootServices() called.

  @param  Event                   Pointer to this event
  @param  Context                 Event hanlder private data

**/
VOID
EFIAPI
OhcExitBootService (
  EFI_EVENT                      Event,
  VOID                           *Context
  )

{
  USB2_HC_DEV   *Ohc;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter\n"));

  Ohc = (USB2_HC_DEV *) Context;

  //
  // Reset the Host Controller
  //
  OhcResetHC (Ohc, OHC_RESET_TIMEOUT);
  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave\n"));
}


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
  )
{
  EFI_STATUS              Status;
  USB2_HC_DEV             *Ohc;
  EFI_PCI_IO_PROTOCOL     *PciIo;
  UINT64                  Supports;
  UINT64                  OriginalPciAttributes;
  BOOLEAN                 PciAttributesSaved;
  EFI_DEVICE_PATH_PROTOCOL  *HcDevicePath;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter for %p\n", Controller));

  //
  // Open the PciIo Protocol, then enable the USB host controller
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": first bail out for %p with %r\n", Controller, Status));
    return Status;
  }

  //
  // Open Device Path Protocol for on USB host controller
  //
  HcDevicePath = NULL;
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &HcDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  PciAttributesSaved = FALSE;
  //
  // Save original PCI attributes
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationGet,
                    0,
                    &OriginalPciAttributes
                    );

  if (EFI_ERROR (Status)) {
    goto CLOSE_PCIIO;
  }
  PciAttributesSaved = TRUE;

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (!EFI_ERROR (Status)) {
    Supports &= EFI_PCI_DEVICE_ENABLE;
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationEnable,
                      Supports,
                      NULL
                      );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to enable controller (%r)\n", Status));
    goto CLOSE_PCIIO;
  }

  //
  // Create then install USB2_HC_PROTOCOL
  //
  Ohc = OhcCreateUsb2Hc (PciIo, HcDevicePath, OriginalPciAttributes);

  if (Ohc == NULL) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to create USB2_HC\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto CLOSE_PCIIO;
  }

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiUsb2HcProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Ohc->Usb2Hc
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to install USB2_HC Protocol (%r)\n", Status));
    goto FREE_POOL;
  }

  //
  // Robustnesss improvement such as for Duet platform
  // Default is not required.
  //
  if (FeaturePcdGet (PcdTurnOffUsbLegacySupport)) {
    OhcClearLegacySupport (Ohc);
  }

  Status = OhcInitHC (Ohc);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to init host controller (%r)\n", Status));
    goto UNINSTALL_USBHC;
  }

  //
  // Start the asynchronous interrupt monitor
  //
  Status = gBS->SetTimer (Ohc->PollTimer, TimerPeriodic, OHC_ASYNC_POLL_INTERVAL);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": failed to start async interrupt monitor (%r)\n", Status));

    OhcHaltHC (Ohc, OHC_GENERIC_TIMEOUT);
    goto UNINSTALL_USBHC;
  }

  //
  // Create event to stop the HC when exit boot service.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  OhcExitBootService,
                  Ohc,
                  &gEfiEventExitBootServicesGuid,
                  &Ohc->ExitBootServiceEvent
                  );
  if (EFI_ERROR (Status)) {
    goto UNINSTALL_USBHC;
  }

  //
  // Install the component name protocol, don't fail the start
  // because of something for display.
  //
  AddUnicodeString2 (
    "eng",
    gOhciComponentName.SupportedLanguages,
    &Ohc->ControllerNameTable,
    L"USB Open Host Controller",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gOhciComponentName2.SupportedLanguages,
    &Ohc->ControllerNameTable,
    L"USB Open Host Controller",
    FALSE
    );


  DEBUG ((EFI_D_INFO, __FUNCTION__ ": OHCI started for controller @ %p\n", Controller));
  return EFI_SUCCESS;

UNINSTALL_USBHC:
  gBS->UninstallProtocolInterface (
         Controller,
         &gEfiUsb2HcProtocolGuid,
         &Ohc->Usb2Hc
         );

FREE_POOL:
  OhcFreeSched (Ohc);
  gBS->CloseEvent (Ohc->PollTimer);
  gBS->FreePool (Ohc);

CLOSE_PCIIO:
  if (PciAttributesSaved) {
    //
    // Restore original PCI attributes
    //
    PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSet,
                    OriginalPciAttributes,
                    NULL
                    );
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave for %p with %r\n", Controller, Status));
  return Status;
}


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
  )
{
  EFI_STATUS            Status;
  EFI_USB2_HC_PROTOCOL  *Usb2Hc;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  USB2_HC_DEV           *Ohc;

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": enter for %p\n", Controller));

  //
  // Test whether the Controller handler passed in is a valid
  // Usb controller handle that should be supported, if not,
  // return the error status directly
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  (VOID **) &Usb2Hc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": first bail out for %p with %r\n", Controller, Status));
    return Status;
  }

  Ohc   = OHC_FROM_THIS (Usb2Hc);
  PciIo = Ohc->PciIo;

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  Usb2Hc
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, __FUNCTION__ ": second bail out for %p with %r\n", Controller, Status));
    return Status;
  }

  //
  // Stop AsyncRequest Polling timer then stop the OHCI driver
  // and uninstall the OHCI protocol.
  //
  gBS->SetTimer (Ohc->PollTimer, TimerCancel, OHC_ASYNC_POLL_INTERVAL);
  OhcHaltHC (Ohc, OHC_GENERIC_TIMEOUT);

  if (Ohc->PollTimer != NULL) {
    gBS->CloseEvent (Ohc->PollTimer);
  }

  if (Ohc->ExitBootServiceEvent != NULL) {
    gBS->CloseEvent (Ohc->ExitBootServiceEvent);
  }

  OhcFreeSched (Ohc);

  if (Ohc->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (Ohc->ControllerNameTable);
  }

  //
  // Restore original PCI attributes
  //
  PciIo->Attributes (
                  PciIo,
                  EfiPciIoAttributeOperationSet,
                  Ohc->OriginalPciAttributes,
                  NULL
                  );

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  FreePool (Ohc);

  DEBUG ((EFI_D_INFO, __FUNCTION__ ": leave for %p\n", Controller));
  return EFI_SUCCESS;
}
