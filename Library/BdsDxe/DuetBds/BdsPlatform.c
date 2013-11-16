/*++

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  BdsPlatform.c

Abstract:

  This file include all platform action which can be customized
  by IBV/OEM.

--*/

#include <Library/ShiftKeysLib.h>

#include "BdsPlatform.h"

#define EFI_MEMORY_PRESENT      0x0100000000000000ULL
#define EFI_MEMORY_INITIALIZED  0x0200000000000000ULL
#define EFI_MEMORY_TESTED       0x0400000000000000ULL

#define IS_PCI_ISA_PDECODE(_p)        IS_CLASS3 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_ISA_PDECODE, 0)
#define SCAN_ESC        0x0017
#define SCAN_F1         0x000B

extern BOOLEAN  gConnectAllHappened;
extern USB_CLASS_FORMAT_DEVICE_PATH gUsbClassKeyboardDevicePath;

EFI_GUID    *gTableGuidArray[] = {&gEfiAcpi20TableGuid,
                                  &gEfiAcpiTableGuid,
                                  &gEfiSmbiosTableGuid,
                                  &gEfiMpsTableGuid};

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *mGcdMemoryTypeNames[] = {
  "NonExist ",  // EfiGcdMemoryTypeNonExistent
  "Reserved ",  // EfiGcdMemoryTypeReserved
  "SystemMem",  // EfiGcdMemoryTypeSystemMemory
  "MMIO     ",  // EfiGcdMemoryTypeMemoryMappedIo
  "Unknown  "   // EfiGcdMemoryTypeMaximum
};

//
// BDS Platform Functions
//

VOID
GetSystemTablesFromHob (
  VOID
  )
/*++

Routine Description:
  Find GUID'ed HOBs that contain EFI_PHYSICAL_ADDRESS of ACPI, SMBIOS, MPs tables

Arguments:
  None

Returns:
  None.

--*/
{
  EFI_PEI_HOB_POINTERS        GuidHob;
  EFI_PEI_HOB_POINTERS        HobStart;
  EFI_PHYSICAL_ADDRESS        *Table;
  UINTN                       Index;

  //
  // Get Hob List
  //
  HobStart.Raw = GetHobList ();
  //
  // Iteratively add ACPI Table, SMBIOS Table, MPS Table to EFI System Table
  //
  for (Index = 0; Index < sizeof (gTableGuidArray) / sizeof (*gTableGuidArray); ++Index) {
    GuidHob.Raw = GetNextGuidHob (gTableGuidArray[Index], HobStart.Raw);
    if (GuidHob.Raw != NULL) {
      Table = GET_GUID_HOB_DATA (GuidHob.Guid);
      if (Table != NULL) {
        //
        // Check if Mps Table/Smbios Table/Acpi Table exists in E/F seg,
        // According to UEFI Spec, we should make sure Smbios table,
        // ACPI table and Mps tables kept in memory of specified type
        //
        ConvertSystemTable(gTableGuidArray[Index], (VOID**)&Table);
        gBS->InstallConfigurationTable (gTableGuidArray[Index], (VOID *)Table);
      }
    }
  }

  return;
}

VOID
DumpGcdMemoryMap (
  VOID
)
{
  UINTN                            NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;
  UINTN                            Index;

  gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  DBG ("GCDMemType Range                             Capabilities     Attributes      \n");
  DBG ("========== ================================= ================ ================\n");
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    DBG ("%a  %016lx-%016lx %016lx %016lx%c\n",
        mGcdMemoryTypeNames[MIN (MemorySpaceMap[Index].GcdMemoryType, EfiGcdMemoryTypeMaximum)],
        MemorySpaceMap[Index].BaseAddress,
        MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length - 1,
        MemorySpaceMap[Index].Capabilities,
        MemorySpaceMap[Index].Attributes,
        MemorySpaceMap[Index].ImageHandle == NULL ? ' ' : '*'
        );
  }
  DBG ("\n");
  FreePool (MemorySpaceMap);
}

