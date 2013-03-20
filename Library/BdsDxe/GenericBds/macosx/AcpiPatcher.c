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
#if 0
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
#endif
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
  UINT32                                            eCntR;

  EFI_ACPI_DESCRIPTION_HEADER                           *ApicTable;
  EFI_ACPI_DESCRIPTION_HEADER                           *NewApicTable;
  EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER   *ApicHeader;
  EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE           *ProcLocalApic;
  EFI_ACPI_2_0_IO_APIC_STRUCTURE                        *IoAPIC;
  EFI_ACPI_2_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE      *ISOverride;
  EFI_ACPI_2_0_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE  *NMISource;
  EFI_ACPI_2_0_LOCAL_APIC_NMI_STRUCTURE                 *LocalApicNMI;
  EFI_ACPI_2_0_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE    *LocalApicAdrrOverride;
  EFI_ACPI_2_0_IO_SAPIC_STRUCTURE                       *IoSapic;
  EFI_ACPI_2_0_PROCESSOR_LOCAL_SAPIC_STRUCTURE          *ProcessorLocalSapic;
  EFI_ACPI_2_0_PLATFORM_INTERRUPT_SOURCES_STRUCTURE     *PlatformIntSource;

  UINT64                                                AddrApic;
  UINT32                                                TableLength;
  UINT32                                                ApicLength;
  UINT8                                                 ProcCount;
  UINT8                                                 Type;

  buffer = NULL;
  bufferLen = 0;
  dsdt = EFI_SYSTEM_TABLE_MAX_ADDRESS; //0xFE000000;
  Facs = NULL;
  FadtPointer = NULL;
  newFadt   = NULL;
  PathDsdt      = L"DSDT.aml";
  PathPatched   = L"\\EFI\\mini\\acpi\\patched\\";
  PathToACPITables = AllocateZeroPool (250);
  rf = NULL;
  RsdPointer = NULL;
  Status = EFI_SUCCESS;
  xf = NULL;
  NewApicTable = NULL;
  ApicTable = NULL;

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

#if 0
  Print (L"FADT = 0x%x\r\n", FadtPointer);
  Xsdt = (XSDT_TABLE*) (UINTN) RsdPointer->XsdtAddress;
  Print (L"XSDT = 0x%x\r\n", Xsdt);
  eCntR = (Rsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT32);
  eCntX = (Xsdt->Header.Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);
  Print (L"eCntR=0x%x  eCntX=0x%x\r\n", eCntR, eCntX);
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

    RsdPointer->XsdtAddress = (UINTN) Xsdt;
    RsdPointer->Checksum = 0;
    RsdPointer->Checksum = (UINT8) (256 - CalculateSum8 ((UINT8*) RsdPointer, 20));
#if 0
    RsdPointer->ExtendedChecksum = 0;
    RsdPointer->ExtendedChecksum = (UINT8) (256 - CalculateSum8 ((UINT8*) RsdPointer, RsdPointer->Length));
