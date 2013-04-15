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

#include "macosx.h"

#define MAX_HANDLE        0xFEFF
#define SMBIOS_PTR        SIGNATURE_32('_','S','M','_')
#define MAX_TABLE_SIZE    512

static EFI_GUID             *gTableGuidArray[] = {&gEfiSmbiosTableGuid};

VOID                        *Smbios;  //pointer to SMBIOS data
SMBIOS_TABLE_ENTRY_POINT    *EntryPoint; //SmbiosEps original
SMBIOS_TABLE_ENTRY_POINT    *SmbiosEpsNew; //new SmbiosEps

//for patching
SMBIOS_STRUCTURE_POINTER    SmbiosTable;
SMBIOS_STRUCTURE_POINTER    newSmbiosTable;
UINT16                      NumberOfRecords;
UINT16                      MaxStructureSize;

UINT8*                      Current; //pointer to the current end of tables
EFI_SMBIOS_TABLE_HEADER*    Record;
EFI_SMBIOS_HANDLE           Handle;
EFI_SMBIOS_TYPE             Type;


UINTN       Index, Size, NewSize, MaxSize;
UINTN       stringNumber;
UINTN       TableSize;

UINTN
iStrLen (
  CHAR8* String,
  UINTN MaxLen
)
{
  UINTN Len;
  CHAR8*  BA;

  Len = 0;

  if (MaxLen > 0) {
    for (Len = 0; Len < MaxLen; Len++) {
      if (String[Len] == 0) {
        break;
      }
    }

    BA = &String[Len - 1];

    while ((Len != 0) && ((*BA == ' ') || (*BA == 0))) {
      BA--;
      Len--;
    }
  } else {
    BA = String;

    while (*BA) {
      BA++;
      Len++;
    }
  }

  return Len;
}

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
  SMBIOS_STRUCTURE_POINTER LSmbiosTable
)
{
  CHAR8  *AChar;
  UINT16  Length;

  AChar = (CHAR8 *) (LSmbiosTable.Raw + LSmbiosTable.Hdr->Length);

  while ((*AChar != 0) || (*(AChar + 1) != 0)) {
    AChar ++; //stop at 00 - first 0
  }

  Length = (UINT16) ((UINTN) AChar - (UINTN) LSmbiosTable.Raw + 2); //length includes 00
  return Length;
}


EFI_SMBIOS_HANDLE
LogSmbiosTable (
  SMBIOS_STRUCTURE_POINTER LSmbiosTable
)
{
  UINT16  Length;

  Length = SmbiosTableLength (LSmbiosTable);

  if (Length > MaxStructureSize) {
    MaxStructureSize = Length;
  }

  CopyMem (Current, LSmbiosTable.Raw, Length);
  Current += Length;
  NumberOfRecords++;
  return LSmbiosTable.Hdr->Handle;
}

EFI_STATUS
UpdateSmbiosString (
  SMBIOS_STRUCTURE_POINTER LSmbiosTable,
  SMBIOS_TABLE_STRING *Field,
  CHAR8 *Buffer
)
{
  CHAR8 *AString;
  CHAR8 *C1; //pointers for copy
  CHAR8 *C2;
  UINTN Length;
  UINTN ALength, BLength;
  UINT8 LIndex;

  Length = SmbiosTableLength (LSmbiosTable);
  LIndex = 1;

  if (LSmbiosTable.Raw == NULL || Buffer == NULL || Field == NULL) {
    return EFI_NOT_FOUND;
  }

  AString = (CHAR8*) (LSmbiosTable.Raw + LSmbiosTable.Hdr->Length); //first string

  while (LIndex != *Field) {
    if (*AString) {
      LIndex++;
    }

    while (*AString != 0) {
      AString++;  //skip string at index
    }

    AString++; //next string

    if (*AString == 0) {
      //this is end of the table
      if (*Field == 0) {
        AString[1] = 0; //one more zero
      }

      *Field = LIndex; //index of the next string that  is empty

      if (LIndex == 1) {
        AString--; //first string has no leading zero
      }

      break;
    }
  }

  // AString is at place to copy
  ALength = iStrLen (AString, 0);
  BLength = iStrLen (Buffer, SMBIOS_STRING_MAX_LENGTH);

  if (BLength > ALength) {
    //Shift right
    C1 = (CHAR8*) LSmbiosTable.Raw + Length; //old end
    C2 = C1  + BLength - ALength; //new end
    *C2 = 0;

    while (C1 != AString) {
      *(--C2) = *(--C1);
    }
  } else if (BLength < ALength) {
    //Shift left
    C1 = AString + ALength; //old start
    C2 = AString + BLength; //new start

    while (C1 != ((CHAR8*) LSmbiosTable.Raw + Length)) {
      *C2++ = *C1++;
    }

    *C2 = 0;
    *(--C2) = 0; //end of table
  }

  CopyMem (AString, Buffer, BLength);
  *(AString + BLength) = 0; // not sure there is 0
  Length = SmbiosTableLength (SmbiosTable);
  C1 = (CHAR8*) (LSmbiosTable.Raw + LSmbiosTable.Hdr->Length);
  return EFI_SUCCESS;
}