VOID
UpdateMemoryMap (
  VOID
  )
{
  EFI_STATUS                      Status;
  EFI_PEI_HOB_POINTERS            GuidHob;
  VOID                            *Table;
  MEMORY_DESC_HOB                 MemoryDescHob;
  UINTN                           Index;
  EFI_PHYSICAL_ADDRESS            Memory;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR Descriptor;
  EFI_PHYSICAL_ADDRESS            FirstNonConventionalAddr;
  UINTN                           NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR *MemorySpaceMap;
  EFI_GCD_MEMORY_TYPE             GcdType;

  GuidHob.Raw = GetFirstGuidHob (&gLdrMemoryDescriptorGuid);
  if (GuidHob.Raw == NULL) {
    return;
  }
  Table = GET_GUID_HOB_DATA (GuidHob.Guid);
  if (Table == NULL) {
    return;
  }
  MemoryDescHob.MemDescCount = *(UINTN *)Table;
  MemoryDescHob.MemDesc      = *(EFI_MEMORY_DESCRIPTOR **)((UINTN)Table + sizeof(UINTN));
#if 1
  DumpGcdMemoryMap ();
#endif
  gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if ((MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeReserved &&
        (MemorySpaceMap[Index].Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
        (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED))) {
      //
      // For those reserved memory that have not been tested, simply promote to system memory.
      //
      GcdType = EfiGcdMemoryTypeSystemMemory;
      
      if ((MemorySpaceMap[Index].ImageHandle != NULL)){
        gDS->FreeMemorySpace (
               MemorySpaceMap[Index].BaseAddress,
               MemorySpaceMap[Index].Length
               );
        GcdType = EfiGcdMemoryTypeReserved;
      }

      gDS->RemoveMemorySpace (
             MemorySpaceMap[Index].BaseAddress,
             MemorySpaceMap[Index].Length
             );

      gDS->AddMemorySpace (
             GcdType,
             MemorySpaceMap[Index].BaseAddress,
             MemorySpaceMap[Index].Length,
             MemorySpaceMap[Index].Capabilities &~
             (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED | EFI_MEMORY_RUNTIME | EFI_MEMORY_UC | EFI_MEMORY_WC | EFI_MEMORY_WT)
             );
    }
  }
  
  FreePool (MemorySpaceMap);
#if 1
  DumpGcdMemoryMap ();
#endif
  //
  // Add ACPINVS, ACPIReclaim, and Reserved memory to MemoryMap
  //
  FirstNonConventionalAddr = 0xFFFFFFFF;
#if 0
  DBG ("Index  Type  Physical Start    Physical End      Number of Pages   Virtual Start     Attribute\n");
#endif
  for (Index = 0; Index < MemoryDescHob.MemDescCount; Index++) {
#if 0
    DBG ("%02d     %02d    %016lx  %016lx  %016lx  %016lx  %016x\n",
         Index,
         MemoryDescHob.MemDesc[Index].Type,
         MemoryDescHob.MemDesc[Index].PhysicalStart,
         MemoryDescHob.MemDesc[Index].PhysicalStart + MemoryDescHob.MemDesc[Index].NumberOfPages * 4096 -1,
         MemoryDescHob.MemDesc[Index].NumberOfPages,
         MemoryDescHob.MemDesc[Index].VirtualStart,
         MemoryDescHob.MemDesc[Index].Attribute);
#endif
    if (MemoryDescHob.MemDesc[Index].PhysicalStart < 0x100000) {
      continue;
    }
    if (MemoryDescHob.MemDesc[Index].PhysicalStart >= 0x100000000ULL) {
      continue;
    }
    if ((MemoryDescHob.MemDesc[Index].Type == EfiReservedMemoryType) ||
        (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesData) ||
        (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesCode) ||
        (MemoryDescHob.MemDesc[Index].Type == EfiACPIReclaimMemory) ||
        (MemoryDescHob.MemDesc[Index].Type == EfiACPIMemoryNVS)) {

      if (MemoryDescHob.MemDesc[Index].PhysicalStart < FirstNonConventionalAddr) {
        FirstNonConventionalAddr = MemoryDescHob.MemDesc[Index].PhysicalStart;
      }

      if ((MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesData) ||
          (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesCode)) {
        //
        // For RuntimeSevicesData and RuntimeServicesCode, they are BFV or DxeCore.
        // The memory type is assigned in EfiLdr
        //
        Status = gDS->GetMemorySpaceDescriptor (MemoryDescHob.MemDesc[Index].PhysicalStart, &Descriptor);
        if (EFI_ERROR (Status)) {
          continue;
        }
        if (Descriptor.GcdMemoryType != EfiGcdMemoryTypeReserved) {
          //
          // BFV or tested DXE core
          //
          continue;
        }
        //
        // Untested DXE Core region, free and remove
        //
        Status = gDS->FreeMemorySpace (
                        MemoryDescHob.MemDesc[Index].PhysicalStart,
                        LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT)
                        );
        if (EFI_ERROR (Status)) {
          continue;
        }
        Status = gDS->RemoveMemorySpace (
                        MemoryDescHob.MemDesc[Index].PhysicalStart,
                        LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT)
                        );
        if (EFI_ERROR (Status)) {
          continue;
        }

        //
        // Convert Runtime type to BootTime type
        //
        if (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesData) {
          MemoryDescHob.MemDesc[Index].Type = EfiBootServicesData;
        } else {
          MemoryDescHob.MemDesc[Index].Type = EfiBootServicesCode;
        }

        //
        // PassThrough, let below code add and alloate.
        //
      }
      //
      // ACPI or reserved memory
      //
      Status = gDS->AddMemorySpace (
                      EfiGcdMemoryTypeSystemMemory,
                      MemoryDescHob.MemDesc[Index].PhysicalStart,
                      LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT),
                      MemoryDescHob.MemDesc[Index].Attribute
                      );
      if (!(MemoryDescHob.MemDesc[Index].Type == EfiACPIReclaimMemory) &&
          !(MemoryDescHob.MemDesc[Index].Type == EfiACPIMemoryNVS)) {
        if (EFI_ERROR (Status)) {
          continue;
        }
      }

      Memory = MemoryDescHob.MemDesc[Index].PhysicalStart;
      Status = gBS->AllocatePages (
                      AllocateAddress,
                      (EFI_MEMORY_TYPE)MemoryDescHob.MemDesc[Index].Type,
                      (UINTN)MemoryDescHob.MemDesc[Index].NumberOfPages,
                      &Memory
                      );
      if (EFI_ERROR (Status)) {
        //
        // For the page added, it must be allocated.
        //
        continue;
      }
    }
  }
#if 0
  DBG ("\n");
#endif
  /**
  *
  *  thanks for this fix dmazar! 
  *
  **/
  for (Index = 0; Index < MemoryDescHob.MemDescCount; Index++) {
    if (MemoryDescHob.MemDesc[Index].PhysicalStart < 0x100000) {
      continue;
    }
    if (MemoryDescHob.MemDesc[Index].PhysicalStart >= 0x100000000ULL) {
      continue;
    }
    if (MemoryDescHob.MemDesc[Index].Type != EfiConventionalMemory) {
      continue;
    }
    if (MemoryDescHob.MemDesc[Index].PhysicalStart < FirstNonConventionalAddr) {
      continue;
    }
    // this is our candidate - add it
#if 0
    Status = gDS->GetMemorySpaceDescriptor (MemoryDescHob.MemDesc[Index].PhysicalStart, &Descriptor);
    if (EFI_ERROR (Status)) {
      continue;
    }
    
    if (Descriptor.GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
      MemoryDescHob.MemDesc[Index].Type = EfiReservedMemoryType;
      continue;
    }

    gDS->RemoveMemorySpace (
          MemoryDescHob.MemDesc[Index].PhysicalStart,
          LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT)
          );
#endif

    gDS->AddMemorySpace (
          EfiGcdMemoryTypeSystemMemory,
          MemoryDescHob.MemDesc[Index].PhysicalStart,
          LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT),
          MemoryDescHob.MemDesc[Index].Attribute
          );
    
#if 0
    Memory = MemoryDescHob.MemDesc[Index].PhysicalStart;
    gBS->AllocatePages (
          AllocateAddress,
          (EFI_MEMORY_TYPE)MemoryDescHob.MemDesc[Index].Type,
          (UINTN)MemoryDescHob.MemDesc[Index].NumberOfPages,
          &Memory
          );
#endif
  }
#if 0
  DumpGcdMemoryMap ();
#endif
}

EFI_STATUS
DisableUsbLegacySupport (
  VOID
  )
{
  EFI_STATUS                            Status;
  EFI_HANDLE                            *HandleArray;
  UINTN                                 HandleArrayCount;
  UINTN                                 Index;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  UINT8                                 Class[3];
  UINT16                                Command;
  UINT32                                HcCapParams;
  UINT32                                ExtendCap;
  UINT32                                Value;
  UINT32                                TimeOut;
  UINT32                                SaveValue;
  
  /* Avoid double action, if already done in usb drivers */

  if (FeaturePcdGet (PcdTurnOffUsbLegacySupport)) {
    return EFI_SUCCESS;
  }

  //
  // Find the usb host controller
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleArrayCount,
                  &HandleArray
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleArrayCount; Index++) {
      Status = gBS->HandleProtocol (
                      HandleArray[Index],
                      &gEfiPciIoProtocolGuid,
                      (VOID **)&PciIo
                      );
      if (!EFI_ERROR (Status)) {
        //
        // Find the USB host controller controller
        //
        Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x09, 3, &Class);
        if (!EFI_ERROR (Status)) {
          if ((PCI_CLASS_SERIAL == Class[2]) &&
              (PCI_CLASS_SERIAL_USB == Class[1])) {
            if (PCI_IF_UHCI == Class[0]) {
              //
              // Found the UHCI, then disable the legacy support
              //
              Command = 0;
              Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0xC0, 1, &Command);
            } else if (PCI_IF_EHCI == Class[0]) {
              //
              // Found the EHCI, then disable the legacy support
              //
              Status = PciIo->Mem.Read (
                                   PciIo,
                                   EfiPciIoWidthUint32,
                                   0,                   //EHC_BAR_INDEX
                                   (UINT64) 0x08,       //EHC_HCCPARAMS_OFFSET
                                   1,
                                   &HcCapParams
                                   );
              
              ExtendCap = (HcCapParams >> 8) & 0xFF;
              //
              // Disable the SMI in USBLEGCTLSTS firstly
              //
#ifdef USB_FIXUP
              DEBUG ((DEBUG_INFO, "EHCI: EECP = 0x%X, USBLEGCTLSTS = 0x%X", ExtendCap, (ExtendCap + 4)));
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &SaveValue);
              Value = SaveValue & 0xFFFF0000;
              DEBUG ((DEBUG_INFO, ", Save val = 0x%X, Write val = 0x%X", SaveValue, Value));
#else
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);
              Value &= 0xFFFF0000;
#endif
              PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);

              //
              // Get EHCI Ownership from legacy bios
              //
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
              Value |= (0x1 << 24);
              PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

              TimeOut = 40;
              while (TimeOut--) {
                gBS->Stall (500);

                PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

                if ((Value & 0x01010000) == 0x01000000) {
                  break;
                }
              }
