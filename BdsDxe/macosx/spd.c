/*
 * spd.c - serial presence detect memory information
 implementation for reading memory spd

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 *
 * Originally restored from pcefi10.5 by netkas
 * Dynamic mem detection original impl. by Rekursor
 * System profiler fix and other fixes by Mozodojo.
 * Slice 2011  remade for UEFI
 */

#include <macosx.h>

#include "spd.h"
#include "memvendors.h"
#include "Library/IoLib.h"

#define SPD_TO_SMBIOS_SIZE (sizeof(spd_mem_to_smbios)/sizeof(UINT8))
#define READ_SPD(spd, base, slot, x) spd[x] = smb_read_byte_intel(base, 0x50 + slot, x)
#define SMST(a) ((UINT8)((spd[a] & 0xf0) >> 4))
#define SLST(a) ((UINT8)(spd[a] & 0x0f))
#define PCI_COMMAND_OFFSET 0x04

CHAR8 *spd_memory_types[] = {
  "RAM",          /* 00h  Undefined */
  "FPM",          /* 01h  FPM */
  "EDO",          /* 02h  EDO */
  "",             /* 03h  PIPELINE NIBBLE */
  "SDRAM",        /* 04h  SDRAM */
  "",             /* 05h  MULTIPLEXED ROM */
  "DDR SGRAM",    /* 06h  SGRAM DDR */
  "DDR SDRAM",    /* 07h  SDRAM DDR */
  "DDR2 SDRAM",   /* 08h  SDRAM DDR 2 */
  "",             /* 09h  Undefined */
  "",             /* 0Ah  Undefined */
  "DDR3 SDRAM"    /* 0Bh  SDRAM DDR 3 */
};

UINT8 spd_mem_to_smbios[] = {
  UNKNOWN_MEM_TYPE,   /* 00h  Undefined */
  UNKNOWN_MEM_TYPE,   /* 01h  FPM */
  UNKNOWN_MEM_TYPE,   /* 02h  EDO */
  UNKNOWN_MEM_TYPE,   /* 03h  PIPELINE NIBBLE */
  SMB_MEM_TYPE_SDRAM,   /* 04h  SDRAM */
  SMB_MEM_TYPE_ROM,   /* 05h  MULTIPLEXED ROM */
  SMB_MEM_TYPE_SGRAM,   /* 06h  SGRAM DDR */
  SMB_MEM_TYPE_DDR,   /* 07h  SDRAM DDR */
  SMB_MEM_TYPE_DDR2,    /* 08h  SDRAM DDR 2 */
  UNKNOWN_MEM_TYPE,   /* 09h  Undefined */
  UNKNOWN_MEM_TYPE,   /* 0Ah  Undefined */
  SMB_MEM_TYPE_DDR3   /* 0Bh  SDRAM DDR 3 */
};

PCI_TYPE00                      mPci;

/** Read one byte from the intel i2c, used for reading SPD on intel chipsets only. */

UINT8
smb_read_byte_intel (
  UINT32 base,
  UINT8 adr,
  UINT8 cmd
)
{
  UINT64 t, t1, t2;

  IoWrite8 (base + SMBHSTSTS, 0x1f);        // reset SMBus Controller
  IoWrite8 (base + SMBHSTDAT, 0xff);
  t1 = AsmReadTsc(); //rdtsc(l1, h1);

  while (IoRead8 (base + SMBHSTSTS) & 0x01) {  // wait until read
    t2 = AsmReadTsc(); //rdtsc(l2, h2);
    t = DivU64x32 ((t2 - t1), (UINT32) DivU64x32 (gCPUStructure.TSCFrequency, 100));

    if (t > 5) {
      return 0xFF;  // break
    }
  }

  IoWrite8 (base + SMBHSTCMD, cmd);
  IoWrite8 (base + SMBHSTADD, (adr << 1) | 0x01);
  IoWrite8 (base + SMBHSTCNT, 0x48);
  t1 = AsmReadTsc();

  while (!(IoRead8 (base + SMBHSTSTS) & 0x02)) { // wait til command finished
    t2 = AsmReadTsc();
    t = DivU64x32 (t2 - t1, (UINT32) DivU64x32 (gCPUStructure.TSCFrequency, 100));

    if (t > 5) {
      break;  // break after 5ms
    }
  }

  return IoRead8 (base + SMBHSTDAT);
}

/** Get Vendor Name from spd, 2 cases handled DDR3 and DDR2,
    have different formats, always return a valid ptr.*/

