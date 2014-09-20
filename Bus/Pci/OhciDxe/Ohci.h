/** @file
  Provides the definition of Usb Hc Protocol and OHCI controller 
  private data structure.

Copyright(c) 2013 Intel Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
* Neither the name of Intel Corporation nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef _OHCI_H
#define _OHCI_H

#include <Uefi.h>

#include <Protocol/Usb2HostController.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>

#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>

#include <IndustryStandard/Pci.h>

typedef struct _USB_OHCI_HC_DEV USB_OHCI_HC_DEV;

#include "UsbHcMem.h"
#include "OhciReg.h"
#include "OhciSched.h"
#include "OhciUrb.h"
#include "Descriptor.h"
#include "ComponentName.h"
#include "OhciDebug.h"

extern EFI_DRIVER_BINDING_PROTOCOL   gOhciDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gOhciComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gOhciComponentName2;

#define USB_OHCI_HC_DEV_SIGNATURE     SIGNATURE_32 ('o','h','c','i')

typedef struct _HCCA_MEMORY_BLOCK {
#if 0
  ED_DESCRIPTOR             *HccaInterruptTable[32];
#endif
  UINT32                    HccaInterruptTable[32];
  UINT16                    HccaFrameNumber;
  UINT16                    HccaPad;
#if 0
  TD_DESCRIPTOR             *HccaDoneHead;
#endif
  UINT32                    HccaDoneHead;
  UINT8                     Reserved[116];
} HCCA_MEMORY_BLOCK;

struct _USB_OHCI_HC_DEV {
  UINTN                     Signature;
  EFI_USB2_HC_PROTOCOL      Usb2Hc;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT64                    OriginalPciAttributes; 

  HCCA_MEMORY_BLOCK         *HccaMemoryBlock;
  VOID                      *HccaMemoryBuf;
  VOID                      *HccaMemoryMapping;
  UINTN                     HccaMemoryPages;
  
  ED_DESCRIPTOR             *IntervalList[6][32];
  INTERRUPT_CONTEXT_ENTRY   *InterruptContextList;
  VOID                      *MemPool;

  UINT32                    ToggleFlag;
 
  EFI_EVENT                 HouseKeeperTimer;

  // ExitBootServicesEvent is used to stop the OHC DMA operation 
  // after exit boot service.

  EFI_EVENT                  ExitBootServiceEvent;

  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;  
};

#define USB2_OHCI_HC_DEV_FROM_THIS(a)    CR(a, USB_OHCI_HC_DEV, Usb2Hc, USB_OHCI_HC_DEV_SIGNATURE)

// Func List

/**
  Retrieves the Host Controller capabilities.

  @param  This                  The EFI_USB2_HC_PROTOCOL instance.
  @param  MaxSpeed              Max speed supported by the controller.
  @param  PortNumber            Number of the root hub ports.
  @param  Is64BitCapable        Whether the controller supports 64-bit memory
                                addressing.

  @retval EFI_SUCCESS           Host controller capability were retrieved successfully.
  @retval EFI_INVALID_PARAMETER Either of the three capability pointer is NULL.
  @retval EFI_DEVICE_ERROR      An error was encountered while attempting to
                                retrieve the capabilities.
**/

EFI_STATUS
EFIAPI
OhciGetCapability (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT UINT8                 *MaxSpeed,
  OUT UINT8                 *PortNumber,
  OUT UINT8                 *Is64BitCapable
);

/**
  Provides software reset for the USB host controller.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  Attributes            A bit mask of the reset operation to perform.

  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_INVALID_PARAMETER Attributes is not valid.
  @retval EFI_UNSUPPORTED       The type of reset specified by Attributes is
                                not currently supported by the host controller.
  @retval EFI_DEVICE_ERROR      Host controller isn't halted to reset.
**/

EFI_STATUS
EFIAPI
OhciReset (
  IN EFI_USB2_HC_PROTOCOL *This,
  IN UINT16               Attributes
);

/**
  Retrieve the current state of the USB host controller.

  @param  This                  This EFI_USB2_HC_PROTOCOL instance.
  @param  State                 Variable to return the current host controller
                                state.

  @retval EFI_SUCCESS           Host controller state was returned in State.
  @retval EFI_INVALID_PARAMETER State is NULL.
  @retval EFI_DEVICE_ERROR      An error was encountered while attempting to
                                retrieve the host controller's current state.
**/