#ifdef USB_FIXUP
#if 0
              SaveValue &= ~0x003F;
#endif
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);
              DEBUG ((DEBUG_INFO, ", New val = 0x%X\n", Value));
              PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &SaveValue);
#endif
            }
          }
        }
      }
    }
  } else {
    return Status;
  }
  gBS->FreePool (HandleArray);
  return EFI_SUCCESS;
}

VOID
EFIAPI
PlatformBdsInit (
  VOID
  )
/*++

Routine Description:

  Platform Bds init. Include the platform firmware vendor, revision
  and so crc check.

Arguments:

Returns:

  None.

--*/
{
  GetSystemTablesFromHob ();
  UpdateMemoryMap ();
  //
  // Append Usb Keyboard short form DevicePath into "ConInDev"
  //
  BdsLibUpdateConsoleVariable (
    VarConsoleInpDev,
    (EFI_DEVICE_PATH_PROTOCOL *) &gUsbClassKeyboardDevicePath,
    NULL
    );
}

UINT64
GetPciExpressBaseAddressForRootBridge (
  IN UINTN    HostBridgeNumber,
  IN UINTN    RootBridgeNumber
  )
/*++

Routine Description:
  This routine is to get PciExpress Base Address for this RootBridge

Arguments:
  HostBridgeNumber - The number of HostBridge
  RootBridgeNumber - The number of RootBridge

Returns:
  UINT64 - PciExpressBaseAddress for this HostBridge and RootBridge

--*/
{
  EFI_PCI_EXPRESS_BASE_ADDRESS_INFORMATION *PciExpressBaseAddressInfo;
  UINTN                                    BufferSize;
  UINT32                                   Index;
  UINT32                                   Number;
  EFI_PEI_HOB_POINTERS                     GuidHob;

  //
  // Get PciExpressAddressInfo Hob
  //
  PciExpressBaseAddressInfo = NULL;
  BufferSize                = 0;
  GuidHob.Raw = GetFirstGuidHob (&gEfiPciExpressBaseAddressGuid);
  if (GuidHob.Raw != NULL) {
    PciExpressBaseAddressInfo = GET_GUID_HOB_DATA (GuidHob.Guid);
    BufferSize                = GET_GUID_HOB_DATA_SIZE (GuidHob.Guid);
  } else {
    return 0;
  }

  //
  // Search the PciExpress Base Address in the Hob for current RootBridge
  //
  Number = (UINT32)(BufferSize / sizeof(EFI_PCI_EXPRESS_BASE_ADDRESS_INFORMATION));
  for (Index = 0; Index < Number; Index++) {
    if ((PciExpressBaseAddressInfo[Index].HostBridgeNumber == HostBridgeNumber) &&
        (PciExpressBaseAddressInfo[Index].RootBridgeNumber == RootBridgeNumber)) {
      return PciExpressBaseAddressInfo[Index].PciExpressBaseAddress;
    }
  }

  //
  // Do not find the PciExpress Base Address in the Hob
  //
  return 0;
}

VOID
PatchPciRootBridgeDevicePath (
  IN UINTN    HostBridgeNumber,
  IN UINTN    RootBridgeNumber,
  IN PLATFORM_ROOT_BRIDGE_DEVICE_PATH  *RootBridge
  )
{
  UINT64  PciExpressBase;

  PciExpressBase = GetPciExpressBaseAddressForRootBridge (HostBridgeNumber, RootBridgeNumber);

  DEBUG ((EFI_D_INFO, "Get PciExpress Address from Hob: 0x%X\n", PciExpressBase));

  if (PciExpressBase != 0) {
    RootBridge->PciRootBridge.HID = EISA_PNP_ID(0x0A08);
  }
}

EFI_STATUS
ConnectRootBridge (
  VOID
  )
