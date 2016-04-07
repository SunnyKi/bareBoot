/**
 smbios.c
 original idea of SMBIOS patching by mackerintel
 implementation for UEFI smbios table patching
  Slice 2011.

 portion copyright Intel
 Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 Module Name:

 SmbiosGen.c
 **/

#include <IndustryStandard/SmBios.h>

#include <macosx.h>

#include "cpu.h"
#include "OsxSmBios.h"

#define MAX_HANDLE        0xFEFF
#define SMBIOS_PTR        SIGNATURE_32('_','S','M','_')
#define MAX_TABLE_SIZE    512

static EFI_GUID             *gTableGuidArray[] = {&gEfiSmbiosTableGuid};

VOID                        *Smbios;  //pointer to SMBIOS data
SMBIOS_TABLE_ENTRY_POINT    *EntryPoint; //SmbiosEps original
SMBIOS_TABLE_ENTRY_POINT    *SmbiosEpsNew; //new SmbiosEps

//for patching

UINT16                      NumberOfRecords;
UINT16                      MaxStructureSize;

typedef union {
  SMBIOS_STRUCTURE_POINTER    std;
  OSXSMBIOS_STRUCTURE_POINTER osx;
} ALL_SMBIOS_STRUCTURE_POINTER;

ALL_SMBIOS_STRUCTURE_POINTER    SmbiosTable;
ALL_SMBIOS_STRUCTURE_POINTER    newSmbiosTable;

UINT8*                      Current; //pointer to the current end of tables
EFI_SMBIOS_TABLE_HEADER*    Record;
EFI_SMBIOS_HANDLE           Handle;
EFI_SMBIOS_TYPE             Type;


UINTN       Index, Size, NewSize, MaxSize;
UINTN       stringNumber;
UINTN       TableSize;

VOID*
GetSmbiosTablesFromHob (
  VOID
)
{
  EFI_PHYSICAL_ADDRESS       *Table;
  EFI_PEI_HOB_POINTERS        GuidHob;

  GuidHob.Raw = GetFirstGuidHob (&gEfiSmbiosTableGuid);

  if (GuidHob.Raw != NULL) {
    Table = GET_GUID_HOB_DATA (GuidHob.Guid);

    if (Table != NULL) {
      return (VOID *) (UINTN) * Table;
    }
  }

  return NULL;
}

/** 
 Internal functions for flat SMBIOS 
**/

UINT16
SmbiosTableLength (
  ALL_SMBIOS_STRUCTURE_POINTER LSmbiosTable
)
{
  CHAR8  *AChar;
  UINT16  Length;

  AChar = (CHAR8 *) (LSmbiosTable.std.Raw + LSmbiosTable.std.Hdr->Length);

  while ((*AChar != 0) || (*(AChar + 1) != 0)) {
    AChar ++; //stop at 00 - first 0
  }

  Length = (UINT16) ((UINTN) AChar - (UINTN) LSmbiosTable.std.Raw + 2); //length includes 00
  return Length;
}


EFI_SMBIOS_HANDLE
LogSmbiosTable (
  ALL_SMBIOS_STRUCTURE_POINTER LSmbiosTable
)
{
  UINT16  Length;

  Length = SmbiosTableLength (LSmbiosTable);

  if (Length > MaxStructureSize) {
    MaxStructureSize = Length;
  }

  CopyMem (Current, LSmbiosTable.std.Raw, Length);
  Current += Length;
  NumberOfRecords++;
  return LSmbiosTable.std.Hdr->Handle;
}

EFI_STATUS
UpdateSmbiosString (
  ALL_SMBIOS_STRUCTURE_POINTER LSmbiosTable,
  SMBIOS_TABLE_STRING *Field,
  CHAR8 *Buffer
)
{
  CHAR8 *AString;
  CHAR8 *C1; // pointers for copy
  CHAR8 *C2;
  UINTN Length;
  UINTN ALength, BLength;
  UINT8 LIndex;

  Length = SmbiosTableLength (LSmbiosTable);
  LIndex = 1;

  if (LSmbiosTable.std.Raw == NULL || Buffer == NULL || Field == NULL) {
    return EFI_NOT_FOUND;
  }

  AString = (CHAR8*) (LSmbiosTable.std.Raw + LSmbiosTable.std.Hdr->Length); // first string

  while (LIndex != *Field) {
    if (*AString) {
      LIndex++;
    }

    while (*AString != '\0') {
      AString++;  // skip string at index
    }

    AString++; // next string

    if (*AString == '\0') {
      // this is end of the table
      if (*Field == '\0') {
        AString[1] = '\0'; // one more zero
      }

      *Field = LIndex; // index of the next string that is empty

      if (LIndex == 1) {
        AString--; // first string has no leading zero
      }

      break;
    }
  }

  // AString is at place to copy
  ALength = AsciiStrnLenS (AString, SMBIOS_STRING_MAX_LENGTH);
  BLength = AsciiStrnLenS (Buffer, SMBIOS_STRING_MAX_LENGTH);

  if (BLength > ALength) {
    // Shift right
    C1 = (CHAR8*) LSmbiosTable.std.Raw + Length; // old end
    C2 = C1  + BLength - ALength; // new end
    *C2 = '\0';

    while (C1 != AString) {
      *(--C2) = *(--C1);
    }
  } else if (BLength < ALength) {
    // Shift left
    C1 = AString + ALength; // old start
    C2 = AString + BLength; // new start

    while (C1 != ((CHAR8*) LSmbiosTable.std.Raw + Length)) {
      *C2++ = *C1++;
    }

    *C2 = '\0';
    *(--C2) = '\0'; //end of table
  }

  CopyMem (AString, Buffer, BLength);
  *(AString + BLength) = '\0'; // not sure there is 0
  Length = SmbiosTableLength (SmbiosTable);
  C1 = (CHAR8*) (LSmbiosTable.std.Raw + LSmbiosTable.std.Hdr->Length);
  return EFI_SUCCESS;
}