EFI_STATUS
EFIAPI
OhciGetState (
  IN  EFI_USB2_HC_PROTOCOL *This,
  OUT EFI_USB_HC_STATE     *State
);

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
OhciSetState (
  IN EFI_USB2_HC_PROTOCOL *This,
  IN EFI_USB_HC_STATE     State
);

/**
  Submits control transfer to a target USB device.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  Device Speed          Indicates target device speed.
  @param  MaxPaketLength        Indicates the maximum packet size that the
                                default control transfer endpoint is capable of
                                sending or receiving.
  @param  Request               A pointer to the USB device request that will be sent
                                to the USB device.
  @param  TransferDirection     Specifies the data direction for the transfer.
                                There are three values available, DataIn, DataOut
                                and NoData.
  @param  Data                  A pointer to the buffer of data that will be transmitted
                                to USB device or received from USB device.
  @param  DataLength            Indicates the size, in bytes, of the data buffer
                                specified by Data.
  @param  TimeOut               Indicates the maximum time, in microseconds,
                                which the transfer is allowed to complete.
  @param  Translator            A pointer to the transaction translator data.
  @param  TransferResult        A pointer to the detailed result information generated
                                by this control transfer.

  @retval EFI_SUCCESS           The control transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The control transfer could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The control transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The control transfer failed due to host controller or device error.
                                Caller should check TranferResult for detailed error information.
**/

EFI_STATUS
EFIAPI
OhciControlTransfer (
  IN     EFI_USB2_HC_PROTOCOL    *This,
  IN     UINT8                   DeviceAddress,
  IN     UINT8                   DeviceSpeed,
  IN     UINTN                   MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST  *Request,
  IN     EFI_USB_DATA_DIRECTION  TransferDirection,
  IN OUT VOID                    *Data                 OPTIONAL,
  IN OUT UINTN                   *DataLength           OPTIONAL,
  IN     UINTN                   TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT    UINT32                  *TransferResult
);

/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  This                A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress       Represents the address of the target device on the USB.
  @param  EndPointAddress     The combination of an endpoint number and an endpoint direction of the
                              target USB device.
  @param  DeviceSpeed         Indicates device speed.
  @param  MaximumPacketLength Indicates the maximum packet size the target endpoint is capable of
                              sending or receiving.
  @param  DataBuffersNumber   Number of data buffers prepared for the transfer.
  @param  Data                Array of pointers to the buffers of data that will be transmitted to USB
                              device or received from USB device.
  @param  DataLength          When input, indicates the size, in bytes, of the data buffers specified by
                              Data. When output, indicates the actually transferred data size.
  @param  DataToggle          A pointer to the data toggle value.
  @param  TimeOut             Indicates the maximum time, in milliseconds, which the transfer is
                              allowed to complete.
  @param  Translator          A pointer to the transaction translator data.
  @param  TransferResult      A pointer to the detailed result information of the bulk transfer.

  @retval EFI_SUCCESS           The bulk transfer was completed successfully.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The bulk transfer could not be submitted due to a lack of resources.
  @retval EFI_TIMEOUT           The bulk transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The bulk transfer failed due to host controller or device error.
                                Caller should check TransferResult for detailed error information.
**/

EFI_STATUS
EFIAPI
OhciBulkTransfer (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     UINT8                              DataBuffersNumber,
  IN OUT VOID                               *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN OUT UINTN                              *DataLength,
  IN OUT UINT8                              *DataToggle,
  IN     UINTN                              TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT    UINT32                             *TransferResult
);