/*++

Routine Description:

  Connect RootBridge

Arguments:

  None.

Returns:

  EFI_SUCCESS             - Connect RootBridge successfully.
  EFI_STATUS              - Connect RootBridge fail.

--*/
{
  EFI_STATUS                Status;
  EFI_HANDLE                RootHandle;

  //
  // Patch Pci Root Bridge Device Path
  //
  PatchPciRootBridgeDevicePath (0, 0, &gPlatformRootBridge0);

  //
  // Make all the PCI_IO protocols on PCI Seg 0 show up
  //
  BdsLibConnectDevicePath (gPlatformRootBridges[0]);

  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &gPlatformRootBridges[0],
                  &RootHandle
                  );
  DEBUG ((EFI_D_INFO, "Pci Root bridge handle is 0x%X\n", RootHandle));

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->ConnectController (RootHandle, NULL, NULL, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PrepareLpcBridgeDevicePath (
  IN EFI_HANDLE                DeviceHandle
  )
/*++

Routine Description:

  Add IsaKeyboard to ConIn,
  add IsaSerial to ConOut, ConIn, ErrOut.
  LPC Bridge: 06 01 00

Arguments:

  DeviceHandle            - Handle of PCIIO protocol.

Returns:

  EFI_SUCCESS             - LPC bridge is added to ConOut, ConIn, and ErrOut.
  EFI_STATUS              - No LPC bridge is added.

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;

  DevicePath = NULL;
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID*)&DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  TempDevicePath = DevicePath;

  //
  // Register Keyboard
  //
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gPnpPs2KeyboardDeviceNode);

  BdsLibUpdateConsoleVariable (VarConsoleInp, DevicePath, NULL);

  //
  // Register COM1
  //
  DevicePath = TempDevicePath;
  gPnp16550ComPortDeviceNode.UID = 0;

  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gPnp16550ComPortDeviceNode);
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gUartDeviceNode);
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode);

  BdsLibUpdateConsoleVariable (VarConsoleOut, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarConsoleInp, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarErrorOut, DevicePath, NULL);

  //
  // Register COM2
  //
  DevicePath = TempDevicePath;
  gPnp16550ComPortDeviceNode.UID = 1;

  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gPnp16550ComPortDeviceNode);
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gUartDeviceNode);
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode);

  BdsLibUpdateConsoleVariable (VarConsoleOut, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarConsoleInp, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarErrorOut, DevicePath, NULL);

  return EFI_SUCCESS;
}

EFI_STATUS
GetGopDevicePath (
   IN  EFI_DEVICE_PATH_PROTOCOL *PciDevicePath,
   OUT EFI_DEVICE_PATH_PROTOCOL **GopDevicePath
   )
{
  UINTN                           Index;
  EFI_STATUS                      Status;
  EFI_HANDLE                      PciDeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL        *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *TempPciDevicePath;
  UINTN                           GopHandleCount;
  EFI_HANDLE                      *GopHandleBuffer;

  if (PciDevicePath == NULL || GopDevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize the GopDevicePath to be PciDevicePath
  //
  *GopDevicePath    = PciDevicePath;
  TempPciDevicePath = PciDevicePath;

  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &TempPciDevicePath,
                  &PciDeviceHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Try to connect this handle, so that GOP dirver could start on this
  // device and create child handles with GraphicsOutput Protocol installed
  // on them, then we get device paths of these child handles and select
  // them as possible console device.
  //
  gBS->ConnectController (PciDeviceHandle, NULL, NULL, FALSE);    // 2,6 sec !!!!!!
  DBG ("    BdsPlatorm: GetGopDevicePath->ConnectController\n");

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &GopHandleCount,
                  &GopHandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Add all the child handles as possible Console Device
    //
    for (Index = 0; Index < GopHandleCount; Index++) {
      Status = gBS->HandleProtocol (GopHandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID*)&TempDevicePath);
      if (EFI_ERROR (Status)) {
        continue;
      }
      if (CompareMem (
            PciDevicePath,
            TempDevicePath,
            GetDevicePathSize (PciDevicePath) - END_DEVICE_PATH_LENGTH
            ) == 0) {
        //
        // In current implementation, we only enable one of the child handles
        // as console device, i.e. sotre one of the child handle's device
        // path to variable "ConOut"
        // In futhure, we could select all child handles to be console device
        //

        *GopDevicePath = TempDevicePath;

        //
        // Delete the PCI device's path that added by GetPlugInPciVgaDevicePath()
        // Add the integrity GOP device path.
        //
        BdsLibUpdateConsoleVariable (VarConsoleOutDev, NULL, PciDevicePath);
        BdsLibUpdateConsoleVariable (VarConsoleOutDev, TempDevicePath, NULL);
      }
    }
    gBS->FreePool (GopHandleBuffer);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PreparePciVgaDevicePath (
  IN EFI_HANDLE                DeviceHandle
  )
/*++

Routine Description:

  Add PCI VGA to ConOut.
  PCI VGA: 03 00 00

Arguments:

  DeviceHandle            - Handle of PCIIO protocol.

Returns:

  EFI_SUCCESS             - PCI VGA is added to ConOut.
  EFI_STATUS              - No PCI VGA device is added.

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *GopDevicePath;

  DevicePath = NULL;
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID*)&DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  GetGopDevicePath (DevicePath, &GopDevicePath);
  DevicePath = GopDevicePath;

  BdsLibUpdateConsoleVariable (VarConsoleOut, DevicePath, NULL);

  return EFI_SUCCESS;
}

EFI_STATUS
PreparePciSerialDevicePath (
  IN EFI_HANDLE                DeviceHandle
  )
/*++

Routine Description:

  Add PCI Serial to ConOut, ConIn, ErrOut.
  PCI Serial: 07 00 02

Arguments:

  DeviceHandle            - Handle of PCIIO protocol.

Returns:

  EFI_SUCCESS             - PCI Serial is added to ConOut, ConIn, and ErrOut.
  EFI_STATUS              - No PCI Serial device is added.

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = NULL;
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID*)&DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gUartDeviceNode);
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gTerminalTypeDeviceNode);

  BdsLibUpdateConsoleVariable (VarConsoleOut, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarConsoleInp, DevicePath, NULL);
  BdsLibUpdateConsoleVariable (VarErrorOut, DevicePath, NULL);

  return EFI_SUCCESS;
}

EFI_STATUS
DetectAndPreparePlatformPciDevicePath (
  BOOLEAN DetectVgaOnly
  )
/*++

Routine Description:

  Do platform specific PCI Device check and add them to ConOut, ConIn, ErrOut

Arguments:

  DetectVgaOnly           - Only detect VGA device if it's TRUE.

Returns:

  EFI_SUCCESS             - PCI Device check and Console variable update successfully.
  EFI_STATUS              - PCI Device check or Console variable update fail.

--*/
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;

  //
  // Start to check all the PciIo to find all possible device
  //
  HandleCount = 0;
  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID*)&PciIo);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Check for all PCI device
    //
    Status = PciIo->Pci.Read (
                      PciIo,
                      EfiPciIoWidthUint32,
                      0,
                      sizeof (Pci) / sizeof (UINT32),
                      &Pci
                      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (!DetectVgaOnly) {
      //
      // Here we decide whether it is LPC Bridge
      //
      if ((IS_PCI_LPC (&Pci)) ||
          ((IS_PCI_ISA_PDECODE (&Pci)) && (Pci.Hdr.VendorId == 0x8086) && (Pci.Hdr.DeviceId == 0x7110))) {
        //
        // Add IsaKeyboard to ConIn,
        // add IsaSerial to ConOut, ConIn, ErrOut
        //
        PrepareLpcBridgeDevicePath (HandleBuffer[Index]);
        continue;
      }
      //
      // Here we decide which Serial device to enable in PCI bus
      //
      if (IS_PCI_16550SERIAL (&Pci)) {
        //
        // Add them to ConOut, ConIn, ErrOut.
        //
        PreparePciSerialDevicePath (HandleBuffer[Index]);
        continue;
      }
    }

    //
    // Here we decide which VGA device to enable in PCI bus
    //
    if (IS_PCI_VGA (&Pci)) {
      //
      // Add them to ConOut.
      //
      PreparePciVgaDevicePath (HandleBuffer[Index]);
      continue;
    }
  }

  gBS->FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

EFI_STATUS
PlatformBdsConnectConsole (
  IN BDS_CONSOLE_CONNECT_ENTRY   *PlatformConsole
  )
