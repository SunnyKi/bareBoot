/** @file
  The OHCI controller driver.

Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <debug.h>
#include <Library/ShiftKeysLib.h>

#include "Ohci.h"

//
// Two arrays used to translate the OHCI port state (change)
// to the UEFI protocol's port state (change).
//
USB_PORT_STATE_MAP  mUsbPortStateMap[] = {
  {OHC_PORTSC_CCS,   USB_PORT_STAT_CONNECTION},
  {OHC_PORTSC_PED,   USB_PORT_STAT_ENABLE},
  {OHC_PORTSC_OCA,   USB_PORT_STAT_OVERCURRENT},
  {OHC_PORTSC_RESET, USB_PORT_STAT_RESET}
};

USB_PORT_STATE_MAP  mUsbPortChangeMap[] = {
  {OHC_PORTSC_CSC, USB_PORT_STAT_C_CONNECTION},
  {OHC_PORTSC_PEC, USB_PORT_STAT_C_ENABLE},
  {OHC_PORTSC_OCC, USB_PORT_STAT_C_OVERCURRENT},
  {OHC_PORTSC_PRC, USB_PORT_STAT_C_RESET}
};

USB_PORT_STATE_MAP  mUsbHubPortStateMap[] = {
  {OHC_HUB_PORTSC_CCS,   USB_PORT_STAT_CONNECTION},
  {OHC_HUB_PORTSC_PED,   USB_PORT_STAT_ENABLE},
  {OHC_HUB_PORTSC_OCA,   USB_PORT_STAT_OVERCURRENT},
  {OHC_HUB_PORTSC_RESET, USB_PORT_STAT_RESET}
};

USB_PORT_STATE_MAP  mUsbHubPortChangeMap[] = {
  {OHC_HUB_PORTSC_CSC, USB_PORT_STAT_C_CONNECTION},
  {OHC_HUB_PORTSC_PEC, USB_PORT_STAT_C_ENABLE},
  {OHC_HUB_PORTSC_OCC, USB_PORT_STAT_C_OVERCURRENT},
  {OHC_HUB_PORTSC_PRC, USB_PORT_STAT_C_RESET}
};

EFI_DRIVER_BINDING_PROTOCOL  gOhciDriverBinding = {
  OhcDriverBindingSupported,
  OhcDriverBindingStart,
  OhcDriverBindingStop,
  0x30,
  NULL,
  NULL
};

//
// Template for Ohci's Usb2 Host Controller Protocol Instance.
//
EFI_USB2_HC_PROTOCOL gOhciUsb2HcTemplate = {
  OhcGetCapability,
  OhcReset,
  OhcGetState,
  OhcSetState,
  OhcControlTransfer,
  OhcBulkTransfer,
  OhcAsyncInterruptTransfer,
  OhcSyncInterruptTransfer,
  OhcIsochronousTransfer,
  OhcAsyncIsochronousTransfer,
  OhcGetRootHubPortStatus,
  OhcSetRootHubPortFeature,
  OhcClearRootHubPortFeature,
  0x3,
  0x0
};

/**
  Retrieves the capability of root hub ports.

  @param  This                  The EFI_USB2_HC_PROTOCOL instance.
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
  USB_OHCI_INSTANCE  *Ohc;
  EFI_TPL            OldTpl;

  if ((MaxSpeed == NULL) || (PortNumber == NULL) || (Is64BitCapable == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl          = gBS->RaiseTPL (OHC_TPL);

  Ohc             = OHC_FROM_THIS (This);
  *MaxSpeed       = EFI_USB_SPEED_SUPER;
  *PortNumber     = (UINT8) (Ohc->HcSParams1.Data.MaxPorts);
  *Is64BitCapable = (UINT8) (Ohc->HcCParams.Data.Ac64);
  DEBUG ((EFI_D_INFO, "OhcGetCapability: %d ports, 64 bit %d\n", *PortNumber, *Is64BitCapable));

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
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN UINT16                Attributes
  )
{
  USB_OHCI_INSTANCE  *Ohc;
  EFI_STATUS         Status;
  EFI_TPL            OldTpl;

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

  OldTpl = gBS->RaiseTPL (OHC_TPL);

  switch (Attributes) {
  case EFI_USB_HC_RESET_GLOBAL:
  //
  // Flow through, same behavior as Host Controller Reset
  //
  case EFI_USB_HC_RESET_HOST_CONTROLLER:
    if ((Ohc->DebugCapSupOffset != 0xFFFFFFFF) && ((OhcReadExtCapReg (Ohc, Ohc->DebugCapSupOffset) & 0xFF) == OHC_CAP_USB_DEBUG) &&
        ((OhcReadExtCapReg (Ohc, Ohc->DebugCapSupOffset + OHC_DC_DCCTRL) & BIT0) != 0)) {
      Status = EFI_SUCCESS;
      goto ON_EXIT;
    }
    //
    // Host Controller must be Halt when Reset it
    //
    if (!OhcIsHalt (Ohc)) {
      Status = OhcHaltHC (Ohc, OHC_GENERIC_TIMEOUT);

      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto ON_EXIT;
      }
    }

    Status = OhcResetHC (Ohc, OHC_RESET_TIMEOUT);
    ASSERT (!(OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, OHC_USBSTS_CNR)));

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
    //
    // Clean up the asynchronous transfers, currently only
    // interrupt supports asynchronous operation.
    //
    OhciDelAllAsyncIntTransfers (Ohc);
    OhcFreeSched (Ohc);

    OhcInitSched (Ohc);
    break;

  case EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG:
  case EFI_USB_HC_RESET_HOST_WITH_DEBUG:
    Status = EFI_UNSUPPORTED;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  DEBUG ((EFI_D_INFO, "OhcReset: status %r\n", Status));
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
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT EFI_USB_HC_STATE      *State
  )
{
  USB_OHCI_INSTANCE  *Ohc;
  EFI_TPL            OldTpl;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (OHC_TPL);

  Ohc    = OHC_FROM_THIS (This);

  if (OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, OHC_USBSTS_HALT)) {
    *State = EfiUsbHcStateHalt;
  } else {
    *State = EfiUsbHcStateOperational;
  }

  DEBUG ((EFI_D_INFO, "OhcGetState: current state %d\n", *State));
  gBS->RestoreTPL (OldTpl);

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
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN EFI_USB_HC_STATE      State
  )
{
  USB_OHCI_INSTANCE   *Ohc;
  EFI_STATUS          Status;
  EFI_USB_HC_STATE    CurState;
  EFI_TPL             OldTpl;

  Status = OhcGetState (This, &CurState);

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (CurState == State) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (OHC_TPL);

  Ohc    = OHC_FROM_THIS (This);

  switch (State) {
  case EfiUsbHcStateHalt:
    Status = OhcHaltHC (Ohc, OHC_GENERIC_TIMEOUT);
    break;

  case EfiUsbHcStateOperational:
    if (OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, OHC_USBSTS_HSE)) {
      Status = EFI_DEVICE_ERROR;
      break;
    }

    //
    // Software must not write a one to this field unless the host controller
    // is in the Halted state. Doing so will yield undefined results.
    // refers to Spec[OHCI1.0-2.3.1]
    //
    if (!OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, OHC_USBSTS_HALT)) {
      Status = EFI_DEVICE_ERROR;
      break;
    }

    Status = OhcRunHC (Ohc, OHC_GENERIC_TIMEOUT);
    break;

  case EfiUsbHcStateSuspend:
    Status = EFI_UNSUPPORTED;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_INFO, "OhcSetState: status %r\n", Status));
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
  IN  EFI_USB2_HC_PROTOCOL  *This,
  IN  UINT8                 PortNumber,
  OUT EFI_USB_PORT_STATUS   *PortStatus
  )
{
  USB_OHCI_INSTANCE       *Ohc;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  UINTN                   Index;
  UINTN                   MapSize;
  EFI_STATUS              Status;
  USB_DEV_ROUTE           ParentRouteChart;
  EFI_TPL                 OldTpl;

  if (PortStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (OHC_TPL);

  Ohc       = OHC_FROM_THIS (This);
  Status    = EFI_SUCCESS;

  TotalPort = Ohc->HcSParams1.Data.MaxPorts;

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset                       = (UINT32) (OHC_PORTSC_OFFSET + (0x10 * PortNumber));
  PortStatus->PortStatus       = 0;
  PortStatus->PortChangeStatus = 0;

  State = OhcReadOpReg (Ohc, Offset);

  //
  // According to OHCI 1.0 spec, bit 10~13 of the root port status register identifies the speed of the attached device.
  //
  switch ((State & OHC_PORTSC_PS) >> 10) {
  case 2:
    PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
    break;

  case 3:
    PortStatus->PortStatus |= USB_PORT_STAT_HIGH_SPEED;
    break;

  case 4:
    PortStatus->PortStatus |= USB_PORT_STAT_SUPER_SPEED;
    break;

  default:
    break;
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
  //
  // Bit5~8 reflects its current link state.
  //
  if ((State & OHC_PORTSC_PLS) >> 5 == 3) {
    PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
  }

  MapSize = sizeof (mUsbPortChangeMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (OHC_BIT_IS_SET (State, mUsbPortChangeMap[Index].HwState)) {
      PortStatus->PortChangeStatus = (UINT16) (PortStatus->PortChangeStatus | mUsbPortChangeMap[Index].UefiState);
    }
  }

  //
  // Poll the root port status register to enable/disable corresponding device slot if there is a device attached/detached.
  // For those devices behind hub, we get its attach/detach event by hooking Get_Port_Status request at control transfer for those hub.
  //
  ParentRouteChart.Dword = 0;
  OhcPollPortStatusChange (Ohc, ParentRouteChart, PortNumber, PortStatus);

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
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  )
{
  USB_OHCI_INSTANCE       *Ohc;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  UINT8                   SlotId;
  USB_DEV_ROUTE           RouteChart;
  EFI_STATUS              Status;
  EFI_TPL                 OldTpl;

  OldTpl = gBS->RaiseTPL (OHC_TPL);

  Ohc    = OHC_FROM_THIS (This);
  Status = EFI_SUCCESS;

  TotalPort = (Ohc->HcSParams1.Data.MaxPorts);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset = (UINT32) (OHC_PORTSC_OFFSET + (0x10 * PortNumber));
  State  = OhcReadOpReg (Ohc, Offset);

  //
  // Mask off the port status change bits, these bits are
  // write clean bit
  //
  State &= ~ (BIT1 | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22 | BIT23);

  switch (PortFeature) {
  case EfiUsbPortEnable:
    //
    // Ports may only be enabled by the xHC. Software cannot enable a port by writing a '1' to this flag.
    // A port may be disabled by software writing a '1' to this flag.
    //
    Status = EFI_SUCCESS;
    break;

  case EfiUsbPortSuspend:
    State |= OHC_PORTSC_LWS;
    OhcWriteOpReg (Ohc, Offset, State);
    State &= ~OHC_PORTSC_PLS;
    State |= (3 << 5) ;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortReset:
    DEBUG ((EFI_D_INFO, "OhcUsbPortReset!\n"));
    //
    // Make sure Host Controller not halt before reset it
    //
    if (OhcIsHalt (Ohc)) {
      Status = OhcRunHC (Ohc, OHC_GENERIC_TIMEOUT);

      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_INFO, "OhcSetRootHubPortFeature :failed to start HC - %r\n", Status));
        break;
      }
    }

    RouteChart.Route.RouteString = 0;
    RouteChart.Route.RootPortNum = PortNumber + 1;
    RouteChart.Route.TierNum     = 1;
    //
    // If the port reset operation happens after the usb super speed device is enabled,
    // The subsequent configuration, such as getting device descriptor, will fail.
    // So here a workaround is introduced to skip the reset operation if the device is enabled.
    //
    SlotId = OhcRouteStringToSlotId (Ohc, RouteChart);
    if (SlotId == 0) {
      //
      // 4.3.1 Resetting a Root Hub Port
      // 1) Write the PORTSC register with the Port Reset (PR) bit set to '1'.
      //
      State |= OHC_PORTSC_RESET;
      OhcWriteOpReg (Ohc, Offset, State);
      OhcWaitOpRegBit(Ohc, Offset, OHC_PORTSC_PRC, TRUE, OHC_GENERIC_TIMEOUT);
    }
    break;

  case EfiUsbPortPower:
    //
    // Not supported, ignore the operation
    //
    Status = EFI_SUCCESS;
    break;

  case EfiUsbPortOwner:
    //
    // OHCI root hub port don't has the owner bit, ignore the operation
    //
    Status = EFI_SUCCESS;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  DEBUG ((EFI_D_INFO, "OhcSetRootHubPortFeature: status %r\n", Status));
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
  IN EFI_USB2_HC_PROTOCOL  *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
  )
{
  USB_OHCI_INSTANCE       *Ohc;
  UINT32                  Offset;
  UINT32                  State;
  UINT32                  TotalPort;
  EFI_STATUS              Status;
  EFI_TPL                 OldTpl;

  OldTpl = gBS->RaiseTPL (OHC_TPL);

  Ohc       = OHC_FROM_THIS (This);
  Status    = EFI_SUCCESS;

  TotalPort = (Ohc->HcSParams1.Data.MaxPorts);

  if (PortNumber >= TotalPort) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset = OHC_PORTSC_OFFSET + (0x10 * PortNumber);

  //
  // Mask off the port status change bits, these bits are
  // write clean bit
  //
  State  = OhcReadOpReg (Ohc, Offset);
  State &= ~ (BIT1 | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22 | BIT23);

  switch (PortFeature) {
  case EfiUsbPortEnable:
    //
    // Ports may only be enabled by the xHC. Software cannot enable a port by writing a '1' to this flag.
    // A port may be disabled by software writing a '1' to this flag.
    //
    State |= OHC_PORTSC_PED;
    State &= ~OHC_PORTSC_RESET;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortSuspend:
    State |= OHC_PORTSC_LWS;
    OhcWriteOpReg (Ohc, Offset, State);
    State &= ~OHC_PORTSC_PLS;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortReset:
    //
    // PORTSC_RESET BIT(4) bit is RW1S attribute, which means Write-1-to-set status:
    // Register bits indicate status when read, a clear bit may be set by
    // writing a '1'. Writing a '0' to RW1S bits has no effect.
    //
    break;

  case EfiUsbPortOwner:
    //
    // OHCI root hub port don't has the owner bit, ignore the operation
    //
    break;

  case EfiUsbPortConnectChange:
    //
    // Clear connect status change
    //
    State |= OHC_PORTSC_CSC;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortEnableChange:
    //
    // Clear enable status change
    //
    State |= OHC_PORTSC_PEC;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortOverCurrentChange:
    //
    // Clear PortOverCurrent change
    //
    State |= OHC_PORTSC_OCC;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortResetChange:
    //
    // Clear Port Reset change
    //
    State |= OHC_PORTSC_PRC;
    OhcWriteOpReg (Ohc, Offset, State);
    break;

  case EfiUsbPortPower:
  case EfiUsbPortSuspendChange:
    //
    // Not supported or not related operation
    //
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

ON_EXIT:
  DEBUG ((EFI_D_INFO, "OhcClearRootHubPortFeature: status %r\n", Status));
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
  @param  Timeout               Indicates the maximum timeout, in millisecond.
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
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST              *Request,
  IN     EFI_USB_DATA_DIRECTION              TransferDirection,
  IN OUT VOID                                *Data,
  IN OUT UINTN                               *DataLength,
  IN     UINTN                               Timeout,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
  USB_OHCI_INSTANCE       *Ohc;
  URB                     *Urb;
  UINT8                   Endpoint;
  UINT8                   Index;
  UINT8                   DescriptorType;
  UINT8                   SlotId;
  UINT8                   TTT;
  UINT8                   MTT;
  UINT32                  MaxPacket0;
  EFI_USB_HUB_DESCRIPTOR  *HubDesc;
  EFI_TPL                 OldTpl;
  EFI_STATUS              Status;
  EFI_STATUS              RecoveryStatus;
  UINTN                   MapSize;
  EFI_USB_PORT_STATUS     PortStatus;
  UINT32                  State;

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
      (MaximumPacketLength != 32) && (MaximumPacketLength != 64) &&
      (MaximumPacketLength != 512)
      ) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) && (MaximumPacketLength != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_SUPER) && (MaximumPacketLength != 512)) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (OHC_TPL);

  Ohc             = OHC_FROM_THIS (This);

  Status          = EFI_DEVICE_ERROR;
  *TransferResult = EFI_USB_ERR_SYSTEM;

  if (OhcIsHalt (Ohc) || OhcIsSysError (Ohc)) {
    DEBUG ((EFI_D_ERROR, "OhcControlTransfer: HC halted at entrance\n"));
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = OhcBusDevAddrToSlotId (Ohc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  //
  // Hook the Set_Address request from UsbBus.
  // According to OHCI 1.0 spec, the Set_Address request is replaced by OHCI's Address_Device cmd.
  //
  if ((Request->Request     == USB_REQ_SET_ADDRESS) &&
      (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE))) {
    //
    // Reset the BusDevAddr field of all disabled entries in UsbDevContext array firstly.
    // This way is used to clean the history to avoid using wrong device address by OhcAsyncInterruptTransfer().
    //
    for (Index = 0; Index < 255; Index++) {
      if (!Ohc->UsbDevContext[Index + 1].Enabled &&
          (Ohc->UsbDevContext[Index + 1].SlotId == 0) &&
          (Ohc->UsbDevContext[Index + 1].BusDevAddr == (UINT8)Request->Value)) {
        Ohc->UsbDevContext[Index + 1].BusDevAddr = 0;
      }
    }
    //
    // The actual device address has been assigned by OHCI during initializing the device slot.
    // So we just need establish the mapping relationship between the device address requested from UsbBus
    // and the actual device address assigned by OHCI. The the following invocations through EFI_USB2_HC_PROTOCOL interface
    // can find out the actual device address by it.
    //
    Ohc->UsbDevContext[SlotId].BusDevAddr = (UINT8)Request->Value;
    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }
  
  //
  // If the port reset operation happens after the usb super speed device is enabled,
  // The subsequent configuration, such as getting device descriptor, will fail.
  // So here a workaround is introduced to skip the reset operation if the device is enabled.
  //
  if ((Request->Request     == USB_REQ_SET_FEATURE) &&
      (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_CLASS, USB_TARGET_OTHER)) &&
      (Request->Value       == EfiUsbPortReset)) {
    if (DeviceSpeed == EFI_USB_SPEED_SUPER) {
      Status = EFI_SUCCESS;
      goto ON_EXIT;
    }
  }

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  // Note that we encode the direction in address although default control
  // endpoint is bidirectional. OhcCreateUrb expects this
  // combination of Ep addr and its direction.
  //
  Endpoint = (UINT8) (0 | ((TransferDirection == EfiUsbDataIn) ? 0x80 : 0));
  Urb = OhcCreateUrb (
          Ohc,
          DeviceAddress,
          Endpoint,
          DeviceSpeed,
          MaximumPacketLength,
          OHC_CTRL_TRANSFER,
          Request,
          Data,
          *DataLength,
          NULL,
          NULL
          );

  if (Urb == NULL) {
    DEBUG ((EFI_D_ERROR, "OhcControlTransfer: failed to create URB"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = OhcExecTransfer (Ohc, FALSE, Urb, Timeout);

  //
  // Get the status from URB. The result is updated in OhcCheckUrbResult
  // which is called by OhcExecTransfer
  //
  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  } else if (*TransferResult == EFI_USB_ERR_STALL) {
    RecoveryStatus = OhcRecoverHaltedEndpoint(Ohc, Urb);
    if (EFI_ERROR (RecoveryStatus)) {
      DEBUG ((EFI_D_ERROR, "OhcControlTransfer: OhcRecoverHaltedEndpoint failed\n"));
    }
    Status = EFI_DEVICE_ERROR;
    goto FREE_URB;
  } else {
    goto FREE_URB;
  }

  Ohc->PciIo->Flush (Ohc->PciIo);
  
  if (Urb->DataMap != NULL) {
    Status = Ohc->PciIo->Unmap (Ohc->PciIo, Urb->DataMap);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto FREE_URB;
    }  
  }

  //
  // Hook Get_Descriptor request from UsbBus as we need evaluate context and configure endpoint.
  // Hook Get_Status request form UsbBus as we need trace device attach/detach event happened at hub.
  // Hook Set_Config request from UsbBus as we need configure device endpoint.
  //
  if ((Request->Request     == USB_REQ_GET_DESCRIPTOR) &&
      ((Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE)) || 
      ((Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_CLASS, USB_TARGET_DEVICE))))) {
    DescriptorType = (UINT8)(Request->Value >> 8);
    if ((DescriptorType == USB_DESC_TYPE_DEVICE) && ((*DataLength == sizeof (EFI_USB_DEVICE_DESCRIPTOR)) || ((DeviceSpeed == EFI_USB_SPEED_FULL) && (*DataLength == 8)))) {
        ASSERT (Data != NULL);
        //
        // Store a copy of device scriptor as hub device need this info to configure endpoint.
        //
        CopyMem (&Ohc->UsbDevContext[SlotId].DevDesc, Data, *DataLength);
        if (Ohc->UsbDevContext[SlotId].DevDesc.BcdUSB == 0x0300) {
          //
          // If it's a usb3.0 device, then its max packet size is a 2^n.
          //
          MaxPacket0 = 1 << Ohc->UsbDevContext[SlotId].DevDesc.MaxPacketSize0;
        } else {
          MaxPacket0 = Ohc->UsbDevContext[SlotId].DevDesc.MaxPacketSize0;
        }
        Ohc->UsbDevContext[SlotId].ConfDesc = AllocateZeroPool (Ohc->UsbDevContext[SlotId].DevDesc.NumConfigurations * sizeof (EFI_USB_CONFIG_DESCRIPTOR *));
        if (Ohc->HcCParams.Data.Csz == 0) {
          Status = OhcEvaluateContext (Ohc, SlotId, MaxPacket0);
        } else {
          Status = OhcEvaluateContext64 (Ohc, SlotId, MaxPacket0);
        }
    } else if (DescriptorType == USB_DESC_TYPE_CONFIG) {
      ASSERT (Data != NULL);
      if (*DataLength == ((UINT16 *)Data)[1]) {
        //
        // Get configuration value from request, Store the configuration descriptor for Configure_Endpoint cmd.
        //
        Index = (UINT8)Request->Value;
        ASSERT (Index < Ohc->UsbDevContext[SlotId].DevDesc.NumConfigurations);
        Ohc->UsbDevContext[SlotId].ConfDesc[Index] = AllocateZeroPool(*DataLength);
        CopyMem (Ohc->UsbDevContext[SlotId].ConfDesc[Index], Data, *DataLength);
      }
    } else if (((DescriptorType == USB_DESC_TYPE_HUB) ||
               (DescriptorType == USB_DESC_TYPE_HUB_SUPER_SPEED)) && (*DataLength > 2)) {
      ASSERT (Data != NULL);
      HubDesc = (EFI_USB_HUB_DESCRIPTOR *)Data;
      ASSERT (HubDesc->NumPorts <= 15);
      //
      // The bit 5,6 of HubCharacter field of Hub Descriptor is TTT.
      //
      TTT = (UINT8)((HubDesc->HubCharacter & (BIT5 | BIT6)) >> 5);
      if (Ohc->UsbDevContext[SlotId].DevDesc.DeviceProtocol == 2) {
        //
        // Don't support multi-TT feature for super speed hub now.
        //
        MTT = 0;
        DEBUG ((EFI_D_ERROR, "OHCI: Don't support multi-TT feature for Hub now. (force to disable MTT)\n"));
      } else {
        MTT = 0;
      }

      if (Ohc->HcCParams.Data.Csz == 0) {
        Status = OhcConfigHubContext (Ohc, SlotId, HubDesc->NumPorts, TTT, MTT);
      } else {
        Status = OhcConfigHubContext64 (Ohc, SlotId, HubDesc->NumPorts, TTT, MTT);
      }
    }
  } else if ((Request->Request     == USB_REQ_SET_CONFIG) &&
             (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE))) {
    //
    // Hook Set_Config request from UsbBus as we need configure device endpoint.
    //
    for (Index = 0; Index < Ohc->UsbDevContext[SlotId].DevDesc.NumConfigurations; Index++) {
      if (Ohc->UsbDevContext[SlotId].ConfDesc[Index]->ConfigurationValue == (UINT8)Request->Value) {
        if (Ohc->HcCParams.Data.Csz == 0) {
          Status = OhcSetConfigCmd (Ohc, SlotId, DeviceSpeed, Ohc->UsbDevContext[SlotId].ConfDesc[Index]);
        } else {
          Status = OhcSetConfigCmd64 (Ohc, SlotId, DeviceSpeed, Ohc->UsbDevContext[SlotId].ConfDesc[Index]);
        }
        break;
      }
    }
  } else if ((Request->Request     == USB_REQ_GET_STATUS) &&
             (Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_CLASS, USB_TARGET_OTHER))) {
    ASSERT (Data != NULL);
    //
    // Hook Get_Status request from UsbBus to keep track of the port status change.
    //
    State                       = *(UINT32 *)Data;
    PortStatus.PortStatus       = 0;
    PortStatus.PortChangeStatus = 0;

    if (DeviceSpeed == EFI_USB_SPEED_SUPER) {
      //
      // For super speed hub, its bit10~12 presents the attached device speed.
      //
      if ((State & OHC_PORTSC_PS) >> 10 == 0) {
        PortStatus.PortStatus |= USB_PORT_STAT_SUPER_SPEED;
      }
    } else {
      //
      // For high or full/low speed hub, its bit9~10 presents the attached device speed.
      //
      if (OHC_BIT_IS_SET (State, BIT9)) {
        PortStatus.PortStatus |= USB_PORT_STAT_LOW_SPEED;
      } else if (OHC_BIT_IS_SET (State, BIT10)) {
        PortStatus.PortStatus |= USB_PORT_STAT_HIGH_SPEED;
      }
    }

    //
    // Convert the OHCI port/port change state to UEFI status
    //
    MapSize = sizeof (mUsbHubPortStateMap) / sizeof (USB_PORT_STATE_MAP);
    for (Index = 0; Index < MapSize; Index++) {
      if (OHC_BIT_IS_SET (State, mUsbHubPortStateMap[Index].HwState)) {
        PortStatus.PortStatus = (UINT16) (PortStatus.PortStatus | mUsbHubPortStateMap[Index].UefiState);
      }
    }

    MapSize = sizeof (mUsbHubPortChangeMap) / sizeof (USB_PORT_STATE_MAP);
    for (Index = 0; Index < MapSize; Index++) {
      if (OHC_BIT_IS_SET (State, mUsbHubPortChangeMap[Index].HwState)) {
        PortStatus.PortChangeStatus = (UINT16) (PortStatus.PortChangeStatus | mUsbHubPortChangeMap[Index].UefiState);
      }
    }

    OhcPollPortStatusChange (Ohc, Ohc->UsbDevContext[SlotId].RouteString, (UINT8)Request->Index, &PortStatus);

    *(UINT32 *)Data = *(UINT32*)&PortStatus;
  }

FREE_URB:
  FreePool (Urb);

ON_EXIT:

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcControlTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
  }

  gBS->RestoreTPL (OldTpl);

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
  @param  Timeout               Indicates the maximum time, in millisecond, which
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
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     UINT8                               DataBuffersNumber,
  IN OUT VOID                                *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN OUT UINTN                               *DataLength,
  IN OUT UINT8                               *DataToggle,
  IN     UINTN                               Timeout,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
  USB_OHCI_INSTANCE       *Ohc;
  URB                     *Urb;
  UINT8                   SlotId;
  EFI_STATUS              Status;
  EFI_STATUS              RecoveryStatus;
  EFI_TPL                 OldTpl;

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
      ((EFI_USB_SPEED_HIGH == DeviceSpeed) && (MaximumPacketLength > 512)) ||
      ((EFI_USB_SPEED_SUPER == DeviceSpeed) && (MaximumPacketLength > 1024))) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (OHC_TPL);

  Ohc             = OHC_FROM_THIS (This);

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;

  if (OhcIsHalt (Ohc) || OhcIsSysError (Ohc)) {
    DEBUG ((EFI_D_ERROR, "OhcBulkTransfer: HC is halted\n"));
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = OhcBusDevAddrToSlotId (Ohc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  //
  Urb = OhcCreateUrb (
          Ohc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          MaximumPacketLength,
          OHC_BULK_TRANSFER,
          NULL,
          Data[0],
          *DataLength,
          NULL,
          NULL
          );

  if (Urb == NULL) {
    DEBUG ((EFI_D_ERROR, "OhcBulkTransfer: failed to create URB\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = OhcExecTransfer (Ohc, FALSE, Urb, Timeout);

  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  } else if (*TransferResult == EFI_USB_ERR_STALL) {
    RecoveryStatus = OhcRecoverHaltedEndpoint(Ohc, Urb);
    if (EFI_ERROR (RecoveryStatus)) {
      DEBUG ((EFI_D_ERROR, "OhcBulkTransfer: OhcRecoverHaltedEndpoint failed\n"));
    }
    Status = EFI_DEVICE_ERROR;
  }

  Ohc->PciIo->Flush (Ohc->PciIo);
  OhcFreeUrb (Ohc, Urb);

ON_EXIT:

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcBulkTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
  }
  gBS->RestoreTPL (OldTpl);

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
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     BOOLEAN                             IsNewTransfer,
  IN OUT UINT8                               *DataToggle,
  IN     UINTN                               PollingInterval,
  IN     UINTN                               DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK     CallBackFunction,
  IN     VOID                                *Context OPTIONAL
  )
{
  USB_OHCI_INSTANCE       *Ohc;
  URB                     *Urb;
  EFI_STATUS              Status;
  UINT8                   SlotId;
  UINT8                   Index;
  UINT8                   *Data;
  EFI_TPL                 OldTpl;

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

  OldTpl = gBS->RaiseTPL (OHC_TPL);

  Ohc    = OHC_FROM_THIS (This);

  //
  // Delete Async interrupt transfer request.
  //
  if (!IsNewTransfer) {
    //
    // The delete request may happen after device is detached.
    //
    for (Index = 0; Index < 255; Index++) {
      if (Ohc->UsbDevContext[Index + 1].BusDevAddr == DeviceAddress) {
        break;
      }
    }

    if (Index == 255) {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    Status = OhciDelAsyncIntTransfer (Ohc, DeviceAddress, EndPointAddress);
    DEBUG ((EFI_D_INFO, "OhcAsyncInterruptTransfer: remove old transfer for addr %d, Status = %r\n", DeviceAddress, Status));
    goto ON_EXIT;
  }

  Status = EFI_SUCCESS;

  if (OhcIsHalt (Ohc) || OhcIsSysError (Ohc)) {
    DEBUG ((EFI_D_ERROR, "OhcAsyncInterruptTransfer: HC is halt\n"));
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = OhcBusDevAddrToSlotId (Ohc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  Data = AllocateZeroPool (DataLength);

  if (Data == NULL) {
    DEBUG ((EFI_D_ERROR, "OhcAsyncInterruptTransfer: failed to allocate buffer\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Urb = OhcCreateUrb (
          Ohc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          MaximumPacketLength,
          OHC_INT_TRANSFER_ASYNC,
          NULL,
          Data,
          DataLength,
          CallBackFunction,
          Context
          );

  if (Urb == NULL) {
    DEBUG ((EFI_D_ERROR, "OhcAsyncInterruptTransfer: failed to create URB\n"));
    FreePool (Data);
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  InsertHeadList (&Ohc->AsyncIntTransfers, &Urb->UrbList);
  //
  // Ring the doorbell
  //
  Status = RingIntTransferDoorBell (Ohc, Urb);

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
  @param  Timeout               Maximum time, in second, to complete.
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
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN OUT VOID                                *Data,
  IN OUT UINTN                               *DataLength,
  IN OUT UINT8                               *DataToggle,
  IN     UINTN                               Timeout,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
  USB_OHCI_INSTANCE       *Ohc;
  URB                     *Urb;
  UINT8                   SlotId;
  EFI_STATUS              Status;
  EFI_STATUS              RecoveryStatus;
  EFI_TPL                 OldTpl;

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

  OldTpl = gBS->RaiseTPL (OHC_TPL);

  Ohc     = OHC_FROM_THIS (This);

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;

  if (OhcIsHalt (Ohc) || OhcIsSysError (Ohc)) {
    DEBUG ((EFI_D_ERROR, "EhcSyncInterruptTransfer: HC is halt\n"));
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = OhcBusDevAddrToSlotId (Ohc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  Urb = OhcCreateUrb (
          Ohc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          MaximumPacketLength,
          OHC_INT_TRANSFER_SYNC,
          NULL,
          Data,
          *DataLength,
          NULL,
          NULL
          );

  if (Urb == NULL) {
    DEBUG ((EFI_D_ERROR, "OhcSyncInterruptTransfer: failed to create URB\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = OhcExecTransfer (Ohc, FALSE, Urb, Timeout);

  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;

  if (*TransferResult == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  } else if (*TransferResult == EFI_USB_ERR_STALL) {
    RecoveryStatus = OhcRecoverHaltedEndpoint(Ohc, Urb);
    if (EFI_ERROR (RecoveryStatus)) {
      DEBUG ((EFI_D_ERROR, "OhcSyncInterruptTransfer: OhcRecoverHaltedEndpoint failed\n"));
    }
    Status = EFI_DEVICE_ERROR;
  }

  Ohc->PciIo->Flush (Ohc->PciIo);
  OhcFreeUrb (Ohc, Urb);

ON_EXIT:
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcSyncInterruptTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
  }
  gBS->RestoreTPL (OldTpl);

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
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     UINT8                               DataBuffersNumber,
  IN OUT VOID                                *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                               DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT    UINT32                              *TransferResult
  )
{
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
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     UINT8                               DataBuffersNumber,
  IN OUT VOID                                *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                               DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN     VOID                                *Context
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Entry point for EFI drivers.

  @param  ImageHandle       EFI_HANDLE.
  @param  SystemTable       EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS       Success.
  @retval Others            Fail.

**/
EFI_STATUS
EFIAPI
OhcDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  if (ShiftKeyPressed () & EFI_LEFT_ALT_PRESSED) {
    return EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gOhciDriverBinding,
             ImageHandle,
             &gOhciComponentName,
             &gOhciComponentName2
             );
  } else {
    return EFI_UNSUPPORTED;
  }
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
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  //
  // Test whether the controller belongs to Ohci type
  //
  if ((UsbClassCReg.BaseCode != PCI_CLASS_SERIAL) ||
      (UsbClassCReg.SubClassCode != PCI_CLASS_SERIAL_USB) ||
      (UsbClassCReg.ProgInterface != PCI_IF_OHCI)) {
    Status = EFI_UNSUPPORTED;
  }
  