ALL_SMBIOS_STRUCTURE_POINTER
GetSmbiosTableFromType (
  SMBIOS_TABLE_ENTRY_POINT *LSmbios,
  UINT8 LType,
  UINTN LIndex
)
{
  ALL_SMBIOS_STRUCTURE_POINTER LSmbiosTable;
  UINTN                    SmbiosTypeIndex;

  SmbiosTypeIndex = 0;
  LSmbiosTable.std.Raw = (UINT8 *) (UINTN) LSmbios->TableAddress;

  if (LSmbiosTable.std.Raw == NULL) {
    return LSmbiosTable;
  }

  while ((SmbiosTypeIndex != LIndex) || (LSmbiosTable.std.Hdr->Type != LType)) {
    if (LSmbiosTable.std.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
      LSmbiosTable.std.Raw = NULL;
      return LSmbiosTable;
    }

    if (LSmbiosTable.std.Hdr->Type == LType) {
      SmbiosTypeIndex++;
    }

    LSmbiosTable.std.Raw = (UINT8 *) (LSmbiosTable.std.Raw + SmbiosTableLength (LSmbiosTable));
  }

  return LSmbiosTable;
}

CHAR8*
GetSmbiosString (
  ALL_SMBIOS_STRUCTURE_POINTER LSmbiosTable,
  SMBIOS_TABLE_STRING String
)
{
  CHAR8      *AString;
  UINT8      Lindex;

  Lindex = 1;
  AString = (CHAR8 *) (LSmbiosTable.std.Raw + LSmbiosTable.std.Hdr->Length); //first string

  while (Lindex != String) {
    while (*AString != 0) {
      AString++;
    }

    AString++; //skip zero ending

    if (*AString == 0) {
      return AString; //this is end of the table
    }

    Lindex++;
  }

  return AString; //return pointer to Ascii string
}

/**
 Patching Functions 
**/

VOID
PatchTableType0 (
  VOID
)
{
  // BIOS information
  //
  SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_BIOS_INFORMATION, 0);

  if (SmbiosTable.std.Raw == NULL) {
    DBG ("Smbios: Type 0 (BIOS information) not found\n");
    return;
  }

  TableSize = SmbiosTableLength (SmbiosTable);
  ZeroMem ((VOID*) newSmbiosTable.std.Type0, MAX_TABLE_SIZE);
  CopyMem ((VOID*) newSmbiosTable.std.Type0, (VOID*) SmbiosTable.std.Type0, TableSize); //can't point to union
  newSmbiosTable.std.Type0->BiosSegment = 0; //like in Mac
  newSmbiosTable.std.Type0->SystemBiosMajorRelease = 0;
  newSmbiosTable.std.Type0->SystemBiosMinorRelease = 1;
  newSmbiosTable.std.Type0->BiosCharacteristics.BiosCharacteristicsNotSupported = 0;

  if (AsciiStrnLenS (gSettings.VendorName, sizeof (gSettings.VendorName)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type0->Vendor, gSettings.VendorName);
  }

  if (AsciiStrnLenS (gSettings.RomVersion, sizeof (gSettings.RomVersion)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type0->BiosVersion, gSettings.RomVersion);
  }

  if (AsciiStrnLenS (gSettings.ReleaseDate, sizeof (gSettings.ReleaseDate)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type0->BiosReleaseDate, gSettings.ReleaseDate);
  }

  newSmbiosTable.std.Type0->Hdr.Handle = NumberOfRecords;
  Handle = LogSmbiosTable (newSmbiosTable);
}

VOID
PatchTableType1 (
  VOID
)
{
  // System Information
  //
  CHAR8   Buffer[100];
  CHAR8*  s;

  ZeroMem (Buffer, sizeof (Buffer));
  SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_SYSTEM_INFORMATION, 0);

  if (SmbiosTable.std.Raw == NULL) {
    DBG ("Smbios: Type 1 (System Information) not found\n");
    return;
  }

  gUuid = SmbiosTable.std.Type1->Uuid;

#ifdef BOOT_DEBUG
  DBG ("Smbios: gUuid = %g (rfc4112)\n", &gUuid);
#endif

  s = GetSmbiosString(SmbiosTable, SmbiosTable.std.Type1->ProductName);

  CopyMem (gSettings.OEMProduct, s, AsciiStrSize (s));

  CopyMem (gSettings.EthMacAddr, &SmbiosTable.std.Type1->Uuid.Data4[2], 6);
  gSettings.EthMacAddrLen = 6;

  // Increase table size
  Size = SmbiosTable.std.Type1->Hdr.Length; // old size
  TableSize = SmbiosTableLength (SmbiosTable); // including strings
  NewSize = sizeof (SMBIOS_TABLE_TYPE1);
  ZeroMem ((VOID*) newSmbiosTable.std.Type1, MAX_TABLE_SIZE);
  CopyMem ((VOID*) newSmbiosTable.std.Type1, (VOID*) SmbiosTable.std.Type1, Size); // copy main table
  CopyMem ((CHAR8*) newSmbiosTable.std.Type1 + NewSize, (CHAR8*) SmbiosTable.std.Type1 + Size, TableSize - Size); // copy strings
  newSmbiosTable.std.Type1->Hdr.Length = (UINT8) NewSize;
  newSmbiosTable.std.Type1->WakeUpType = SystemWakeupTypePowerSwitch;

  if (IsGuidValid (&gSystemID)) {
    newSmbiosTable.std.Type1->Uuid = gSystemID;
  } else {
    if (IsGuidValid (&gPlatformUuid)) {
      newSmbiosTable.std.Type1->Uuid = gPlatformUuid;
    } else {
      newSmbiosTable.std.Type1->Uuid = gUuid;
    }
  }

  if (AsciiStrnLenS (gSettings.ManufactureName, sizeof (gSettings.ManufactureName)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type1->Manufacturer, gSettings.ManufactureName);
  }

  if (AsciiStrnLenS (gSettings.ProductName, sizeof (gSettings.ProductName)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type1->ProductName, gSettings.ProductName);
  }

  if (AsciiStrnLenS (gSettings.VersionNr, sizeof (gSettings.VersionNr)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type1->Version, gSettings.VersionNr);
  }

  if (AsciiStrnLenS (gSettings.SerialNr, sizeof (gSettings.SerialNr)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type1->SerialNumber, gSettings.SerialNr);
  }

  if (AsciiStrnLenS (gSettings.BoardNumber, sizeof (gSettings.BoardNumber)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type1->SKUNumber, gSettings.BoardNumber);
  }

  if (AsciiStrnLenS (gSettings.FamilyName, sizeof (gSettings.FamilyName)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type1->Family, gSettings.FamilyName);
  }

  newSmbiosTable.std.Type1->Hdr.Handle = NumberOfRecords;
  Handle = LogSmbiosTable (newSmbiosTable);
  return;
}