/**
  Submits an interrupt transfer to an interrupt endpoint of a USB device.

  @param  Ohc                   Device private data
  @param  DeviceAddress         Represents the address of the target device on the USB,
                                which is assigned during USB enumeration.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint
                                direction of the target USB device. Each endpoint address
                                supports data transfer in one direction except the
                                control endpoint (whose default endpoint address is 0).
                                It is the caller's responsibility to make sure that
                                the EndPointAddress represents an interrupt endpoint.
  @param  DeviceSpeed           Indicates target device speed.
  @param  MaximumPacketLength       Indicates the maximum packet size the target endpoint
                                is capable of sending or receiving.
  @param  IsNewTransfer         If TRUE, an asynchronous interrupt pipe is built between
                                the host and the target interrupt endpoint.
                                If FALSE, the specified asynchronous interrupt pipe
                                is canceled.
  @param  DataToggle            A pointer to the data toggle value.  On input, it is valid
                                when IsNewTransfer is TRUE, and it indicates the initial
                                data toggle value the asynchronous interrupt transfer
                                should adopt.
                                On output, it is valid when IsNewTransfer is FALSE,
                                and it is updated to indicate the data toggle value of
                                the subsequent asynchronous interrupt transfer.
  @param  PollingInterval       Indicates the interval, in milliseconds, that the
                                asynchronous interrupt transfer is polled.
                                This parameter is required when IsNewTransfer is TRUE.
  @param  UCBuffer              Uncacheable buffer
  @param  DataLength            Indicates the length of data to be received at the
                                rate specified by PollingInterval from the target
                                asynchronous interrupt endpoint.  This parameter
                                is only required when IsNewTransfer is TRUE.
  @param  CallBackFunction      The Callback function.This function is called at the
                                rate specified by PollingInterval.This parameter is
                                only required when IsNewTransfer is TRUE.
  @param  Context               The context that is passed to the CallBackFunction.
                                This is an optional parameter and may be NULL.
  @param  IsPeriodic            Periodic interrupt or not
  @param  OutputED              The correspoding ED carried out
  @param  OutputTD              The correspoding TD carried out


  @retval EFI_SUCCESS           The asynchronous interrupt transfer request has been successfully
                                submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
**/

EFI_STATUS
OhciInterruptTransfer (
  IN     USB_OHCI_HC_DEV                  *Ohc,
  IN     UINT8                            DeviceAddress,
  IN     UINT8                            EndPointAddress,
  IN     UINT8                            DeviceSpeed,
  IN     UINTN                            MaximumPacketLength,
  IN     BOOLEAN                          IsNewTransfer,
  IN OUT UINT8                            *DataToggle        OPTIONAL,
  IN     UINTN                            PollingInterval    OPTIONAL,
  IN     VOID                             *UCBuffer          OPTIONAL,
  IN     UINTN                            DataLength         OPTIONAL,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK  CallBackFunction   OPTIONAL,
  IN     VOID                             *Context           OPTIONAL,
  IN     BOOLEAN                          IsPeriodic         OPTIONAL,
  OUT    ED_DESCRIPTOR                    **OutputED         OPTIONAL,
  OUT    TD_DESCRIPTOR                    **OutputTD         OPTIONAL
);

/**
  Submits an asynchronous interrupt transfer to an interrupt endpoint of a USB device.
  Translator parameter doesn't exist in UEFI2.0 spec, but it will be updated in the following specification version.

  @param  This                A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress       Represents the address of the target device on the USB.
  @param  EndPointAddress     The combination of an endpoint number and an endpoint direction of the
                              target USB device.
  @param  DeviceSpeed         Indicates device speed.
  @param  MaximumPacketLength Indicates the maximum packet size the target endpoint is capable of
                              sending or receiving.
  @param  IsNewTransfer       If TRUE, an asynchronous interrupt pipe is built between the host and the
                              target interrupt endpoint. If FALSE, the specified asynchronous interrupt
                              pipe is canceled. If TRUE, and an interrupt transfer exists for the target
                              end point, then EFI_INVALID_PARAMETER is returned.
  @param  DataToggle          A pointer to the data toggle value.
  @param  PollingInterval     Indicates the interval, in milliseconds, that the asynchronous interrupt
                              transfer is polled.
  @param  DataLength          Indicates the length of data to be received at the rate specified by
                              PollingInterval from the target asynchronous interrupt endpoint.
  @param  Translator          A pointr to the transaction translator data.
  @param  CallBackFunction    The Callback function. This function is called at the rate specified by
                              PollingInterval.
  @param  Context             The context that is passed to the CallBackFunction. This is an
                              optional parameter and may be NULL.

  @retval EFI_SUCCESS           The asynchronous interrupt transfer request has been successfully
                                submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
**/

