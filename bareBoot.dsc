## @file
#  An EFI/Framework Emulation Platform with UEFI HII interface supported.
#
#  Developer's UEFI Emulation. DUET provides an EFI/UEFI IA32/X64 environment on legacy BIOS,
#  to help developing and debugging native EFI/UEFI drivers.
#
#  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution. The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################

[Defines]

  PLATFORM_NAME                  = bareBoot
  PLATFORM_GUID                  = 199E24E0-0989-42AA-87F2-611A8C397E72
  PLATFORM_VERSION               = 0.4
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/bareBoot/$(ARCH)
  SUPPORTED_ARCHITECTURES        = IA32 | X64
  BUILD_TARGETS                  = NOOPT | DEBUG | RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = bareBoot.fdf

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

[LibraryClasses]

  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf

  # Entry point

  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

  # Basic

  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf

!ifdef EDK2PCI
  PciHostBridgeLib|MdeModulePkg/Library/PciHostBridgeLibNull/PciHostBridgeLibNull.inf
  PciSegmentLib|MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf
!endif

  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf

#  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
#  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
#  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf

#  IoApicLib|PcAtChipsetPkg/Library/BaseIoApicLib/BaseIoApicLib.inf

  LocalApicLib|UefiCpuPkg/Library/BaseXApicLib/BaseXApicLib.inf

  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf

  # UEFI & PI

  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf

  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
  
  # Generic Modules

  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf

  CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf

  # Platform

  TimerLib|Library/BBTimerLib/BBTimerLib.inf

  # Misc

  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf

!if "$(TARGET)" == "RELEASE"
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
!else
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
!endif

  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf

#  EblNetworkLib|EmbeddedPkg/Library/EblNetworkLib/EblNetworkLib.inf
#  CpuExceptionHandlerLib|MdeModulePkg/Library/CpuExceptionHandlerLibNull/CpuExceptionHandlerLibNull.inf

  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf

!ifndef OLDCPUDXE
  MpInitLib|UefiCpuPkg/Library/MpInitLib/DxeMpInitLib.inf
!endif

  # To save size, use NULL library for DebugLib and ReportStatusCodeLib.
  # If need status code output, do library instance overriden as below DxeMain.inf does

  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf  
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  MemLogLib|Library/MemLogLibDefault/MemLogLibDefault.inf
  PListLib|Library/PListLib/PListLib.inf
  ShiftKeysLib|Library/ShiftKeysLib/ShiftKeysLib.inf
  LegacyBiosThunkLib|Library/LegacyBiosThunkLib/LegacyBiosThunkLib.inf

  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

[LibraryClasses.common.DXE_CORE]

  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf

###################################################################################################

[Components]

  BootSector2/BootSector2.inf

  DxeIpl/DxeIpl.inf 
  MdeModulePkg/Core/Dxe/DxeMain.inf 

  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf

  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <PcdsFixedAtBuild>
      gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
    <LibraryClasses>
      AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
      TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
      VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
  }

  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf

#  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf

  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf

  GraphicsConsoleDxe/GraphicsConsoleDxe.inf

#  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf

  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  SmbiosGenDxe/SmbiosGen.inf

  EfiLdr/EfiLdr.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }

  BdsDxe/BdsDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }

# ----+++++++------

  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf

!ifdef OLDCPUDXE
  Cpu/CpuDxe/Cpu.inf
!else
  UefiCpuPkg/CpuDxe/CpuDxe.inf
!endif

  PcAtChipsetPkg/8259InterruptControllerDxe/8259.inf
  AcpiResetDxe/Reset.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf

  PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  PcAtChipsetPkg/8254TimerDxe/8254Timer.inf
  LegacyRegion2Dxe/LegacyRegion2Dxe.inf

!ifdef EDK2PCI
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf {
    <LibraryClasses>
      PciHostBridgeLib|CorebootPayloadPkg/Library/PciHostBridgeLib/PciHostBridgeLib.inf
  }
!else
  PciRootBridgeNoEnumerationDxe/PciRootBridgeNoEnumeration.inf
  PciBusNoEnumerationDxe/PciBusNoEnumeration.inf
!endif

#  IntelFrameworkModulePkg/Bus/Pci/VgaMiniPortDxe/VgaMiniPortDxe.inf
#  IntelFrameworkModulePkg/Universal/Console/VgaClassDxe/VgaClassDxe.inf

  # IDE/AHCI Support

#  IntelFrameworkModulePkg/Csm/BiosThunk/BlockIoDxe/BlockIoDxe.inf

!ifdef BLOCKIO
  BlockIoDxe/BlockIoDxe.inf
!else
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
!endif

  # Usb Support

!ifdef SPEEDUP
  Speedup/Bus/Pci/UhciDxe/UhciDxe.inf
  Speedup/Bus/Pci/EhciDxe/EhciDxe.inf
  Speedup/Bus/Pci/XhciDxe/XhciDxe.inf
  Speedup/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  Speedup/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  Speedup/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
!else
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
!endif

  QuarkSocPkg/QuarkSouthCluster/Usb/Ohci/Dxe/OhciDxe.inf

  # ISA Support

  PcAtChipsetPkg/IsaAcpiDxe/IsaAcpi.inf
  IntelFrameworkModulePkg/Bus/Isa/IsaBusDxe/IsaBusDxe.inf
  IntelFrameworkModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2keyboardDxe.inf
  BiosKeyboard/KeyboardDxe.inf

  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

!ifdef EDK2PART
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
!else
  PartitionDxe/PartitionDxe.inf
!endif


  # Bios Thunk

  BiosVideoThunkDxe/BiosVideo.inf

  FatPkg/EnhancedFatDxe/Fat.inf

  # HFS+

!ifdef VBOXHFS
  VBoxFsDxe/VBoxHfs.inf
!endif

  # DataHub

  IntelFrameworkModulePkg/Universal/DataHubDxe/DataHubDxe.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFixedAtBuild]

  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor|L"Apple"
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiFloppyAEnable|FALSE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiFloppyBEnable|FALSE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiCom1Enable|TRUE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiCom2Enable|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|TRUE

!if "$(TARGET)" == "RELEASE"
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x00
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x00
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x00000000
!else
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0F
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x804FEFCF
!endif

[PcdsPatchableInModule]

  gPcAtChipsetPkgTokenSpaceGuid.Pcd8259LegacyModeMask|0xFFFC
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|0

###################################################################################################
#
# BuildOptions Section - Define the module specific tool chain flags that should be used as
#                        the default flags for a module. These flags are appended to any 
#                        standard flags that are defined by the build process. They can be 
#                        applied for any modules or only those modules with the specific 
#                        module style (EDK or EDKII) specified in [Components] section.
#
###################################################################################################

[BuildOptions]

!ifdef BLOCKIO
  DEFINE DEF_BLOCKIO = -DBLOCKIO
!endif
!ifdef SPEEDUP
  DEFINE DEF_SPEEDUP = -DSPEEDUP
!endif
!ifdef MEMLOG2SERIAL
  DEFINE DEF_MEMLOG2SERIAL = -DMEMLOG2SERIAL
!endif
!if "$(TARGET)" == "RELEASE"
  DEFINE DEF_NDEBUG = -DMDEPKG_NDEBUG
!else
  DEFINE DEF_NDEBUG = 
!endif

  *_*_*_CC_FLAGS   = -DDISABLE_NEW_DEPRECATED_INTERFACES $(DEF_NDEBUG) $(DEF_BLOCKIO) $(DEF_SPEEDUP) $(DEF_MEMLOG2SERIAL)
