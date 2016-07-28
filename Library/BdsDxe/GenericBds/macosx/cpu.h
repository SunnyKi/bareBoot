/* $Id: cpu.h $ */
/** @file
 * cpu.h
 */
#ifndef _CPU_H_
#define _CPU_H_

#define CLKNUM              1193182         /* formerly 1193167 1193182 */
#define CALIBRATE_TIME_MSEC 30 /* msecs */
#define CALIBRATE_LATCH     ((CLKNUM * CALIBRATE_TIME_MSEC + 1000/2)/1000)

/* CPUID Index */
#define CPUID_0   0
#define CPUID_1   1
#define CPUID_2   2
#define CPUID_3   3
#define CPUID_4   4
#define CPUID_80  5
#define CPUID_81  6
#define CPUID_87  7
#define CPUID_88  8
#define CPUID_0B  9

#define CPU_MODEL_DOTHAN        0x0D
#define CPU_MODEL_YONAH         0x0E
#define CPU_MODEL_MEROM         0x0F  /* same as CONROE but mobile */
#define CPU_MODEL_CELERON       0x16  /* ever see? */
#define CPU_MODEL_PENRYN        0x17
#define CPU_MODEL_NEHALEM       0x1A
#define CPU_MODEL_ATOM          0x1C
#define CPU_MODEL_XEON_MP       0x1D  /* ever see? */
#define CPU_MODEL_FIELDS        0x1E
#define CPU_MODEL_DALES         0x1F
#define CPU_MODEL_CLARKDALE     0x25
#define CPU_MODEL_LINCROFT      0x27
#define CPU_MODEL_SANDY_BRIDGE  0x2A
#define CPU_MODEL_WESTMERE      0x2C
#define CPU_MODEL_JAKETOWN      0x2D  /* ever see? */
#define CPU_MODEL_NEHALEM_EX    0x2E
#define CPU_MODEL_WESTMERE_EX   0x2F
#define CPU_MODEL_IVY_BRIDGE    0x3A
#define CPU_MODEL_IVY_BRIDGE_E5 0x3E
#define CPU_MODEL_HASWELL       0x3C  /* Haswell DT */
#define CPU_MODEL_HASWELL_MB    0x3F  /* Haswell MB */
#define CPU_MODEL_HASWELL_ULT   0x45  /* Haswell ULT */
#define CPU_MODEL_CRYSTALWELL   0x46
#define CPU_MODEL_BROADWELL_HQ  0x47
#define CPU_MODEL_SKYLAKE_U     0x4E
#define CPU_MODEL_SKYLAKE_S     0x5E

#define CPU_VENDOR_INTEL  0x756E6547
#define CPU_VENDOR_AMD    0x68747541
/* Unknown CPU */
#define CPU_STRING_UNKNOWN  "Unknown CPU Type"

/* CPU defines */
#define bit(n)      (1UL << (n))
#define _Bit(n)     (1ULL << n)
#define _HBit(n)    (1ULL << ((n) + 32))

#define bitmask(h,l)    ((bit(h) | (bit(h) - 1)) & ~(bit(l) - 1))
#define bitfield(x,h,l) (((x) & bitmask(h, l)) >> l)
#define quad(hi,lo)     (LShiftU64 ((hi), 32) | (lo))
/*
 * The CPUID_FEATURE_XXX values define 64-bit values
 * returned in %ecx:%edx to a CPUID request with %eax of 1:
 */