EFI_STATUS
EFIAPI
OhciAsyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL                                *This,
  IN     UINT8                                               DeviceAddress,
  IN     UINT8                                               EndPointAddress,
  IN     UINT8                                               DeviceSpeed,
  IN     UINTN                                               MaxiumPacketLength,
  IN     BOOLEAN                                             IsNewTransfer,
  IN OUT UINT8                                               *DataToggle,
  IN     UINTN                                               PollingInterval  OPTIONAL,
  IN     UINTN                                               DataLength       OPTIONAL,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR                  *Translator      OPTIONAL,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK                     CallBackFunction OPTIONAL,
  IN     VOID                                                *Context         OPTIONAL
);

/**
  Submits synchronous interrupt transfer to an interrupt endpoint of a USB device.
  Translator parameter doesn't exist in UEFI2.0 spec, but it will be updated in the following specification version.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint direction of the
                                target USB device.
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Indicates the maximum packet size the target endpoint is capable of
                                sending or receiving.
  @param  Data                  A pointer to the buffer of data that will be transmitted to USB device or
                                received from USB device.
  @param  DataLength            On input, the size, in bytes, of the data buffer specified by Data. On
                                output, the number of bytes transferred.
  @param  DataToggle            A pointer to the data toggle value.
  @param  TimeOut               Indicates the maximum time, in milliseconds, which the transfer is
                                allowed to complete.
  @param  Translator            A pointr to the transaction translator data.
  @param  TransferResult        A pointer to the detailed result information from the synchronous
                                interrupt transfer.

  @retval EFI_SUCCESS           The synchronous interrupt transfer was completed successfully.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The synchronous interrupt transfer could not be submitted due to a lack of resources.
  @retval EFI_TIMEOUT           The synchronous interrupt transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The synchronous interrupt transfer failed due to host controller or device error.
                                Caller should check TransferResult for detailed error information.
**/

EFI_STATUS
EFIAPI
OhciSyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL                        *This,
  IN     UINT8                                       DeviceAddress,
  IN     UINT8                                       EndPointAddress,
  IN     UINT8                                       DeviceSpeed,
  IN     UINTN                                       MaximumPacketLength,
  IN OUT VOID                                        *Data,
  IN OUT UINTN                                       *DataLength,
  IN OUT UINT8                                       *DataToggle,
  IN     UINTN                                       TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR          *Translator,
  OUT    UINT32                                      *TransferResult
);

/**
  Submits isochronous transfer to an isochronous endpoint of a USB device.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint direction of the
                                target USB device.
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Indicates the maximum packet size the target endpoint is capable of
                                sending or receiving.
  @param  DataBuffersNumber     Number of data buffers prepared for the transfer.
  @param  Data                  Array of pointers to the buffers of data that will be transmitted to USB
                                device or received from USB device.
  @param  DataLength            Specifies the length, in bytes, of the data to be sent to or received from
                                the USB device.
  @param  Translator            A pointer to the transaction translator data.
  @param  TransferResult        A pointer to the detailed result information of the isochronous transfer.

  @retval EFI_SUCCESS           The isochronous transfer was completed successfully.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The isochronous transfer could not be submitted due to a lack of resources.
  @retval EFI_TIMEOUT           The isochronous transfer cannot be completed within the one USB frame time.
  @retval EFI_DEVICE_ERROR      The isochronous transfer failed due to host controller or device error.
                                Caller should check TransferResult for detailed error information.
**/

EFI_STATUS
EFIAPI
OhciIsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     UINT8                              DataBuffersNumber,
  IN OUT VOID                               *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                              DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT    UINT32                             *TransferResult
);