SMBIOS_STRUCTURE_POINTER
GetSmbiosTableFromType (
  SMBIOS_TABLE_ENTRY_POINT *LSmbios,
  UINT8 LType,
  UINTN LIndex
)
{
  SMBIOS_STRUCTURE_POINTER LSmbiosTable;
  UINTN                    SmbiosTypeIndex;

  SmbiosTypeIndex = 0;
  LSmbiosTable.Raw = (UINT8 *) (UINTN) LSmbios->TableAddress;

  if (LSmbiosTable.Raw == NULL) {
    return LSmbiosTable;
  }

  while ((SmbiosTypeIndex != LIndex) || (LSmbiosTable.Hdr->Type != LType)) {
    if (LSmbiosTable.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
      LSmbiosTable.Raw = NULL;
      return LSmbiosTable;
    }

    if (LSmbiosTable.Hdr->Type == LType) {
      SmbiosTypeIndex++;
    }

    LSmbiosTable.Raw = (UINT8 *) (LSmbiosTable.Raw + SmbiosTableLength (LSmbiosTable));
  }

  return LSmbiosTable;
}

CHAR8*
GetSmbiosString (
  SMBIOS_STRUCTURE_POINTER LSmbiosTable,
  SMBIOS_TABLE_STRING String
)
{
  CHAR8      *AString;
  UINT8      Lindex;

  Lindex = 1;
  AString = (CHAR8 *) (LSmbiosTable.Raw + LSmbiosTable.Hdr->Length); //first string

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

  if (SmbiosTable.Raw == NULL) {
    DBG ("Smbios: Type 0 (BIOS information) not found\n");
    return;
  }

  TableSize = SmbiosTableLength (SmbiosTable);
  ZeroMem ((VOID*) newSmbiosTable.Type0, MAX_TABLE_SIZE);
  CopyMem ((VOID*) newSmbiosTable.Type0, (VOID*) SmbiosTable.Type0, TableSize); //can't point to union
  newSmbiosTable.Type0->BiosSegment = 0; //like in Mac
  newSmbiosTable.Type0->SystemBiosMajorRelease = 0;
  newSmbiosTable.Type0->SystemBiosMinorRelease = 1;
  newSmbiosTable.Type0->BiosCharacteristics.BiosCharacteristicsNotSupported = 0;

  if (iStrLen (gSettings.VendorName, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type0->Vendor, gSettings.VendorName);
  }

  if (iStrLen (gSettings.RomVersion, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type0->BiosVersion, gSettings.RomVersion);
  }

  if (iStrLen (gSettings.ReleaseDate, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type0->BiosReleaseDate, gSettings.ReleaseDate);
  }

  newSmbiosTable.Type0->Hdr.Handle = NumberOfRecords;
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

  ZeroMem (Buffer, sizeof (Buffer));
  SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_SYSTEM_INFORMATION, 0);

  if (SmbiosTable.Raw == NULL) {
    DBG ("Smbios: Type 1 (System Information) not found\n");
    return;
  }

  gUuid = SmbiosTable.Type1->Uuid;

  AsciiSPrint (Buffer, 100, "%02x%02x%02x%02x%02x%02x",
    SmbiosTable.Type1->Uuid.Data4[2],
    SmbiosTable.Type1->Uuid.Data4[3],
    SmbiosTable.Type1->Uuid.Data4[4],
    SmbiosTable.Type1->Uuid.Data4[5],
    SmbiosTable.Type1->Uuid.Data4[6],
    SmbiosTable.Type1->Uuid.Data4[7]
  );

  gSettings.EthMacAddr = AllocateZeroPool ((AsciiStrLen(Buffer) >> 1));
  gSettings.MacAddrLen = hex2bin (Buffer, gSettings.EthMacAddr, (INT32)(AsciiStrLen(Buffer) >> 1));