/*++

Routine Description:

  Connect the predefined platform default console device. Always try to find
  and enable the vga device if have.

Arguments:

  PlatformConsole         - Predfined platform default console device array.

Returns:

  EFI_SUCCESS             - Success connect at least one ConIn and ConOut
                            device, there must have one ConOut device is
                            active vga device.

  EFI_STATUS              - Return the status of
                            BdsLibConnectAllDefaultConsoles ()

--*/
{
  EFI_STATUS                         Status;
  UINTN                              Index;
  EFI_DEVICE_PATH_PROTOCOL           *VarConout;
  EFI_DEVICE_PATH_PROTOCOL           *VarConin;
  UINTN                              DevicePathSize;

  //
  // Connect RootBridge
  //
  ConnectRootBridge (); //

  VarConout = BdsLibGetVariableAndSize (
                VarConsoleOut,
                &gEfiGlobalVariableGuid,
                &DevicePathSize
                );
  VarConin = BdsLibGetVariableAndSize (
               VarConsoleInp,
               &gEfiGlobalVariableGuid,
               &DevicePathSize
               );

  if (VarConout == NULL || VarConin == NULL) {
    //
    // Do platform specific PCI Device check and add them to ConOut, ConIn, ErrOut
    //
    DetectAndPreparePlatformPciDevicePath (FALSE);

    //
    // Have chance to connect the platform default console,
    // the platform default console is the minimue device group
    // the platform should support
    //
    for (Index = 0; PlatformConsole[Index].DevicePath != NULL; ++Index) {
      //
      // Update the console variable with the connect type
      //
      if ((PlatformConsole[Index].ConnectType & CONSOLE_IN) == CONSOLE_IN) {
        BdsLibUpdateConsoleVariable (VarConsoleInp, PlatformConsole[Index].DevicePath, NULL);
      }
      if ((PlatformConsole[Index].ConnectType & CONSOLE_OUT) == CONSOLE_OUT) {
        BdsLibUpdateConsoleVariable (VarConsoleOut, PlatformConsole[Index].DevicePath, NULL);
      }
      if ((PlatformConsole[Index].ConnectType & STD_ERROR) == STD_ERROR) {
        BdsLibUpdateConsoleVariable (VarErrorOut, PlatformConsole[Index].DevicePath, NULL);
      }
    }
  } else {
    //
    // Only detect VGA device and add them to ConOut
    //
    DetectAndPreparePlatformPciDevicePath (TRUE);
  }

  //
  // The ConIn devices connection will start the USB bus, should disable all
  // Usb legacy support firstly.
  // Caution: Must ensure the PCI bus driver has been started. Since the
  // ConnectRootBridge() will create all the PciIo protocol, it's safe here now
  //
#ifdef SPEEDUP
#ifndef BLOCKIO
  Status = DisableUsbLegacySupport ();
#else
  if (ShiftKeyPressed () & EFI_LEFT_ALT_PRESSED) {
    Status = DisableUsbLegacySupport ();
  }
#endif
#else
  Status = FixUsbOwnership ();
#endif

  //
  // Connect the all the default console with current cosole variable
  //
  DBG ("  BdsPlatorm: Starting BdsLibConnectAllDefaultConsoles\n");  // 2.6 sec
  Status = BdsLibConnectAllDefaultConsoles ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

VOID
PlatformBdsConnectSequence (
  VOID
  )
/*++

Routine Description:

  Connect with predeined platform connect sequence,
  the OEM/IBV can customize with their own connect sequence.

Arguments:

  None.

Returns:

  None.

--*/
{
  UINTN Index;

  Index = 0;

  //
  // Here we can get the customized platform connect sequence
  // Notes: we can connect with new variable which record the
  // last time boots connect device path sequence
  //
  while (gPlatformConnectSequence[Index] != NULL) {
    //
    // Build the platform boot option
    //
    BdsLibConnectDevicePath (gPlatformConnectSequence[Index]);
    Index++;
  }

}

VOID
PlatformBdsGetDriverOption (
  IN OUT LIST_ENTRY              *BdsDriverLists
  )
/*++

Routine Description:

  Load the predefined driver option, OEM/IBV can customize this
  to load their own drivers

Arguments:

  BdsDriverLists  - The header of the driver option link list.

Returns:

  None.

--*/
{
  UINTN Index;

  Index = 0;

  //
  // Here we can get the customized platform driver option
  //
  while (gPlatformDriverOption[Index] != NULL) {
    //
    // Build the platform boot option
    //
    BdsLibRegisterNewOption (BdsDriverLists, NULL, gPlatformDriverOption[Index], NULL, L"DriverOrder", FALSE);
    Index++;
  }

}

VOID
PlatformBdsDiagnostics (
  IN EXTENDMEM_COVERAGE_LEVEL    MemoryTestLevel,
  IN BOOLEAN                     QuietBoot,
  IN BASEM_MEMORY_TEST           BaseMemoryTest
  )
/*++

Routine Description:

  Perform the platform diagnostic, such like test memory. OEM/IBV also
  can customize this fuction to support specific platform diagnostic.

Arguments:

  MemoryTestLevel  - The memory test intensive level

  QuietBoot        - Indicate if need to enable the quiet boot

  BaseMemoryTest   - A pointer to BdsMemoryTest()

Returns:

  None.

--*/
{
  EFI_STATUS  Status;

  //
  // Here we can decide if we need to show
  // the diagnostics screen
  // Notes: this quiet boot code should be remove
  // from the graphic lib
  //
#if 0
  if (QuietBoot) {
    Status = EnableQuietBoot (PcdGetPtr(PcdLogoFile));
    if (EFI_ERROR (Status)) {
      DisableQuietBoot ();
      return;
    }

    //
    // Perform system diagnostic
    //
    Status = BaseMemoryTest (MemoryTestLevel);
    if (EFI_ERROR (Status)) {
      DisableQuietBoot ();
    }

    return ;
  }
#endif
  //
  // Perform system diagnostic
  //
  Status = BaseMemoryTest (MemoryTestLevel);
}

#if 0
VOID
EnableSmbus (
             VOID
             )
{
  EFI_STATUS            Status;
  EFI_HANDLE            *HandleBuffer;
  EFI_GUID              **ProtocolGuidArray;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINTN                 HandleCount;
  UINTN                 ArrayCount;
  UINTN                 HandleIndex;
  UINTN                 ProtocolIndex;
  UINTN                 Segment;
  UINTN                 Bus;
  UINTN                 Device;
  UINTN                 Function;
	UINT32                rcba;
  UINT32                *fdr;

  Status = gBS->LocateHandleBuffer (AllHandles, NULL, NULL, &HandleCount, &HandleBuffer);

  if (!EFI_ERROR (Status)) {
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      Status = gBS->ProtocolsPerHandle (HandleBuffer[HandleIndex], &ProtocolGuidArray, &ArrayCount);

      if (!EFI_ERROR (Status)) {
        for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {
          if (CompareGuid (&gEfiPciIoProtocolGuid, ProtocolGuidArray[ProtocolIndex])) {
            Status = gBS->OpenProtocol (HandleBuffer[HandleIndex],
                                        &gEfiPciIoProtocolGuid,
                                        (VOID **) &PciIo,
                                        gImageHandle,
                                        NULL,
                                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                                        );

            if (!EFI_ERROR (Status)) {
              /* Read PCI BUS */
              Status = PciIo->Pci.Read (
                                        PciIo,
                                        EfiPciIoWidthUint32,
                                        0,
                                        sizeof (gPci) / sizeof (UINT32),
                                        &gPci
                                        );

              if (gPci.Hdr.VendorId == 0x8086) {
                Status = PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);
                /**
                 *
                 * if found LPC - write 0 in SMbus Disabled (3 bit Function Disable register 0x3418)
                 *
                 **/
                if ((Bus == 0) && (Device == 0x1F) && (Function == 0)) {
                  Status = PciIo->Pci.Read (
                                            PciIo,
                                            EfiPciIoWidthUint32,
                                            (UINT64) (0xF0 & ~3),
                                            1,
                                            &rcba
                                            );
                  rcba &= ~1;
                  Print (L"RCBA = 0x%x\n", rcba);
                  fdr = ((UINT32 *) (UINTN) (rcba + 0x3418));
                  Print (L"fdr = 0x%x\n", *((UINT32 *) (UINTN) (rcba + 0x3418)));
                  *fdr &= ~0x8;
                  Print (L"fdr = 0x%x\n", *((UINT32 *) (UINTN) (rcba + 0x3418)));
                  Pause (NULL);
                }
              }
            }
          }
        }
      }
    }
  }
}
#endif