ON_EXIT:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Create and initialize a USB_OHCI_INSTANCE structure.

  @param  PciIo                  The PciIo on this device.
  @param  DevicePath             The device path of host controller.
  @param  OriginalPciAttributes  Original PCI attributes.

  @return The allocated and initialized USB_OHCI_INSTANCE structure if created,
          otherwise NULL.

**/
USB_OHCI_INSTANCE*
OhcCreateUsbHc (
  IN EFI_PCI_IO_PROTOCOL       *PciIo,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN UINT64                    OriginalPciAttributes
  )
{
  USB_OHCI_INSTANCE       *Ohc;
  EFI_STATUS              Status;
  UINT32                  PageSize;
  UINT16                  ExtCapReg;

  Ohc = AllocateZeroPool (sizeof (USB_OHCI_INSTANCE));

  if (Ohc == NULL) {
    return NULL;
  }

  //
  // Initialize private data structure
  //
  Ohc->Signature             = OHCI_INSTANCE_SIG;
  Ohc->PciIo                 = PciIo;
  Ohc->DevicePath            = DevicePath;
  Ohc->OriginalPciAttributes = OriginalPciAttributes;
  CopyMem (&Ohc->Usb2Hc, &gOhciUsb2HcTemplate, sizeof (EFI_USB2_HC_PROTOCOL));

  InitializeListHead (&Ohc->AsyncIntTransfers);

  //
  // Be caution that the Offset passed to OhcReadCapReg() should be Dword align
  //
  Ohc->CapLength        = OhcReadCapReg8 (Ohc, OHC_CAPLENGTH_OFFSET);
  Ohc->HcSParams1.Dword = OhcReadCapReg (Ohc, OHC_HCSPARAMS1_OFFSET);
  Ohc->HcSParams2.Dword = OhcReadCapReg (Ohc, OHC_HCSPARAMS2_OFFSET);
  Ohc->HcCParams.Dword  = OhcReadCapReg (Ohc, OHC_HCCPARAMS_OFFSET);
  Ohc->DBOff            = OhcReadCapReg (Ohc, OHC_DBOFF_OFFSET);
  Ohc->RTSOff           = OhcReadCapReg (Ohc, OHC_RTSOFF_OFFSET);

  //
  // This PageSize field defines the page size supported by the xHC implementation.
  // This xHC supports a page size of 2^(n+12) if bit n is Set. For example,
  // if bit 0 is Set, the xHC supports 4k byte page sizes.
  //
  PageSize      = OhcReadOpReg(Ohc, OHC_PAGESIZE_OFFSET) & OHC_PAGESIZE_MASK;
  Ohc->PageSize = 1 << (HighBitSet32(PageSize) + 12);

  ExtCapReg            = (UINT16) (Ohc->HcCParams.Data.ExtCapReg);
  Ohc->ExtCapRegBase   = ExtCapReg << 2;
  Ohc->UsbLegSupOffset = OhcGetCapabilityAddr (Ohc, OHC_CAP_USB_LEGACY);
  Ohc->DebugCapSupOffset = OhcGetCapabilityAddr (Ohc, OHC_CAP_USB_DEBUG);

  DEBUG ((EFI_D_INFO, "OhcCreateUsb3Hc: Capability length 0x%x\n", Ohc->CapLength));
  DEBUG ((EFI_D_INFO, "OhcCreateUsb3Hc: HcSParams1 0x%x\n", Ohc->HcSParams1));
  DEBUG ((EFI_D_INFO, "OhcCreateUsb3Hc: HcSParams2 0x%x\n", Ohc->HcSParams2));
  DEBUG ((EFI_D_INFO, "OhcCreateUsb3Hc: HcCParams 0x%x\n", Ohc->HcCParams));
  DEBUG ((EFI_D_INFO, "OhcCreateUsb3Hc: DBOff 0x%x\n", Ohc->DBOff));
  DEBUG ((EFI_D_INFO, "OhcCreateUsb3Hc: RTSOff 0x%x\n", Ohc->RTSOff));
  DEBUG ((EFI_D_INFO, "OhcCreateUsb3Hc: UsbLegSupOffset 0x%x\n", Ohc->UsbLegSupOffset));
  DEBUG ((EFI_D_INFO, "OhcCreateUsb3Hc: DebugCapSupOffset 0x%x\n", Ohc->DebugCapSupOffset));

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
    goto ON_ERROR;
  }

  return Ohc;