#ifdef BOOT_DEBUG
  ZeroMem (Buffer, sizeof (Buffer));
  AsciiSPrint (Buffer, 100, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
    SmbiosTable.Type1->Uuid.Data1,
    SmbiosTable.Type1->Uuid.Data2,
    SmbiosTable.Type1->Uuid.Data3,
    SmbiosTable.Type1->Uuid.Data4[0],
    SmbiosTable.Type1->Uuid.Data4[1],
    SmbiosTable.Type1->Uuid.Data4[2],
    SmbiosTable.Type1->Uuid.Data4[3],
    SmbiosTable.Type1->Uuid.Data4[4],
    SmbiosTable.Type1->Uuid.Data4[5],
    SmbiosTable.Type1->Uuid.Data4[6],
    SmbiosTable.Type1->Uuid.Data4[7]
  );
  DBG ("Smbios: gUuid = %a\n", &Buffer);
#endif

  //Increase table size
  Size = SmbiosTable.Type1->Hdr.Length; //old size
  TableSize = SmbiosTableLength (SmbiosTable); //including strings
  NewSize = sizeof (SMBIOS_TABLE_TYPE1);
  ZeroMem ((VOID*) newSmbiosTable.Type1, MAX_TABLE_SIZE);
  CopyMem ((VOID*) newSmbiosTable.Type1, (VOID*) SmbiosTable.Type1, Size); //copy main table
  CopyMem ((CHAR8*) newSmbiosTable.Type1 + NewSize, (CHAR8*) SmbiosTable.Type1 + Size, TableSize - Size); //copy strings
  newSmbiosTable.Type1->Hdr.Length = (UINT8) NewSize;
  newSmbiosTable.Type1->WakeUpType = SystemWakeupTypePowerSwitch;

  if (!EFI_ERROR (SystemIDStatus)) {
    newSmbiosTable.Type1->Uuid = gSystemID;
  } else {
    if (!EFI_ERROR (PlatformUuidStatus)) {
      newSmbiosTable.Type1->Uuid = gPlatformUuid;
    } else {
      newSmbiosTable.Type1->Uuid = gUuid;
    }
  }

  if (iStrLen (gSettings.ManufactureName, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type1->Manufacturer, gSettings.ManufactureName);
  }

  if (iStrLen (gSettings.ProductName, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type1->ProductName, gSettings.ProductName);
  }

  if (iStrLen (gSettings.VersionNr, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type1->Version, gSettings.VersionNr);
  }

  if (iStrLen (gSettings.SerialNr, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type1->SerialNumber, gSettings.SerialNr);
  }

  if (iStrLen (gSettings.BoardNumber, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type1->SKUNumber, gSettings.BoardNumber);
  }

  if (iStrLen (gSettings.FamilyName, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type1->Family, gSettings.FamilyName);
  }

  newSmbiosTable.Type1->Hdr.Handle = NumberOfRecords;
  Handle = LogSmbiosTable (newSmbiosTable);
  return;
}

VOID
PatchTableType2and3 (
  VOID
)
{
  // System Chassis Information
  //
  SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE, 0);

  if (SmbiosTable.Raw == NULL) {
    DBG ("Smbios: Type 3 (System Chassis Information) not found\n");
    return;
  }

  Size = SmbiosTable.Type3->Hdr.Length; //old size
  TableSize = SmbiosTableLength (SmbiosTable); //including strings
  NewSize = 0x15; //sizeof(SMBIOS_TABLE_TYPE3);
  ZeroMem ((VOID*) newSmbiosTable.Type3, MAX_TABLE_SIZE);

  if (NewSize > Size) {
    CopyMem ((VOID*) newSmbiosTable.Type3, (VOID*) SmbiosTable.Type3, Size); //copy main table
    CopyMem ((CHAR8*) newSmbiosTable.Type3 + NewSize, (CHAR8*) SmbiosTable.Type3 + Size, TableSize - Size); //copy strings
    newSmbiosTable.Type3->Hdr.Length = (UINT8) NewSize;
  } else {
    CopyMem ((VOID*) newSmbiosTable.Type3, (VOID*) SmbiosTable.Type3, TableSize); //copy full table
  }
#if 0
  newSmbiosTable.Type3->BootupState = ChassisStateSafe;
  newSmbiosTable.Type3->PowerSupplyState = ChassisStateSafe;
  newSmbiosTable.Type3->ThermalState = ChassisStateOther;
  newSmbiosTable.Type3->SecurityStatus = ChassisSecurityStatusNone;
  newSmbiosTable.Type3->NumberofPowerCords = 1;
  newSmbiosTable.Type3->ContainedElementCount = 0;
  newSmbiosTable.Type3->ContainedElementRecordLength = 0;