VOID
PatchTableType2and3 (
  VOID
)
{
  CHAR8* s;
  // System Chassis Information
  //
  SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE, 0);

  if (SmbiosTable.std.Raw == NULL) {
    DBG ("Smbios: Type 3 (System Chassis Information) not found\n");
    return;
  }

  Size = SmbiosTable.std.Type3->Hdr.Length; //old size
  TableSize = SmbiosTableLength (SmbiosTable); //including strings
  NewSize = 0x15; //sizeof(SMBIOS_TABLE_TYPE3);
  ZeroMem ((VOID*) newSmbiosTable.std.Type3, MAX_TABLE_SIZE);

  if (NewSize > Size) {
    CopyMem ((VOID*) newSmbiosTable.std.Type3, (VOID*) SmbiosTable.std.Type3, Size); //copy main table
    CopyMem ((CHAR8*) newSmbiosTable.std.Type3 + NewSize, (CHAR8*) SmbiosTable.std.Type3 + Size, TableSize - Size); //copy strings
    newSmbiosTable.std.Type3->Hdr.Length = (UINT8) NewSize;
  } else {
    CopyMem ((VOID*) newSmbiosTable.std.Type3, (VOID*) SmbiosTable.std.Type3, TableSize); //copy full table
  }
#if 0
  newSmbiosTable.std.Type3->BootupState = ChassisStateSafe;
  newSmbiosTable.std.Type3->PowerSupplyState = ChassisStateSafe;
  newSmbiosTable.std.Type3->ThermalState = ChassisStateOther;
  newSmbiosTable.std.Type3->SecurityStatus = ChassisSecurityStatusNone;
  newSmbiosTable.std.Type3->NumberofPowerCords = 1;
  newSmbiosTable.std.Type3->ContainedElementCount = 0;
  newSmbiosTable.std.Type3->ContainedElementRecordLength = 0;
#endif
  if (AsciiStrnLenS (gSettings.ChassisManufacturer, sizeof (gSettings.ChassisManufacturer)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type3->Manufacturer, gSettings.ChassisManufacturer);
  }

  if (AsciiStrnLenS (gSettings.BoardNumber, sizeof (gSettings.BoardNumber)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type3->Version, gSettings.BoardNumber);
  }

  if (AsciiStrnLenS (gSettings.SerialNr, sizeof (gSettings.SerialNr)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type3->SerialNumber, gSettings.SerialNr);
  }

  if (AsciiStrnLenS (gSettings.ChassisAssetTag, sizeof (gSettings.ChassisAssetTag)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type3->AssetTag, gSettings.ChassisAssetTag);
  }

  newSmbiosTable.std.Type3->Hdr.Handle = NumberOfRecords;
  Handle = LogSmbiosTable (newSmbiosTable);

  // BaseBoard Information
  //
  SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION, 0);

  if (SmbiosTable.std.Raw == NULL) {
    DBG ("Smbios: Type 2 (BaseBoard Information) not found\n");
    return;
  }

  s = GetSmbiosString (SmbiosTable, SmbiosTable.std.Type2->ProductName);
  CopyMem (gSettings.OEMBoard, s, AsciiStrSize (s));
  s = GetSmbiosString(SmbiosTable, SmbiosTable.std.Type2->Manufacturer);
  CopyMem (gSettings.OEMVendor, s, AsciiStrSize (s));
  
  Size = SmbiosTable.std.Type2->Hdr.Length; //old size
  TableSize = SmbiosTableLength (SmbiosTable); //including strings
  NewSize = 0x0F; //sizeof(SMBIOS_TABLE_TYPE2);
  ZeroMem ((VOID*) newSmbiosTable.std.Type2, MAX_TABLE_SIZE);

  if (NewSize > Size) {
    CopyMem ((VOID*) newSmbiosTable.std.Type2, (VOID*) SmbiosTable.std.Type2, Size); //copy main table
    CopyMem ((CHAR8*) newSmbiosTable.std.Type2 + NewSize, (CHAR8*) SmbiosTable.std.Type2 + Size, TableSize - Size); //copy strings
    newSmbiosTable.std.Type2->Hdr.Length = (UINT8) NewSize;
  } else {
    CopyMem ((VOID*) newSmbiosTable.std.Type2, (VOID*) SmbiosTable.std.Type2, TableSize); //copy full table
  }

  newSmbiosTable.std.Type2->ChassisHandle = Handle;
  newSmbiosTable.std.Type2->BoardType = BaseBoardTypeMotherBoard;
  ZeroMem ((VOID*) &newSmbiosTable.std.Type2->FeatureFlag, sizeof (BASE_BOARD_FEATURE_FLAGS));
  newSmbiosTable.std.Type2->FeatureFlag.Motherboard = 1;
  newSmbiosTable.std.Type2->FeatureFlag.Replaceable = 1;

  if (AsciiStrnLenS (gSettings.BoardManufactureName, sizeof (gSettings.BoardManufactureName)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type2->Manufacturer, gSettings.BoardManufactureName);
  }

  if (AsciiStrnLenS (gSettings.BoardNumber, sizeof (gSettings.BoardNumber)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type2->ProductName, gSettings.BoardNumber);
  }

  if (AsciiStrnLenS (gSettings.BoardVersion, sizeof (gSettings.BoardVersion)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type2->Version, gSettings.BoardVersion);
  }

  if (AsciiStrnLenS (gSettings.LocationInChassis, sizeof (gSettings.LocationInChassis)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type2->LocationInChassis, gSettings.LocationInChassis);
  }

  /* MLB is higher priority */
  if (AsciiStrnLenS (gSettings.MLB, sizeof (gSettings.MLB)) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type2->SerialNumber, gSettings.MLB);
  } else {
    if (AsciiStrnLenS (gSettings.BoardSerialNumber, sizeof (gSettings.BoardSerialNumber)) > 0) {
      UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type2->SerialNumber, gSettings.BoardSerialNumber);
    }
  }

  //Slice - for the table2 one patch more needed
  //
  /** spec
  Field 0x0E - Identifies the number (0 to 255) of Contained Object Handles that follow
  Field 0x0F - A list of handles of other structures (for example, Baseboard, Processor, Port, System Slots, Memory Device) that are contained by this baseboard
  It may be good before our patching but changed after. We should at least check if all tables mentioned here are present in final structure
   I just set 0 as in iMac11
  **/
  newSmbiosTable.std.Type2->NumberOfContainedObjectHandles = 0;
  newSmbiosTable.std.Type2->Hdr.Handle = NumberOfRecords;
  Handle = LogSmbiosTable (newSmbiosTable);
  return;
}

