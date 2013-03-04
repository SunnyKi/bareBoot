/**
initial concept of DSDT patching by mackerintel

 Re-Work by Slice 2011.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "macosx.h"

#define XXXX_SIGN           SIGNATURE_32('X','X','X','X')
#define HPET_SIGN           SIGNATURE_32('H','P','E','T')
#define UEFI_SIGN           SIGNATURE_32('U','E','F','I')
#define SHOK_SIGN           SIGNATURE_32('S','H','O','K')
#define SLIC_SIGN           SIGNATURE_32('S','L','I','C')
#define TAMG_SIGN           SIGNATURE_32('T','A','M','G')
#define ASF_SIGN            SIGNATURE_32('A','S','F','!')
#define APIC_SIGN           SIGNATURE_32('A','P','I','C')
#define HPET_OEM_ID         { 'A', 'P', 'P', 'L', 'E', ' ' }
#define HPET_OEM_TABLE_ID   { 'A', 'p', 'p', 'l', 'e', '0', '0', ' ' }
#define HPET_CREATOR_ID     { 'L', 'o', 'k', 'i' }
#define NUM_TABLES          19

CONST CHAR8 oemID[6]       = HPET_OEM_ID;
CONST CHAR8 oemTableID[8]  = HPET_OEM_TABLE_ID;
CONST CHAR8 creatorID[4]   = HPET_CREATOR_ID;

RSDT_TABLE                    *Rsdt = NULL;
XSDT_TABLE                    *Xsdt = NULL;

CHAR16* ACPInames[NUM_TABLES] = {
  L"SSDT.aml",
  L"SSDT-0.aml",
  L"SSDT-1.aml",
  L"SSDT-2.aml",
  L"SSDT-3.aml",
  L"SSDT-4.aml",
  L"SSDT-5.aml",
  L"SSDT-6.aml",
  L"SSDT-7.aml",
  L"SSDT-8.aml",
  L"SSDT-9.aml",
  L"APIC.aml",
  L"BOOT.aml",
  L"HPET.aml",
  L"MCFG.aml",
  L"SLIC.aml",
  L"SLIT.aml",
  L"SRAT.aml",
  L"UEFI.aml"
};

UINT32*
ScanRSDT (
  UINT32 Signature
)
{
  EFI_ACPI_DESCRIPTION_HEADER     *Table;
  UINTN                           Index;
  UINT32                          EntryCount;
  UINT32                          *EntryPtr;

  EntryCount = (Rsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT32);
  EntryPtr = &Rsdt->Entry;

  for (Index = 0; Index < EntryCount; Index++, EntryPtr++) {
    Table = (EFI_ACPI_DESCRIPTION_HEADER*) ((UINTN) (*EntryPtr));

    if (Table->Signature == Signature) {
      return EntryPtr;
    }
  }

  return NULL;
}

UINT64*
ScanXSDT (
  UINT32 Signature
)
{
  EFI_ACPI_DESCRIPTION_HEADER   *Table;
  UINTN                         Index;
  UINT32                        EntryCount;
  UINT64                        *BasePtr;
  UINT64                        Entry64;

  EntryCount = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);
  BasePtr = (UINT64*) (& (Xsdt->Entry));

  for (Index = 0; Index < EntryCount; Index ++, BasePtr++) {
    CopyMem (&Entry64, (VOID*) BasePtr, sizeof (UINT64));
    Table = (EFI_ACPI_DESCRIPTION_HEADER *) ((UINTN) (Entry64));

    if (Table->Signature == Signature) {
      return BasePtr;
    }
  }

  return NULL;
}

VOID
DropTableFromRSDT (
  UINT32 Signature
)
{
  EFI_ACPI_DESCRIPTION_HEADER     *Table;
  UINTN                           Index, Index2;
  UINT32                          EntryCount;
  UINT32                          *EntryPtr, *Ptr, *Ptr2;

  EntryCount = (Rsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT32);
  EntryPtr = &Rsdt->Entry;

  for (Index = 0; Index < EntryCount; Index++, EntryPtr++) {
#if 0
    Print (L"num = %d, addr = 0x%x, ", Index, *EntryPtr);
#endif

    if (*EntryPtr == 0) {
      Rsdt->Header.Length -= sizeof (UINT32);
      continue;
    }

    Table = (EFI_ACPI_DESCRIPTION_HEADER*) ((UINTN) (*EntryPtr));
#if 0
    Print (L"sig = 0x%x\r\n", Table->Signature);
#endif

    if (Table->Signature != Signature) {
      continue;
    }

    Ptr = EntryPtr;
    Ptr2 = Ptr + 1;

    for (Index2 = Index; Index2 < EntryCount; Index2++) {
      *Ptr++ = *Ptr2++;
    }

    EntryPtr--;
    Rsdt->Header.Length -= sizeof (UINT32);
  }
}

VOID
DropTableFromXSDT (
  UINT32 Signature
)
{
  EFI_ACPI_DESCRIPTION_HEADER     *Table;
  UINTN                           Index, Index2;
  UINT32                          EntryCount;
  UINT64                          *BasePtr, *Ptr, *Ptr2;
  UINT64                          Entry64;

  EntryCount = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);
  BasePtr = (UINT64*) (& (Xsdt->Entry));

  for (Index = 0; Index < EntryCount; Index++, BasePtr++) {
#if 0
    Print (L"num = %d, addr = 0x%x, ", Index, *BasePtr);
#endif

    if (*BasePtr == 0) {
      Xsdt->Header.Length -= sizeof (UINT64);
      continue;
    }

    CopyMem (&Entry64, (VOID*) BasePtr, sizeof (UINT64));
    Table = (EFI_ACPI_DESCRIPTION_HEADER*) ((UINTN) (Entry64));
#if 0
    Print (L"sig = 0x%x\r\n", Table->Signature);
#endif

    if (Table->Signature != Signature) {
      continue;
    }

    Ptr = BasePtr;
    Ptr2 = Ptr + 1;

    for (Index2 = Index; Index2 < EntryCount; Index2++) {
      *Ptr++ = *Ptr2++;
    }

    BasePtr--;
    Xsdt->Header.Length -= sizeof (UINT64);
  }
}

EFI_STATUS
InsertTable (
  VOID* Table,
  UINTN Length
)
{
  EFI_STATUS              Status;
  EFI_PHYSICAL_ADDRESS    BufferPtr;
  UINT32                  *Ptr;
  UINT64                  *XPtr;

  Status    = EFI_SUCCESS;
  BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;

  if (Table == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->AllocatePages (
             AllocateMaxAddress,
             EfiACPIReclaimMemory,
             EFI_SIZE_TO_PAGES (Length),
             &BufferPtr
           );

  if (!EFI_ERROR (Status)) {
    CopyMem ((VOID*) (UINTN) BufferPtr, (VOID*) Table, Length);

    if (Rsdt) {
      Ptr = (UINT32*) ((UINTN) Rsdt + Rsdt->Header.Length);
      *Ptr = (UINT32) (UINTN) BufferPtr;
      Rsdt->Header.Length += sizeof (UINT32);
    }

    if (Xsdt) {
      XPtr = (UINT64*) ((UINTN) Xsdt + Xsdt->Header.Length);
      *XPtr = (UINT64) (UINTN) BufferPtr;
      Xsdt->Header.Length += sizeof (UINT64);
    }
  }

  return Status;
}

EFI_STATUS
PatchACPI (
  IN EFI_FILE *FHandle
)
{
  EFI_STATUS                                        Status;
  UINTN                                             Index;
  EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER      *RsdPointer;
  EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE         *FadtPointer;
  EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE         *newFadt;
  EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER  *Hpet;
  EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE      *Facs;
  EFI_PHYSICAL_ADDRESS                              dsdt;
  EFI_PHYSICAL_ADDRESS                              BufferPtr;
  UINT8                                             *buffer;
  UINTN                                             bufferLen;
  UINT32                                            *rf;
  UINT64                                            *xf;
  UINT64                                            XDsdt;
  UINT64                                            BiosDsdt;
  UINT64                                            XFirmwareCtrl;
  UINT32                                            *pEntryR;
  UINT64                                            *pEntryX;
  CHAR16                                            *PathToACPITables;
  CHAR16                                            *PathPatched;
  CHAR16                                            *PathDsdt;
  UINT32                                            eCntR; //, CntSSDT=0;
#if 0
  SSDT_TABLE                                        *Ssdt = NULL;
#endif

  buffer = NULL;
  bufferLen = 0;
  dsdt = EFI_SYSTEM_TABLE_MAX_ADDRESS; //0xFE000000;
  Facs = NULL;
  FadtPointer = NULL;
  Hpet    = NULL;
  newFadt   = NULL;
  PathDsdt      = L"DSDT.aml";
  PathPatched   = L"\\EFI\\mini\\acpi\\patched\\";
  PathToACPITables = AllocateZeroPool (250);
  rf = NULL;
  RsdPointer = NULL;
  Status = EFI_SUCCESS;
  xf = NULL;

  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gEfiAcpi20TableGuid)) {
      RsdPointer = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER*) gST->ConfigurationTable[Index].VendorTable;
#if 0
      Print (L"ACPI 2.0\r\n");
      Print (L"RSDT = 0x%x\r\n", (RSDT_TABLE*) (UINTN) RsdPointer->RsdtAddress);
      Print (L"XSDT = 0x%x\r\n", (XSDT_TABLE*) (UINTN) RsdPointer->XsdtAddress);
#endif
      break;
    } else if (CompareGuid (&gST->ConfigurationTable[Index].VendorGuid, &gEfiAcpi10TableGuid)) {
      RsdPointer = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER*) gST->ConfigurationTable[Index].VendorTable;
#if 0
      Print (L"ACPI 1.0\r\n");
#endif
      continue;
    }
  }

  if (RsdPointer == NULL) {
    return EFI_UNSUPPORTED;
  }

  Rsdt = (RSDT_TABLE*) (UINTN) RsdPointer->RsdtAddress;
  rf = ScanRSDT (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE);

  if (rf != NULL) {
    FadtPointer = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE*) (UINTN) (*rf);
  }

#if 0
  Print (L"FADT = 0x%x\r\n", FadtPointer);
  Xsdt = (XSDT_TABLE*) (UINTN) RsdPointer->XsdtAddress;
  Print (L"XSDT = 0x%x\r\n", Xsdt);
  eCntR = (Rsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT32);
  eCntX = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);
  Print (L"eCntR=0x%x  eCntX=0x%x\r\n", eCntR, eCntX);

  if (Xsdt == NULL) {
    DropTableFromRSDT (UEFI_SIGN);
    DropTableFromRSDT (SHOK_SIGN);
    DropTableFromRSDT (ASF_SIGN);
    DropTableFromRSDT (SLIC_SIGN);
    DropTableFromRSDT (TAMG_SIGN);
    eCntR = (Rsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT32);
    eCntX = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);
    Print (L"eCntR=0x%x\r\n", eCntR);
#endif
    BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
    Status = gBS->AllocatePages (AllocateMaxAddress, EfiACPIReclaimMemory, 1, &BufferPtr);

    if (!EFI_ERROR (Status)) {
      Xsdt = (XSDT_TABLE*) (UINTN) BufferPtr;
      Xsdt->Header.Signature = 0x54445358; //EFI_ACPI_2_0_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE
      eCntR = (Rsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT32);
      Xsdt->Header.Length = eCntR * sizeof (UINT64) + sizeof (EFI_ACPI_DESCRIPTION_HEADER);
      Xsdt->Header.Revision = 1;
      CopyMem ((CHAR8 *) &Xsdt->Header.OemId, (CHAR8 *) &FadtPointer->Header.OemId, 6);
      Xsdt->Header.OemTableId = Rsdt->Header.OemTableId;
      Xsdt->Header.OemRevision = Rsdt->Header.OemRevision;
      Xsdt->Header.CreatorId = Rsdt->Header.CreatorId;
      Xsdt->Header.CreatorRevision = Rsdt->Header.CreatorRevision;
      pEntryR = (UINT32*) (&(Rsdt->Entry));
      pEntryX = (UINT64*) (&(Xsdt->Entry));

      for (Index = 0; Index < eCntR; Index ++) {
#if 0
        Print (L"RSDT entry = 0x%x\r\n", *pEntryR);
#endif

        if (*pEntryR != 0) {
          *pEntryX = 0;
          CopyMem ((VOID*) pEntryX, (VOID*) pEntryR, sizeof (UINT32));
          pEntryR++;
          pEntryX++;
        } else {
#if 0
          Print (L"entry addr = 0x%x, skip it", *pEntryR);
#endif
          Xsdt->Header.Length -= sizeof (UINT64);
          pEntryR++;
        }
      }

      RsdPointer->XsdtAddress = (UINT64) Xsdt;
      RsdPointer->Checksum = 0;
      RsdPointer->Checksum = (UINT8) (256 - CalculateSum8 ((CHAR8*) RsdPointer, 20));
#if 0
      RsdPointer->ExtendedChecksum = 0;
      RsdPointer->ExtendedChecksum = (UINT8) (256 - CalculateSum8 ((CHAR8*) RsdPointer, RsdPointer->Length));
#endif
    } else {
      Print (L"no allocate page for new xsdt\r\n");
      return EFI_OUT_OF_RESOURCES;
    }

#if 0
  } else if (RsdPointer->Revision >= 2 && (RsdPointer->XsdtAddress < (UINT64) (UINTN) - 1)) {
    Xsdt = (XSDT_TABLE*) (UINTN) RsdPointer->XsdtAddress;

    if (& (Xsdt->Entry)) {
      eCntR = (Rsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT32);
      eCntX = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);
      Print (L"XSDT = 0x%x\r\n", Xsdt);
      Print (L"FADT = 0x%x\r\n", FadtPointer);

      if (eCntR > eCntX) {
        Print (L"eCntR=0x%x != eCntX=0x%x\r\n", eCntR, eCntX);
        Rsdt->Header.Length = eCntX * sizeof (UINT32) + sizeof (EFI_ACPI_DESCRIPTION_HEADER);
        eCntR = eCntX;
      } else {
        Print (L"eCntR=0x%x != eCntX=0x%x\r\n", eCntR, eCntX);
        Xsdt->Header.Length = eCntR * sizeof (UINT64) + sizeof (EFI_ACPI_DESCRIPTION_HEADER);
      }

      pEntryR = (UINT32*) (& (Rsdt->Entry));
      pEntryX = (UINT64*) (& (Xsdt->Entry));

      for (Index = 0; Index < eCntR; Index ++) {
        Print (L"old = 0x%x ", *pEntryX);
        *pEntryX = 0;
        CopyMem ((VOID*) pEntryX, (VOID*) pEntryR, sizeof (UINT32));
        Print (L"new entry XSDT = 0x%x\r\n", (UINT64) *pEntryX);
        pEntryR++;
        pEntryX++;
      }
    }

    xf = ScanXSDT (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE);

    if (xf) {
      FadtPointer = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE*) (UINTN) (*xf);
    }
  }

  Xsdt = (XSDT_TABLE*) (UINTN) RsdPointer->XsdtAddress;
#endif
  xf = ScanXSDT (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE);

  if (xf) {
    FadtPointer = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE*) (UINTN) (*xf);
  }

  if (!FadtPointer) {
    Print (L"no FADT entry in RSDT\r\n");
    return EFI_NOT_FOUND;
  }

// -===== APIC =====-
#if 0
  EFI_ACPI_DESCRIPTION_HEADER                           *ApicTable;
  EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER   *ApicHeader;
  EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE           *ProcLocalApic;
  xf = ScanXSDT (APIC_SIGN);

  if (xf != NULL) {
    ApicTable = (EFI_ACPI_DESCRIPTION_HEADER*) (UINTN) (*xf);
    ApicHeader = (EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *) (*xf + sizeof (EFI_ACPI_DESCRIPTION_HEADER));
    ProcLocalApic = (EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE *) (*xf + sizeof (EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER));
    Print (L"ApicTable = 0x%x, ApicHeader = 0x%x, ProcLocalApic = 0x%x\r\n", ApicTable, ApicHeader, ProcLocalApic);

    while ((ProcLocalApic->Type == 0)) { // && (ProcLocalApic->Flags == 1)) {
      Print (L" ProcId = %d, ApicId = %d, Flags = %d\r\n", ProcLocalApic->AcpiProcessorId, ProcLocalApic->ApicId, ProcLocalApic->Flags);

      if ((ProcLocalApic->AcpiProcessorId) != ProcLocalApic->ApicId) {
        ProcLocalApic->AcpiProcessorId = ProcLocalApic->ApicId;
#if 0
        ProcLocalApic->ApicId = ProcLocalApic->AcpiProcessorId;
#endif
        Pause (L"Found (ProcId ) != ApicId !!!\r\n");
      }

      ProcLocalApic++;
      Print (L"ProcLocalApic = 0x%x (ProcLocalApic->Length = %d)\r\n", ProcLocalApic, ProcLocalApic->Length);
    }

    ApicTable->Checksum = 0;
    ApicTable->Checksum = (UINT8) (256 - CalculateSum8 ((CHAR8*) ApicTable, ApicTable->Length));
  } else {
    Pause (L"No APIC table Found !!!\r\n");
  }
#endif

  BiosDsdt = FadtPointer->XDsdt;

  if (BiosDsdt == 0) {
    BiosDsdt = FadtPointer->Dsdt;
  }

  Facs = (EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE*) (UINTN) (FadtPointer->FirmwareCtrl);
  BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
  Status = gBS->AllocatePages (AllocateMaxAddress, EfiACPIReclaimMemory, 1, &BufferPtr);

  if (!EFI_ERROR (Status)) {
    UINT32 oldLength = ((EFI_ACPI_DESCRIPTION_HEADER*) FadtPointer)->Length;
    newFadt = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE*) (UINTN) BufferPtr;
    CopyMem ((UINT8*) newFadt, (UINT8*) FadtPointer, oldLength);
    newFadt->Header.Length = 0xF4;
#if 0
    CopyMem ((UINT8*) newFadt->Header.OemId, (UINT8*) "APPLE  ", 6);
#endif
    newFadt->Header.Revision = EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION;
    newFadt->Reserved0 = 0; //ACPIspec said it should be 0, while 1 is possible, but no more

    if (gSettings.PMProfile) {
      newFadt->PreferredPmProfile = gSettings.PMProfile;
    } else {
      newFadt->PreferredPmProfile = gMobile ? 2 : 1;
    }

#if 0
    if (gSettings.EnableC6 || gSettings.EnableISS) {
      newFadt->CstCnt = 0x85; //as in Mac
    }
#endif

    newFadt->PLvl2Lat = 0x65;
    newFadt->PLvl3Lat = 0x3E9;
    newFadt->IaPcBootArch = 0x3;
    newFadt->Flags |= 0x400; //Reset Register Supported
    newFadt->ResetReg.AddressSpaceId     = 1;
    newFadt->ResetReg.RegisterBitWidth   = 8;
    newFadt->ResetReg.RegisterBitOffset  = 0;
    newFadt->ResetReg.Reserved           = 1;
    newFadt->ResetReg.Address               = gSettings.ResetAddr;
    newFadt->ResetValue                     = gSettings.ResetVal;
    newFadt->Reserved2[0]        = 0;
    newFadt->Reserved2[1]        = 0;
    newFadt->Reserved2[2]        = 0;
// dsdt + xdsdt
    XDsdt = newFadt->XDsdt;

    if (BiosDsdt) {
      newFadt->XDsdt = BiosDsdt;
      newFadt->Dsdt = (UINT32) BiosDsdt;
    } else if (newFadt->Dsdt) {
      newFadt->XDsdt = (UINT64) (newFadt->Dsdt);
    } else if (XDsdt) {
      newFadt->Dsdt = (UINT32) XDsdt;
    }

    UnicodeSPrint (PathToACPITables, 250, L"%s%s", PathPatched, PathDsdt);

    if (FileExists (FHandle, PathToACPITables)) {
      Status = egLoadFile (FHandle, PathToACPITables , &buffer, &bufferLen);

      if (!EFI_ERROR (Status)) {
        Status = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIReclaimMemory,
                   EFI_SIZE_TO_PAGES (bufferLen),
                   &dsdt
                 );

        if (!EFI_ERROR (Status)) {
          CopyMem ((VOID*) (UINTN) dsdt, buffer, bufferLen);
          newFadt->Dsdt  = (UINT32) dsdt;
          newFadt->XDsdt = dsdt;
        }
      }
    }

// facs + xfacs
    XFirmwareCtrl = newFadt->XFirmwareCtrl;

    if (Facs) {
      newFadt->FirmwareCtrl = (UINT32) (UINTN) Facs;
    }

    if (newFadt->FirmwareCtrl) {
      newFadt->XFirmwareCtrl = (UINT64) (newFadt->FirmwareCtrl);
    } else if (newFadt->XFirmwareCtrl) {
      newFadt->FirmwareCtrl = (UINT32) XFirmwareCtrl;
    }

    Facs->Version = EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION;
// extra blocks
    newFadt->XPm1aEvtBlk.AddressSpaceId     = 1;
    newFadt->XPm1aEvtBlk.RegisterBitWidth   = 0x20;
    newFadt->XPm1aEvtBlk.RegisterBitOffset  = 0;
    newFadt->XPm1aEvtBlk.Reserved           = 0;
    newFadt->XPm1aEvtBlk.Address = (UINT64) (newFadt->Pm1aEvtBlk);
    newFadt->XPm1bEvtBlk.AddressSpaceId     = 1;
    newFadt->XPm1bEvtBlk.RegisterBitWidth   = 0;
    newFadt->XPm1bEvtBlk.RegisterBitOffset  = 0;
    newFadt->XPm1bEvtBlk.Reserved           = 0;
    newFadt->XPm1bEvtBlk.Address            = (UINT64) (newFadt->Pm1bEvtBlk);
    newFadt->XPm1aCntBlk.AddressSpaceId     = 1;
    newFadt->XPm1aCntBlk.RegisterBitWidth   = 0x10;
    newFadt->XPm1aCntBlk.RegisterBitOffset  = 0;
    newFadt->XPm1aCntBlk.Reserved           = 0;
    newFadt->XPm1aCntBlk.Address = (UINT64) (newFadt->Pm1aCntBlk);
    newFadt->XPm1bCntBlk.AddressSpaceId     = 1;
    newFadt->XPm1bCntBlk.RegisterBitWidth   = 0;
    newFadt->XPm1bCntBlk.RegisterBitOffset  = 0;
    newFadt->XPm1bCntBlk.Reserved           = 0;
    newFadt->XPm1bCntBlk.Address = (UINT64) (newFadt->Pm1bCntBlk);
    newFadt->XPm2CntBlk.AddressSpaceId     = 1;
    newFadt->XPm2CntBlk.RegisterBitWidth   = 8;
    newFadt->XPm2CntBlk.RegisterBitOffset  = 0;
    newFadt->XPm2CntBlk.Reserved           = 0;
    newFadt->XPm2CntBlk.Address  = (UINT64) (newFadt->Pm2CntBlk);
    newFadt->XPmTmrBlk.AddressSpaceId     = 1;
    newFadt->XPmTmrBlk.RegisterBitWidth   = 0x20;
    newFadt->XPmTmrBlk.RegisterBitOffset  = 0;
    newFadt->XPmTmrBlk.Reserved           = 0;
    newFadt->XPmTmrBlk.Address   = (UINT64) (newFadt->PmTmrBlk);
    newFadt->XGpe0Blk.AddressSpaceId     = 1;
    newFadt->XGpe0Blk.RegisterBitWidth   = 0x80;
    newFadt->XGpe0Blk.RegisterBitOffset  = 0;
    newFadt->XGpe0Blk.Reserved           = 0;
    newFadt->XGpe0Blk.Address    = (UINT64) (newFadt->Gpe0Blk);
    newFadt->XGpe1Blk.AddressSpaceId     = 1;
    newFadt->XGpe1Blk.RegisterBitWidth   = 0;
    newFadt->XGpe1Blk.RegisterBitOffset  = 0;
    newFadt->XGpe1Blk.Reserved           = 0;
    newFadt->XGpe1Blk.Address    = (UINT64) (newFadt->Gpe1Blk);
    FadtPointer = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE*) newFadt;
    FadtPointer->Header.Checksum = 0;
    FadtPointer->Header.Checksum = (UINT8) (256 - CalculateSum8 ((CHAR8*) FadtPointer, FadtPointer->Header.Length));

// We are sure that Fadt is the first entry in RSDT/XSDT table
    if (Rsdt != NULL) {
      Rsdt->Entry = (UINT32) (UINTN) newFadt;
    }

    if (Xsdt != NULL) {
      Xsdt->Entry = (UINT64) ((UINT32) (UINTN) newFadt);
    }
  }

  xf = ScanXSDT (HPET_SIGN);

  if (xf == NULL) { //we want to make the new table if OEM is not found
    BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
    Status = gBS->AllocatePages (AllocateMaxAddress, EfiACPIReclaimMemory, 1, &BufferPtr);

    if (!EFI_ERROR (Status)) {
      Hpet = (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER*) (UINTN) BufferPtr;
      Hpet->Header.Signature = EFI_ACPI_3_0_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE;
      Hpet->Header.Length = sizeof (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER);
      Hpet->Header.Revision = EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_REVISION;
      CopyMem (&Hpet->Header.OemId, oemID, 6);
      CopyMem (&Hpet->Header.OemTableId, oemTableID, sizeof (oemTableID));
      Hpet->Header.OemRevision = 0x00000001;
      CopyMem (&Hpet->Header.CreatorId, creatorID, sizeof (creatorID));
      Hpet->EventTimerBlockId = 0x8086A201; // we should remember LPC VendorID to place here
      Hpet->BaseAddressLower32Bit.AddressSpaceId = EFI_ACPI_2_0_SYSTEM_IO;
      Hpet->BaseAddressLower32Bit.RegisterBitWidth = 0x40; //64bit
      Hpet->BaseAddressLower32Bit.RegisterBitOffset = 0x00;
      Hpet->BaseAddressLower32Bit.Address = 0xFED00000; //Physical Addr.
      Hpet->HpetNumber = 0;
      Hpet->MainCounterMinimumClockTickInPeriodicMode = 0x0080;
      Hpet->PageProtectionAndOemAttribute = EFI_ACPI_64KB_PAGE_PROTECTION;
      // Flags |= EFI_ACPI_4KB_PAGE_PROTECTION , EFI_ACPI_64KB_PAGE_PROTECTION
      // verify checksum
      Hpet->Header.Checksum = 0;
      Hpet->Header.Checksum = (UINT8) (256 - CalculateSum8 ((CHAR8*) Hpet, Hpet->Header.Length));

      //then we have to install new table into Xsdt
      if (Xsdt != NULL) {
        UINT32  EntryCount;

        EntryCount = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);
        xf = (UINT64*) (&(Xsdt->Entry)) + EntryCount;
        Xsdt->Header.Length += sizeof (UINT64);
        *xf = (UINT64) (UINTN) Hpet;
      }
    }
  }

  if (gSettings.DropSSDT) {
    DropTableFromRSDT (EFI_ACPI_4_0_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE);
    DropTableFromXSDT (EFI_ACPI_4_0_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE);
  }

#if 0
  else {
    DropTableFromRSDT (XXXX_SIGN);
    DropTableFromXSDT (XXXX_SIGN);
  }

  for (Index = 0; Index < NUM_TABLES; Index++) {
    UnicodeSPrint (PathToACPITables, 250, L"%s%s", PathPatched, ACPInames[Index]);

    if (FileExists (FHandle, PathToACPITables)) {
      CntSSDT++;
    }
  }

  if (CntSSDT > 0) {
    eCntR = Rsdt->Header.Length + CntSSDT * sizeof (UINT32;
            eCntX = Xsdt->Header.Length + CntSSDT * sizeof (UINT64);
            BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
            Status = gBS->AllocatePages (AllocateMaxAddress, EfiACPIReclaimMemory, 1, &BufferPtr);

    for (Index = 0; Index < NUM_TABLES; Index++) {
    UnicodeSPrint (PathToACPITables, 250, L"%s%s", PathPatched, ACPInames[Index]);

      if (FileExists (FHandle, PathToACPITables)) CntSSDT {
        Status = egLoadFile (FHandle, PathToACPITables, &buffer, &bufferLen);

        if (!EFI_ERROR (Status)) {
          Status = InsertTable ((VOID*) buffer, bufferLen);
        }
      }
    }
  }

#endif

  for (Index = 0; Index < NUM_TABLES; Index++) {
    UnicodeSPrint (PathToACPITables, 250, L"%s%s", PathPatched, ACPInames[Index]);

    if (FileExists (FHandle, PathToACPITables)) {
      Status = egLoadFile (FHandle, PathToACPITables, &buffer, &bufferLen);

      if (!EFI_ERROR (Status)) {
        Status = InsertTable ((VOID*) buffer, bufferLen);
      }

#if 0
      Print (L"    load table %s with status %d", PathToACPITables, Status);
#endif
    }
  }

  if (Rsdt) {
    Rsdt->Header.Checksum = 0;
    Rsdt->Header.Checksum = (UINT8) (256 - CalculateSum8 ((CHAR8*) Rsdt, Rsdt->Header.Length));
  }

  if (Xsdt) {
    Xsdt->Header.Checksum = 0;
    Xsdt->Header.Checksum = (UINT8) (256 - CalculateSum8 ((CHAR8*) Xsdt, Xsdt->Header.Length));
  }

#if 0
  Pause (L"Press Any Key.....\r\n");
#endif
  return EFI_SUCCESS;
}