#endif
  if (iStrLen (gSettings.ChassisManufacturer, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type3->Manufacturer, gSettings.ChassisManufacturer);
  }

  if (iStrLen (gSettings.BoardNumber, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type3->Version, gSettings.BoardNumber);
  }

  if (iStrLen (gSettings.SerialNr, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type3->SerialNumber, gSettings.SerialNr);
  }

  if (iStrLen (gSettings.ChassisAssetTag, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type3->AssetTag, gSettings.ChassisAssetTag);
  }

  newSmbiosTable.Type3->Hdr.Handle = NumberOfRecords;
  Handle = LogSmbiosTable (newSmbiosTable);

  // BaseBoard Information
  //
  SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION, 0);

  if (SmbiosTable.Raw == NULL) {
    DBG ("Smbios: Type 2 (BaseBoard Information) not found\n");
    return;
  }

  Size = SmbiosTable.Type2->Hdr.Length; //old size
  TableSize = SmbiosTableLength (SmbiosTable); //including strings
  NewSize = 0x0F; //sizeof(SMBIOS_TABLE_TYPE2);
  ZeroMem ((VOID*) newSmbiosTable.Type2, MAX_TABLE_SIZE);

  if (NewSize > Size) {
    CopyMem ((VOID*) newSmbiosTable.Type2, (VOID*) SmbiosTable.Type2, Size); //copy main table
    CopyMem ((CHAR8*) newSmbiosTable.Type2 + NewSize, (CHAR8*) SmbiosTable.Type2 + Size, TableSize - Size); //copy strings
    newSmbiosTable.Type2->Hdr.Length = (UINT8) NewSize;
  } else {
    CopyMem ((VOID*) newSmbiosTable.Type2, (VOID*) SmbiosTable.Type2, TableSize); //copy full table
  }

  newSmbiosTable.Type2->ChassisHandle = Handle;
  newSmbiosTable.Type2->BoardType = BaseBoardTypeMotherBoard;
  ZeroMem ((VOID*) &newSmbiosTable.Type2->FeatureFlag, sizeof (BASE_BOARD_FEATURE_FLAGS));
  newSmbiosTable.Type2->FeatureFlag.Motherboard = 1;
  newSmbiosTable.Type2->FeatureFlag.Replaceable = 1;

  if (iStrLen (gSettings.BoardManufactureName, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type2->Manufacturer, gSettings.BoardManufactureName);
  }

  if (iStrLen (gSettings.BoardNumber, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type2->ProductName, gSettings.BoardNumber);
  }

  if (iStrLen (gSettings.BoardVersion, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type2->Version, gSettings.BoardVersion);
  }

  if (iStrLen (gSettings.BoardSerialNumber, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type2->SerialNumber, gSettings.BoardSerialNumber);
  }

  if (iStrLen (gSettings.LocationInChassis, 64) > 0) {
    UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type2->LocationInChassis, gSettings.LocationInChassis);
  }

  //Slice - for the table2 one patch more needed
  //
  /** spec
  Field 0x0E - Identifies the number (0 to 255) of Contained Object Handles that follow
  Field 0x0F - A list of handles of other structures (for example, Baseboard, Processor, Port, System Slots, Memory Device) that are contained by this baseboard
  It may be good before our patching but changed after. We should at least check if all tables mentioned here are present in final structure
   I just set 0 as in iMac11
  **/
  newSmbiosTable.Type2->NumberOfContainedObjectHandles = 0;
  newSmbiosTable.Type2->Hdr.Handle = NumberOfRecords;
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

    if (SmbiosTable.Raw == NULL) {
      break;
    }

    TableSize = SmbiosTableLength (SmbiosTable);
    ZeroMem ((VOID*) newSmbiosTable.Type7, MAX_TABLE_SIZE);
    CopyMem ((VOID*) newSmbiosTable.Type7, (VOID*) SmbiosTable.Type7, TableSize);

    CoreCache = newSmbiosTable.Type7->CacheConfiguration & 7;

    if (newSmbiosTable.Type7->SocketDesignation == 0) {
      SSocketD[1] = (CHAR8) (0x31 + CoreCache);
      UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type7->SocketDesignation, SSocketD);
    }

    newSmbiosTable.Type7->Hdr.Handle = NumberOfRecords;
    HandelLayer[CoreCache] = newSmbiosTable.Type7->Hdr.Handle;
    Handle = LogSmbiosTable (newSmbiosTable);
  }
  // Processor Information
  //
  for (CpuNumber = 0; CpuNumber < gCPUStructure.Cores; CpuNumber++) {

    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION, CpuNumber);
    if (SmbiosTable.Raw == NULL) {
      break;
    }

    // we make SMBios v2.6 while it may be older so we have to increase size
    Size = SmbiosTable.Type4->Hdr.Length; //old size
    TableSize = SmbiosTableLength (SmbiosTable); //including strings
    NewSize = sizeof (SMBIOS_TABLE_TYPE4);

    ZeroMem ((VOID*) newSmbiosTable.Type4, MAX_TABLE_SIZE);
    CopyMem ((VOID*) newSmbiosTable.Type4, (VOID*) SmbiosTable.Type4, Size); //copy main table
    CopyMem ((CHAR8*) newSmbiosTable.Type4 + NewSize, (CHAR8*) SmbiosTable.Type4 + Size, TableSize - Size); //copy strings
    newSmbiosTable.Type4->Hdr.Length = (UINT8) NewSize;

    newSmbiosTable.Type4->MaxSpeed = (UINT16) DivU64x32 (gCPUStructure.CPUFrequency, 1000000);
    newSmbiosTable.Type4->CurrentSpeed = (UINT16) DivU64x32 (gCPUStructure.CPUFrequency, 1000000);
    if (gCPUStructure.Model < CPU_MODEL_NEHALEM) {
        newSmbiosTable.Type4->ExternalClock = (UINT16) DivU64x32 (gCPUStructure.FSBFrequency, 1000000);
    } else {
      newSmbiosTable.Type4->ExternalClock = 0;
    }

    newSmbiosTable.Type4->L1CacheHandle = HandelLayer[0];
    newSmbiosTable.Type4->L2CacheHandle = HandelLayer[1];
    newSmbiosTable.Type4->L3CacheHandle = HandelLayer[2];

    if (SmbiosTable.Type4->ProcessorVersion == 0) { //if no BrandString we can add
      if (AsciiStrLen (gCPUStructure.BrandString) != 0) {
        UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type4->ProcessorVersion, gCPUStructure.BrandString);
      }
    }

    if (Size <= 0x20) {
      newSmbiosTable.Type4->SerialNumber = 0;
      newSmbiosTable.Type4->AssetTag = 0;
      newSmbiosTable.Type4->PartNumber = 0;
    }

    if (Size <= 0x23) {
      newSmbiosTable.Type4->CoreCount = gCPUStructure.Cores;
      newSmbiosTable.Type4->ThreadCount = gCPUStructure.Threads;
      newSmbiosTable.Type4->EnabledCoreCount = gCPUStructure.Cores;
      newSmbiosTable.Type4->ProcessorCharacteristics = (UINT16) gCPUStructure.Features;
      ProcChar |= (gCPUStructure.ExtFeatures & CPUID_EXTFEATURE_EM64T) ? 0x04 : 0;
      ProcChar |= (gCPUStructure.Cores > 1) ? 0x08 : 0;
      ProcChar |= (gCPUStructure.Cores < gCPUStructure.Threads) ? 0x10 : 0;
      ProcChar |= (gCPUStructure.ExtFeatures & CPUID_EXTFEATURE_XD) ? 0x20 : 0;
      ProcChar |= (gCPUStructure.Features & CPUID_FEATURE_VMX) ? 0x40 : 0;
      ProcChar |= (gCPUStructure.Features & CPUID_FEATURE_EST) ? 0x80 : 0;
      newSmbiosTable.Type4->ProcessorCharacteristics = ProcChar;
    }

    if (Size <= 0x28) {
      newSmbiosTable.Type4->ProcessorFamily2 = newSmbiosTable.Type4->ProcessorFamily;
    }

    newSmbiosTable.Type4->Hdr.Handle = NumberOfRecords;
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
  UINT8 tableTypes[15] = {6, 8, 9, 10, 11, 18, 21, 22, 27, 28, 32, 33};
  UINTN IndexType;

  // Different types
  for (IndexType = 0; IndexType < 12; IndexType++) {
    for (Index = 0; Index < 16; Index++) {
      SmbiosTable = GetSmbiosTableFromType (EntryPoint, tableTypes[IndexType], Index);

      if (SmbiosTable.Raw == NULL) {
        continue;
      }
      
      switch (tableTypes[IndexType]) {
        case 6:
          SmbiosTable.Type6->Hdr.Handle = NumberOfRecords;
          break;
        case 8:
          SmbiosTable.Type8->Hdr.Handle = NumberOfRecords;
          break;
        case 9:
          SmbiosTable.Type9->Hdr.Handle = NumberOfRecords;
          break;
        case 10:
          SmbiosTable.Type10->Hdr.Handle = NumberOfRecords;
          break;
        case 11:
          SmbiosTable.Type11->Hdr.Handle = NumberOfRecords;
          break;
        case 18:
          SmbiosTable.Type18->Hdr.Handle = NumberOfRecords;
          break;
        case 21:
          SmbiosTable.Type21->Hdr.Handle = NumberOfRecords;
          break;
        case 22:
          SmbiosTable.Type22->Hdr.Handle = NumberOfRecords;
          break;
        case 27:
          SmbiosTable.Type27->Hdr.Handle = NumberOfRecords;
          break;
        case 28:
          SmbiosTable.Type28->Hdr.Handle = NumberOfRecords;
          break;
        case 32:
          SmbiosTable.Type32->Hdr.Handle = NumberOfRecords;
          break;
        case 33:
          SmbiosTable.Type33->Hdr.Handle = NumberOfRecords;
          break;
      }
      LogSmbiosTable (SmbiosTable);
    }
  }

  return;
}