/**
  Submits nonblocking isochronous transfer to an isochronous endpoint of a USB device.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Represents the address of the target device on the USB.
  @param  EndPointAddress       The combination of an endpoint number and an endpoint direction of the
                                target USB device.
  @param  DeviceSpeed           Indicates device speed.
  @param  MaximumPacketLength   Indicates the maximum packet size the target endpoint is capable of
                                sending or receiving.
  @param  DataBuffersNumber     Number of data buffers prepared for the transfer.
  @param  Data                  Array of pointers to the buffers of data that will be transmitted to USB
                                device or received from USB device.
  @param  DataLength            Specifies the length, in bytes, of the data to be sent to or received from
                                the USB device.
  @param  Translator            A pointer to the transaction translator data.
  @param  IsochronousCallback   The Callback function. This function is called if the requested
                                isochronous transfer is completed.
  @param  Context               Data passed to the IsochronousCallback function. This is an
                                optional parameter and may be NULL.

  @retval EFI_SUCCESS           The asynchronous isochronous transfer request has been successfully
                                submitted or canceled.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The asynchronous isochronous transfer could not be submitted due to
                                a lack of resources.
**/

EFI_STATUS
EFIAPI
OhciAsyncIsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     UINT8                              DataBuffersNumber,
  IN OUT VOID                               *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                              DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    IsochronousCallBack,
  IN     VOID                               *Context OPTIONAL
);

/**
  Retrieves the number of root hub ports.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  NumOfPorts            A pointer to the number of the root hub ports.

  @retval EFI_SUCCESS           The port number was retrieved successfully.
**/

EFI_STATUS
EFIAPI
OhciGetRootHubNumOfPorts (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT UINT8                *NumOfPorts
);

/**
  Retrieves the current status of a USB root hub port.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL.
  @param  PortNumber            Specifies the root hub port from which the status
                                is to be retrieved.  This value is zero-based. For example,
                                if a root hub has two ports, then the first port is numbered 0,
                                and the second port is numbered 1.
  @param  PortStatus            A pointer to the current port status bits and
                                port status change bits.

  @retval EFI_SUCCESS           The status of the USB root hub port specified by PortNumber
                                was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER Port number not valid
**/

EFI_STATUS
EFIAPI
OhciGetRootHubPortStatus (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  IN  UINT8                PortNumber,
  OUT EFI_USB_PORT_STATUS  *PortStatus
);

/**
  Sets a feature for the specified root hub port.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL.
  @param  PortNumber            Specifies the root hub port whose feature
                                is requested to be set.
  @param  PortFeature           Indicates the feature selector associated
                                with the feature set request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was set for the
                                USB root hub port specified by PortNumber.
  @retval EFI_DEVICE_ERROR      Set feature failed because of hardware issue
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
**/

EFI_STATUS
EFIAPI
OhciSetRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL   *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
);

/**
  Clears a feature for the specified root hub port.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber            Specifies the root hub port whose feature
                                is requested to be cleared.
  @param  PortFeature           Indicates the feature selector associated with the
                                feature clear request.
                  
  @retval EFI_SUCCESS           The feature specified by PortFeature was cleared for the
                                USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
  @retval EFI_DEVICE_ERROR      Some error happened when clearing feature
**/

EFI_STATUS
EFIAPI
OhciClearRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL   *This,
  IN UINT8                 PortNumber,
  IN EFI_USB_PORT_FEATURE  PortFeature
);

/**
  Test to see if this driver supports ControllerHandle. Any
  ControllerHandle that has UsbHcProtocol installed will be supported.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.
**/

EFI_STATUS
EFIAPI
OHCIDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
);

/**
  Starting the Usb OHCI Driver.

  @param  This                  Protocol instance pointer.
  @param  Controller            Handle of device to test.
  @param  RemainingDevicePath   Not used.

  @retval EFI_SUCCESS           This driver supports this device.
  @retval EFI_UNSUPPORTED       This driver does not support this device.
  @retval EFI_DEVICE_ERROR      This driver cannot be started due to device Error.
  @retval EFI_OUT_OF_RESOURCES  Failed due to resource shortage.
**/

EFI_STATUS
EFIAPI
OHCIDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
);

/**
  Stop this driver on ControllerHandle. Support stoping any child handles
  created by this driver.

  @param  This                  Protocol instance pointer.
  @param  Controller            Handle of device to stop driver on.
  @param  NumberOfChildren      Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer     List of handles for the children we need to stop.

  @return EFI_SUCCESS
  @return others
**/

EFI_STATUS
EFIAPI
OHCIDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
);

#endif