VOID
PatchTableType4and7 (
  VOID
)
{
  UINT16              ProcChar;
  UINTN               CpuNumber;
  CHAR8               *SSocketD;
  UINT16              CoreCache;
  UINT16              HandelLayer[MAX_CACHE_COUNT];

  ProcChar = 0;
  CoreCache = 0;
  
  // Value 0xFFFF means no cache
  HandelLayer[0] = 0xFFFF;
  HandelLayer[1] = 0xFFFF;
  HandelLayer[2] = 0xFFFF;
  HandelLayer[3] = 0xFFFF;
  SSocketD = "Lx-Cache";

  // Cache Information
  //
  for (Index = 0; Index < MAX_CACHE_COUNT; Index++) {
    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_CACHE_INFORMATION, Index);

    if (SmbiosTable.std.Raw == NULL) {
      break;
    }

    TableSize = SmbiosTableLength (SmbiosTable);
    ZeroMem ((VOID*) newSmbiosTable.std.Type7, MAX_TABLE_SIZE);
    CopyMem ((VOID*) newSmbiosTable.std.Type7, (VOID*) SmbiosTable.std.Type7, TableSize);

    CoreCache = newSmbiosTable.std.Type7->CacheConfiguration & 7;

    if (newSmbiosTable.std.Type7->SocketDesignation == 0) {
      SSocketD[1] = (CHAR8) (0x31 + CoreCache);
      UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type7->SocketDesignation, SSocketD);
    }

    newSmbiosTable.std.Type7->Hdr.Handle = NumberOfRecords;
    HandelLayer[CoreCache] = newSmbiosTable.std.Type7->Hdr.Handle;
    Handle = LogSmbiosTable (newSmbiosTable);
  }
  // Processor Information
  //
  for (CpuNumber = 0; CpuNumber < gCPUStructure.Cores; CpuNumber++) {

    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION, CpuNumber);
    if (SmbiosTable.std.Raw == NULL) {
      break;
    }

    // we make SMBios v2.6 while it may be older so we have to increase size
    Size = SmbiosTable.std.Type4->Hdr.Length; //old size
    TableSize = SmbiosTableLength (SmbiosTable); //including strings
    NewSize = sizeof (SMBIOS_TABLE_TYPE4);

    ZeroMem ((VOID*) newSmbiosTable.std.Type4, MAX_TABLE_SIZE);
    CopyMem ((VOID*) newSmbiosTable.std.Type4, (VOID*) SmbiosTable.std.Type4, Size); //copy main table
    CopyMem ((CHAR8*) newSmbiosTable.std.Type4 + NewSize, (CHAR8*) SmbiosTable.std.Type4 + Size, TableSize - Size); //copy strings
    newSmbiosTable.std.Type4->Hdr.Length = (UINT8) NewSize;

    newSmbiosTable.std.Type4->MaxSpeed = (UINT16) DivU64x32 (gCPUStructure.CPUFrequency, 1000000);
    newSmbiosTable.std.Type4->CurrentSpeed = (UINT16) DivU64x32 (gCPUStructure.CPUFrequency, 1000000);
    if (gCPUStructure.Model < CPU_MODEL_NEHALEM) {
        newSmbiosTable.std.Type4->ExternalClock = (UINT16) DivU64x32 (gCPUStructure.FSBFrequency, 1000000);
    } else {
      newSmbiosTable.std.Type4->ExternalClock = 0;
    }

    newSmbiosTable.std.Type4->L1CacheHandle = HandelLayer[0];
    newSmbiosTable.std.Type4->L2CacheHandle = HandelLayer[1];
    newSmbiosTable.std.Type4->L3CacheHandle = HandelLayer[2];

    if (SmbiosTable.std.Type4->ProcessorVersion == 0) { //if no BrandString we can add
      if (AsciiStrLen (gCPUStructure.BrandString) != 0) {
        UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type4->ProcessorVersion, gCPUStructure.BrandString);
      }
    }

    if (Size <= 0x20) {
      newSmbiosTable.std.Type4->SerialNumber = 0;
      newSmbiosTable.std.Type4->AssetTag = 0;
      newSmbiosTable.std.Type4->PartNumber = 0;
    }

    if (Size <= 0x23) {
      newSmbiosTable.std.Type4->CoreCount = gCPUStructure.Cores;
      newSmbiosTable.std.Type4->ThreadCount = gCPUStructure.Threads;
      newSmbiosTable.std.Type4->EnabledCoreCount = gCPUStructure.Cores;
      newSmbiosTable.std.Type4->ProcessorCharacteristics = (UINT16) gCPUStructure.Features;
      ProcChar |= (gCPUStructure.ExtFeatures & CPUID_EXTFEATURE_EM64T) ? 0x04 : 0;
      ProcChar |= (gCPUStructure.Cores > 1) ? 0x08 : 0;
      ProcChar |= (gCPUStructure.Cores < gCPUStructure.Threads) ? 0x10 : 0;
      ProcChar |= (gCPUStructure.ExtFeatures & CPUID_EXTFEATURE_XD) ? 0x20 : 0;
      ProcChar |= (gCPUStructure.Features & CPUID_FEATURE_VMX) ? 0x40 : 0;
      ProcChar |= (gCPUStructure.Features & CPUID_FEATURE_EST) ? 0x80 : 0;
      newSmbiosTable.std.Type4->ProcessorCharacteristics = ProcChar;
    }

    if (Size <= 0x28) {
      newSmbiosTable.std.Type4->ProcessorFamily2 = newSmbiosTable.std.Type4->ProcessorFamily;
    }

    newSmbiosTable.std.Type4->Hdr.Handle = NumberOfRecords;
    Handle = LogSmbiosTable (newSmbiosTable);
  }
  return;
}