ON_ERROR:
  FreePool (Ohc);
  return NULL;
}

/**
  One notified function to stop the Host Controller when gBS->ExitBootServices() called.

  @param  Event                   Pointer to this event
  @param  Context                 Event hanlder private data

**/
VOID
EFIAPI
OhcExitBootService (
  EFI_EVENT  Event,
  VOID       *Context
  )

{
  USB_OHCI_INSTANCE    *Ohc;
  EFI_PCI_IO_PROTOCOL  *PciIo;

  Ohc = (USB_OHCI_INSTANCE*) Context;
  PciIo = Ohc->PciIo;

  //
  // Stop AsyncRequest Polling timer then stop the OHCI driver
  // and uninstall the OHCI protocl.
  //
  gBS->SetTimer (Ohc->PollTimer, TimerCancel, 0);
  OhcHaltHC (Ohc, OHC_GENERIC_TIMEOUT);

  if (Ohc->PollTimer != NULL) {
    gBS->CloseEvent (Ohc->PollTimer);
  }

  OhcClearBiosOwnership (Ohc);

  //
  // Restore original PCI attributes
  //
  PciIo->Attributes (
                  PciIo,
                  EfiPciIoAttributeOperationSet,
                  Ohc->OriginalPciAttributes,
                  NULL
                  );
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
  EFI_PCI_IO_PROTOCOL     *PciIo;
  UINT64                  Supports;
  UINT64                  OriginalPciAttributes;
  BOOLEAN                 PciAttributesSaved;
  USB_OHCI_INSTANCE       *Ohc;
  EFI_DEVICE_PATH_PROTOCOL  *HcDevicePath;

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
    DEBUG ((EFI_D_ERROR, "OhcDriverBindingStart: failed to enable controller\n"));
    goto CLOSE_PCIIO;
  }

  //
  // Create then install USB2_HC_PROTOCOL
  //
  Ohc = OhcCreateUsbHc (PciIo, HcDevicePath, OriginalPciAttributes);

  if (Ohc == NULL) {
    DEBUG ((EFI_D_ERROR, "OhcDriverBindingStart: failed to create USB2_HC\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  OhcSetBiosOwnership (Ohc);

  OhcResetHC (Ohc, OHC_RESET_TIMEOUT);
  ASSERT (OhcIsHalt (Ohc));

  //
  // After Chip Hardware Reset wait until the Controller Not Ready (CNR) flag
  // in the USBSTS is '0' before writing any xHC Operational or Runtime registers.
  //
  ASSERT (!(OHC_REG_BIT_IS_SET (Ohc, OHC_USBSTS_OFFSET, OHC_USBSTS_CNR)));

  //
  // Initialize the schedule
  //
  OhcInitSched (Ohc);

  //
  // Start the Host Controller
  //
  OhcRunHC(Ohc, OHC_GENERIC_TIMEOUT);

  //
  // Start the asynchronous interrupt monitor
  //
  Status = gBS->SetTimer (Ohc->PollTimer, TimerPeriodic, OHC_ASYNC_TIMER_INTERVAL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcDriverBindingStart: failed to start async interrupt monitor\n"));
    OhcHaltHC (Ohc, OHC_GENERIC_TIMEOUT);
    goto FREE_POOL;
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
    goto FREE_POOL;
  }

  //
  // Install the component name protocol, don't fail the start
  // because of something for display.
  //
  AddUnicodeString2 (
    "eng",
    gOhciComponentName.SupportedLanguages,
    &Ohc->ControllerNameTable,
    L"eXtensible Host Controller (USB 3.0)",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gOhciComponentName2.SupportedLanguages,
    &Ohc->ControllerNameTable,
    L"eXtensible Host Controller (USB 3.0)",
    FALSE
    );

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiUsb2HcProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Ohc->Usb2Hc
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "OhcDriverBindingStart: failed to install USB2_HC Protocol\n"));
    goto FREE_POOL;
  }

  DEBUG ((EFI_D_INFO, "OhcDriverBindingStart: OHCI started for controller @ %x\n", Controller));
  return EFI_SUCCESS;

