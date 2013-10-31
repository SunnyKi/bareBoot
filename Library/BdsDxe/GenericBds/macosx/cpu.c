/*
 cpu.c
 implementation for cpu

 Remade by Slice 2011 based on Apple's XNU sources
 Portion copyright from Chameleon project. Thanks to all who involved to it.
 */
/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <macosx.h>

#include "cpu.h"
#include "Library/IoLib.h"

#define EAX 0
#define EBX 1
#define ECX 2
#define EDX 3

VOID
SetPIT2Mode0 (
  IN   UINT64  Value
)
{
  IoWrite8 (0x43, 0xB0);
  IoWrite8 (0x42, (UINT8) Value);
  IoWrite8 (0x42, (UINT8) (((UINT16) Value) >> 8));
}

UINT32 PollPIT2Gate (
  VOID
)
{
  UINT32 count;

  count = 0;
  do {
    count++;
  } while ((IoRead8 (0x61) & 0x20) == 0);

  return count;
}

UINT64
MeasureTSCFrequency (
  VOID
)
{
  UINT64 tscStart;
  UINT64 tscEnd;
  UINT64 tscDelta;
  UINT32 pollCount;
  UINT64 retval;
  UINTN i;

  tscDelta = 0xffffffffffffffffULL;
  retval = 0;

  for (i = 0; i < 3; ++i) {
    // enable PIT2
    IoAndThenOr8 (0x61, 0xFC, 0x01);
    SetPIT2Mode0 (CALIBRATE_LATCH);
    tscStart = AsmReadTsc();
    pollCount = PollPIT2Gate();
    tscEnd = AsmReadTsc();

    if (pollCount <= 1) {
      continue;
    }

    if ((tscEnd - tscStart) <= CALIBRATE_TIME_MSEC) {
      continue;
    }

    if ((tscEnd - tscStart) < tscDelta) {
      tscDelta = tscEnd - tscStart;
    }
  }

  if (tscDelta > 0x100000000ULL) {
    retval = 0;
  } else {
    retval = DivU64x32 (MultU64x32 (tscDelta, 1000), 30);
  }
  // disable PIT2
  IoAnd8 (0x61, 0xFC);
  return retval;
}

VOID
DoCpuid (
  UINT32 selector,
  UINT32 *data
)
{
  AsmCpuid (selector, data, data + 1, data + 2, data + 3);
}

VOID
DoCpuidEx (
  UINT32 selector1,
  UINT32 selector2,
  UINT32 *data
)
{
  AsmCpuidEx (selector1, selector2, data, data + 1, data + 2, data + 3);
}