#endif
  } else {
    Print (L"no allocate page for new xsdt\r\n");
    return EFI_OUT_OF_RESOURCES;
  }

  FadtPointer = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE*) (UINTN)
    *(ScanXSDT (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE));

  if (FadtPointer == NULL) {
    Print (L"no FADT entry in RSDT\r\n");
    return EFI_NOT_FOUND;
  }
  
  // -===== APIC =====-
  if (gSettings.PatchAPIC) {
    xf = ScanXSDT (APIC_SIGN);
    if (xf != NULL) {
      ApicTable = (EFI_ACPI_DESCRIPTION_HEADER*) (UINTN) (*xf);

      BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
      Status = gBS->AllocatePages (AllocateMaxAddress, EfiACPIReclaimMemory, 1, &BufferPtr);
      if (!EFI_ERROR (Status)) {
        NewApicTable = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) BufferPtr;
        Print (L"ApicTable = 0x%x, ApicTable->Length = 0x%x\r\n", ApicTable, ApicTable->Length );
        CopyMem ((UINT8*) NewApicTable, (UINT8*) ApicTable, ApicTable->Length);
        NewApicTable->Length = (ApicTable->Length + 6);
        Print (L"NewApicTable = 0x%x, NewApicTable->Length = 0x%x\r\n", NewApicTable, NewApicTable->Length);
        Print (L"NewApicTable->Signature = 0x%x\r\n", NewApicTable->Signature);
        AddrApic = (UINTN) NewApicTable;
        LocalApicNMI = (EFI_ACPI_2_0_LOCAL_APIC_NMI_STRUCTURE *) (UINTN) (AddrApic + ApicTable->Length);
        LocalApicNMI->Type = 4;
        LocalApicNMI->Length = 6;
        LocalApicNMI->AcpiProcessorId = 0xFF;
        LocalApicNMI->Flags = 5;
        LocalApicNMI->LocalApicLint = 1;
        NewApicTable->Checksum = 0;
        NewApicTable->Checksum = (UINT8) (256 - CalculateSum8 ((UINT8*) NewApicTable, NewApicTable->Length));
        *xf = (UINTN) NewApicTable;
        Print (L"xf = 0x%x,\r\n", *xf);
      } else {
        Print (L"error AllocatePages \r\n");
      }
      
      xf = ScanXSDT (APIC_SIGN);
      if (xf != NULL) {
        ProcCount = 0;
        ApicLength = 0;

        ApicTable = (EFI_ACPI_DESCRIPTION_HEADER*) (UINTN) (*xf);
        TableLength = ApicTable->Length;
        ApicHeader = (EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *) (UINTN) (*xf + sizeof (EFI_ACPI_DESCRIPTION_HEADER));
        ProcLocalApic = (EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE *) (UINTN) (*xf + sizeof (EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER));

        ApicLength += (sizeof (EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER) +
                       sizeof (EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE));
        AddrApic = (UINTN) ProcLocalApic;
        Type = ProcLocalApic->Type;
#ifdef ACPI_DEBUG
        Print (L" TableLength = 0x%x, ApicTable = 0x%x, ApicHeader = 0x%x\r\n", TableLength, ApicTable, ApicHeader);
        Print (L"AddrApic = 0x%x\r\n", AddrApic);
        Print (L"Next Type = %d\r\n", Type);
#endif
        while (ApicLength <= TableLength) {
          switch (Type) {
            case EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC:
              ProcLocalApic = (EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE *) (UINTN) (AddrApic);
#ifdef ACPI_DEBUG
              Print (L" ProcId = %d, ApicId = %d, Flags = %d\r\n", ProcLocalApic->AcpiProcessorId, ProcLocalApic->ApicId, ProcLocalApic->Flags);
#endif
              ProcCount++;

              ProcLocalApic++;
              AddrApic = (UINTN) ProcLocalApic;
              Type = ProcLocalApic->Type;
              ApicLength += ProcLocalApic->Length;
#ifdef ACPI_DEBUG
              Print (L"AddrApic = 0x%x\r\n", AddrApic);
              Print (L"Next Type = %d\r\n", Type);
#endif
              break;

            case EFI_ACPI_2_0_IO_APIC:
              IoAPIC = (EFI_ACPI_2_0_IO_APIC_STRUCTURE *) (UINTN) (AddrApic);
#ifdef ACPI_DEBUG
              Print (L"IoApicId = %d, Address = 0x%x, Interrupt = %d, Length = 0x%x\r\n",  IoAPIC->IoApicId, IoAPIC->IoApicAddress, IoAPIC->GlobalSystemInterruptBase, IoAPIC->Length);
#endif
              IoAPIC++;
              AddrApic = (UINTN) IoAPIC;
              Type = IoAPIC->Type;
              ApicLength += IoAPIC->Length;
#ifdef ACPI_DEBUG
              Print (L"AddrApic = 0x%x\r\n", AddrApic);
              Print (L"Next Type = %d\r\n", Type);
#endif
              break;

            case EFI_ACPI_2_0_INTERRUPT_SOURCE_OVERRIDE:
              ISOverride = (EFI_ACPI_2_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE *) (UINTN) (AddrApic);
#ifdef ACPI_DEBUG
              Print (L" Source = 0x%x, Interrupt = %d, Flags = 0x%x, Length = %d\r\n", ISOverride->Source, ISOverride->GlobalSystemInterrupt, ISOverride->Flags, ISOverride->Length);
#endif
              ISOverride++;
              AddrApic = (UINTN) ISOverride;
              Type = ISOverride->Type;
              ApicLength += ISOverride->Length;
#ifdef ACPI_DEBUG
              Print (L"AddrApic = 0x%x\r\n", AddrApic);
              Print (L"Next Type = %d\r\n", Type);
#endif
              break;

            case EFI_ACPI_2_0_NON_MASKABLE_INTERRUPT_SOURCE:
              NMISource = (EFI_ACPI_2_0_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE *) (UINTN) (AddrApic);

              NMISource++;
              AddrApic = (UINTN) NMISource;
              Type = NMISource->Type;
              ApicLength += NMISource->Length;
#ifdef ACPI_DEBUG
              Print (L"AddrApic = 0x%x\r\n", AddrApic);
              Print (L"Next Type = %d\r\n", Type);
#endif
              break;

            case EFI_ACPI_2_0_LOCAL_APIC_NMI:
              LocalApicNMI = (EFI_ACPI_2_0_LOCAL_APIC_NMI_STRUCTURE *) (UINTN) (AddrApic);
#ifdef ACPI_DEBUG
              Print (L" AcpiProcessorId = 0x%x, Flags = 0x%x, Length = %d\r\n", LocalApicNMI->AcpiProcessorId, LocalApicNMI->Flags, LocalApicNMI->Length);
#endif
              LocalApicNMI++;
              AddrApic = (UINTN) LocalApicNMI;
              Type = LocalApicNMI->Type;
              ApicLength += sizeof (EFI_ACPI_2_0_LOCAL_APIC_NMI_STRUCTURE);
#ifdef ACPI_DEBUG
              Print (L"AddrApic = 0x%x\r\n", AddrApic);
              Print (L"Next Type = %d\r\n", Type);
#endif
              break;

            case EFI_ACPI_2_0_LOCAL_APIC_ADDRESS_OVERRIDE:
              LocalApicAdrrOverride = (EFI_ACPI_2_0_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE *) (UINTN) (AddrApic);

              LocalApicAdrrOverride++;
              AddrApic = (UINTN) LocalApicAdrrOverride;
              Type = LocalApicAdrrOverride->Type;
              ApicLength += LocalApicAdrrOverride->Length;
#ifdef ACPI_DEBUG
              Print (L"AddrApic = 0x%x\r\n", AddrApic);
              Print (L"Next Type = %d\r\n", Type);
#endif
              break;

            case EFI_ACPI_2_0_IO_SAPIC:
              IoSapic = (EFI_ACPI_2_0_IO_SAPIC_STRUCTURE *) (UINTN) (AddrApic);

              IoSapic++;
              AddrApic = (UINTN) &IoSapic;
              Type = IoSapic->Type;
              ApicLength += IoSapic->Length;
#ifdef ACPI_DEBUG
              Print (L"AddrApic = 0x%x\r\n", AddrApic);
              Print (L"Next Type = %d\r\n", Type);
#endif
              break;

            case EFI_ACPI_2_0_PROCESSOR_LOCAL_SAPIC:
              ProcessorLocalSapic = (EFI_ACPI_2_0_PROCESSOR_LOCAL_SAPIC_STRUCTURE *) (UINTN) (AddrApic);

              ProcessorLocalSapic++;
              AddrApic = (UINTN) &ProcessorLocalSapic;
              Type = ProcessorLocalSapic->Type;
              ApicLength += ProcessorLocalSapic->Length;
#ifdef ACPI_DEBUG
              Print (L"AddrApic = 0x%x\r\n", AddrApic);
              Print (L"Next Type = %d\r\n", Type);
#endif
              break;

            case EFI_ACPI_2_0_PLATFORM_INTERRUPT_SOURCES:
              PlatformIntSource = (EFI_ACPI_2_0_PLATFORM_INTERRUPT_SOURCES_STRUCTURE *) (UINTN) (AddrApic);

              PlatformIntSource++;
              AddrApic = (UINTN) &PlatformIntSource;
              Type = PlatformIntSource->Type;
              ApicLength += PlatformIntSource->Length;
#ifdef ACPI_DEBUG
              Print (L"AddrApic = 0x%x\r\n", AddrApic);
              Print (L"Next Type = %d\r\n", Type);
#endif
              break;
              
            default:
              Print (L"subtable type error\r\n");
              break;
          }
        }
      }

      *(UINT32 *)(UINTN) (0xfee00000 + 0x360) = 0x400;
      
#ifdef ACPI_DEBUG
      Print (L" Checksum = 0x%x\r\n", ApicTable->Checksum);
#endif
    } else {
      Print (L"No APIC table Found or parse error !!!\r\n");
    }  