VOID
PatchMemoryTables (
  VOID
)
{
  UINT16  map;
  UINT16  Handle16;
  UINT16  Handle17[MAX_SLOT_COUNT];
  UINT16  Memory17[MAX_SLOT_COUNT];
  UINT16  Handle19;
  UINT32  TotalSystemMemory;
  UINT32  TotalEnd;
  UINT8   PartWidth;
  UINTN   j;

  TotalEnd  = 0;
  PartWidth = 1;
  Handle16  = 0xFFFE;
  gRAM      = AllocateZeroPool (sizeof (MEM_STRUCTURE));

  // Physical Memory Array
  //
  SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, 0);

  if (SmbiosTable.Raw == NULL) {
    DBG ("Smbios: Type 16 (Physical Memory Array) not found\n");
    return;
  }

  gRAM->MaxMemorySlots = (UINT8) SmbiosTable.Type16->NumberOfMemoryDevices;

  if (gRAM->MaxMemorySlots == 0) {
    gRAM->MaxMemorySlots = MAX_SLOT_COUNT;
  }

  TableSize = SmbiosTableLength (SmbiosTable);
  ZeroMem ((VOID*) newSmbiosTable.Type16, MAX_TABLE_SIZE);
  CopyMem ((VOID*) newSmbiosTable.Type16, (VOID*) SmbiosTable.Type16, TableSize);

  newSmbiosTable.Type16->Hdr.Handle = NumberOfRecords;
  Handle16 = LogSmbiosTable (newSmbiosTable);

  // Memory Device
  //
  gRAM->MemoryModules = 0;
  TotalSystemMemory = 0;

  for (Index = 0; Index < gRAM->MaxMemorySlots; Index++) {

    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_MEMORY_DEVICE, Index);
    if (SmbiosTable.Raw == NULL) {
      continue;
    }
    if (SmbiosTable.Type17->Size > 0) {
      gRAM->MemoryModules++;
    }
  }

  gRAM->SpdDetected = FALSE;
  DBG ("Smbios: Starting ScanSPD\n");
  ScanSPD ();

  for (Index = 0; Index < gRAM->MaxMemorySlots; Index++) {

    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_MEMORY_DEVICE, Index);
    if (SmbiosTable.Raw == NULL) {
      continue;
    }

    TableSize = SmbiosTableLength (SmbiosTable);
    ZeroMem ((VOID*) newSmbiosTable.Type17, MAX_TABLE_SIZE);
    CopyMem ((VOID*) newSmbiosTable.Type17, (VOID*) SmbiosTable.Type17, TableSize);
    newSmbiosTable.Type17->MemoryArrayHandle = Handle16;

    map = gRAM->Map[Index];

    if ((SmbiosTable.Type17->Size & 0x8000) == 0) {
      TotalSystemMemory += SmbiosTable.Type17->Size; //Mb
      Memory17[Index] = (UINT16)(SmbiosTable.Type17->Size > 0 ? TotalSystemMemory : 0);
      DBG ("Smbios: Memory17[%d] = %d\n", Index, Memory17[Index]);
    }

    if (gRAM->SpdDetected) {

      if (gRAM->DIMM[map].InUse) {

        DBG ("Smbios: SPD detected and slot %d (mapped to %d) used\n", Index, map);

        if ((gRAM->DIMM[map].Type != MemoryTypeUnknown) &&
            (gRAM->DIMM[map].Type != MemoryTypeOther)  &&
            (gRAM->DIMM[map].Type != 0)) {
          newSmbiosTable.Type17->MemoryType = gRAM->DIMM[map].Type;
        }

        if (iStrLen (gRAM->DIMM[map].Vendor, 64) > 0) {
          UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type17->Manufacturer, gRAM->DIMM[map].Vendor);
        }

        if (iStrLen (gRAM->DIMM[map].SerialNo, 64) > 0) {
          UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type17->SerialNumber, gRAM->DIMM[map].SerialNo);
        }

        if (iStrLen (gRAM->DIMM[map].PartNo, 64) > 0) {
          UpdateSmbiosString (newSmbiosTable, &newSmbiosTable.Type17->PartNumber, gRAM->DIMM[map].PartNo);
        }

        if (gRAM->DIMM[map].Frequency > 0) {
          newSmbiosTable.Type17->Speed = (UINT16) gRAM->DIMM[map].Frequency;
        }

        if (gRAM->DIMM[map].ModuleSize > 0) {
          newSmbiosTable.Type17->Size = (UINT16) gRAM->DIMM[map].ModuleSize;
        }
      } else {
        newSmbiosTable.Type17->Speed = 0;
        newSmbiosTable.Type17->Size  = 0;
      }

      newSmbiosTable.Type17->Hdr.Handle = NumberOfRecords;
      Handle17[Index] = LogSmbiosTable (newSmbiosTable);

      if ((gRAM->DIMM[map].InUse) &&
          (gRAM->DIMM[map].spd[0] != 0) &&
          (gRAM->DIMM[map].spd[0] != 0xFF)) {
        ZeroMem ((VOID*) newSmbiosTable.Type130, MAX_TABLE_SIZE);
        newSmbiosTable.Type130->Hdr.Type = 130;
        newSmbiosTable.Type130->Hdr.Length = (UINT8) (10 + (gRAM->DIMM[map].SpdSize));
        newSmbiosTable.Type130->Type17Handle = Handle17[Index];
        newSmbiosTable.Type130->Offset = 0;
        newSmbiosTable.Type130->Size = (gRAM->DIMM[map].SpdSize);
        CopyMem ((UINT8 *) newSmbiosTable.Type130->Data, gRAM->DIMM[map].spd, gRAM->DIMM[map].SpdSize);

        newSmbiosTable.Type130->Hdr.Handle = NumberOfRecords;
        Handle = LogSmbiosTable (newSmbiosTable);
        
        DBG ("Smbios: Type130 Lenght = 0x%x, Handle = 0x%x, Size = 0x%x\n",
             newSmbiosTable.Type130->Hdr.Length,
             Handle,
             newSmbiosTable.Type130->Size
             );
      }
    } else {
      newSmbiosTable.Type17->Hdr.Handle = NumberOfRecords;
      Handle17[Index] = LogSmbiosTable (newSmbiosTable);
    }
  }
  //
  // Generate Memory Array Mapped Address info (TYPE 19)
  //
  for (Index = 0; Index < (UINTN) (gRAM->MaxMemorySlots * 2); Index++) {
    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS, Index);

    if (SmbiosTable.Raw == NULL) {
      break;
    }

    if (SmbiosTable.Type19->EndingAddress > TotalEnd) {
      TotalEnd = SmbiosTable.Type19->EndingAddress;
    }

    PartWidth = SmbiosTable.Type19->PartitionWidth;
  }

  if (TotalEnd == 0) {
    TotalEnd =  (TotalSystemMemory << 10) - 1;
  }

  ZeroMem ((VOID*) newSmbiosTable.Type19, MAX_TABLE_SIZE);
  newSmbiosTable.Type19->Hdr.Type = EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS;
  newSmbiosTable.Type19->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE19);
  newSmbiosTable.Type19->MemoryArrayHandle = Handle16;
  newSmbiosTable.Type19->StartingAddress = 0;
  newSmbiosTable.Type19->EndingAddress = TotalEnd;
  newSmbiosTable.Type19->PartitionWidth = PartWidth;

  newSmbiosTable.Type19->Hdr.Handle = NumberOfRecords;
  Handle19 = LogSmbiosTable (newSmbiosTable);

  //
  // Generate Memory Array Mapped Address info (TYPE 20)
  //
  for (Index = 0; Index < (UINTN) (gRAM->MaxMemorySlots * 2); Index++) {
    SmbiosTable = GetSmbiosTableFromType (EntryPoint, EFI_SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS, Index);

    if (SmbiosTable.Raw == NULL) {
      return;
    }
    
    if (SmbiosTable.Type20->EndingAddress < 2) {
      continue;
    }

    TableSize = SmbiosTableLength (SmbiosTable);
    ZeroMem ((VOID*) newSmbiosTable.Type20, MAX_TABLE_SIZE);
    CopyMem ((VOID*) newSmbiosTable.Type20, (VOID*) SmbiosTable.Type20, TableSize);

    for (j = 0; j < gRAM->MaxMemorySlots; j++) {
      if ((((UINT32) Memory17[j]  << 10) - 1) <= newSmbiosTable.Type20->EndingAddress) {
        newSmbiosTable.Type20->MemoryDeviceHandle = Handle17[j];
				Memory17[j] = 0;
        break;
      }
    }
    newSmbiosTable.Type20->MemoryArrayMappedAddressHandle = Handle19;

    newSmbiosTable.Type20->Hdr.Handle = NumberOfRecords;
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
    ZeroMem ((VOID*) newSmbiosTable.Type128, MAX_TABLE_SIZE);
  
    newSmbiosTable.Type128->Hdr.Type = 128;
    newSmbiosTable.Type128->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE128);
    newSmbiosTable.Type128->FirmwareFeatures = 0xc0007417; //imac112 -> 0x1403
    newSmbiosTable.Type128->FirmwareFeaturesMask = 0xc0007fff; // 0xffff

    //    FW_REGION_RESERVED   = 0,
    //    FW_REGION_RECOVERY   = 1,
    //    FW_REGION_MAIN       = 2, gHob->MemoryAbove1MB.PhysicalStart + ResourceLength
    //                           or fix as 0x200000 - 0x600000
    //    FW_REGION_NVRAM      = 3,
    //    FW_REGION_CONFIG     = 4,
    //    FW_REGION_DIAGVAULT  = 5,

    newSmbiosTable.Type128->RegionCount = 2;
    newSmbiosTable.Type128->RegionType[0] = FW_REGION_MAIN;
    newSmbiosTable.Type128->FlashMap[0].StartAddress = 0xFFE00000; //0xF0000;
    newSmbiosTable.Type128->FlashMap[0].EndAddress = 0xFFF1FFFF;
    newSmbiosTable.Type128->RegionType[1] = FW_REGION_NVRAM; //Efivar
    newSmbiosTable.Type128->FlashMap[1].StartAddress = 0x15000; //0xF0000;
    newSmbiosTable.Type128->FlashMap[1].EndAddress = 0x1FFFF;
    //region type=1 also present in mac

    newSmbiosTable.Type128->Hdr.Handle = NumberOfRecords;
    LogSmbiosTable (newSmbiosTable);
    return;
}