CHAR8*
getVendorName (
  RAM_SLOT_INFO* slot,
  UINT32 base,
  UINT8 slot_num
)
{
  UINT8 bank;
  UINT8 code;
  INTN  i;
  UINT8 * spd;

  bank = 0;
  code = 0;
  i = 0;
  spd = (UINT8 *) slot->spd;

  if (spd[SPD_MEMORY_TYPE] == SPD_MEMORY_TYPE_SDRAM_DDR3) { // DDR3
    bank = (spd[SPD_DDR3_MEMORY_BANK] & 0x07f); // constructors like Patriot use b7=1
    code = spd[SPD_DDR3_MEMORY_CODE];

    for (i = 0; i < VEN_MAP_SIZE; i++)
      if (bank == vendorMap[i].bank && code == vendorMap[i].code) {
        return vendorMap[i].name;
      }
  } else if (spd[SPD_MEMORY_TYPE] == SPD_MEMORY_TYPE_SDRAM_DDR2) {
    if (spd[64] == 0x7f) {
      for (i = 64; i < 72 && spd[i] == 0x7f; i++) {
        bank++;
        READ_SPD (spd, base, slot_num, (UINT8) (i + 1)); // prefetch next spd byte to read for next loop
      }

      READ_SPD (spd, base, slot_num, (UINT8) i);
      code = spd[i];
    } else {
      code = spd[64];
      bank = 0;
    }

    for (i = 0; i < VEN_MAP_SIZE; i++)
      if (bank == vendorMap[i].bank && code == vendorMap[i].code) {
        return vendorMap[i].name;
      }
  }

  /* OK there is no vendor id here lets try to match the partnum if it exists */
  if (AsciiStrStr (slot->PartNo, "GU332") == slot->PartNo) { // Unifosa fingerprint
    return "Unifosa";
  }

  return "NoName";
}

/** Get Default Memory Module Speed (no overclocking handled) */

UINT16
getDDRspeedMhz (
  UINT8 * spd
)
{
  UINT16  divisor = spd[10];
  UINT16  dividend = spd[11];
  UINT16  ratio = spd[12];
  UINT32  EppIdentifierString;
  UINT8   ProfileForOptimal;
  UINT8   CycleTime;

  divisor = 0;
  dividend = 0;
  ratio = 0;
  CycleTime = 0;

  // Check if an XMP profile is enabled
  // thnx to apianti!
  if (spd[SPD_MEMORY_TYPE] == SPD_MEMORY_TYPE_SDRAM_DDR3) {
    divisor = spd[10];
    dividend = spd[11];
    ratio = spd[12];
    if ((spd[SPD_XMP_SIG1] == SPD_XMP_SIG1_VALUE) &&
       (spd[SPD_XMP_SIG2] == SPD_XMP_SIG2_VALUE) &&
       ((spd[SPD_XMP_PROFILES] & 3) != 0)) {
     if ((spd[SPD_XMP_PROFILES] & 3) == 1) {
       // Use first profile
       divisor = spd[SPD_XMP_PROF1_DIVISOR];
       dividend = spd[SPD_XMP_PROF1_DIVIDEND];
       ratio = spd[SPD_XMP_PROF1_RATIO];
     } else {
       // Use second profile
       divisor = spd[SPD_XMP_PROF2_DIVISOR];
       dividend = spd[SPD_XMP_PROF2_DIVIDEND];
       ratio = spd[SPD_XMP_PROF2_RATIO];
     }
     // Check values are sane
     if ((dividend != 0) && (divisor != 0) && (ratio != 0)) {
       // Convert to MHz from nanoseconds - 2 * (1000 / nanoseconds)
       return ((2000 * dividend) / (divisor * ratio));
     }
   }
    // Check values are sane
    if ((dividend != 0) && (divisor != 0) && (ratio != 0)) {
      // Convert to MHz from nanoseconds - 2 * (1000 / nanoseconds)
      return ((2000 * dividend) / (divisor * ratio));
    } else {
      switch (spd[12])  {
        case 0x0f:
          return 1066;

        case 0x0c:
          return 1333;

        case 0x0a:
          return 1600;

        case 0x14:
        default:
          return 800;
      }
    }

  } else if (spd[SPD_MEMORY_TYPE] == SPD_MEMORY_TYPE_SDRAM_DDR2)  {
    EppIdentifierString = (spd[EPP_IDENTIFIER_STRING + 2] << 16) +
                          (spd[EPP_IDENTIFIER_STRING + 1] << 8) +
                          spd[EPP_IDENTIFIER_STRING];
    if (EppIdentifierString == 0x4E566D) {
      ProfileForOptimal = spd[PROFILE_FOR_OPTIMAL_PERFORMANCE] & 0x3;
      if (spd[EPP_PROFILE_TYPE_IDENTIFIER] == 0xB1) {
        switch (ProfileForOptimal) {
          case 0:
            CycleTime = spd[CYCLE_TIME_FULL_PF0];
            break;
          case 1:
            CycleTime = spd[CYCLE_TIME_FULL_PF1];
            break;

          default:
            break;
        }
        switch (CycleTime) {
          case 0x1E:
            return 1066;

          case 0x25:
          default:
            return 800;
        }
      } else if (spd[EPP_PROFILE_TYPE_IDENTIFIER] == 0xA1) {
        switch (ProfileForOptimal) {
          case 0:
            CycleTime = spd[CYCLE_TIME_ABBR_PF0];
            break;
          case 1:
            CycleTime = spd[CYCLE_TIME_ABBR_PF1];
            break;
          case 2:
            CycleTime = spd[CYCLE_TIME_ABBR_PF2];
            break;
          case 3:
            CycleTime = spd[CYCLE_TIME_ABBR_PF3];
            break;

          default:
            break;
        }
        switch (CycleTime) {
          case 0x1E:
            return 1066;

          case 0x25:
          default:
            return 800;
        }
      }
    }
    switch (spd[9]) {
      case 0x50:
        return 400;

      case 0x3d:
        return 533;

      case 0x30:
        return 667;

      case 0x25:
      default:
        return 800;
    }
  }
  return 800; // default freq for unknown types //shit! DDR1 = 533
}