FREE_POOL:
  gBS->CloseEvent (Ohc->PollTimer);
  OhcFreeSched (Ohc);
  FreePool (Ohc);

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
  USB_OHCI_INSTANCE     *Ohc;
  UINT8                 Index;

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
    return Status;
  }

  Ohc   = OHC_FROM_THIS (Usb2Hc);
  PciIo = Ohc->PciIo;

  //
  // Stop AsyncRequest Polling timer then stop the OHCI driver
  // and uninstall the OHCI protocl.
  //
  gBS->SetTimer (Ohc->PollTimer, TimerCancel, 0);

  //
  // Disable the device slots occupied by these devices on its downstream ports.
  // Entry 0 is reserved.
  //
  for (Index = 0; Index < 255; Index++) {
    if (!Ohc->UsbDevContext[Index + 1].Enabled ||
        (Ohc->UsbDevContext[Index + 1].SlotId == 0)) {
      continue;
    }
    if (Ohc->HcCParams.Data.Csz == 0) {
      OhcDisableSlotCmd (Ohc, Ohc->UsbDevContext[Index + 1].SlotId);
    } else {
      OhcDisableSlotCmd64 (Ohc, Ohc->UsbDevContext[Index + 1].SlotId);
    }
  }

  OhcHaltHC (Ohc, OHC_GENERIC_TIMEOUT);
  OhcClearBiosOwnership (Ohc);

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  Usb2Hc
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Ohc->PollTimer != NULL) {
    gBS->CloseEvent (Ohc->PollTimer);
  }

  if (Ohc->ExitBootServiceEvent != NULL) {
    gBS->CloseEvent (Ohc->ExitBootServiceEvent);
  }

  OhciDelAllAsyncIntTransfers (Ohc);
  OhcFreeSched (Ohc);

  if (Ohc->ControllerNameTable) {
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

  return EFI_SUCCESS;
}