VOID
PatchTableTypeSome (
  VOID
)
{
  //some unused but interesting tables. Just log as is
  //
  UINT8 tableTypes[12] = {6, 8, 9, 10, 11, 18, 21, 22, 27, 28, 32, 33};
  UINTN IndexType;

  // Different types
  for (IndexType = 0; IndexType < 12; IndexType++) {
    for (Index = 0; Index < 16; Index++) {
      SmbiosTable = GetSmbiosTableFromType (EntryPoint, tableTypes[IndexType], Index);

      if (SmbiosTable.std.Raw == NULL) {
        continue;
      }
      
      switch (tableTypes[IndexType]) {
        case 6:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type6, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type6, (VOID*) SmbiosTable.std.Type6, TableSize);
          newSmbiosTable.std.Type6->Hdr.Handle = NumberOfRecords;
          break;
        case 8:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type8, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type8, (VOID*) SmbiosTable.std.Type8, TableSize);
          newSmbiosTable.std.Type8->Hdr.Handle = NumberOfRecords;
          break;
        case 9:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type9, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type9, (VOID*) SmbiosTable.std.Type9, TableSize);
          newSmbiosTable.std.Type9->Hdr.Handle = NumberOfRecords;
          break;
        case 10:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type10, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type10, (VOID*) SmbiosTable.std.Type10, TableSize);
          newSmbiosTable.std.Type10->Hdr.Handle = NumberOfRecords;
          break;
        case 11:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type11, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type11, (VOID*) SmbiosTable.std.Type11, TableSize);
          newSmbiosTable.std.Type11->Hdr.Handle = NumberOfRecords;
          break;
        case 18:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type18, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type18, (VOID*) SmbiosTable.std.Type18, TableSize);
          newSmbiosTable.std.Type18->Hdr.Handle = NumberOfRecords;
          break;
        case 21:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type21, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type21, (VOID*) SmbiosTable.std.Type21, TableSize);
          newSmbiosTable.std.Type21->Hdr.Handle = NumberOfRecords;
          break;
        case 22:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type22, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type22, (VOID*) SmbiosTable.std.Type22, TableSize);
          newSmbiosTable.std.Type22->Hdr.Handle = NumberOfRecords;
          break;
        case 27:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type27, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type27, (VOID*) SmbiosTable.std.Type27, TableSize);
          newSmbiosTable.std.Type27->Hdr.Handle = NumberOfRecords;
          break;
        case 28:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type28, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type28, (VOID*) SmbiosTable.std.Type28, TableSize);
          newSmbiosTable.std.Type28->Hdr.Handle = NumberOfRecords;
          break;
        case 32:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type32, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type32, (VOID*) SmbiosTable.std.Type32, TableSize);
          newSmbiosTable.std.Type32->Hdr.Handle = NumberOfRecords;
          break;
        case 33:
          TableSize = SmbiosTableLength (SmbiosTable);
          ZeroMem ((VOID*) newSmbiosTable.std.Type33, MAX_TABLE_SIZE);
          CopyMem ((VOID*) newSmbiosTable.std.Type33, (VOID*) SmbiosTable.std.Type33, TableSize);
          newSmbiosTable.std.Type33->Hdr.Handle = NumberOfRecords;
          break;
      }
      LogSmbiosTable (newSmbiosTable);
    }
  }

  return;
}