CHAR16*
MakeProductNameDir (
  IN CHAR16 *ProductName
  )
{
  CHAR16 *tp;

  if (ProductName == NULL) {
    return NULL;
  }

  tp = ProductName + StrLen (ProductName);

  /* Trim trailing nonprintables */
  while (tp >= ProductName && (*tp <= L' ' || *tp == 0x00FF)) {
    *tp = 0x0000;
    tp--;
  }
  if (StrLen (ProductName) < 1) {
    FreePool (ProductName);
    return NULL;
  }
  tp = AllocateZeroPool (StrSize (L"\\EFI\\bareboot\\") + StrSize (ProductName) + StrSize (L"\\"));
  StrCpy (tp, L"\\EFI\\bareboot\\");
  StrCat (tp, ProductName);
  StrCat (tp, L"\\");
  FreePool (ProductName);
  return tp;
}

VOID
EFIAPI
PlatformBdsPolicyBehavior (
  IN BASEM_MEMORY_TEST           BaseMemoryTest
  )
/*++

Routine Description:

  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.

Arguments:

  DriverOptionList - The header of the driver option link list

  BootOptionList   - The header of the boot option link list

Returns:

  None.

--*/
{
  EFI_STATUS                          Status;
  EFI_BOOT_MODE                       BootMode;
#if 0
  CHAR16                              *TmpUStr;
  EFI_INPUT_KEY                       mKey;
#else
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL   *SimpleTextInEx;
  EFI_KEY_DATA                        mKeyData;
#endif
  EFI_SMBIOS_HANDLE                 SmbiosHandle;
  EFI_SMBIOS_PROTOCOL               *Smbios;
  EFI_SMBIOS_TABLE_HEADER           *Record;
  SMBIOS_TABLE_TYPE1                *Type1Record;
  SMBIOS_TABLE_TYPE2                *Type2Record;
  BOOLEAN                           GotIt[2];
  CHAR16                            *TmpString1;
  CHAR16                            *TmpString2;
  UINT8                             StrIndex;
  
  gPNDirExists = FALSE;
  gPNConfigPlist = NULL;
  gPNAcpiDir = NULL;
  TmpString1 = NULL;
  TmpString2 = NULL;
  GotIt[0] = FALSE;
  GotIt[1] = FALSE;
  
  DBG ("BdsPlatorm: Starting BdsLibGetBootMode\n");
  Status = BdsLibGetBootMode (&BootMode);
  ASSERT (BootMode == BOOT_WITH_FULL_CONFIGURATION);
  // very long function:
  DBG ("BdsPlatorm: Starting PlatformBdsConnectConsole\n"); // 5.2 sec
  PlatformBdsConnectConsole (gPlatformConsole);
  DBG ("BdsPlatorm: Starting PlatformBdsDiagnostics\n");
  PlatformBdsDiagnostics (IGNORE, TRUE, BaseMemoryTest);
#if 0
  EnableSmbus ();
#endif
  DBG ("BdsPlatorm: Starting BdsLibConnectAllDriversToAllControllers\n");  // 2.3 sec
  BdsLibConnectAllDriversToAllControllers ();

  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **) &Smbios
                );
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;

  do {
    Status = Smbios->GetNext (Smbios, &SmbiosHandle, NULL, &Record, NULL);
    if (EFI_ERROR(Status)) {
      break;
    }
    if (Record->Type == EFI_SMBIOS_TYPE_SYSTEM_INFORMATION) {
      Type1Record = (SMBIOS_TABLE_TYPE1 *) Record;
      StrIndex = Type1Record->ProductName;
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)Type1Record + Type1Record->Hdr.Length), StrIndex, &TmpString1);
      GotIt[0] = TRUE;
    }
    if (Record->Type == EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION) {
      Type2Record = (SMBIOS_TABLE_TYPE2 *) Record;
      StrIndex = Type2Record->ProductName;
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)Type2Record + Type2Record->Hdr.Length), StrIndex, &TmpString2);
      GotIt[1] = TRUE;
    }
  } while (!(GotIt[0] && GotIt[1]));
  DBG ("BdsPlatorm: DMI TableType1->ProductName = '%s'\n", TmpString1);
  DBG ("BdsPlatorm: DMI TableType2->ProductName = '%s'\n", TmpString2);

  gProductNameDir = MakeProductNameDir (TmpString1);
  DBG ("BdsPlatorm: ProductNameDir = '%s'\n", gProductNameDir);

  gProductNameDir2 = MakeProductNameDir (TmpString2);
  DBG ("BdsPlatorm: ProductNameDir2 = '%s'\n", gProductNameDir2);

  DBG ("BdsPlatorm: Starting BdsLibEnumerateAllBootOption\n");
  BdsLibEnumerateAllBootOption (&gBootOptionList);

  AddBootArgs = "\0 23456789012345678901234567890123456789";
  if (ShiftKeyPressed () & EFI_LEFT_SHIFT_PRESSED) {
    AsciiStrCat (AddBootArgs, " -v");
  }
  if (ShiftKeyPressed () & EFI_LEFT_CONTROL_PRESSED) {
    AsciiStrCat (AddBootArgs, " -s");
  }
  if ((ShiftKeyPressed () & EFI_RIGHT_ALT_PRESSED) ||
      (ShiftKeyPressed () & EFI_RIGHT_CONTROL_PRESSED)) {
    AsciiStrCat (AddBootArgs, " -x");
  }
  if (ShiftKeyPressed () & EFI_LEFT_ALT_PRESSED) {
#if 0
    Status = gST->ConIn->Reset (gST->ConIn, TRUE);
#endif
    PlatformBdsEnterFrontPage (0xffff, TRUE);
  }

#if 0
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &mKey);
  if (!EFI_ERROR (Status)) {
    if (mKey.ScanCode == SCAN_F1) {
      Status = gST->ConIn->Reset (gST->ConIn, TRUE);
      PlatformBdsEnterFrontPage (0xffff, TRUE);
    }
  }