#ifdef ACPI_DEBUG
    Pause (NULL);
    Pause (NULL);
    Pause (NULL);
#endif
  }
  
  // --------------------
  
  BiosDsdt = FadtPointer->XDsdt;

  if (BiosDsdt == 0) {
    BiosDsdt = FadtPointer->Dsdt;
  }

  Facs = (EFI_ACPI_4_0_FIRMWARE_ACPI_CONTROL_STRUCTURE*) (UINTN) (FadtPointer->FirmwareCtrl);
  BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
  Status = gBS->AllocatePages (AllocateMaxAddress, EfiACPIReclaimMemory, 1, &BufferPtr);

  if (!EFI_ERROR (Status)) {
    newFadt = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE*) (UINTN) BufferPtr;
    CopyMem ((UINT8*) newFadt, (UINT8*) FadtPointer, ((EFI_ACPI_DESCRIPTION_HEADER*) FadtPointer)->Length);

    if ((newFadt->Header.Revision == EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION) ||
        (newFadt->Header.Length < 0xF4)) {
      newFadt->Header.Length = 0xF4;
      newFadt->Header.Revision = EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION;
      newFadt->Reserved0 = 0; //ACPIspec said it should be 0, while 1 is possible, but no more
      newFadt->IaPcBootArch = 0x3;
      // Reset Register Supported
      newFadt->Flags |= EFI_ACPI_2_0_RESET_REG_SUP; 
      newFadt->ResetReg.AddressSpaceId     = 1;
      newFadt->ResetReg.RegisterBitWidth   = 8;
      newFadt->ResetReg.RegisterBitOffset  = 0;
      newFadt->ResetReg.Reserved           = 1;
      newFadt->Reserved2[0]        = 0;
      newFadt->Reserved2[1]        = 0;
      newFadt->Reserved2[2]        = 0;
      if (gSettings.ResetAddr == 0) {
        newFadt->ResetReg.Address = 0x64;
      }
      if (gSettings.ResetVal == 0) {
        newFadt->ResetValue = 0xFE;
      }
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
    }
    
    CopyMem ((UINT8*) newFadt->Header.OemId, (UINT8*) "APPLE  ", 6);

    if (gSettings.PMProfile != 0) {
      newFadt->PreferredPmProfile = gSettings.PMProfile;
    } else {
      newFadt->PreferredPmProfile = gMobile ? 2 : 1;
    }
    
    if ((gSettings.ResetAddr != 0) &&
        (gSettings.ResetVal != 0)) {
      newFadt->Flags |= 0x400;
      newFadt->ResetReg.Address = gSettings.ResetAddr;
      newFadt->ResetValue = gSettings.ResetVal;
    }

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
      
    FadtPointer = (EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE*) newFadt;
    FadtPointer->Header.Checksum = 0;
    FadtPointer->Header.Checksum = (UINT8) (256 - CalculateSum8 ((UINT8*) FadtPointer, FadtPointer->Header.Length));

    // We are sure that Fadt is the first entry in RSDT/XSDT table
    if (Rsdt != NULL) {
      Rsdt->Entry = (UINT32) (UINTN) newFadt;
    }

    if (Xsdt != NULL) {
      Xsdt->Entry = (UINT64) ((UINT32) (UINTN) newFadt);
    }
  }

  // DropSSDT = Yes
  if (gSettings.DropSSDT) {
    DropTableFromRSDT (EFI_ACPI_4_0_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE);
    DropTableFromXSDT (EFI_ACPI_4_0_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE);
  }
  
  // Load SSDTs
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
    Rsdt->Header.Checksum = (UINT8) (256 - CalculateSum8 ((UINT8*) Rsdt, Rsdt->Header.Length));
  }

  if (Xsdt) {
    Xsdt->Header.Checksum = 0;
    Xsdt->Header.Checksum = (UINT8) (256 - CalculateSum8 ((UINT8*) Xsdt, Xsdt->Header.Length));
  }

#if 0
  Pause (NULL);
#endif
  return EFI_SUCCESS;
}
