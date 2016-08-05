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

/* nms was here */

#include <macosx.h>
#include <Library/IoLib.h>

#include "cpu.h"

#define EAX 0
#define EBX 1
#define ECX 2
#define EDX 3

CPU_STRUCTURE                   gCPUStructure;
BOOLEAN                         SSSE3;

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

  tscDelta = ~((UINT64) 0);
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
ACpuProps (
  VOID
)
{
  UINT64                msr;

  DoCpuid(0x80000008, gCPUStructure.CPUID[CPUID_88]);

  gCPUStructure.CoresPerPackage =  ((UINT8) gCPUStructure.CPUID[CPUID_88][ECX]) + 1;

  if (gCPUStructure.Extfamily == 0x00) {
    msr = AsmReadMsr64 (K8_FIDVID_STATUS);
    gCPUStructure.MaxRatio = (UINT32) (RShiftU64 ((RShiftU64 (msr, 16) & 0x3f), 2) + 4);
  } else {
    msr = AsmReadMsr64 (K10_COFVID_STATUS);
    gCPUStructure.MaxRatio = (UINT32) DivU64x32 (((msr & 0x3f) + 0x10),
                                                 (1 << ((RShiftU64 (msr, 6) & 0x7))));
  }
  if (gCPUStructure.MaxRatio == 0) {
    gCPUStructure.MaxRatio = 1;
  }
  gCPUStructure.FSBFrequency = DivU64x32 (LShiftU64 (gCPUStructure.TSCFrequency, 1),
                                          gCPUStructure.MaxRatio);
  gCPUStructure.MaxRatio *= 5;
}