VOID
GetCpuProps (
  VOID
)
{
  EFI_STATUS            Status;
  EFI_HANDLE            *HandleBuffer;
  EFI_GUID              **ProtocolGuidArray;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  PCI_TYPE00            Pci;
  UINT64                msr;
  UINT64                flex_ratio;
  UINTN                 HandleCount;
  UINTN                 ArrayCount;
  UINTN                 HandleIndex;
  UINTN                 ProtocolIndex;
  UINTN                 Segment;
  UINTN                 Bus;
  UINTN                 Device;
  UINTN                 Function;
  UINT32                reg[4];
  UINT32                qpimult;
  UINT32                multiplier;
  UINT16                qpibusspeed; //units=MHz
  UINT16                did;
  UINT16                vid;
  CHAR8                 str[128];
  CHAR8                 *s;

  msr = 0;
  flex_ratio = 0;
  qpimult = 2;
  multiplier = 0;
  s = NULL;
  gCPUStructure.TurboMsr = 0;
  
  gCPUStructure.MaxRatio = 10;
  gCPUStructure.ProcessorInterconnectSpeed = 0;

  AsmWriteMsr64 (MSR_IA32_BIOS_SIGN_ID, 0);
  gCPUStructure.MicroCode = RShiftU64 (AsmReadMsr64 (MSR_IA32_BIOS_SIGN_ID), 32);
  gCPUStructure.ProcessorFlag = RShiftU64 (AsmReadMsr64 (MSR_IA32_PLATFORM_ID), 50) & 3;

  DoCpuid (0x00000000, gCPUStructure.CPUID[CPUID_0]);
  DoCpuid (0x00000001, gCPUStructure.CPUID[CPUID_1]);
  DoCpuid (0x80000000, gCPUStructure.CPUID[CPUID_80]);

  gCPUStructure.Vendor      = gCPUStructure.CPUID[CPUID_0][EBX];
  gCPUStructure.Signature   = gCPUStructure.CPUID[CPUID_1][EAX];
  gCPUStructure.Stepping    = (UINT8) bitfield (gCPUStructure.CPUID[CPUID_1][EAX], 3, 0);
  gCPUStructure.Model       = (UINT8) bitfield (gCPUStructure.CPUID[CPUID_1][EAX], 7, 4);
  gCPUStructure.Family      = (UINT8) bitfield (gCPUStructure.CPUID[CPUID_1][EAX], 11, 8);
  gCPUStructure.Type        = (UINT8) bitfield (gCPUStructure.CPUID[CPUID_1][EAX], 13, 12);
  gCPUStructure.Extmodel    = (UINT8) bitfield (gCPUStructure.CPUID[CPUID_1][EAX], 19, 16);
  gCPUStructure.Extfamily   = (UINT8) bitfield (gCPUStructure.CPUID[CPUID_1][EAX], 27, 20);
  gCPUStructure.Features    = quad (gCPUStructure.CPUID[CPUID_1][ECX], gCPUStructure.CPUID[CPUID_1][EDX]);

  if (gCPUStructure.Family == 0x0f) {
    gCPUStructure.Family += gCPUStructure.Extfamily;
  }

  if ((gCPUStructure.Family == 0x0f) ||
      (gCPUStructure.Family == 0x06)) {
    gCPUStructure.Model       += (gCPUStructure.Extmodel << 4);
  }

  if (gCPUStructure.CPUID[CPUID_80][EAX] >= 0x80000001) {
    DoCpuid (0x80000001, gCPUStructure.CPUID[CPUID_81]);
    gCPUStructure.ExtFeatures = quad (gCPUStructure.CPUID[CPUID_81][ECX], gCPUStructure.CPUID[CPUID_81][EDX]);
  }

  if (gCPUStructure.CPUID[CPUID_80][EAX] >= 0x80000007) {
    DoCpuid (0x80000007, gCPUStructure.CPUID[CPUID_87]);
    gCPUStructure.ExtFeatures |= gCPUStructure.CPUID[CPUID_87][EDX] & (UINT32) CPUID_EXTFEATURE_TSCI;
  }  
  //
  // Cores & Threads count
  //
  // Number of logical processors per physical processor package
  gCPUStructure.LogicalPerPackage = bitfield (gCPUStructure.CPUID[CPUID_1][EBX], 23, 16);
#if 0
  if ( gCPUStructure.LogicalPerPackage > 1) {
    gCPUStructure.HTTEnabled = TRUE;
  } else {
    gCPUStructure.HTTEnabled = FALSE;
  }
#endif
  // Number of APIC IDs reserved per package
  DoCpuidEx (0x00000004, 0, gCPUStructure.CPUID[CPUID_4]);
  gCPUStructure.CoresPerPackage =  bitfield (gCPUStructure.CPUID[CPUID_4][EAX], 31, 26) + 1;
  
#if 0
  // Total number of threads serviced by this cache
  gCPUStructure.ThreadsPerCache =  bitfield (gCPUStructure.CPUID[CPUID_4][EAX], 25, 14) + 1;

  DoCpuidEx (0x0000000B, 0, gCPUStructure.CPUID[CPUID_0B]);
  gCPUStructure.Threads = (UINT8) bitfield (gCPUStructure.CPUID[CPUID_4][EBX], 15, 0);

  DoCpuidEx (0x0000000B, 1, gCPUStructure.CPUID[CPUID_0B]);
  gCPUStructure.Cores = (UINT8) bitfield (gCPUStructure.CPUID[CPUID_4][EBX], 15, 0);
#endif

  if ((gCPUStructure.CPUID[CPUID_1][ECX] & bit(9)) != 0) {
    SSSE3 = TRUE;
  }

  if (gCPUStructure.Vendor == CPU_VENDOR_INTEL) {
    switch (gCPUStructure.Model) {

      case CPU_MODEL_NEHALEM:
      case CPU_MODEL_FIELDS:
      case CPU_MODEL_CLARKDALE:
      case CPU_MODEL_NEHALEM_EX:
      case CPU_MODEL_JAKETOWN:
      case CPU_MODEL_SANDY_BRIDGE:
      case CPU_MODEL_IVY_BRIDGE:
      case CPU_MODEL_IVY_BRIDGE_E5:
        msr = AsmReadMsr64 (MSR_CORE_THREAD_COUNT);
        gCPUStructure.Cores   = (UINT8) bitfield ((UINT32) msr, 31, 16);
        gCPUStructure.Threads = (UINT8) bitfield ((UINT32) msr, 15,  0);
        break;

      case CPU_MODEL_DALES:
      case CPU_MODEL_WESTMERE:
      case CPU_MODEL_WESTMERE_EX:
        msr = AsmReadMsr64 (MSR_CORE_THREAD_COUNT);
        gCPUStructure.Cores   = (UINT8) bitfield ((UINT32) msr, 19, 16);
        gCPUStructure.Threads = (UINT8) bitfield ((UINT32) msr, 15,  0);
        break;

      default:
        gCPUStructure.Cores   = (UINT8) (gCPUStructure.CoresPerPackage & 0xff);
        gCPUStructure.Threads = (UINT8) (gCPUStructure.LogicalPerPackage & 0xff);
        break;
    }
  }
  //
  // Brand String
  //
  if (gCPUStructure.CPUID[CPUID_80][0] >= 0x80000004) {
    ZeroMem (str, 128);
    DoCpuid (0x80000002, reg);
    CopyMem (&str[0], (CHAR8 *) reg,  16);
    DoCpuid (0x80000003, reg);
    CopyMem (&str[16], (CHAR8 *) reg,  16);
    DoCpuid (0x80000004, reg);
    CopyMem (&str[32], (CHAR8 *) reg,  16);

    for (s = str; *s != '\0'; s++) {
      if (*s != ' ') {
        break;
      }
    }

    AsciiStrnCpy (gCPUStructure.BrandString, s, 48);

    if (AsciiStrStr ((CONST CHAR8*) gCPUStructure.BrandString, (CONST CHAR8*) CPU_STRING_UNKNOWN) != NULL ) {
      gCPUStructure.BrandString[0] = '\0';
    }

    gCPUStructure.BrandString[47] = '\0';
  }

  //
  // TSC calibration
  //
  gCPUStructure.CPUFrequency = MeasureTSCFrequency ();
  gCPUStructure.TSCFrequency = gCPUStructure.CPUFrequency;

  if ((gCPUStructure.Vendor == CPU_VENDOR_INTEL) &&
      (gCPUStructure.Family == 0x06) &&
      (gCPUStructure.Model >= 0x0c)) {

    switch (gCPUStructure.Model) {

      case CPU_MODEL_SANDY_BRIDGE:
      case CPU_MODEL_JAKETOWN:
      case CPU_MODEL_IVY_BRIDGE:
      case CPU_MODEL_IVY_BRIDGE_E5:
      case CPU_MODEL_HASWELL:
        msr = AsmReadMsr64 (MSR_PLATFORM_INFO);
        gCPUStructure.MaxRatio = (UINT8) (RShiftU64 (msr, 8) & 0xff);
        if (gCPUStructure.MaxRatio != 0) {
          gCPUStructure.FSBFrequency = DivU64x32 (gCPUStructure.CPUFrequency, gCPUStructure.MaxRatio);
        }
        msr = AsmReadMsr64 (MSR_FLEX_RATIO);
        if ((msr & 0x10000) != 0) {
          flex_ratio = (UINT8) RShiftU64 (msr, 8);

          if (flex_ratio == 0) {
            AsmWriteMsr64 (MSR_FLEX_RATIO, (msr & 0xFFFFFFFFFFFEFFFFULL));
            gBS->Stall (10);
            msr = AsmReadMsr64 (MSR_FLEX_RATIO);
          }
        }
        break;
    
      case CPU_MODEL_NEHALEM:     // Core i7 LGA1366, Xeon 5500, "Bloomfield", "Gainstown", 45nm
      case CPU_MODEL_FIELDS:      // Core i7, i5 LGA1156, "Clarksfield", "Lynnfield", "Jasper", 45nm
      case CPU_MODEL_DALES:       // Core i7, i5, Nehalem
      case CPU_MODEL_CLARKDALE:   // Core i7, i5, i3 LGA1156, "Westmere", "Clarkdale", , 32nm
      case CPU_MODEL_WESTMERE:    // Core i7 LGA1366, Six-core, "Westmere", "Gulftown", 32nm
      case CPU_MODEL_NEHALEM_EX:  // Core i7, Nehalem-Ex Xeon, "Beckton"
      case CPU_MODEL_WESTMERE_EX: // Core i7, Nehalem-Ex Xeon, "Eagleton"
        msr = AsmReadMsr64 (MSR_PLATFORM_INFO);
        gCPUStructure.MaxRatio = (UINT8) (RShiftU64(msr, 8) & 0xff);
        gCPUStructure.TurboMsr = msr + 1;
        if (gCPUStructure.MaxRatio != 0) {
          gCPUStructure.FSBFrequency = DivU64x32 (gCPUStructure.CPUFrequency, gCPUStructure.MaxRatio);
        }
        break;

      case CPU_MODEL_ATOM:        // Core i7 & Atom
      case CPU_MODEL_DOTHAN:      // Pentium M, Dothan, 90nm
      case CPU_MODEL_YONAH:       // Core Duo/Solo, Pentium M DC
      case CPU_MODEL_MEROM:       // Core Xeon, Core 2 DC, 65nm
      case CPU_MODEL_PENRYN:      // Core 2 Duo/Extreme, Xeon, 45nm
        msr = AsmReadMsr64 (MSR_IA32_PERF_STATUS);
        gCPUStructure.MaxRatio = ((UINT8) RShiftU64 (msr, 8)) & 0x1F;
        gCPUStructure.TurboMsr = ((UINT32) RShiftU64(msr, 40)) & 0x1F;
        gCPUStructure.SubDivider = ((UINT32) RShiftU64 (msr, 14)) & 0x1;
        gCPUStructure.MaxRatio = gCPUStructure.MaxRatio * 10 + gCPUStructure.SubDivider * 5;
        if (gCPUStructure.MaxRatio != 0) {
          gCPUStructure.FSBFrequency = DivU64x32 (
                                         MultU64x32 (gCPUStructure.CPUFrequency, 10),
                                         gCPUStructure.MaxRatio
                                       );
        }
        break;

      default:
        gCPUStructure.FSBFrequency = 100000000ull;
        break;
    }
#if 0
    if (AsmReadMsr64 (MSR_IA32_PLATFORM_ID) & (1 << 28)) {
      gCPUStructure.Mobile = TRUE;
    }
#endif
  }

  if ((gCPUStructure.Model == CPU_MODEL_NEHALEM) ||
      (gCPUStructure.Model == CPU_MODEL_NEHALEM_EX)) {
    Status = gBS->LocateHandleBuffer (AllHandles, NULL, NULL, &HandleCount, &HandleBuffer);

    if (!EFI_ERROR (Status)) {
      for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
        Status = gBS->ProtocolsPerHandle (HandleBuffer[HandleIndex], &ProtocolGuidArray, &ArrayCount);

        if (!EFI_ERROR (Status)) {
          for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {
            if (CompareGuid (&gEfiPciIoProtocolGuid, ProtocolGuidArray[ProtocolIndex])) {
              Status = gBS->OpenProtocol (
                              HandleBuffer[HandleIndex],
                              &gEfiPciIoProtocolGuid,
                              (VOID **) &PciIo,
                              gImageHandle,
                              NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL
                            );

              if (!EFI_ERROR (Status)) {
                Status = PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);

                if ((Bus & 0x3F) != 0x3F) {
                  continue;
                }

                Status = PciIo->Pci.Read (
                           PciIo,
                           EfiPciIoWidthUint32,
                           0,
                           sizeof (Pci) / sizeof (UINT32),
                           &Pci
                         );
                vid = Pci.Hdr.VendorId & 0xFFFF;
                did = Pci.Hdr.DeviceId & 0xFF00;

                if ((vid == 0x8086) &&
                    (did >= 0x2C00) &&
                    (Device == 2) &&
                    (Function == 1)) {
                  Status = PciIo->Mem.Read (
                             PciIo,
                             EfiPciIoWidthUint32,
                             EFI_PCI_IO_PASS_THROUGH_BAR,
                             0x50,
                             1,
                             &qpimult
                           );

                  if (EFI_ERROR (Status)) {
                    continue;
                  }

                  qpimult &= 0x7F;
                  break;
                }
              }
            }
          }
        }
      }
    }

    qpibusspeed = (UINT16) (qpimult * 2 * (UINT32) DivU64x32 (gCPUStructure.FSBFrequency, 1000000));

    if (qpibusspeed % 100 != 0) {
      qpibusspeed = ((qpibusspeed + 50) / 100) * 100;
    }

    gCPUStructure.ProcessorInterconnectSpeed = qpibusspeed;
  }

  DBG ("GetCpuProps: BrandString - %a\n", gCPUStructure.BrandString);
  DBG ("GetCpuProps: Vendor/Model/ExtModel: 0x%x/0x%x/0x%x\n", gCPUStructure.Vendor, gCPUStructure.Model, gCPUStructure.Extmodel);
  DBG ("GetCpuProps: Signature: 0x%x\n", gCPUStructure.Signature);
  DBG ("GetCpuProps: Family/ExtFamily:      0x%x/0x%x\n", gCPUStructure.Family,  gCPUStructure.Extfamily);
  DBG ("GetCpuProps: Features: 0x%08x\n", gCPUStructure.Features);
  DBG ("GetCpuProps: Cores: %d\n", gCPUStructure.Cores);
  DBG ("GetCpuProps: Threads: %d\n", gCPUStructure.Threads);
#if 0
  if (gCPUStructure.HTTEnabled) {
    DBG ("GetCpuProps: HTT enabled\n");
  } else {
    DBG ("GetCpuProps: HTT disabled\n");
  }
#endif
  DBG ("GetCpuProps: Number of logical processors per physical processor package: %d\n", gCPUStructure.LogicalPerPackage);
  DBG ("GetCpuProps: Number of APIC IDs reserved per package: %d\n", gCPUStructure.CoresPerPackage);
  DBG ("GetCpuProps: FSB: %lld Hz\n", gCPUStructure.FSBFrequency);
  DBG ("GetCpuProps: TSC: %lld Hz\n", gCPUStructure.TSCFrequency);
  DBG ("GetCpuProps: CPU: %lld Hz\n", gCPUStructure.CPUFrequency);
  DBG ("GetCpuProps: ProcessorInterconnectSpeed: %d MHz\n", gCPUStructure.ProcessorInterconnectSpeed);

  return;
}