VOID
PatchMemoryTables (
  VOID
)
{
  //UINT16  map;
  UINT16  Handle16;
  UINT16  Handle17[MAX_RAM_SLOTS];
  UINT16  Memory17[MAX_RAM_SLOTS];
  UINT16  Handle19;
  UINT32  TotalSystemMemory;
  UINT32  TotalEnd;
  UINT8   PartWidth;
  UINTN   j;
  RAM_SLOT_INFO *slotPtr;

  TotalEnd  = 0;
  PartWidth = 1;
  Handle16  = 0xFFFE;
  ZeroMem (Memory17, sizeof (Memory17));
  gRAM      = AllocateZeroPool (sizeof (MEM_STRUCTURE));

  // Physical Memory Array
  //
  SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, 0);

  if (SmbiosTable.std.Raw == NULL) {
    DBG ("Smbios: Type 16 (Physical Memory Array) not found\n");
    return;
  }

  gRAM->MaxMemorySlots = (UINT8) SmbiosTable.std.Type16->NumberOfMemoryDevices;

  if (gRAM->MaxMemorySlots == 0) {
    gRAM->MaxMemorySlots = MAX_RAM_SLOTS;
  }

  TableSize = SmbiosTableLength (SmbiosTable);
  ZeroMem ((VOID*) newSmbiosTable.std.Type16, MAX_TABLE_SIZE);
  CopyMem ((VOID*) newSmbiosTable.std.Type16, (VOID*) SmbiosTable.std.Type16, TableSize);

  newSmbiosTable.std.Type16->Hdr.Handle = NumberOfRecords;
  Handle16 = LogSmbiosTable (newSmbiosTable);

  // Memory Device
  //
  gRAM->MemoryModules = 0;
  TotalSystemMemory = 0;

  for (Index = 0; Index < gRAM->MaxMemorySlots; Index++) {

    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_MEMORY_DEVICE, Index);
    if (SmbiosTable.std.Raw == NULL) {
      continue;
    }
    if (SmbiosTable.std.Type17->Size > 0) {
      gRAM->MemoryModules++;
    }
  }

  gRAM->SpdDetected = FALSE;
  if (gSettings.SPDScan) {
    DBG ("Smbios: Starting ScanSPD\n");
    ScanSPD ();
  } else {
    DBG ("Smbios: SPD scanning is disabled by the user (config.plist)\n");
  }

  for (Index = 0; Index < gRAM->MaxMemorySlots; Index++) {

    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_MEMORY_DEVICE, Index);
    if (SmbiosTable.std.Raw == NULL) {
      continue;
    }

    TableSize = SmbiosTableLength (SmbiosTable);
    ZeroMem ((VOID*) newSmbiosTable.std.Type17, MAX_TABLE_SIZE);
    CopyMem ((VOID*) newSmbiosTable.std.Type17, (VOID*) SmbiosTable.std.Type17, TableSize);
    newSmbiosTable.std.Type17->MemoryArrayHandle = Handle16;

    if (gRAM->SpdDetected) {
      slotPtr = &gRAM->DIMM[Index];
      if (slotPtr->InUse) {

        DBG ("Smbios: SPD detected and slot %d used\n", Index);

        if ((slotPtr->Type != MemoryTypeUnknown) &&
            (slotPtr->Type != MemoryTypeOther)  &&
            (slotPtr->Type != 0)) {
          newSmbiosTable.std.Type17->MemoryType = slotPtr->Type;
        }

        if (AsciiStrnLenS (slotPtr->Vendor, sizeof (slotPtr->Vendor)) > 0) {
          UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type17->Manufacturer, slotPtr->Vendor);
        }

        if (AsciiStrnLenS (slotPtr->SerialNo, sizeof (slotPtr->SerialNo)) > 0) {
          UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type17->SerialNumber, slotPtr->SerialNo);
        }

        if (AsciiStrnLenS (slotPtr->PartNo, sizeof (slotPtr->PartNo)) > 0) {
          UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type17->PartNumber, slotPtr->PartNo);
        }

        if (slotPtr->Frequency > 0) {
          newSmbiosTable.std.Type17->Speed = (UINT16) slotPtr->Frequency;
        }

        if (slotPtr->ModuleSize > 0) {
          newSmbiosTable.std.Type17->Size = (UINT16) slotPtr->ModuleSize;
        }
      } else {
        newSmbiosTable.std.Type17->Speed = 0;
        newSmbiosTable.std.Type17->Size  = 0;
        newSmbiosTable.std.Type17->MemoryType  = MemoryTypeUnknown;
      }

      if ((newSmbiosTable.std.Type17->Size & 0x8000) == 0) {
        TotalSystemMemory += newSmbiosTable.std.Type17->Size; //Mb
        Memory17[Index] = (UINT16)(newSmbiosTable.std.Type17->Size > 0 ? TotalSystemMemory : 0);
        DBG ("Smbios: Memory17[%d] = %d\n", Index, Memory17[Index]);
      }

      newSmbiosTable.std.Type17->Hdr.Handle = NumberOfRecords;
      Handle17[Index] = LogSmbiosTable (newSmbiosTable);

      if ((gRAM->DIMM[Index].InUse) &&
          (gRAM->DIMM[Index].spd[0] != 0) &&
          (gRAM->DIMM[Index].spd[0] != 0xFF)) {
        ZeroMem ((VOID*) newSmbiosTable.osx.Type130, MAX_TABLE_SIZE);
        newSmbiosTable.osx.Type130->Hdr.Type = 130;
        newSmbiosTable.osx.Type130->Hdr.Length = (UINT8) (10 + (gRAM->DIMM[Index].SpdSize));
        newSmbiosTable.osx.Type130->Type17Handle = Handle17[Index];
        newSmbiosTable.osx.Type130->Offset = 0;
        newSmbiosTable.osx.Type130->Size = (gRAM->DIMM[Index].SpdSize);
        CopyMem ((UINT8 *) newSmbiosTable.osx.Type130->Data, gRAM->DIMM[Index].spd, gRAM->DIMM[Index].SpdSize);

        newSmbiosTable.osx.Type130->Hdr.Handle = NumberOfRecords;
        Handle = LogSmbiosTable (newSmbiosTable);
        
        DBG ("Smbios: Type130 Length = 0x%x, Handle = 0x%x, Size = 0x%x\n",
             newSmbiosTable.osx.Type130->Hdr.Length,
             Handle,
             newSmbiosTable.osx.Type130->Size
             );
      }
    } else {
      if ((newSmbiosTable.std.Type17->Size > 0) &&
          ((newSmbiosTable.std.Type17->MemoryType == 0) ||
           (newSmbiosTable.std.Type17->MemoryType == MemoryTypeOther) ||
           (newSmbiosTable.std.Type17->MemoryType == MemoryTypeUnknown))) {
        newSmbiosTable.std.Type17->MemoryType = MemoryTypeDdr;
      }
      
      if (AsciiStrnLenS (GetSmbiosString (newSmbiosTable, newSmbiosTable.std.Type17->Manufacturer), 64) == 0){
        UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type17->Manufacturer, "unknown");
      }
      if (AsciiStrnLenS (GetSmbiosString (newSmbiosTable, newSmbiosTable.std.Type17->SerialNumber), 64) == 0){
        UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type17->SerialNumber, "unknown");
      }
      if (AsciiStrnLenS (GetSmbiosString (newSmbiosTable, newSmbiosTable.std.Type17->PartNumber), 64) == 0){
        UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type17->PartNumber, "unknown");
      }

      if (gSettings.cMemDevice[Index].InUse) {
        if (gSettings.cMemDevice[Index].MemoryType != 0x02) {
          newSmbiosTable.std.Type17->MemoryType =  gSettings.cMemDevice[Index].MemoryType;
        }
        if (gSettings.cMemDevice[Index].Speed != 0x00) {
          newSmbiosTable.std.Type17->Speed = gSettings.cMemDevice[Index].Speed;
        }
        if (gSettings.cMemDevice[Index].Size != 0xffff) {
          newSmbiosTable.std.Type17->Size = gSettings.cMemDevice[Index].Size;
        }

        if (gSettings.cMemDevice[Index].DeviceLocator != NULL) {
          UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type17->DeviceLocator,
                              gSettings.cMemDevice[Index].DeviceLocator);
        }
        if (gSettings.cMemDevice[Index].BankLocator != NULL) {
          UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type17->BankLocator,
                              gSettings.cMemDevice[Index].BankLocator);
        }
        if (gSettings.cMemDevice[Index].Manufacturer != NULL) {
          UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type17->Manufacturer,
                              gSettings.cMemDevice[Index].Manufacturer);
        }
        if (gSettings.cMemDevice[Index].SerialNumber != NULL) {
          UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type17->SerialNumber,
                              gSettings.cMemDevice[Index].SerialNumber);
        }
        if (gSettings.cMemDevice[Index].PartNumber != NULL) {
          UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.std.Type17->PartNumber,
                              gSettings.cMemDevice[Index].PartNumber);
        }
      }

      if ((newSmbiosTable.std.Type17->Size & 0x8000) == 0) {
        TotalSystemMemory += newSmbiosTable.std.Type17->Size; //Mb
        Memory17[Index] = (UINT16)(newSmbiosTable.std.Type17->Size > 0 ? TotalSystemMemory : 0);
        DBG ("Smbios: Memory17[%d] = %d\n", Index, Memory17[Index]);
      }

      newSmbiosTable.std.Type17->Hdr.Handle = NumberOfRecords;
      Handle17[Index] = LogSmbiosTable (newSmbiosTable);
    }
  }
  //
  // Generate Memory Array Mapped Address info (TYPE 19)
  //
  for (Index = 0; Index < (UINTN) (gRAM->MaxMemorySlots * 2); Index++) {
    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, Index);

    if (SmbiosTable.std.Raw == NULL) {
      break;
    }

    if (SmbiosTable.std.Type19->EndingAddress > TotalEnd) {
      TotalEnd = SmbiosTable.std.Type19->EndingAddress;
    }

    PartWidth = SmbiosTable.std.Type19->PartitionWidth;
  }

  if (TotalEnd == 0) {
    TotalEnd =  (TotalSystemMemory << 10) - 1;
  }

  ZeroMem ((VOID*) newSmbiosTable.std.Type19, MAX_TABLE_SIZE);
  newSmbiosTable.std.Type19->Hdr.Type = EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS;
  newSmbiosTable.std.Type19->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE19);
  newSmbiosTable.std.Type19->MemoryArrayHandle = Handle16;
  newSmbiosTable.std.Type19->StartingAddress = 0;
  newSmbiosTable.std.Type19->EndingAddress = TotalEnd;
  newSmbiosTable.std.Type19->PartitionWidth = PartWidth;

  newSmbiosTable.std.Type19->Hdr.Handle = NumberOfRecords;
  Handle19 = LogSmbiosTable (newSmbiosTable);

  //
  // Generate Memory Array Mapped Address info (TYPE 20)
  //
  for (Index = 0; Index < (UINTN) (gRAM->MaxMemorySlots * 2); Index++) {
    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS, Index);

    if (SmbiosTable.std.Raw == NULL) {
      return;
    }
    
    if (SmbiosTable.std.Type20->EndingAddress < 2) {
      continue;
    }

    TableSize = SmbiosTableLength (SmbiosTable);
    ZeroMem ((VOID*) newSmbiosTable.std.Type20, MAX_TABLE_SIZE);
    CopyMem ((VOID*) newSmbiosTable.std.Type20, (VOID*) SmbiosTable.std.Type20, TableSize);

    for (j = 0; j < gRAM->MaxMemorySlots; j++) {
      if ((((UINT32) Memory17[j]  << 10) - 1) <= newSmbiosTable.std.Type20->EndingAddress) {
        newSmbiosTable.std.Type20->MemoryDeviceHandle = Handle17[j];
        Memory17[j] = 0;
        break;
      }
    }
    newSmbiosTable.std.Type20->MemoryArrayMappedAddressHandle = Handle19;

    newSmbiosTable.std.Type20->Hdr.Handle = NumberOfRecords;
    LogSmbiosTable (newSmbiosTable);
  }

  return;
}