#else
  Status = gBS->LocateProtocol (
                  &gEfiSimpleTextInputExProtocolGuid,
                  NULL,
                  (VOID **) &SimpleTextInEx
                  );
  
  if (!EFI_ERROR (Status)) {
    Status = SimpleTextInEx->ReadKeyStrokeEx (
                               SimpleTextInEx,
                               &mKeyData
                               );
    
    if (mKeyData.Key.ScanCode == SCAN_F1) {
#if 0
      (mKeyData.KeyState.KeyShiftState & EFI_LEFT_ALT_PRESSED) {
#endif
      Status = SimpleTextInEx->Reset (
                                 SimpleTextInEx,
                                 TRUE
                                 );
      PlatformBdsEnterFrontPage (0xffff, TRUE);
    }
  }
#endif
  DBG ("BdsPlatorm: Starting PlatformBdsEnterFrontPage\n");
  PlatformBdsEnterFrontPage (gSettings.BootTimeout, TRUE);
  return ;
}

VOID
EFIAPI
PlatformBdsBootSuccess (
  IN  BDS_COMMON_OPTION *Option
  )
/*++

Routine Description:

  Hook point after a boot attempt succeeds. We don't expect a boot option to
  return, so the EFI 1.0 specification defines that you will default to an
  interactive mode and stop processing the BootOrder list in this case. This
  is alos a platform implementation and can be customized by IBV/OEM.

Arguments:

  Option - Pointer to Boot Option that succeeded to boot.

Returns:

  None.

--*/
{
  CHAR16  *TmpStr;

  //
  // If Boot returned with EFI_SUCCESS and there is not in the boot device
  // select loop then we need to pop up a UI and wait for user input.
  //
  TmpStr = Option->StatusString;
  if (TmpStr != NULL) {
    BdsLibOutputStrings (gST->ConOut, TmpStr, Option->Description, L"\n\r", NULL);
    gBS->FreePool (TmpStr);
  }
}

VOID
EFIAPI
PlatformBdsBootFail (
  IN  BDS_COMMON_OPTION  *Option,
  IN  EFI_STATUS         Status,
  IN  CHAR16             *ExitData,
  IN  UINTN              ExitDataSize
  )
/*++

Routine Description:

  Hook point after a boot attempt fails.

Arguments:

  Option - Pointer to Boot Option that failed to boot.

  Status - Status returned from failed boot.

  ExitData - Exit data returned from failed boot.

  ExitDataSize - Exit data size returned from failed boot.

Returns:

  None.

--*/
{
  CHAR16  *TmpStr;

  //
  // If Boot returned with failed status then we need to pop up a UI and wait
  // for user input.
  //
  TmpStr = Option->StatusString;
  if (TmpStr != NULL) {
    BdsLibOutputStrings (gST->ConOut, TmpStr, Option->Description, L"\n\r", NULL);
    gBS->FreePool (TmpStr);
  }

}

EFI_STATUS
ConvertSystemTable (
  IN     EFI_GUID        *TableGuid,
  IN OUT VOID            **Table
  )