#define CPUID_FEATURE_FPU     _Bit(0) /* Floating point unit on-chip */
#define CPUID_FEATURE_VME     _Bit(1) /* Virtual Mode Extension */
#define CPUID_FEATURE_DE      _Bit(2) /* Debugging Extension */
#define CPUID_FEATURE_PSE     _Bit(3) /* Page Size Extension */
#define CPUID_FEATURE_TSC     _Bit(4) /* Time Stamp Counter */
#define CPUID_FEATURE_MSR     _Bit(5) /* Model Specific Registers */
#define CPUID_FEATURE_PAE     _Bit(6) /* Physical Address Extension */
#define CPUID_FEATURE_MCE     _Bit(7) /* Machine Check Exception */
#define CPUID_FEATURE_CX8     _Bit(8) /* CMPXCHG8B */
#define CPUID_FEATURE_APIC    _Bit(9) /* On-chip APIC */
#define CPUID_FEATURE_SEP     _Bit(11)  /* Fast System Call */
#define CPUID_FEATURE_MTRR    _Bit(12)  /* Memory Type Range Register */
#define CPUID_FEATURE_PGE     _Bit(13)  /* Page Global Enable */
#define CPUID_FEATURE_MCA     _Bit(14)  /* Machine Check Architecture */
#define CPUID_FEATURE_CMOV    _Bit(15)  /* Conditional Move Instruction */
#define CPUID_FEATURE_PAT     _Bit(16)  /* Page Attribute Table */
#define CPUID_FEATURE_PSE36   _Bit(17)  /* 36-bit Page Size Extension */
#define CPUID_FEATURE_PSN     _Bit(18)  /* Processor Serial Number */
#define CPUID_FEATURE_CLFSH   _Bit(19)  /* CLFLUSH Instruction supported */
#define CPUID_FEATURE_DS      _Bit(21)  /* Debug Store */
#define CPUID_FEATURE_ACPI    _Bit(22)  /* Thermal monitor and Clock Ctrl */
#define CPUID_FEATURE_MMX     _Bit(23)  /* MMX supported */
#define CPUID_FEATURE_FXSR    _Bit(24)  /* Fast floating pt save/restore */
#define CPUID_FEATURE_SSE     _Bit(25)  /* Streaming SIMD extensions */
#define CPUID_FEATURE_SSE2    _Bit(26)  /* Streaming SIMD extensions 2 */
#define CPUID_FEATURE_SS      _Bit(27)  /* Self-Snoop */
#define CPUID_FEATURE_HTT     _Bit(28)  /* Hyper-Threading Technology */
#define CPUID_FEATURE_TM      _Bit(29)  /* Thermal Monitor (TM1) */
#define CPUID_FEATURE_PBE     _Bit(31)  /* Pend Break Enable */
#define CPUID_FEATURE_SSE3    _HBit(0)  /* Streaming SIMD extensions 3 */
#define CPUID_FEATURE_PCLMULQDQ _HBit(1) /* PCLMULQDQ Instruction */
#define CPUID_FEATURE_MONITOR _HBit(3)  /* Monitor/mwait */
#define CPUID_FEATURE_DSCPL   _HBit(4)  /* Debug Store CPL */
#define CPUID_FEATURE_VMX     _HBit(5)  /* VMX */
#define CPUID_FEATURE_SMX     _HBit(6)  /* SMX */
#define CPUID_FEATURE_EST     _HBit(7)  /* Enhanced SpeedsTep (GV3) */
#define CPUID_FEATURE_TM2     _HBit(8)  /* Thermal Monitor 2 */
#define CPUID_FEATURE_SSSE3   _HBit(9)  /* Supplemental SSE3 instructions */
#define CPUID_FEATURE_CID     _HBit(10) /* L1 Context ID */
#define CPUID_FEATURE_CX16    _HBit(13) /* CmpXchg16b instruction */
#define CPUID_FEATURE_xTPR    _HBit(14) /* Send Task PRiority msgs */
#define CPUID_FEATURE_PDCM    _HBit(15) /* Perf/Debug Capability MSR */
#define CPUID_FEATURE_DCA     _HBit(18) /* Direct Cache Access */
#define CPUID_FEATURE_SSE4_1  _HBit(19) /* Streaming SIMD extensions 4.1 */
#define CPUID_FEATURE_SSE4_2  _HBit(20) /* Streaming SIMD extensions 4.2 */
#define CPUID_FEATURE_xAPIC   _HBit(21) /* Extended APIC Mode */
#define CPUID_FEATURE_POPCNT  _HBit(23) /* POPCNT instruction */
#define CPUID_FEATURE_AES     _HBit(25) /* AES instructions */
#define CPUID_FEATURE_VMM     _HBit(31) /* VMM (Hypervisor) present */
/*
 * The CPUID_EXTFEATURE_XXX values define 64-bit values
 * returned in %ecx:%edx to a CPUID request with %eax of 0x80000001:
 */
#define CPUID_EXTFEATURE_SYSCALL   _Bit(11) /* SYSCALL/sysret */
#define CPUID_EXTFEATURE_XD      _Bit(20) /* eXecute Disable */
#define CPUID_EXTFEATURE_1GBPAGE   _Bit(26)     /* 1G-Byte Page support */
#define CPUID_EXTFEATURE_RDTSCP    _Bit(27) /* RDTSCP */
#define CPUID_EXTFEATURE_EM64T     _Bit(29) /* Extended Mem 64 Technology */
#if 0
#define CPUID_EXTFEATURE_LAHF    _HBit(20)  /* LAFH/SAHF instructions */
#endif
// New definition with Snow kernel
#define CPUID_EXTFEATURE_LAHF    _HBit(0) /* LAHF/SAHF instructions */
/*
 * The CPUID_EXTFEATURE_XXX values define 64-bit values
 * returned in %ecx:%edx to a CPUID request with %eax of 0x80000007:
 */
#define CPUID_EXTFEATURE_TSCI      _Bit(8)  /* TSC Invariant */
#define CPUID_CACHE_SIZE  16  /* Number of descriptor values */
#define CPUID_MWAIT_EXTENSION _Bit(0) /* enumeration of WMAIT extensions */
#define CPUID_MWAIT_BREAK _Bit(1) /* interrupts are break events     */

/* Known MSR registers */
#define MSR_IA32_PLATFORM_ID        0x0017
#define MSR_EBC_FREQUENCY_ID        0x002C   /* Used for Netburst (0x0F family) */
#define MSR_CORE_THREAD_COUNT       0x0035   /* limited use - not for Penryn or older */
#define MSR_IA32_BIOS_SIGN_ID       0x008B   /* microcode version */
#define MSR_FSB_FREQ                0x00CD   /* limited use - not for i7            */
#define MSR_PLATFORM_INFO           0x00CE   /* limited use - MinRatio for i7 but Max for Yonah */
/* turbo for penryn */
#define MSR_IA32_EXT_CONFIG         0x00EE   /* limited use - not for i7            */
#define MSR_FLEX_RATIO              0x0194   /* limited use - not for Penryn or older     */
//see no value on most CPUs
#define MSR_IA32_PERF_STATUS        0x0198
#define MSR_IA32_PERF_CONTROL       0x0199
#define MSR_IA32_CLOCK_MODULATION   0x019A
#define MSR_THERMAL_STATUS          0x019C
#define MSR_IA32_MISC_ENABLE        0x01A0
#define MSR_THERMAL_TARGET          0x01A2   /* limited use - not for Penryn or older     */
#define MSR_TURBO_RATIO_LIMIT       0x01AD   /* limited use - not for Penryn or older     */
//AMD
#define K8_FIDVID_STATUS        0xC0010042
#define K10_COFVID_STATUS       0xC0010071
#define DEFAULT_FSB             100000          /* for now, hardcoding 100MHz for old CPUs */
//

#endif