VOID
PatchTableType131 (
  VOID
)
{
  newSmbiosTable.Type131->Hdr.Type = 131;
  newSmbiosTable.Type131->Hdr.Length = sizeof (SMBIOS_STRUCTURE) + 2;
  newSmbiosTable.Type131->ProcessorType = gSettings.CpuType;

  newSmbiosTable.Type131->Hdr.Handle = NumberOfRecords;
  Handle = LogSmbiosTable (newSmbiosTable);
  return;
}

VOID
PatchTableType132 (
  VOID
)
{
  SmbiosTable = GetSmbiosTableFromType (EntryPoint, 132, 0);

  if (SmbiosTable.Raw == NULL) {
      ZeroMem ((VOID*) newSmbiosTable.Type132, MAX_TABLE_SIZE);
      newSmbiosTable.Type132->Hdr.Type = 132;
      newSmbiosTable.Type132->Hdr.Length = sizeof (SMBIOS_STRUCTURE) + 2;

      if (gCPUStructure.ProcessorInterconnectSpeed != 0) {
        newSmbiosTable.Type132->ProcessorBusSpeed = (UINT16) gCPUStructure.ProcessorInterconnectSpeed;
      }

      if (gSettings.ProcessorInterconnectSpeed != 0) {
        newSmbiosTable.Type132->ProcessorBusSpeed = (UINT16) gSettings.ProcessorInterconnectSpeed;
      }
    
      newSmbiosTable.Type132->Hdr.Handle = NumberOfRecords;
      Handle = LogSmbiosTable (newSmbiosTable);
  } else {
    newSmbiosTable.Type132->Hdr.Handle = NumberOfRecords;
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
  newSmbiosTable.Raw = (UINT8*) AllocateZeroPool (MAX_TABLE_SIZE);

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
  FreePool ((VOID*) newSmbiosTable.Raw);
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