/** Get DDR3 or DDR2 serial number, 0 most of the times, always return a valid ptr */

CHAR8*
getDDRSerial (
  UINT8* spd
)
{
  CHAR8* asciiSerial; //[16];

  asciiSerial = AllocateZeroPool (16);

  if (spd[SPD_MEMORY_TYPE] == SPD_MEMORY_TYPE_SDRAM_DDR3) { 
    AsciiSPrint (asciiSerial, 16, "%X%X%X%X%X%X%X%X",
                 SMST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR3),
                 SLST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR3),
                 SMST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR3 + 1),
                 SLST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR3 + 1),
                 SMST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR3 + 2),
                 SLST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR3 + 2),
                 SMST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR3 + 3),
                 SLST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR3 + 3)
                 );
  } else if (spd[SPD_MEMORY_TYPE] == SPD_MEMORY_TYPE_SDRAM_DDR2) {
    AsciiSPrint (asciiSerial, 16, "%X%X%X%X%X%X%X%X",
                 SMST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR2),
                 SLST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR2),
                 SMST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR2 + 1),
                 SLST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR2 + 1),
                 SMST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR2 + 2),
                 SLST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR2 + 2),
                 SMST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR2 + 3),
                 SLST (SPD_ASSEMBLY_SERIAL_NUMBER_DDR2 + 3)
                 );
  }

  return asciiSerial;
}

/** Get DDR3 or DDR2 Part Number, always return a valid ptr */

CHAR8*
getDDRPartNum (
  UINT8* spd,
  UINT32 base,
  INTN slot
)
{
  CHAR8* asciiPartNo; //[32];
  INTN i, start, index;
  CHAR8 c;

  start = 0;
  index = 0;
  asciiPartNo = AllocateZeroPool (32);

  if (spd[SPD_MEMORY_TYPE] == SPD_MEMORY_TYPE_SDRAM_DDR3) {
    start = SPD_MANUFACTURER_PART_NUMBER_DDR3;
  } else if (spd[SPD_MEMORY_TYPE] == SPD_MEMORY_TYPE_SDRAM_DDR2) {
    start = SPD_MANUFACTURER_PART_NUMBER_DDR2;
  }

  for (i = start; i < start + 32; i++) {
    READ_SPD (spd, base, (UINT8) slot, (UINT8) i);
    c = spd[i];
    if (IS_ALPHA (c) || IS_DIGIT (c) || IS_PUNCT (c)) {
      asciiPartNo[index++] = c;
    } else if (c < 0x20) {
      break;
    }
  }

  return asciiPartNo;
}

/** Read from smbus the SPD content and interpret it for detecting memory attributes */