/** 
 Apple Specific Structures
**/

VOID
PatchTableType128 (
  VOID
)
{
    ZeroMem ((VOID*) newSmbiosTable.osx.Type128, MAX_TABLE_SIZE);
  
    newSmbiosTable.osx.Type128->Hdr.Type = 128;
    newSmbiosTable.osx.Type128->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE128);
    newSmbiosTable.osx.Type128->FirmwareFeatures = 0xc0007417; //imac112 -> 0x1403
    newSmbiosTable.osx.Type128->FirmwareFeaturesMask = 0xc0007fff; // 0xffff

    //    FW_REGION_RESERVED   = 0,
    //    FW_REGION_RECOVERY   = 1,
    //    FW_REGION_MAIN       = 2, gHob->MemoryAbove1MB.PhysicalStart + ResourceLength
    //                           or fix as 0x200000 - 0x600000
    //    FW_REGION_NVRAM      = 3,
    //    FW_REGION_CONFIG     = 4,
    //    FW_REGION_DIAGVAULT  = 5,

    newSmbiosTable.osx.Type128->RegionCount = 2;
    newSmbiosTable.osx.Type128->RegionType[0] = FW_REGION_MAIN;
    newSmbiosTable.osx.Type128->FlashMap[0].StartAddress = 0xFFE00000; //0xF0000;
    newSmbiosTable.osx.Type128->FlashMap[0].EndAddress = 0xFFF1FFFF;
    newSmbiosTable.osx.Type128->RegionType[1] = FW_REGION_NVRAM; //Efivar
    newSmbiosTable.osx.Type128->FlashMap[1].StartAddress = 0x15000; //0xF0000;
    newSmbiosTable.osx.Type128->FlashMap[1].EndAddress = 0x1FFFF;
    //region type=1 also present in mac

    newSmbiosTable.osx.Type128->Hdr.Handle = NumberOfRecords;
    LogSmbiosTable (newSmbiosTable);
    return;
}

VOID
PatchTableType131 (
  VOID
)
{
  newSmbiosTable.osx.Type131->Hdr.Type = 131;
  newSmbiosTable.osx.Type131->Hdr.Length = sizeof (SMBIOS_STRUCTURE) + 2;
  newSmbiosTable.osx.Type131->ProcessorType = gSettings.CpuType;

  newSmbiosTable.osx.Type131->Hdr.Handle = NumberOfRecords;
  Handle = LogSmbiosTable (newSmbiosTable);
  return;
}

VOID
PatchTableType132 (
  VOID
)
{
  SmbiosTable = GetSmbiosTableFromType (EntryPoint, 132, 0);

  if (SmbiosTable.std.Raw == NULL) {
      ZeroMem ((VOID*) newSmbiosTable.osx.Type132, MAX_TABLE_SIZE);
      newSmbiosTable.osx.Type132->Hdr.Type = 132;
      newSmbiosTable.osx.Type132->Hdr.Length = sizeof (SMBIOS_STRUCTURE) + 2;

      if (gCPUStructure.ProcessorInterconnectSpeed != 0) {
        newSmbiosTable.osx.Type132->ProcessorBusSpeed = (UINT16) gCPUStructure.ProcessorInterconnectSpeed;
      }

      if (gSettings.ProcessorInterconnectSpeed != 0) {
        newSmbiosTable.osx.Type132->ProcessorBusSpeed = (UINT16) gSettings.ProcessorInterconnectSpeed;
      }
    
      newSmbiosTable.osx.Type132->Hdr.Handle = NumberOfRecords;
      Handle = LogSmbiosTable (newSmbiosTable);
  } else {
    newSmbiosTable.osx.Type132->Hdr.Handle = NumberOfRecords;
    LogSmbiosTable (SmbiosTable);
  }

  return;
}

