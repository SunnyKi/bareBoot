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
  PLATFORM_GUID                  = 199E24E0-0989-42aa-87F2-611A8C397E72
  PLATFORM_VERSION               = 0.4
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/bareBoot
  SUPPORTED_ARCHITECTURES        = IA32 || X64
  BUILD_TARGETS                  = DEBUG || RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = bareBoot/bareBoot.fdf

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################
[LibraryClasses]
!ifdef $(SOURCE_DEBUG_ENABLE)
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
  DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibSerialPort/DebugCommunicationLibSerialPort.inf
  PeCoffExtraActionLib|SourceLevelDebugPkg/Library/PeCoffExtraActionLibDebug/PeCoffExtraActionLibDebug.inf
  ResetSystemLib|MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
!else
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  PeCoffLib|bareBoot/Library/VBoxPeCoffLib/VBoxPeCoffLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif

  #
  # Entry point
  #
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
#  EfiFileLib|EmbeddedPkg/Library/EfiFileLib/EfiFileLib.inf
#  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
#  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
#  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf

#  IoApicLib|PcAtChipsetPkg/Library/BaseIoApicLib/BaseIoApicLib.inf

  LocalApicLib|UefiCpuPkg/Library/BaseXApicLib/BaseXApicLib.inf

  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  #
  # UEFI & PI
  #
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
#
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
  
  #
  # Generic Modules
  #
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf

  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  #
  # Platform
  #
  TimerLib|bareBoot/Library/DuetTimerLib/DuetTimerLib.inf
  #
  # Misc
  #
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
#  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
  EfiFileLib|EmbeddedPkg/Library/EfiFileLib/EfiFileLib.inf
#  EblNetworkLib|EmbeddedPkg/Library/EblNetworkLib/EblNetworkLib.inf
  CpuExceptionHandlerLib|MdeModulePkg/Library/CpuExceptionHandlerLibNull/CpuExceptionHandlerLibNull.inf

  #
  # To save size, use NULL library for DebugLib and ReportStatusCodeLib.
  # If need status code output, do library instance overriden as below DxeMain.inf does
  #

  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf  
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  MemLogLib|bareBoot/Library/MemLogLibDefault/MemLogLibDefault.inf
  PListLib|bareBoot/Library/PListLib/PListLib.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild]
#  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor|L"Apple"|VOID*|32
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiFloppyAEnable|FALSE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiFloppyBEnable|FALSE
#  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiCom1Enable|FALSE
#  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiCom2Enable|FALSE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiCom1Enable|TRUE
  gPcAtChipsetPkgTokenSpaceGuid.PcdIsaAcpiCom2Enable|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseHardwareFlowControl|TRUE
  gPcAtChipsetPkgTokenSpaceGuid.Pcd8259LegacyModeMask|0xFFFC
!if $(TARGET) == "DEBUG"
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0F
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x804FEFCF
!else
#  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x0
#  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0
#  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x0
!endif
  
[PcdsPatchableInModule]
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|0
  
[PcdsFeatureFlag]
  gEfiMdeModulePkgTokenSpaceGuid.PcdTurnOffUsbLegacySupport|FALSE

###################################################################################################
[Components]
  bareBoot/DxeIpl/DxeIpl.inf 
  MdeModulePkg/Core/Dxe/DxeMain.inf 

  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf

  MdeModulePkg/Universal/Variable/EmuRuntimeDxe/EmuVariableRuntimeDxe.inf
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

  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
#  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  bareBoot/SmbiosGenDxe/SmbiosGen.inf

  bareBoot/EfiLdr/EfiLdr.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
      NULL|IntelFrameworkModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }

  bareBoot/Library/BdsDxe/BdsDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
# ----+++++++------
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf
  UefiCpuPkg/CpuDxe/CpuDxe.inf
  bareBoot/CpuDxe/Cpu.inf
  PcAtChipsetPkg/8259InterruptControllerDxe/8259.inf
  bareBoot/AcpiResetDxe/Reset.inf
  bareBoot/LegacyMetronome/Metronome.inf

  PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  PcAtChipsetPkg/8254TimerDxe/8254Timer.inf
  bareBoot/PciRootBridgeNoEnumerationDxe/PciRootBridgeNoEnumeration.inf
  bareBoot/PciBusNoEnumerationDxe/PciBusNoEnumeration.inf

#  IntelFrameworkModulePkg/Bus/Pci/VgaMiniPortDxe/VgaMiniPortDxe.inf
#  IntelFrameworkModulePkg/Universal/Console/VgaClassDxe/VgaClassDxe.inf

  # IDE/AHCI Support
#  IntelFrameworkModulePkg/Csm/BiosThunk/BlockIoDxe/BlockIoDxe.inf
  bareBoot/BlockIoDxe/BlockIoDxe.inf
  bareBoot/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf


  # Usb Support
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf

  # ISA Support
  PcAtChipsetPkg/IsaAcpiDxe/IsaAcpi.inf
  IntelFrameworkModulePkg/Bus/Isa/IsaBusDxe/IsaBusDxe.inf
  IntelFrameworkModulePkg/Bus/Isa/Ps2KeyboardDxe/Ps2keyboardDxe.inf
  bareBoot/BiosKeyboard/KeyboardDxe.inf

  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  bareBoot/PartitionDxe/PartitionDxe.inf


  # Bios Thunk
  bareBoot/BiosVideoThunkDxe/BiosVideo.inf

  # HFS+
  bareBoot/VBoxFsDxe/VBoxHfs.inf

  # ACPI Support
#  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
#  MdeModulePkg/Universal/Acpi/AcpiPlatformDxe/AcpiPlatformDxe.inf
#  bareBoot/OsxAcpiTableDxe/AcpiTableDxe.inf
#  bareBoot/OsxAcpiPlatformDxe/AcpiPlatformDxe.inf

  # DataHub
  IntelFrameworkModulePkg/Universal/DataHubDxe/DataHubDxe.inf

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
!ifdef $(SOURCE_DEBUG_ENABLE)
  *_*_*_GENFW_FLAGS = --keepexceptiontable
!endif