VOID
read_smb_intel (
  EFI_PCI_IO_PROTOCOL *PciIo
)
{
  INTN            i, i2, maxslots;
  UINT8           spd_type;
  UINT32          base, mmio, hostc;
  UINT16          Command;
  RAM_SLOT_INFO*  slot;

  (void) PciIo->Pci.Read (
             PciIo,
             EfiPciIoWidthUint16,
             PCI_COMMAND_OFFSET,
             1,
             &Command
           );
  Command |= 1;
  (void) PciIo->Pci.Write (
             PciIo,
             EfiPciIoWidthUint16,
             PCI_COMMAND_OFFSET,
             1,
             &Command
           );
  (void) PciIo->Pci.Read (
             PciIo,
             EfiPciIoWidthUint32,
             0x10,
             1,
             &mmio
           );
  (void) PciIo->Pci.Read (
             PciIo,
             EfiPciIoWidthUint32,
             0x20,
             1,
             &base
           );
  base &= 0xFFFE;
  (void) PciIo->Pci.Read (
             PciIo,
             EfiPciIoWidthUint32,
             0x40,
             1,
             &hostc
           );
  maxslots = gRAM->MaxMemorySlots <= 2? 3: gRAM->MaxMemorySlots;
  for (i = 0; i <  maxslots; i++) {
    slot = &gRAM->DIMM[i];
    slot->SpdSize = smb_read_byte_intel (base, (UINT8) (0x50 + i), 0);
    slot->InUse = FALSE;

    // Check spd is present
    if ((slot->SpdSize != 0) && (slot->SpdSize != 0xff)) {
      gRAM->SpdDetected = TRUE;
      slot->InUse = TRUE;

      slot->spd = AllocateZeroPool (MAX_SPD_SIZE);
      for (i2 = 0; i2 < slot->SpdSize; i2++) {
        slot->spd[i2] = smb_read_byte_intel(base, (UINT8) (0x50 + i), (UINT8) i2);
      }

      switch (slot->spd[SPD_MEMORY_TYPE])  {
        case SPD_MEMORY_TYPE_SDRAM_DDR2:
          slot->ModuleSize = ((1 << ((slot->spd[SPD_NUM_ROWS] & 0x0f) +
                                     (slot->spd[SPD_NUM_COLUMNS] & 0x0f) - 17)) *
                                     ((slot->spd[SPD_NUM_DIMM_BANKS] & 0x7) + 1) *
                                     slot->spd[SPD_NUM_BANKS_PER_SDRAM]);
          break;

        case SPD_MEMORY_TYPE_SDRAM_DDR3:
          slot->ModuleSize = ((slot->spd[4] & 0x0f) + 28) + ((slot->spd[8] & 0x7)  + 3);
          slot->ModuleSize -= (slot->spd[7] & 0x7) + 25;
          slot->ModuleSize = ((1 << slot->ModuleSize) * (((slot->spd[7] >> 3) & 0x1f) + 1));
          break;
      }

      DBG ("Spd: %d module size = %d\n", i, slot->ModuleSize);
      
      spd_type = (slot->spd[SPD_MEMORY_TYPE] < ((UINT8) 12) ? slot->spd[SPD_MEMORY_TYPE] : 0);
      slot->Type = spd_mem_to_smbios[spd_type];
      slot->PartNo = getDDRPartNum (slot->spd, base, (UINT8) i);
      slot->Vendor = getVendorName (slot, base, (UINT8) i);
      slot->SerialNo = getDDRSerial (slot->spd);
      slot->Frequency = getDDRspeedMhz (slot->spd);
    }
  }
  // laptops sometimes show slot 0 and 2 with slot 1 empty when only 2 slots are presents so:
  // for laptops case, mapping setup would need to be more generic than this
  if (gRAM->MaxMemorySlots == 2 &&
      gRAM->DIMM[1].InUse == FALSE &&
      gRAM->DIMM[2].InUse == TRUE) {
    CopyMem ((VOID *) &gRAM->DIMM[1], (VOID *) &gRAM->DIMM[2], sizeof (gRAM->DIMM[1]));
    if (gRAM->DIMM[1].InUse) {
      DBG ("Spd: gRAM->DIMM[1].InUse = TRUE\n");
    } else {
      DBG ("Spd: gRAM->DIMM[1].InUse = FALSE\n");
    }
  }
}

VOID
ScanSPD (
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

  /**
  * Scan PCI BUS For SmBus controller 
  **/
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
                         sizeof (mPci) / sizeof (UINT32),
                         &mPci
                       );
#if 0
              // SmBus controller has class = 0x0c0500
              if ((mPci.Hdr.ClassCode[2] == 0x0c) && (mPci.Hdr.ClassCode[1] == 5)
                   && (mPci.Hdr.ClassCode[0] == 0) && (mPci.Hdr.VendorId == 0x8086)) {
#endif
              if (mPci.Hdr.VendorId == 0x8086) {
                Status = PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);
                if ((Bus == 0) && (Device == 0x1F) && (Function == 3)) {
                  read_smb_intel (PciIo);
                }
              }
            }
          }
        }
      }
    }
  }
}