/*++

Routine Description:
  Convert ACPI Table /Smbios Table /MP Table if its location is lower than Address:0x100000
  Assumption here:
   As in legacy Bios, ACPI/Smbios/MP table is required to place in E/F Seg,
   So here we just check if the range is E/F seg,
   and if Not, assume the Memory type is EfiACPIReclaimMemory/EfiACPIMemoryNVS

Arguments:
  TableGuid - Guid of the table
  Table     - pointer to the table

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  EFI_STATUS      Status;
  VOID            *AcpiHeader;
  UINTN           AcpiTableLen;

  //
  // If match acpi guid (1.0, 2.0, or later), Convert ACPI table according to version.
  //
  AcpiHeader = (VOID*)(UINTN)(*(UINT64 *)(*Table));

  if (CompareGuid(TableGuid, &gEfiAcpiTableGuid) || CompareGuid(TableGuid, &gEfiAcpi20TableGuid)){
    if (((EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiHeader)->Reserved == 0x00){
      //
      // If Acpi 1.0 Table, then RSDP structure doesn't contain Length field, use structure size
      //
      AcpiTableLen = sizeof (EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER);
    } else if (((EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiHeader)->Reserved >= 0x02){
      //
      // If Acpi 2.0 or later, use RSDP Length fied.
      //
      AcpiTableLen = ((EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiHeader)->Length;
    } else {
      //
      // Invalid Acpi Version, return
      //
      return EFI_UNSUPPORTED;
    }
    Status = ConvertAcpiTable (AcpiTableLen, Table);
    return Status;
  }

  //
  // If matches smbios guid, convert Smbios table.
  //
  if (CompareGuid(TableGuid, &gEfiSmbiosTableGuid)){
    Status = ConvertSmbiosTable (Table);
    return Status;
  }

  //
  // If the table is MP table?
  //
  if (CompareGuid(TableGuid, &gEfiMpsTableGuid)){
    Status = ConvertMpsTable (Table);
    return Status;
  }

  return EFI_UNSUPPORTED;
}


EFI_STATUS
ConvertAcpiTable (
  IN     UINTN                       TableLen,
  IN OUT VOID                        **Table
  )
/*++

Routine Description:
  Convert RSDP of ACPI Table if its location is lower than Address:0x100000
  Assumption here:
   As in legacy Bios, ACPI table is required to place in E/F Seg,
   So here we just check if the range is E/F seg,
   and if Not, assume the Memory type is EfiACPIReclaimMemory/EfiACPIMemoryNVS

Arguments:
  TableLen  - Acpi RSDP length
  Table     - pointer to the table

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  VOID                  *AcpiTableOri;
  VOID                  *AcpiTableNew;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  BufferPtr;


  AcpiTableOri    =  (VOID *)(UINTN)(*(UINT64*)(*Table));
  if (((UINTN)AcpiTableOri < 0x100000) && ((UINTN)AcpiTableOri > 0xE0000)) {
    BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiACPIMemoryNVS,
                    EFI_SIZE_TO_PAGES(TableLen),
                    &BufferPtr
                    );
    ASSERT_EFI_ERROR (Status);
    AcpiTableNew = (VOID *)(UINTN)BufferPtr;
    CopyMem (AcpiTableNew, AcpiTableOri, TableLen);
  } else {
    AcpiTableNew = AcpiTableOri;
  }
  //
  // Change configuration table Pointer
  //
  *Table = AcpiTableNew;

  return EFI_SUCCESS;
}

EFI_STATUS
ConvertSmbiosTable (
  IN OUT VOID        **Table
  )
/*++

Routine Description:

  Convert Smbios Table if the Location of the SMBios Table is lower than Addres 0x100000
  Assumption here:
   As in legacy Bios, Smbios table is required to place in E/F Seg,
   So here we just check if the range is F seg,
   and if Not, assume the Memory type is EfiACPIMemoryNVS/EfiRuntimeServicesData
Arguments:
  Table     - pointer to the table

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  SMBIOS_TABLE_ENTRY_POINT *SmbiosTableNew;
  SMBIOS_TABLE_ENTRY_POINT *SmbiosTableOri;
  EFI_STATUS               Status;
  UINT32                   SmbiosEntryLen;
  UINT32                   BufferLen;
  EFI_PHYSICAL_ADDRESS     BufferPtr;

  SmbiosTableNew  = NULL;
  SmbiosTableOri  = NULL;

  //
  // Get Smibos configuration Table
  //
  SmbiosTableOri =  (SMBIOS_TABLE_ENTRY_POINT *)(UINTN)(*(UINT64*)(*Table));

  if ((SmbiosTableOri == NULL) ||
      ((UINTN)SmbiosTableOri > 0x100000) ||
      ((UINTN)SmbiosTableOri < 0xF0000)){
    return EFI_SUCCESS;
  }
  //
  // Relocate the Smibos memory
  //
  BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
  if (SmbiosTableOri->SmbiosBcdRevision != 0x21) {
    SmbiosEntryLen  = SmbiosTableOri->EntryPointLength;
  } else {
    //
    // According to Smbios Spec 2.4, we should set entry point length as 0x1F if version is 2.1
    //
    SmbiosEntryLen = 0x1F;
  }
  BufferLen = SmbiosEntryLen + SYS_TABLE_PAD(SmbiosEntryLen) + SmbiosTableOri->TableLength;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(BufferLen),
                  &BufferPtr
                  );
  ASSERT_EFI_ERROR (Status);
  SmbiosTableNew = (SMBIOS_TABLE_ENTRY_POINT *)(UINTN)BufferPtr;
  CopyMem (
    SmbiosTableNew,
    SmbiosTableOri,
    SmbiosEntryLen
    );
  //
  // Get Smbios Structure table address, and make sure the start address is 32-bit align
  //
  BufferPtr += SmbiosEntryLen + SYS_TABLE_PAD(SmbiosEntryLen);
  CopyMem (
    (VOID *)(UINTN)BufferPtr,
    (VOID *)(UINTN)(SmbiosTableOri->TableAddress),
    SmbiosTableOri->TableLength
    );
  SmbiosTableNew->TableAddress = (UINT32)BufferPtr;
  SmbiosTableNew->IntermediateChecksum = 0;
  SmbiosTableNew->IntermediateChecksum =
          CalculateCheckSum8 ((UINT8*)SmbiosTableNew + 0x10, SmbiosEntryLen -0x10);
  //
  // Change the SMBIOS pointer
  //
  *Table = SmbiosTableNew;

  return EFI_SUCCESS;
}

EFI_STATUS
ConvertMpsTable (
  IN OUT VOID          **Table
  )
/*++

Routine Description:

  Convert MP Table if the Location of the SMBios Table is lower than Addres 0x100000
  Assumption here:
   As in legacy Bios, MP table is required to place in E/F Seg,
   So here we just check if the range is E/F seg,
   and if Not, assume the Memory type is EfiACPIMemoryNVS/EfiRuntimeServicesData
Arguments:
  Table     - pointer to the table

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  UINT32                                       Data32;
  UINT32                                       FPLength;
  EFI_LEGACY_MP_TABLE_FLOATING_POINTER         *MpsFloatingPointerOri;
  EFI_LEGACY_MP_TABLE_FLOATING_POINTER         *MpsFloatingPointerNew;
  EFI_LEGACY_MP_TABLE_HEADER                   *MpsTableOri;
  EFI_LEGACY_MP_TABLE_HEADER                   *MpsTableNew;
  VOID                                         *OemTableOri;
  VOID                                         *OemTableNew;
  EFI_STATUS                                   Status;
  EFI_PHYSICAL_ADDRESS                         BufferPtr;

  //
  // Get MP configuration Table
  //
  MpsFloatingPointerOri = (EFI_LEGACY_MP_TABLE_FLOATING_POINTER *)(UINTN)(*(UINT64*)(*Table));
  if (!(((UINTN)MpsFloatingPointerOri <= 0x100000) &&
        ((UINTN)MpsFloatingPointerOri >= 0xF0000))){
    return EFI_SUCCESS;
  }
  //
  // Get Floating pointer structure length
  //
  FPLength = MpsFloatingPointerOri->Length * 16;
  Data32   = FPLength + SYS_TABLE_PAD (FPLength);
  MpsTableOri = (EFI_LEGACY_MP_TABLE_HEADER *)(UINTN)(MpsFloatingPointerOri->PhysicalAddress);
  if (MpsTableOri != NULL) {
    Data32 += MpsTableOri->BaseTableLength;
    Data32 += MpsTableOri->ExtendedTableLength;
    if (MpsTableOri->OemTablePointer != 0x00) {
      Data32 += SYS_TABLE_PAD (Data32);
      Data32 += MpsTableOri->OemTableSize;
    }
  } else {
    return EFI_SUCCESS;
  }
  //
  // Relocate memory
  //
  BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(Data32),
                  &BufferPtr
                  );
  ASSERT_EFI_ERROR (Status);
  MpsFloatingPointerNew = (EFI_LEGACY_MP_TABLE_FLOATING_POINTER *)(UINTN)BufferPtr;
  CopyMem (MpsFloatingPointerNew, MpsFloatingPointerOri, FPLength);
  //
  // If Mp Table exists
  //
  if (MpsTableOri != NULL) {
    //
    // Get Mps table length, including Ext table
    //
    BufferPtr = BufferPtr + FPLength + SYS_TABLE_PAD (FPLength);
    MpsTableNew = (EFI_LEGACY_MP_TABLE_HEADER *)(UINTN)BufferPtr;
    CopyMem (MpsTableNew, MpsTableOri, MpsTableOri->BaseTableLength + MpsTableOri->ExtendedTableLength);

    if ((MpsTableOri->OemTableSize != 0x0000) && (MpsTableOri->OemTablePointer != 0x0000)){
        BufferPtr += MpsTableOri->BaseTableLength + MpsTableOri->ExtendedTableLength;
        BufferPtr += SYS_TABLE_PAD (BufferPtr);
        OemTableNew = (VOID *)(UINTN)BufferPtr;
        OemTableOri = (VOID *)(UINTN)MpsTableOri->OemTablePointer;
        CopyMem (OemTableNew, OemTableOri, MpsTableOri->OemTableSize);
        MpsTableNew->OemTablePointer = (UINT32)(UINTN)OemTableNew;
    }
    MpsTableNew->Checksum = 0;
    MpsTableNew->Checksum = CalculateCheckSum8 ((UINT8*)MpsTableNew, MpsTableOri->BaseTableLength);
    MpsFloatingPointerNew->PhysicalAddress = (UINT32)(UINTN)MpsTableNew;
    MpsFloatingPointerNew->Checksum = 0;
    MpsFloatingPointerNew->Checksum = CalculateCheckSum8 ((UINT8*)MpsFloatingPointerNew, FPLength);
  }
  //
  // Change the pointer
  //
  *Table = MpsFloatingPointerNew;

  return EFI_SUCCESS;
}

/**
  Lock the ConsoleIn device in system table. All key
  presses will be ignored until the Password is typed in. The only way to
  disable the password is to type it in to a ConIn device.

  @param  Password        Password used to lock ConIn device.

  @retval EFI_SUCCESS     lock the Console In Spliter virtual handle successfully.
  @retval EFI_UNSUPPORTED Password not found

**/