VOID
ICpuNehalemProps (
  VOID
)
{
  EFI_STATUS            Status;
  EFI_HANDLE            *HandleBuffer;
  EFI_GUID              **ProtocolGuidArray;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  PCI_TYPE00            Pci;
  UINTN                 HandleCount;
  UINTN                 ArrayCount;
  UINTN                 HandleIndex;
  UINTN                 ProtocolIndex;
  UINTN                 Segment;
  UINTN                 Bus;
  UINTN                 Device;
  UINTN                 Function;
  UINT32                qpimult;
  UINT16                qpibusspeed; //units=MHz
  UINT16                did;
  UINT16                vid;

  qpimult = 2;

  Status = gBS->LocateHandleBuffer (AllHandles, NULL, NULL, &HandleCount, &HandleBuffer);

  if (!EFI_ERROR (Status)) {
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      Status = gBS->ProtocolsPerHandle (HandleBuffer[HandleIndex], &ProtocolGuidArray, &ArrayCount);

      if (EFI_ERROR (Status)) {
        continue;
      }
      for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {
        if (!CompareGuid (&gEfiPciIoProtocolGuid, ProtocolGuidArray[ProtocolIndex])) {
          continue;
        }
        Status = gBS->OpenProtocol (
                        HandleBuffer[HandleIndex],
                        &gEfiPciIoProtocolGuid,
                        (VOID **) &PciIo,
                        gImageHandle,
                        NULL,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );

        if (EFI_ERROR (Status)) {
          continue;
        }
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

  qpibusspeed = (UINT16) (qpimult * 2 * (UINT32) DivU64x32 (gCPUStructure.FSBFrequency, 1000000));

  if (qpibusspeed % 100 != 0) {
    qpibusspeed = ((qpibusspeed + 50) / 100) * 100;
  }

  gCPUStructure.ProcessorInterconnectSpeed = qpibusspeed;
}

VOID
ICpuFamily06 (
  VOID
)
{
  UINT64                msr;
  UINT64                flex_ratio;

  msr = 0;
  flex_ratio = 0;
  
  /* MaxRatio & FSBFrequency */

  switch (gCPUStructure.Model) {
  case CPU_MODEL_HASWELL:
  case CPU_MODEL_HASWELL_MB:
  case CPU_MODEL_HASWELL_ULT:
//  case CPU_MODEL_HASWELL_ULX:
  case CPU_MODEL_IVY_BRIDGE:
  case CPU_MODEL_IVY_BRIDGE_E5:
  case CPU_MODEL_JAKETOWN:
  case CPU_MODEL_SANDY_BRIDGE:
    msr = AsmReadMsr64 (MSR_PLATFORM_INFO);
    gCPUStructure.MaxRatio = (UINT8) msr;
    if (gCPUStructure.MaxRatio != 0) {
      gCPUStructure.FSBFrequency = DivU64x32 (gCPUStructure.CPUFrequency, gCPUStructure.MaxRatio);
    }
    msr = AsmReadMsr64 (MSR_FLEX_RATIO);
    if ((msr & BIT16) != 0) {
      flex_ratio = (UINT8) BitFieldRead64 (msr, 8, 15);

      if (flex_ratio == 0) {
        /* XXX: No flex info avail, no need to bother */
        AsmWriteMsr64 (MSR_FLEX_RATIO, (msr & ~((UINT64) BIT16)));
        gBS->Stall (10);
        msr = AsmReadMsr64 (MSR_FLEX_RATIO);
      }
    }
    break;
  
  case CPU_MODEL_CLARKDALE:   // Core i7, i5, i3 LGA1156, "Westmere", "Clarkdale", , 32nm
  case CPU_MODEL_DALES:       // Core i7, i5, Nehalem
  case CPU_MODEL_FIELDS:      // Core i7, i5 LGA1156, "Clarksfield", "Lynnfield", "Jasper", 45nm
  case CPU_MODEL_NEHALEM:     // Core i7 LGA1366, Xeon 5500, "Bloomfield", "Gainstown", 45nm
  case CPU_MODEL_NEHALEM_EX:  // Core i7, Nehalem-Ex Xeon, "Beckton"
  case CPU_MODEL_WESTMERE:    // Core i7 LGA1366, Six-core, "Westmere", "Gulftown", 32nm
  case CPU_MODEL_WESTMERE_EX: // Core i7, Nehalem-Ex Xeon, "Eagleton"
  case CPU_MODEL_CRYSTALWELL:
  case CPU_MODEL_BROADWELL_HQ:
  case CPU_MODEL_SKYLAKE_U:
  case CPU_MODEL_SKYLAKE_S:
    msr = AsmReadMsr64 (MSR_PLATFORM_INFO);
    gCPUStructure.MaxRatio = (UINT8) BitFieldRead64 (msr, 8, 15);
    gCPUStructure.TurboMsr = msr + 1;  /* XXX: suspicious action! */
    if (gCPUStructure.MaxRatio != 0) {
      gCPUStructure.FSBFrequency = DivU64x32 (gCPUStructure.CPUFrequency, gCPUStructure.MaxRatio);
    }
    break;

  case CPU_MODEL_ATOM:        // Core i7 & Atom
  case CPU_MODEL_DOTHAN:      // Pentium M, Dothan, 90nm
  case CPU_MODEL_MEROM:       // Core Xeon, Core 2 DC, 65nm
  case CPU_MODEL_PENRYN:      // Core 2 Duo/Extreme, Xeon, 45nm
  case CPU_MODEL_YONAH:       // Core Duo/Solo, Pentium M DC
    msr = AsmReadMsr64 (MSR_IA32_PERF_STATUS);
    gCPUStructure.MaxRatio = (UINT8) BitFieldRead64 (msr, 8, 12);
    gCPUStructure.TurboMsr = (UINT32) BitFieldRead64(msr, 40, 44);
    gCPUStructure.SubDivider = (UINT32) BitFieldRead64 (msr, 46, 46);
    gCPUStructure.MaxRatio = gCPUStructure.MaxRatio * 10 + gCPUStructure.SubDivider * 5;
    if (gCPUStructure.MaxRatio != 0) {
      gCPUStructure.FSBFrequency = DivU64x32 (
                                     MultU64x32 (gCPUStructure.CPUFrequency, 10),
                                     gCPUStructure.MaxRatio
                                   );
    }
    break;

  default:
    gCPUStructure.FSBFrequency = 100000000ULL;
    break;
  }
#if 0
  if (AsmReadMsr64 (MSR_IA32_PLATFORM_ID) & (1 << 28)) {
    gCPUStructure.Mobile = TRUE;
  }
#endif
}

/*
 * NetBurst family
 */

VOID
ICpuFamily0F (
  VOID
)
{
  UINT64 msr;
  UINT8  sbspeed;  /* Scalable Bus Speed */

  gCPUStructure.FSBFrequency = 100000000ULL;

  if (gCPUStructure.Model < 2) {
    /* No documented ratio info */
    return;
  }

  msr = AsmReadMsr64(MSR_EBC_FREQUENCY_ID);

  gCPUStructure.MaxRatio = (UINT8) BitFieldRead64 (msr, 24, 31);
  sbspeed = (UINT8) BitFieldRead64 (msr, 16, 18);

  switch (sbspeed) {
  case 0:
    if (gCPUStructure.Model == 3 || gCPUStructure.Model == 4) {
      gCPUStructure.FSBFrequency = 266670000ULL;
    }
    break;

  case 1:
    gCPUStructure.FSBFrequency = 133330000ULL;
    break;

  case 2:
    gCPUStructure.FSBFrequency = 200000000ULL;
    break;

  case 3:
    gCPUStructure.FSBFrequency = 166670000ULL;
    break;

  case 4:
    if (gCPUStructure.Model == 6) {
      gCPUStructure.FSBFrequency = 333330000ULL;
    }
    break;

  default:
    break;
  }
  if (gCPUStructure.MaxRatio > 0) {
    gCPUStructure.FSBFrequency = DivU64x32 (gCPUStructure.CPUFrequency, gCPUStructure.MaxRatio);
  }
}

VOID
ICpuProps (
  VOID
)
{
  UINT64                msr;

  msr = 0;
  
  AsmWriteMsr64 (MSR_IA32_BIOS_SIGN_ID, 0);
  gCPUStructure.MicroCode = BitFieldRead64 (AsmReadMsr64 (MSR_IA32_BIOS_SIGN_ID), 32, 63);
  gCPUStructure.ProcessorFlag = BitFieldRead64 (AsmReadMsr64 (MSR_IA32_PLATFORM_ID), 50, 52);

  if (gCPUStructure.Family == 0x0f) {
    gCPUStructure.Family += gCPUStructure.Extfamily;
  }

  gCPUStructure.Model += (gCPUStructure.Extmodel << 4);

  DoCpuidEx (0x00000004, 0, gCPUStructure.CPUID[CPUID_4]);
  gCPUStructure.CoresPerPackage =  BitFieldRead32 (gCPUStructure.CPUID[CPUID_4][EAX], 26, 31) + 1;

  switch (gCPUStructure.Model) {
  case CPU_MODEL_CLARKDALE:
  case CPU_MODEL_FIELDS:
  case CPU_MODEL_HASWELL:
  case CPU_MODEL_HASWELL_MB:
  case CPU_MODEL_HASWELL_ULT:
  case CPU_MODEL_IVY_BRIDGE:
  case CPU_MODEL_IVY_BRIDGE_E5:
  case CPU_MODEL_JAKETOWN:
  case CPU_MODEL_NEHALEM:
  case CPU_MODEL_NEHALEM_EX:
  case CPU_MODEL_SANDY_BRIDGE:
  case CPU_MODEL_CRYSTALWELL:
  case CPU_MODEL_BROADWELL_HQ:
  case CPU_MODEL_SKYLAKE_U:
  case CPU_MODEL_SKYLAKE_S:
    msr = AsmReadMsr64 (MSR_CORE_THREAD_COUNT);
    gCPUStructure.Cores   = (UINT8) BitFieldRead64 (msr, 16, 31);
    gCPUStructure.Threads = (UINT8) BitFieldRead64 (msr, 0,  15);
    break;

  case CPU_MODEL_DALES:
  case CPU_MODEL_WESTMERE:
  case CPU_MODEL_WESTMERE_EX:
    msr = AsmReadMsr64 (MSR_CORE_THREAD_COUNT);
    gCPUStructure.Cores   = (UINT8) BitFieldRead64 (msr, 16, 19);
    gCPUStructure.Threads = (UINT8) BitFieldRead64 (msr, 0,  15);
    break;

  default:
    gCPUStructure.Cores   = (UINT8) gCPUStructure.CoresPerPackage;
    gCPUStructure.Threads = (UINT8) gCPUStructure.LogicalPerPackage;
    break;
  }

  /* MaxRatio & FSBFrequency */

  switch (gCPUStructure.Family) {
  case 0x06:  /* Family */
    ICpuFamily06 ();
    break;

  case 0x0F:  /* NetBurst Family */
    ICpuFamily0F ();
    break;

  default:  /* Unknown Family */
    break;
  }

  if ((gCPUStructure.Model == CPU_MODEL_NEHALEM) ||
      (gCPUStructure.Model == CPU_MODEL_NEHALEM_EX)) {
    ICpuNehalemProps ();
  }
}

VOID
GetCpuProps (
  VOID
)
{
  UINT32                reg[4];
  CHAR8                 str[128];
  CHAR8                 *s;

  SSSE3 = FALSE;
  gCPUStructure.TurboMsr = 0;
  gCPUStructure.MaxRatio = 10;
  gCPUStructure.ProcessorInterconnectSpeed = 0;

  DoCpuid (0x00000000, gCPUStructure.CPUID[CPUID_0]);
  DoCpuid (0x00000001, gCPUStructure.CPUID[CPUID_1]);
  DoCpuid (0x80000000, gCPUStructure.CPUID[CPUID_80]);

  gCPUStructure.Vendor      = gCPUStructure.CPUID[CPUID_0][EBX];
  gCPUStructure.Signature   = gCPUStructure.CPUID[CPUID_1][EAX];

  gCPUStructure.Stepping    = (UINT8) BitFieldRead32 (gCPUStructure.CPUID[CPUID_1][EAX], 0, 3);
  gCPUStructure.Model       = (UINT8) BitFieldRead32 (gCPUStructure.CPUID[CPUID_1][EAX], 4, 7);
  gCPUStructure.Family      = (UINT8) BitFieldRead32 (gCPUStructure.CPUID[CPUID_1][EAX], 8, 11);
  gCPUStructure.Type        = (UINT8) BitFieldRead32 (gCPUStructure.CPUID[CPUID_1][EAX], 12, 13);
  gCPUStructure.Extmodel    = (UINT8) BitFieldRead32 (gCPUStructure.CPUID[CPUID_1][EAX], 16, 19);
  gCPUStructure.Extfamily   = (UINT8) BitFieldRead32 (gCPUStructure.CPUID[CPUID_1][EAX], 20, 27);

  gCPUStructure.Features    = quad (gCPUStructure.CPUID[CPUID_1][ECX], gCPUStructure.CPUID[CPUID_1][EDX]);

  if (gCPUStructure.CPUID[CPUID_80][EAX] >= 0x80000001) {
    DoCpuid (0x80000001, gCPUStructure.CPUID[CPUID_81]);
    gCPUStructure.ExtFeatures = quad (gCPUStructure.CPUID[CPUID_81][ECX], gCPUStructure.CPUID[CPUID_81][EDX]);
  }

  if (gCPUStructure.CPUID[CPUID_80][EAX] >= 0x80000007) {
    DoCpuid (0x80000007, gCPUStructure.CPUID[CPUID_87]);
    gCPUStructure.ExtFeatures |= gCPUStructure.CPUID[CPUID_87][EDX] & (UINT32) CPUID_EXTFEATURE_TSCI;
  }  

  // Cores & Threads count
 
  gCPUStructure.LogicalPerPackage = BitFieldRead32 (gCPUStructure.CPUID[CPUID_1][EBX], 16, 23);

#if 0
  // Total number of threads serviced by this cache
  gCPUStructure.ThreadsPerCache =  bitfield (gCPUStructure.CPUID[CPUID_4][EAX], 25, 14) + 1;

  DoCpuidEx (0x0000000B, 0, gCPUStructure.CPUID[CPUID_0B]);
  gCPUStructure.Threads = (UINT8) bitfield (gCPUStructure.CPUID[CPUID_4][EBX], 15, 0);

  DoCpuidEx (0x0000000B, 1, gCPUStructure.CPUID[CPUID_0B]);
  gCPUStructure.Cores = (UINT8) bitfield (gCPUStructure.CPUID[CPUID_4][EBX], 15, 0);
#endif

  if ((gCPUStructure.CPUID[CPUID_1][ECX] & BIT9) != 0) {
    SSSE3 = TRUE;
  }

  // Brand String

  gCPUStructure.BrandString[0] = '\0';

  if (gCPUStructure.CPUID[CPUID_80][0] >= 0x80000004) {
    ZeroMem (str, sizeof (str));
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

    if (AsciiStrStr ((CONST CHAR8*) str, (CONST CHAR8*) CPU_STRING_UNKNOWN) == NULL ) {
      AsciiStrnCpy (gCPUStructure.BrandString, s, sizeof (gCPUStructure.BrandString));
      gCPUStructure.BrandString[sizeof (gCPUStructure.BrandString) - 1] = '\0';
    }
  }

  gCPUStructure.TSCFrequency = MeasureTSCFrequency ();
  gCPUStructure.CPUFrequency = gCPUStructure.TSCFrequency;

  switch (gCPUStructure.Vendor) {
  case CPU_VENDOR_AMD:
    ACpuProps();
    break;
  case CPU_VENDOR_INTEL:
    ICpuProps();
    break;
  default:
    break;
  }

  DBG ("%a: BrandString - %a\n", __FUNCTION__, gCPUStructure.BrandString);
  DBG ("%a: Vendor/Model/ExtModel: 0x%x/0x%x/0x%x\n", __FUNCTION__, gCPUStructure.Vendor, gCPUStructure.Model, gCPUStructure.Extmodel);
  DBG ("%a: Signature: 0x%x\n", __FUNCTION__, gCPUStructure.Signature);
  DBG ("%a: Family/ExtFamily:      0x%x/0x%x\n", __FUNCTION__, gCPUStructure.Family,  gCPUStructure.Extfamily);
  DBG ("%a: Features: 0x%08x\n", __FUNCTION__, gCPUStructure.Features);
  DBG ("%a: Cores: %d\n", __FUNCTION__, gCPUStructure.Cores);
  DBG ("%a: Threads: %d\n", __FUNCTION__, gCPUStructure.Threads);
#if 0
  DBG ("%a: HTT %a\n", __FUNCTION__, gCPUStructure.HTTEnabled ? "enabled" : "disabled");
#endif
  DBG ("%a: Number of logical processors per physical processor package: %d\n", __FUNCTION__, gCPUStructure.LogicalPerPackage);
  DBG ("%a: Number of APIC IDs reserved per package: %d\n", __FUNCTION__, gCPUStructure.CoresPerPackage);
  DBG ("%a: FSB: %lld Hz\n", __FUNCTION__, gCPUStructure.FSBFrequency);
  DBG ("%a: TSC: %lld Hz\n", __FUNCTION__, gCPUStructure.TSCFrequency);
  DBG ("%a: CPU: %lld Hz\n", __FUNCTION__, gCPUStructure.CPUFrequency);
  DBG ("%a: ProcessorInterconnectSpeed: %d MHz\n", __FUNCTION__, gCPUStructure.ProcessorInterconnectSpeed);
}