VOID
PatchSmbios (
  VOID
)
{
  SMBIOS_STRUCTURE      *StructurePtr;
  EFI_PEI_HOB_POINTERS  GuidHob;
  EFI_PEI_HOB_POINTERS  HobStart;
  EFI_PHYSICAL_ADDRESS  *Table;
  UINTN                 TableLength;
  EFI_STATUS            Status;
  UINTN                 BufferLen;
  EFI_PHYSICAL_ADDRESS  BufferPtr;

  Status = EFI_SUCCESS;
  newSmbiosTable.std.Raw = (UINT8*) AllocateZeroPool (MAX_TABLE_SIZE);

  Smbios = GetSmbiosTablesFromHob ();

  if (!Smbios) {
    DBG ("Smbios: System Table not found, exiting\n");
    return;
  }
  //
  // original EPS and tables
  EntryPoint = (SMBIOS_TABLE_ENTRY_POINT*) Smbios; //yes, it is old SmbiosEPS
  //
  // how many we need to add for tables 128, 130, 131, 132 and for strings?
  BufferLen = 0x20 + EntryPoint->TableLength + 64 * 10;
  //
  // new place for EPS and tables. Allocated once for both
  BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
  Status = gBS->AllocatePages (
                               AllocateMaxAddress,
                               EfiACPIMemoryNVS,
                               EFI_SIZE_TO_PAGES (BufferLen),
                               &BufferPtr
                               );

  if (EFI_ERROR (Status)) {
    DBG ("Smbios: error allocating pages in EfiACPIMemoryNVS.\n");
    Status = gBS->AllocatePages (
                                 AllocateMaxAddress,
                                 EfiACPIReclaimMemory,
                                 ROUND_PAGE (BufferLen) / EFI_PAGE_SIZE,
                                 &BufferPtr
                                 );

    if (EFI_ERROR (Status)) {
      DBG ("Smbios: error allocating pages in EfiACPIReclaimMemory\n");
    }
  }

  if (BufferPtr) {
    SmbiosEpsNew = (SMBIOS_TABLE_ENTRY_POINT *) (UINTN) BufferPtr; //this is new EPS
  } else {
    SmbiosEpsNew = EntryPoint; //is it possible?!
  }

  ZeroMem (SmbiosEpsNew, BufferLen);
  NumberOfRecords = 0;
  MaxStructureSize = 0;
  //
  //preliminary fill EntryPoint with some data
  CopyMem ((VOID*) SmbiosEpsNew, (VOID *) EntryPoint, sizeof (SMBIOS_TABLE_ENTRY_POINT));

  Smbios = (VOID*) (SmbiosEpsNew + 1);
  Current = (UINT8*) Smbios;            //begin fill tables from here
  SmbiosEpsNew->TableAddress = (UINT32) (UINTN) Current;
  SmbiosEpsNew->EntryPointLength = sizeof (SMBIOS_TABLE_ENTRY_POINT); // no matter on other versions
  SmbiosEpsNew->MajorVersion = 2;
  SmbiosEpsNew->MinorVersion = 6;
  SmbiosEpsNew->SmbiosBcdRevision = 0x26; //Slice - we want to have v2.6

  //
  //Slice - order of patching is significant
  PatchTableType0();
  PatchTableType1();
  PatchTableType2and3();
  PatchTableType4and7();
  PatchMemoryTables();
  PatchTableTypeSome();
  PatchTableType128();
  PatchTableType131();

  if ((gCPUStructure.Model == CPU_MODEL_NEHALEM) ||
      (gCPUStructure.Model == CPU_MODEL_NEHALEM_EX) ||
      (gSettings.ProcessorInterconnectSpeed != 0)) {
    PatchTableType132();
  }

  StructurePtr = (SMBIOS_STRUCTURE*) Current;
  StructurePtr->Type    = SMBIOS_TYPE_END_OF_TABLE;
  StructurePtr->Length  = sizeof (SMBIOS_STRUCTURE);
  StructurePtr->Handle  = SMBIOS_TYPE_INACTIVE; // spec 2.7 p.120
  
  Current += sizeof (SMBIOS_STRUCTURE);
  *Current++ = 0;
  *Current++ = 0; //double 0 at the end
  NumberOfRecords++;

  if (MaxStructureSize > MAX_TABLE_SIZE) {
    DBG ("Smbios: too long SMBIOS\n");
  }
  
  // there is no need to keep all tables in numeric order. It is not needed
  // neither by specs nor by AppleSmbios.kext
  FreePool ((VOID*) newSmbiosTable.std.Raw);
  //
  // Get Hob List
  HobStart.Raw = GetHobList ();
  //
  // Iteratively add Smbios Table to EFI System Table
  for (Index = 0; Index < sizeof (gTableGuidArray) / sizeof (*gTableGuidArray); ++Index) {
    GuidHob.Raw = GetNextGuidHob (gTableGuidArray[Index], HobStart.Raw);

    if (GuidHob.Raw != NULL) {
      Table = GET_GUID_HOB_DATA (GuidHob.Guid);
      TableLength = GET_GUID_HOB_DATA_SIZE (GuidHob);

      if (Table != NULL) {
        SmbiosEpsNew->TableLength = (UINT16) ((UINT32) (UINTN) Current - (UINT32) (UINTN) Smbios);
        SmbiosEpsNew->NumberOfSmbiosStructures = NumberOfRecords;
        SmbiosEpsNew->MaxStructureSize = MaxStructureSize;
        SmbiosEpsNew->IntermediateChecksum = 0;
        SmbiosEpsNew->IntermediateChecksum = (UINT8) (256 - CalculateSum8 ((UINT8*) SmbiosEpsNew + 0x10, SmbiosEpsNew->EntryPointLength - 0x10));
        SmbiosEpsNew->EntryPointStructureChecksum = 0;
        SmbiosEpsNew->EntryPointStructureChecksum = (UINT8) (256 - CalculateSum8 ((UINT8*) SmbiosEpsNew, SmbiosEpsNew->EntryPointLength));
        gBS->InstallConfigurationTable (&gEfiSmbiosTableGuid, (VOID*) SmbiosEpsNew);
        *Table = (UINT32) (UINTN) SmbiosEpsNew;
        gST->Hdr.CRC32 = 0;
        gBS->CalculateCrc32 ((UINT8 *) &gST->Hdr, gST->Hdr.HeaderSize, &gST->Hdr.CRC32);
      }
    }
  }
  return;
}
