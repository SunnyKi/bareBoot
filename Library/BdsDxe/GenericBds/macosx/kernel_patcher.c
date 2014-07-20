/*
 * Copyright (c) 2011-2012 Frank Peng. All rights reserved.
 *
 */

#include <macosx.h>

#include "cpu.h"
#include "LoaderUefi.h"
#include "kernel_patcher.h"
#include "sse3_patcher.h"
#include "sse3_5_patcher.h"
#if 0
#include "device_tree.h"
#endif

#define KERNEL_MAX_SIZE 40000000
#define KERNEL_PATCH_SIGNATURE     0x85d08941e5894855ULL
#define KERNEL_YOS_PATCH_SIGNATURE 0x56415741e5894855ULL

EFI_PHYSICAL_ADDRESS    KernelRelocBase = 0;
BootArgs1               *bootArgs1      = NULL;
BootArgs2               *bootArgs2      = NULL;
CHAR8                   *dtRoot         = NULL;
CHAR8                   *KernVersion    = NULL;
VOID                    *KernelData     = NULL;
UINT32                  KernelSlide     = 0;
BOOLEAN                 isKernelcache   = FALSE;
BOOLEAN                 is64BitKernel   = FALSE;
BOOLEAN                 PatcherInited   = FALSE;
BOOLEAN                 SSSE3;

// notes:
// - 64bit segCmd64->vmaddr is 0xffffff80xxxxxxxx and we are taking
//   only lower 32bit part into PrelinkTextAddr
// - PrelinkTextAddr is segCmd64->vmaddr + KernelRelocBase
UINT32     PrelinkTextAddr = 0;
UINT32     PrelinkTextSize = 0;

// notes:
// - 64bit sect->addr is 0xffffff80xxxxxxxx and we are taking
//   only lower 32bit part into PrelinkInfoAddr
// - PrelinkInfoAddr is sect->addr + KernelRelocBase
UINT32     PrelinkInfoAddr = 0;
UINT32     PrelinkInfoSize = 0;

VOID
SetKernelRelocBase (
  VOID
)
{
    EFI_STATUS      Status;
    UINTN           DataSize = sizeof(KernelRelocBase);
    
    KernelRelocBase = 0;
    // OsxAptioFixDrv will set this
    Status = gRT->GetVariable(
                    L"OsxAptioFixDrv-RelocBase",
                    &gEfiAppleBootGuid,
                    NULL,
                    &DataSize,
                    &KernelRelocBase
                  );
    // KernelRelocBase is now either read or 0
    return;
}

#if 0
VOID
GetSegment (
  IN CHAR8    *Segment,
  OUT UINT32  *Addr,
  OUT UINT32  *Size
)
{
  UINT32  ncmds;
  UINT32  cmdsize;
  UINT32  binaryIndex;
  UINTN   cnt;
  UINT8   *binary;

  struct load_command         *loadCommand;
  struct segment_command      *segCmd;
  struct segment_command_64   *segCmd64;

  binary = (UINT8 *) KernelData;

  if (is64BitKernel) {
    binaryIndex = sizeof (struct mach_header_64);
  } else {
    binaryIndex = sizeof (struct mach_header);
  }
  
  ncmds = MACH_GET_NCMDS (binary);
  
  for (cnt = 0; cnt < ncmds; cnt++) {
    loadCommand = (struct load_command *) (binary + binaryIndex);
    cmdsize = loadCommand->cmdsize;
    
    switch (loadCommand->cmd) {
      case LC_SEGMENT_64: 
        segCmd64 = (struct segment_command_64 *) loadCommand;
#ifdef KERNEL_PATCH_DEBUG
        Print (
          L"%a: segment %a, address 0x%x, size 0x%x\n",
          __FUNCTION__,
          segCmd64->segname,
          segCmd64->vmaddr,
          segCmd64->vmsize
        );
#endif
        if (AsciiStrCmp (segCmd64->segname, Segment) == 0) {
          if (segCmd64->vmsize > 0) {
            *Addr = (UINT32) (segCmd64->vmaddr ? segCmd64->vmaddr + KernelRelocBase : 0);
            *Size = (UINT32) segCmd64->vmsize;
          }
        }
        break;

      case LC_SEGMENT:
        segCmd = (struct segment_command *) loadCommand;
        if (AsciiStrCmp (segCmd->segname, Segment) == 0) {
          if (segCmd->vmsize > 0) {
            *Addr = (UINT32) (segCmd->vmaddr ? segCmd->vmaddr + KernelRelocBase : 0);
            *Size = (UINT32) segCmd->vmsize;
          }
        }
        break;
        
      default:
        break;
    }
    binaryIndex += cmdsize;
  }
  
  return;
}
#endif

VOID
GetSection (
  IN CHAR8    *Segment,
  IN CHAR8    *Section,
  OUT UINT32  *Addr,
  OUT UINT32  *Size
)
{
  UINT32  ncmds;
  UINT32  cmdsize;
  UINT32  binaryIndex;
  UINTN   cnt;
  UINT8   *binary;
  UINT32  sectionIndex;

  struct load_command         *loadCommand;
  struct segment_command      *segCmd;
  struct segment_command_64   *segCmd64;
  struct section              *sect;
  struct section_64           *sect64;

  binary = (UINT8 *) KernelData;

  if (is64BitKernel) {
    binaryIndex = sizeof (struct mach_header_64);
  } else {
    binaryIndex = sizeof (struct mach_header);
  }
  
  ncmds = MACH_GET_NCMDS (binary);
  
  for (cnt = 0; cnt < ncmds; cnt++) {
    loadCommand = (struct load_command *) (binary + binaryIndex);
    cmdsize = loadCommand->cmdsize;
    
    switch (loadCommand->cmd) {
      case LC_SEGMENT_64: 
        segCmd64 = (struct segment_command_64 *) loadCommand;
        if (AsciiStrCmp (segCmd64->segname, Segment) == 0) {
          sectionIndex = sizeof (struct segment_command_64);
          while(sectionIndex < segCmd64->cmdsize) {
            sect64 = (struct section_64 *) ((UINT8 *) segCmd64 + sectionIndex);
            sectionIndex += sizeof (struct section_64);
            if(AsciiStrCmp (sect64->sectname, Section) == 0 &&
               AsciiStrCmp (sect64->segname, Segment) == 0) {
              if (sect64->size > 0) {
                *Addr = (UINT32) (sect64->addr ? sect64->addr + KernelRelocBase : 0);
                *Size = (UINT32) sect64->size;
              }
              break;
            }
          }
        }
        break;

      case LC_SEGMENT:
        segCmd = (struct segment_command *) loadCommand;
        if (AsciiStrCmp (segCmd->segname, Segment) == 0) {
          sectionIndex = sizeof (struct segment_command);
          while(sectionIndex < segCmd->cmdsize) {
            sect = (struct section *) ((UINT8*)segCmd + sectionIndex);
            sectionIndex += sizeof (struct section);
            if(AsciiStrCmp (sect->sectname, Section) == 0 &&
               AsciiStrCmp (sect->segname, Segment) == 0) {
              if (sect->size > 0) { 
                *Addr = (UINT32) (sect->addr ? sect->addr + KernelRelocBase : 0);
                *Size = (UINT32) sect->size;
              }
              break;
            }
          }
        }
        break;
        
      default:
        break;
    }
    binaryIndex += cmdsize;
  }
  
  return;
}

CHAR8*
GetKernelVersion (
  VOID
  )
{
  CHAR8           *s, *s1, *kv;
  UINT32          addr, size;
  UINTN           sec, i, i2, i3, kvBegin;
  
  CHAR8           *secName[] = {
    "__const",
    "__cstring"
  };

  for (sec = 0; sec < sizeof (secName) / sizeof (CHAR8 *); sec++) {
    GetSection ("__TEXT", secName[sec], &addr, &size);
    for (i = addr; i < addr + size; i++) {
      if (AsciiStrnCmp ((CHAR8 *) i, "Darwin Kernel Version", 21) == 0) {
        kvBegin = i + 22;
        i2 = kvBegin;
        s = (CHAR8 *) kvBegin;
        while (AsciiStrnCmp ((CHAR8 *) i2, ":", 1) != 0) {
          i2++;
        }
        kv = (CHAR8 *) AllocateZeroPool (i2 - kvBegin + 1);
        s1 = kv;
        for (i3 = kvBegin; i3 < i2; i3++) {
          *s1++ = *s++;
        }
        *s1 = 0;
        return kv;
      }
    }
  }
  return NULL;
}

#if 1
// Power management patch for kernel 13.0
STATIC UINT8 KernelPatchPmSrc[] = {
  0x55, 0x48, 0x89, 0xe5, 0x41, 0x89, 0xd0, 0x85,
  0xf6, 0x74, 0x6c, 0x48, 0x83, 0xc7, 0x28, 0x90,
  0x8b, 0x05, 0x5e, 0x30, 0x5e, 0x00, 0x85, 0x47,
  0xdc, 0x74, 0x54, 0x8b, 0x4f, 0xd8, 0x45, 0x85,
  0xc0, 0x74, 0x08, 0x44, 0x39, 0xc1, 0x44, 0x89,
  0xc1, 0x75, 0x44, 0x0f, 0x32, 0x89, 0xc0, 0x48,
  0xc1, 0xe2, 0x20, 0x48, 0x09, 0xc2, 0x48, 0x89,
  0x57, 0xf8, 0x48, 0x8b, 0x47, 0xe8, 0x48, 0x85,
  0xc0, 0x74, 0x06, 0x48, 0xf7, 0xd0, 0x48, 0x21,
  0xc2, 0x48, 0x0b, 0x57, 0xf0, 0x49, 0x89, 0xd1,
  0x49, 0xc1, 0xe9, 0x20, 0x89, 0xd0, 0x8b, 0x4f,
  0xd8, 0x4c, 0x89, 0xca, 0x0f, 0x30, 0x8b, 0x4f,
  0xd8, 0x0f, 0x32, 0x89, 0xc0, 0x48, 0xc1, 0xe2,
  0x20, 0x48, 0x09, 0xc2, 0x48, 0x89, 0x17, 0x48,
  0x83, 0xc7, 0x30, 0xff, 0xce, 0x75, 0x99, 0x5d,
  0xc3, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90
};

STATIC UINT8 KernelPatchPmRepl[] = {
  0x55, 0x48, 0x89, 0xe5, 0x41, 0x89, 0xd0, 0x85,
  0xf6, 0x74, 0x73, 0x48, 0x83, 0xc7, 0x28, 0x90,
  0x8b, 0x05, 0x5e, 0x30, 0x5e, 0x00, 0x85, 0x47,
  0xdc, 0x74, 0x5b, 0x8b, 0x4f, 0xd8, 0x45, 0x85,
  0xc0, 0x74, 0x08, 0x44, 0x39, 0xc1, 0x44, 0x89,
  0xc1, 0x75, 0x4b, 0x0f, 0x32, 0x89, 0xc0, 0x48,
  0xc1, 0xe2, 0x20, 0x48, 0x09, 0xc2, 0x48, 0x89,
  0x57, 0xf8, 0x48, 0x8b, 0x47, 0xe8, 0x48, 0x85,
  0xc0, 0x74, 0x06, 0x48, 0xf7, 0xd0, 0x48, 0x21,
  0xc2, 0x48, 0x0b, 0x57, 0xf0, 0x49, 0x89, 0xd1,
  0x49, 0xc1, 0xe9, 0x20, 0x89, 0xd0, 0x8b, 0x4f,
  0xd8, 0x4c, 0x89, 0xca, 0x66, 0x81, 0xf9, 0xe2,
  0x00, 0x74, 0x02, 0x0f, 0x30, 0x8b, 0x4f, 0xd8,
  0x0f, 0x32, 0x89, 0xc0, 0x48, 0xc1, 0xe2, 0x20,
  0x48, 0x09, 0xc2, 0x48, 0x89, 0x17, 0x48, 0x83,
  0xc7, 0x30, 0xff, 0xce, 0x75, 0x92, 0x5d, 0xc3
};
// Power management patch for kernel 12.5
STATIC UINT8 KernelPatchPmSrc2[] = {
  0x55, 0x48, 0x89, 0xe5, 0x41, 0x89, 0xd0, 0x85,
  0xf6, 0x74, 0x69, 0x48, 0x83, 0xc7, 0x28, 0x90,
  0x8b, 0x05, 0xfe, 0xce, 0x5f, 0x00, 0x85, 0x47,
  0xdc, 0x74, 0x51, 0x8b, 0x4f, 0xd8, 0x45, 0x85,
  0xc0, 0x74, 0x05, 0x44, 0x39, 0xc1, 0x75, 0x44,
  0x0f, 0x32, 0x89, 0xc0, 0x48, 0xc1, 0xe2, 0x20,
  0x48, 0x09, 0xc2, 0x48, 0x89, 0x57, 0xf8, 0x48,
  0x8b, 0x47, 0xe8, 0x48, 0x85, 0xc0, 0x74, 0x06,
  0x48, 0xf7, 0xd0, 0x48, 0x21, 0xc2, 0x48, 0x0b,
  0x57, 0xf0, 0x49, 0x89, 0xd1, 0x49, 0xc1, 0xe9,
  0x20, 0x89, 0xd0, 0x8b, 0x4f, 0xd8, 0x4c, 0x89,
  0xca, 0x0f, 0x30, 0x8b, 0x4f, 0xd8, 0x0f, 0x32,
  0x89, 0xc0, 0x48, 0xc1, 0xe2, 0x20, 0x48, 0x09,
  0xc2, 0x48, 0x89, 0x17, 0x48, 0x83, 0xc7, 0x30,
  0xff, 0xce, 0x75, 0x9c, 0x5d, 0xc3, 0x90, 0x90,
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90
};

STATIC UINT8 KernelPatchPmRepl2[] = {
  0x55, 0x48, 0x89, 0xe5, 0x41, 0x89, 0xd0, 0x85,
  0xf6, 0x74, 0x70, 0x48, 0x83, 0xc7, 0x28, 0x90,
  0x8b, 0x05, 0xfe, 0xce, 0x5f, 0x00, 0x85, 0x47,
  0xdc, 0x74, 0x58, 0x8b, 0x4f, 0xd8, 0x45, 0x85,
  0xc0, 0x74, 0x05, 0x44, 0x39, 0xc1, 0x75, 0x4b,
  0x0f, 0x32, 0x89, 0xc0, 0x48, 0xc1, 0xe2, 0x20,
  0x48, 0x09, 0xc2, 0x48, 0x89, 0x57, 0xf8, 0x48,
  0x8b, 0x47, 0xe8, 0x48, 0x85, 0xc0, 0x74, 0x06,
  0x48, 0xf7, 0xd0, 0x48, 0x21, 0xc2, 0x48, 0x0b,
  0x57, 0xf0, 0x49, 0x89, 0xd1, 0x49, 0xc1, 0xe9,
  0x20, 0x89, 0xd0, 0x8b, 0x4f, 0xd8, 0x4c, 0x89,
  0xca, 0x66, 0x81, 0xf9, 0xe2, 0x00, 0x74, 0x02,
  0x0f, 0x30, 0x8b, 0x4f, 0xd8, 0x0f, 0x32, 0x89,
  0xc0, 0x48, 0xc1, 0xe2, 0x20, 0x48, 0x09, 0xc2,
  0x48, 0x89, 0x17, 0x48, 0x83, 0xc7, 0x30, 0xff,
  0xce, 0x75, 0x95, 0x5d, 0xc3, 0x90, 0x90, 0x90
};

STATIC UINT8 KernelPatchPmSrc1010[] = {
  0x55, 0x48, 0x89, 0xe5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
  0x53, 0x50, 0x41, 0x89, 0xd6, 0x41, 0x89, 0xf7, 0x48, 0x89, 0xfb, 0x45,
  0x85, 0xff, 0x0f, 0x84, 0x9b, 0x00, 0x00, 0x00, 0x48, 0x83, 0xc3, 0x28,
  0x4c, 0x8d, 0x25, 0x89, 0xae, 0x53, 0x00, 0x4c, 0x8d, 0x2d, 0x9c, 0xae,
  0x53, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x2e, 0x0f, 0x1f, 0x84, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x8b, 0x05, 0x96, 0x75, 0x69, 0x00, 0x85, 0x43,
  0xdc, 0x74, 0x67, 0x45, 0x85, 0xf6, 0x74, 0x06, 0x44, 0x39, 0x73, 0xd8,
  0x75, 0x5c, 0x83, 0x3d, 0x13, 0xed, 0x5f, 0x00, 0x00, 0x74, 0x10, 0x8b,
  0x53, 0xd8, 0x31, 0xc0, 0x4c, 0x89, 0xe7, 0x4c, 0x89, 0xee, 0xe8, 0xa1,
  0xb0, 0x4d, 0x00, 0x8b, 0x4b, 0xd8, 0x0f, 0x32, 0x89, 0xd1, 0x48, 0xc1,
  0xe1, 0x20, 0x89, 0xc2, 0x48, 0x09, 0xca, 0x48, 0x89, 0x53, 0xf8, 0x48,
  0x8b, 0x43, 0xe8, 0x48, 0x85, 0xc0, 0x74, 0x06, 0x48, 0xf7, 0xd0, 0x48,
  0x21, 0xc2, 0x48, 0x0b, 0x53, 0xf0, 0x8b, 0x4b, 0xd8, 0x89, 0xd0, 0x48,
  0xc1, 0xea, 0x20, 0x0f, 0x30, 0x8b, 0x4b, 0xd8, 0x0f, 0x32, 0x48, 0xc1,
  0xe2, 0x20, 0x89, 0xc0, 0x48, 0x09, 0xd0, 0x48, 0x89, 0x03, 0x48, 0x83,
  0xc3, 0x30, 0x41, 0xff, 0xcf, 0x75, 0x85, 0x48, 0x83, 0xc4, 0x08, 0x5b,
  0x41, 0x5c, 0x41, 0x5d, 0x41, 0x5e, 0x41, 0x5f, 0x5d, 0xc3, 0x66, 0x0f,
  0x1f, 0x44, 0x00, 0x00
};

static UINT8 KernelPatchPmRepl1010[] = {
  0x55, 0x48, 0x89, 0xe5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54,
  0x53, 0x50, 0x41, 0x89, 0xd6, 0x41, 0x89, 0xf7, 0x48, 0x89, 0xfb, 0x45,
  0x85, 0xff, 0x0f, 0x84, 0x9f, 0x00, 0x00, 0x00, 0x48, 0x83, 0xc3, 0x28,
  0x4c, 0x8d, 0x25, 0x89, 0xae, 0x53, 0x00, 0x4c, 0x8d, 0x2d, 0x9c, 0xae,
  0x53, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x2e, 0x0f, 0x1f, 0x84, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x8b, 0x05, 0x96, 0x75, 0x69, 0x00, 0x85, 0x43,
  0xdc, 0x74, 0x6b, 0x45, 0x85, 0xf6, 0x74, 0x06, 0x44, 0x39, 0x73, 0xd8,
  0x74, 0x60, 0x83, 0x3d, 0x13, 0xed, 0x5f, 0x00, 0x00, 0x74, 0x10, 0x8b,
  0x53, 0xd8, 0x31, 0xc0, 0x4c, 0x89, 0xe7, 0x4c, 0x89, 0xee, 0xe8, 0xa1,
  0xb0, 0x4d, 0x00, 0x8b, 0x4b, 0xd8, 0x0f, 0x32, 0x89, 0xd1, 0x48, 0xc1,
  0xe1, 0x20, 0x89, 0xc2, 0x48, 0x09, 0xca, 0x48, 0x89, 0x53, 0xf8, 0x48,
  0x8b, 0x43, 0xe8, 0x48, 0x85, 0xc0, 0x74, 0x06, 0x48, 0xf7, 0xd0, 0x48,
  0x21, 0xc2, 0x48, 0x0b, 0x53, 0xf0, 0x8b, 0x4b, 0xd8, 0x89, 0xd0, 0x48,
  0xc1, 0xea, 0x20, 0x85, 0xc9, 0x74, 0x02, 0x0f, 0x30, 0x8b, 0x4b, 0xd8,
  0x0f, 0x32, 0x48, 0xc1, 0xe2, 0x20, 0x89, 0xc0, 0x48, 0x09, 0xd0, 0x48,
  0x89, 0x03, 0x48, 0x83, 0xc3, 0x30, 0x41, 0xff, 0xcf, 0x75, 0x81, 0x48,
  0x83, 0xc4, 0x08, 0x5b, 0x41, 0x5c, 0x41, 0x5d, 0x41, 0x5e, 0x41, 0x5f,
  0x5d, 0xc3, 0x90, 0x90
};


UINT8 KBELionSearch_X64[] =
{ 0xE8, 0x0C, 0xFD, 0xFF, 0xFF, 0xEB, 0x08, 0x48, 0x89, 0xDF };
UINT8 KBELionReplace_X64[] =
{ 0xE8, 0x0C, 0xFD, 0xFF, 0xFF, 0x90, 0x90, 0x48, 0x89, 0xDF };

UINT8 KBELionSearch_i386[] =
{ 0xE8, 0xAA, 0xFB, 0xFF, 0xFF, 0xEB, 0x08, 0x89, 0x34, 0x24 };
UINT8 KBELionReplace_i386[] =
{ 0xE8, 0xAA, 0xFB, 0xFF, 0xFF, 0x90, 0x90, 0x89, 0x34, 0x24 };

UINT8 KBEMLSearch[] =
{ 0xC6, 0xE8, 0x30, 0x00, 0x00, 0x00, 0xEB, 0x08, 0x48, 0x89, 0xDF };
UINT8 KBEMLReplace[] =
{ 0xC6, 0xE8, 0x30, 0x00, 0x00, 0x00, 0x90, 0x90, 0x48, 0x89, 0xDF };

UINT8 KBEYSearch[] =
{ 0xC6, 0xE8, 0x25, 0x00, 0x00, 0x00, 0xEB, 0x05, 0xE8, 0xCE, 0x02 };
UINT8 KBEYReplace[] =
{ 0xC6, 0xE8, 0x25, 0x00, 0x00, 0x00, 0x90, 0x90, 0xE8, 0xCE, 0x02 };

VOID
EFIAPI
KernelBooterExtensionsPatch (
  VOID
)
{
  UINTN     Num = 0;
  UINT32    addr, size;

  if (KernVersion == NULL) {
    DBG ("%a: kernel version not found.\n", __FUNCTION__);
    return;
  }

  GetSection ("__KLD", "__text", &addr, &size);

  if (is64BitKernel) {
    if (AsciiStrnCmp (KernVersion, "11", 2) == 0) {
      Num = SearchAndReplace (
              (UINT8 *) (UINTN) addr,
              size,
              (CHAR8 *) KBELionSearch_X64,
              sizeof (KBELionSearch_X64),
              (CHAR8 *) KBELionReplace_X64,
              1
              );
    }
    if ((AsciiStrnCmp (KernVersion, "12", 2) == 0) ||
        (AsciiStrnCmp (KernVersion, "13", 2) == 0)) {
      Num = SearchAndReplace (
              (UINT8 *) (UINTN) addr,
              size,
              (CHAR8 *) KBEMLSearch,
              sizeof (KBEMLSearch),
              (CHAR8 *) KBEMLReplace,
              1
              );
    }
    if (AsciiStrnCmp (KernVersion, "14", 2) == 0) {
      Num = SearchAndReplace (
              (UINT8 *) (UINTN) addr,
              size,
              (CHAR8 *) KBEYSearch,
              sizeof (KBEYSearch),
              (CHAR8 *) KBEYReplace,
              1
              );
    }
  } else {
    if (AsciiStrnCmp (KernVersion, "11", 2) == 0) {
      Num = SearchAndReplace (
              (UINT8 *) (UINTN) addr,
              size,
              (CHAR8 *) KBELionSearch_i386,
              sizeof (KBELionSearch_i386),
              (CHAR8 *) KBELionReplace_i386,
              1
              );
    }
  }
  
  DBG ("%a: SearchAndReplace %d times.\n", __FUNCTION__, Num);
#ifdef KERNEL_PATCH_DEBUG
  Print (L"%a: SearchAndReplace %d times.\n", __FUNCTION__, Num);
#endif
}

VOID
KernelPatchPm (
  VOID *kernelData
)
{
  UINT8  *Ptr = (UINT8 *) kernelData;
  UINT8  *End = Ptr + 0x1000000;
  
  // Credits to RehabMan for the kernel patch information
  while (Ptr < End) {
    if (KERNEL_PATCH_SIGNATURE == (*((UINT64 *)Ptr))) {
      // Bytes 19,20 of KernelPm patch for kernel 13.x change between kernel versions, so we skip them in search&replace
      if ((CompareMem(Ptr + sizeof(UINT64),
                      KernelPatchPmSrc + sizeof (UINT64),
                      18 * sizeof (UINT8) - sizeof (UINT64)) == 0) &&
          (CompareMem(Ptr + 20 * sizeof(UINT8),
                      KernelPatchPmSrc + 20 * sizeof (UINT8),
                      sizeof (KernelPatchPmSrc) - 20 * sizeof(UINT8)) == 0)) {
        // Don't copy more than the source here!
        CopyMem (Ptr,
                 KernelPatchPmRepl,
                 18 * sizeof(UINT8));
        CopyMem (Ptr + 20 * sizeof(UINT8),
                 KernelPatchPmRepl + 20*sizeof(UINT8),
                 sizeof(KernelPatchPmSrc) - 20 * sizeof(UINT8));
        break;
      } else if (CompareMem(Ptr + sizeof(UINT64),
                            KernelPatchPmSrc2 + sizeof (UINT64),
                            sizeof (KernelPatchPmSrc2) - sizeof (UINT64)) == 0) {
        // Don't copy more than the source here!
        CopyMem (Ptr,
                 KernelPatchPmRepl2,
                 sizeof(KernelPatchPmSrc2));
        break;
      }
    }
    //rehabman: for 10.10.dp1 (code portion)
    else if (KERNEL_YOS_PATCH_SIGNATURE == (*((UINT64 *)Ptr))) {
      if (0 == CompareMem (Ptr + sizeof (UINT64),
                           KernelPatchPmSrc1010 + sizeof (UINT64),
                           sizeof (KernelPatchPmSrc1010)-sizeof (UINT64))) {
        // copy assuming find/replace are same sizes
        CopyMem (Ptr,
                 KernelPatchPmRepl1010,
                 sizeof (KernelPatchPmRepl1010));
      }
    }
    //rehabman: for 10.10.dp1 (data portion)
    else if (0x00000002000000E2ULL == (*((UINT64 *)Ptr))) {
      (*((UINT64 *)Ptr)) = 0x0000000200000000ULL;
    }
    else if (0x0000004C000000E2ULL == (*((UINT64 *)Ptr))) {
      (*((UINT64 *)Ptr)) = 0x0000004C00000000ULL;
    }
    else if (0x00000190000000E2ULL == (*((UINT64 *)Ptr))) {
      (*((UINT64 *)Ptr)) = 0x0000019000000000ULL;
      break;
    }
    Ptr += 16;
  }
}

VOID
Patcher_SSE3_6 (
  VOID  *kernelData
)
{
  UINT8   *bytes;
  UINT32  patchLocation1;
  UINT32  patchLocation2;
  UINT32  patchlast;
  UINT32  i;
  
  bytes = (UINT8 *) kernelData;
  patchLocation1 = 0;
  patchLocation2 = 0;
  patchlast = 0;
  i = 0;
  
  while (TRUE) {
    if (bytes[i] == 0x66 && bytes[i+1] == 0x0F && bytes[i+2] == 0x6F &&
        bytes[i+3] == 0x44 && bytes[i+4] == 0x0E && bytes[i+5] == 0xF1 &&
        bytes[i-1664-32] == 0x55) {
      patchLocation1 = i-1664-32;
    }
    
    // khasSSE2+..... title
    if (bytes[i] == 0xE3 && bytes[i+1] == 0x07 && bytes[i+2] == 0x00 &&
        bytes[i+3] == 0x00 && bytes[i+4] == 0x80 && bytes[i+5] == 0x07 &&
        bytes[i+6] == 0xFF && bytes[i+7] == 0xFF && bytes[i+8] == 0x24 &&
        bytes[i+9] == 0x01) {
      patchLocation2 = i;
      break;
    }
    i++;
  }
  
  if (!patchLocation1 || !patchLocation2) {
    return;
  }
  
  i = patchLocation1 + 1500;
  while (TRUE) {
    if (bytes[i] == 0x90 && bytes[i+1] == 0x90 && bytes[i+2] == 0x55 ) {
      patchlast = (i+1) - patchLocation1;
      break;
    }
    i++;
  }
  
  if (!patchlast) {
    return;
  }
  
  // patch sse3_64 data
  for (i = 0; i < patchlast; i++) {
    if (i < sizeof (sse3_patcher)) {
      bytes[patchLocation1 + i] = sse3_patcher[i];
    } else {
      bytes[patchLocation1 + i] = 0x90;
    }
  }
  
  // patch kHasSSE3 title
  bytes[patchLocation2 + 0] = 0xFC;
  bytes[patchLocation2 + 1] = 0x05;
  bytes[patchLocation2 + 8] = 0x2C;
  bytes[patchLocation2 + 9] = 0x00;
}

VOID
Patcher_SSE3_5 (
  VOID  *kernelData
)
{
  UINT8   *bytes;
  UINT32  patchLocation1;
  UINT32  patchLocation2;
  UINT32  patchlast;
  UINT32  Length;
  UINT32  i;
  
  bytes = (UINT8 *) kernelData;
  patchLocation1 = 0;
  patchLocation2 = 0;
  patchlast = 0;
  Length = sizeof (kernelData);
  
  for (i = 256; i < (Length - 256); i++) {
    if (bytes[i] == 0x66 && bytes[i+1] == 0x0F && bytes[i+2] == 0x6F &&
        bytes[i+3] == 0x44 && bytes[i+4] == 0x0E && bytes[i+5] == 0xF1 &&
        bytes[i-1680-32] == 0x55) {
      patchLocation1 = i-1680-32;
    }
    
    // khasSSE2+..... title
    if (bytes[i] == 0xF3 && bytes[i+1] == 0x07 && bytes[i+2] == 0x00 &&
        bytes[i+3] == 0x00 && bytes[i+4] == 0x80 && bytes[i+5] == 0x07 &&
        bytes[i+6] == 0xFF && bytes[i+7] == 0xFF && bytes[i+8] == 0x24 &&
        bytes[i+9] == 0x01) {
      patchLocation2 = i;
      break;
    }
  }
  
  if (!patchLocation1 || !patchLocation2)  {
    return;
  }
  
  for (i = (patchLocation1 + 1500); i < Length; i++) {
    if (bytes[i] == 0x90 && bytes[i+1] == 0x90 && bytes[i+2] == 0x55) {
      patchlast = (i+1) - patchLocation1;
      break;
    }
  }
  
  if (!patchlast) {
    return;
  }
  
  // patech sse3_64 data
  for (i = 0; i < patchlast; i++) {
    if (i < sizeof (sse3_5_patcher)) {
      bytes[patchLocation1 + i] = sse3_5_patcher[i];
    } else {
      bytes[patchLocation1 + i] = 0x90;
    }
  }
  
  // patch kHasSSE3 title
  bytes[patchLocation2 + 0] = 0x0C;
  bytes[patchLocation2 + 1] = 0x06;
  bytes[patchLocation2 + 8] = 0x2C;
  bytes[patchLocation2 + 9] = 0x00;
}

VOID
Patcher_SSE3_7 (
  VOID  *kernelData
)
{
  // not support yet
  return;
}

VOID
KernelPatcher_64 (
  VOID* kernelData
)
{
  UINT8       *bytes;
  UINT32      patchLocation=0, patchLocation1=0;
  UINT32      i;
  UINT32      switchaddr=0;
  UINT32      mask_family=0, mask_model=0;
  UINT32      cpuid_family_addr=0, cpuid_model_addr=0;

  if (KernVersion == NULL) {
    DBG ("%a: kernel version not found.\n", __FUNCTION__);
    return;
  }

  bytes = (UINT8 *) kernelData;
  
  DBG ("%a: looking for _cpuid_set_info Unsupported CPU _panic\n", __FUNCTION__);
  // Determine location of _cpuid_set_info _panic call for refrence
  // basically looking for info_p->cpuid_model = bitfield32(reg[eax],  7,  4);
  for (i=0; i<0x1000000; i++) {
    if (bytes[i+ 0] == 0xC7 && bytes[i+ 1] == 0x05 && bytes[i+ 5] == 0x00 &&
        bytes[i+ 6] == 0x07 && bytes[i+ 7] == 0x00 && bytes[i+ 8] == 0x00 &&
        bytes[i+ 9] == 0x00 && bytes[i-5] == 0xE8) {
      // matching bytes[i-5] == 0xE8 for _panic call doesn't seem to always work
      // i made sure _panic call is only place wit this sequence in all the kernels i've looked at
      patchLocation = i-5;
      break;
    }
  }
  if (!patchLocation) {
    DBG ("%a: _cpuid_set_info Unsupported CPU _panic not found\n", __FUNCTION__);
    return;
  }
  // make sure only kernels for OSX 10.6.0 to 10.7.3 are being patched by this approach
  if (AsciiStrnCmp (KernVersion, "10", 2) >= 0 && AsciiStrnCmp (KernVersion, "11.3.0", 6) <= 0) {
    DBG ("%a: will patch kernel for OSX 10.6.0 to 10.7.3\n", __FUNCTION__);
    // remove tsc_init: unknown CPU family panic for kernels prior to 10.6.2 which still had Atom support
    if (AsciiStrnCmp (KernVersion, "10.1", 4) <= 0) {
      for (i=0; i<0x1000000; i++) {
        // find _tsc_init panic address by byte sequence 488d3df4632a00
        if (bytes[i] == 0x48 && bytes[i+1] == 0x8D && bytes[i+2] == 0x3D && bytes[i+3] == 0xF4 &&
            bytes[i+4] == 0x63 && bytes[i+5] == 0x2A && bytes[i+6] == 0x00) {
          patchLocation1 = i+9;
          DBG ("%a: found _tsc_init _panic address at 0x%08x\n", __FUNCTION__, patchLocation1);
          break;
        }
      }
      // NOP _panic call
      if (patchLocation1) {
        bytes[patchLocation1 + 0] = 0x90;
        bytes[patchLocation1 + 1] = 0x90;
        bytes[patchLocation1 + 2] = 0x90;
        bytes[patchLocation1 + 3] = 0x90;
        bytes[patchLocation1 + 4] = 0x90;
      }
    }
    else { // assume patching logic for OSX 10.6.2 to 10.7.3
      /*
       Here is our case from CPUID switch statement, it sets CPUFAMILY_UNKNOWN
       C7051C2C5F0000000000   mov     dword [ds:0xffffff80008a22c0], 0x0 (example from 10.7)
       */
      switchaddr = patchLocation-19;
      DBG ("%a: switch statement patch location is 0x%08x\n", __FUNCTION__, (switchaddr+6));
      
      if (bytes[switchaddr+0] == 0xC7 && bytes[switchaddr+1] == 0x05 &&
          bytes[switchaddr+5] == 0x00 && bytes[switchaddr+6] == 0x00 &&
          bytes[switchaddr+7] == 0x00 && bytes[switchaddr+8] == 0x00) {
        // Determine cpuid_family address from above mov operation
        cpuid_family_addr =
        bytes[switchaddr + 2] <<  0 |
        bytes[switchaddr + 3] <<  8 |
        bytes[switchaddr + 4] << 16 |
        bytes[switchaddr + 5] << 24;
        
        cpuid_family_addr = cpuid_family_addr + (switchaddr+10);

        if (cpuid_family_addr) {
          // Determine cpuid_model address
          // for 10.6.2 kernels it's offset by 299 bytes from cpuid_family address
          if (AsciiStrnCmp (KernVersion, "10.2", 4) == 0) {
            cpuid_model_addr = cpuid_family_addr - 0x12B;
          } else {
            // for 10.6.3 to 10.6.7 it's offset by 303 bytes
            if (AsciiStrnCmp (KernVersion, "10.3", 4) >= 0 && AsciiStrnCmp (KernVersion, "10.7", 4) <= 0) {
              cpuid_model_addr = cpuid_family_addr - 0x12F;
            } else {
              // for 10.6.8+ kernels - by 339 bytes
              cpuid_model_addr = cpuid_family_addr - 0x153;
            }
          }
          DBG ("%a: cpuid_family address = 0x%08x\n", __FUNCTION__, cpuid_family_addr);
          DBG ("%a: cpuid_model address  = 0x%08x\n", __FUNCTION__, cpuid_model_addr);
          switchaddr += 6; // offset 6 bytes in mov operation to write a dword instead of zero
          // calculate mask for patching, cpuid_family mask not needed as we offset on a valid mask
          mask_model   = cpuid_model_addr -  (switchaddr+14);
          DBG ("%a: model mask = 0x%08x\n", __FUNCTION__, mask_model);
          if (gSettings.CpuFamily != 0) {
            DBG ("%a: overriding custom cpuid_family = 0x08x\n", __FUNCTION__, gSettings.CpuFamily);
            bytes[switchaddr+0] = (gSettings.CpuFamily & 0x000000FF) >>  0;
            bytes[switchaddr+1] = (gSettings.CpuFamily & 0x0000FF00) >>  8;
            bytes[switchaddr+2] = (UINT8) ((gSettings.CpuFamily & 0x00FF0000) >> 16);
            bytes[switchaddr+3] = (gSettings.CpuFamily & 0xFF000000) >> 24;
          } else {
            DBG ("%a: overriding cpuid_family as CPUID_INTEL_PENRYN\n", __FUNCTION__);
            bytes[switchaddr+0] = (CPUFAMILY_INTEL_PENRYN & 0x000000FF) >>  0;
            bytes[switchaddr+1] = (CPUFAMILY_INTEL_PENRYN & 0x0000FF00) >>  8;
            bytes[switchaddr+2] = (CPUFAMILY_INTEL_PENRYN & 0x00FF0000) >> 16;
            bytes[switchaddr+3] = (CPUFAMILY_INTEL_PENRYN & 0xFF000000) >> 24;
          }
          // mov  dword [ds:0xffffff80008a216d], 0x2000117
          bytes[switchaddr+4] = 0xC7;
          bytes[switchaddr+5] = 0x05;
          bytes[switchaddr+6] = (mask_model & 0x000000FF) >> 0;
          bytes[switchaddr+7] = (mask_model & 0x0000FF00) >> 8;
          bytes[switchaddr+8] = (UINT8) ((mask_model & 0x00FF0000) >> 16);
          bytes[switchaddr+9] = (mask_model & 0xFF000000) >> 24;
          if (gSettings.CpuIdVars != 0) {
            DBG ("%a: overriding custom cpuid_model = 0x08x\n", __FUNCTION__, gSettings.CpuIdVars);
            bytes[switchaddr+10] = (gSettings.CpuIdVars & 0x000000FF) >>  0; // cpuid_model
            bytes[switchaddr+11] = (gSettings.CpuIdVars & 0x0000FF00) >>  8; // cpuid_extmodel
            bytes[switchaddr+12] = (UINT8) ((gSettings.CpuIdVars & 0x00FF0000) >> 16); // cpuid_extfamily
            bytes[switchaddr+13] = (gSettings.CpuIdVars & 0xFF000000) >> 24; // cpuid_stepping

          } else {
            DBG ("%a: overriding cpuid_model as CPUID_INTEL_PENRYN\n", __FUNCTION__);
            bytes[switchaddr+10] = 0x17; // cpuid_model
            bytes[switchaddr+11] = 0x01; // cpuid_extmodel
            bytes[switchaddr+12] = 0x00; // cpuid_extfamily
            bytes[switchaddr+13] = 0x02; // cpuid_stepping
          }
          // fill remainder with 4 NOPs
          for (i=14; i<18; i++) {
            bytes[switchaddr+i] = 0x90;
          }
        }
      } else {
        DBG ("%a: unable to determine cpuid_family address, patching aborted\n", __FUNCTION__);
        return;
      }
    }
    // patch ssse3
    if (!SSSE3 &&
        (AsciiStrnCmp (KernVersion, "10", 2) == 0)) {
      Patcher_SSE3_6 ((VOID*) bytes);
    }
    if (!SSSE3 &&
        (AsciiStrnCmp (KernVersion, "11", 2) == 0)) {
      Patcher_SSE3_7 ((VOID*) bytes);
    }
  } else {
    DBG ("%a: will patch kernel for OSX 10.7.4+\n", __FUNCTION__);
    /*
     Here is our switchaddress location ... it should be case 20 from CPUID switch statement
     833D78945F0000  cmp        dword [ds:0xffffff80008a21d0], 0x0;
     7417            je         0xffffff80002a8d71
     */
    switchaddr = patchLocation-45;
    DBG ("%a: switch statement patch location = 0x%08x\n", __FUNCTION__, switchaddr);
    
    if(bytes[switchaddr+0] == 0x83 && bytes[switchaddr+1] == 0x3D &&
       bytes[switchaddr+5] == 0x00 && bytes[switchaddr+6] == 0x00 &&
       bytes[switchaddr+7] == 0x74) {
      // Determine cpuid_family address
      // 891D4F945F00    mov        dword [ds:0xffffff80008a21a0], ebx
      cpuid_family_addr =
      bytes[switchaddr - 4] <<  0 |
      bytes[switchaddr - 3] <<  8 |
      bytes[switchaddr - 2] << 16 |
      bytes[switchaddr - 1] << 24;

      cpuid_family_addr = cpuid_family_addr + switchaddr;
      
      if (cpuid_family_addr) {
        // Determine cpuid_model address
        // for 10.6.8+ kernels it's 339 bytes apart from cpuid_family address
        cpuid_model_addr = cpuid_family_addr - 0x153;
        DBG ("%a: cpuid_family address = 0x%08x\n", __FUNCTION__, cpuid_family_addr);
        DBG ("%a: cpuid_model address  = 0x%08x\n", __FUNCTION__, cpuid_model_addr);
        // Calculate masks for patching
        mask_family  = cpuid_family_addr - (switchaddr +15);
        mask_model   = cpuid_model_addr -  (switchaddr +25);
        DBG ("%a: family mask = 0x%08x\n", __FUNCTION__, mask_family);
        DBG ("%a: model mask  = 0x%08x\n", __FUNCTION__, mask_model);
        // retain original
        // test ebx, ebx
        bytes[switchaddr+0] = bytes[patchLocation-13];
        bytes[switchaddr+1] = bytes[patchLocation-12];
        // retain original, but move jump offset by 20 bytes forward
        // jne for above test
        bytes[switchaddr+2] = bytes[patchLocation-11];
        bytes[switchaddr+3] = bytes[patchLocation-10]+0x20;
        // mov ebx, 0x78ea4fbc
        bytes[switchaddr+4] = 0xBB;
        if (gSettings.CpuFamily != 0) {
          DBG ("%a: overriding custom cpuid_family = 0x08x\n", __FUNCTION__, gSettings.CpuFamily);
          bytes[switchaddr+5] = (gSettings.CpuFamily & 0x000000FF) >>  0;
          bytes[switchaddr+6] = (gSettings.CpuFamily & 0x0000FF00) >>  8;
          bytes[switchaddr+7] = (UINT8) ((gSettings.CpuFamily & 0x00FF0000) >> 16);
          bytes[switchaddr+8] = (gSettings.CpuFamily & 0xFF000000) >> 24;
        } else {
          DBG ("%a: overriding cpuid_model as CPUID_INTEL_PENRYN\n", __FUNCTION__);
          bytes[switchaddr+5] = (CPUFAMILY_INTEL_PENRYN & 0x000000FF) >>  0;
          bytes[switchaddr+6] = (CPUFAMILY_INTEL_PENRYN & 0x0000FF00) >>  8;
          bytes[switchaddr+7] = (CPUFAMILY_INTEL_PENRYN & 0x00FF0000) >> 16;
          bytes[switchaddr+8] = (CPUFAMILY_INTEL_PENRYN & 0xFF000000) >> 24;
        }
        // mov dword, ebx
        bytes[switchaddr+9]  = 0x89;
        bytes[switchaddr+10] = 0x1D;
        // cpuid_cpufamily address 0xffffff80008a21a0
        bytes[switchaddr+11] = (mask_family & 0x000000FF) >> 0;
        bytes[switchaddr+12] = (mask_family & 0x0000FF00) >> 8;
        bytes[switchaddr+13] = (UINT8) ((mask_family & 0x00FF0000) >> 16);
        bytes[switchaddr+14] = (mask_family & 0xFF000000) >> 24;
        // mov dword
        bytes[switchaddr+15] = 0xC7;
        bytes[switchaddr+16] = 0x05;
        // cpuid_model address 0xffffff80008b204d
        bytes[switchaddr+17] = (mask_model & 0x000000FF) >> 0;
        bytes[switchaddr+18] = (mask_model & 0x0000FF00) >> 8;
        bytes[switchaddr+19] = (UINT8) ((mask_model & 0x00FF0000) >> 16);
        bytes[switchaddr+20] = (mask_model & 0xFF000000) >> 24;
        if (gSettings.CpuIdVars != 0) {
          DBG ("%a: overriding custom cpuid_model = 0x08x\n", __FUNCTION__, gSettings.CpuIdVars);
          bytes[switchaddr+21] = (gSettings.CpuIdVars & 0x000000FF) >>  0; // cpuid_model
          bytes[switchaddr+22] = (gSettings.CpuIdVars & 0x0000FF00) >>  8; // cpuid_extmodel
          bytes[switchaddr+23] = (UINT8) ((gSettings.CpuIdVars & 0x00FF0000) >> 16); // cpuid_extfamily
          bytes[switchaddr+24] = (gSettings.CpuIdVars & 0xFF000000) >> 24; // cpuid_stepping
          
        } else {
          DBG ("%a: overriding cpuid_model as CPUID_INTEL_PENRYN\n", __FUNCTION__);
          bytes[switchaddr+21] = 0x17; // cpuid_model
          bytes[switchaddr+22] = 0x01; // cpuid_extmodel
          bytes[switchaddr+23] = 0x00; // cpuid_extfamily
          bytes[switchaddr+24] = 0x02; // cpuid_stepping
        }
        // fill remainder with 25 NOPs
        for (i=25; i<25+25; i++) {
          bytes[switchaddr+i] = 0x90;
        }
      }
    } else {
      DBG ("%a: unable to determine cpuid_family address, patching aborted\n", __FUNCTION__);
      return;
    }
  }
}

VOID
KernelPatcher_32 (
  VOID* kernelData
)
{
  UINT8   *bytes;
  UINT32  patchLocation=0, patchLocation1=0;
  UINT32  i;
  UINT32  jumpaddr;
  
  bytes = (UINT8*)kernelData;
  // _cpuid_set_info _panic address
  for (i=0; i<0x1000000; i++) {
    if (bytes[i] == 0xC7 && bytes[i+1] == 0x05 && bytes[i+6] == 0x07 && bytes[i+7] == 0x00 &&
        bytes[i+8] == 0x00 && bytes[i+9] == 0x00 && bytes[i+10] == 0xC7 && bytes[i+11] == 0x05 &&
        bytes[i-5] == 0xE8) {
      patchLocation = i-5;
      break;
    }
  }
  if (!patchLocation) {
    return;
  }
  // this for 10.6.0 and 10.6.1 kernel and remove tsc.c unknow cpufamily panic
  //  c70424540e5900
  // find _tsc_init panic address
  for (i=0; i<0x1000000; i++) {
    // _cpuid_set_info _panic address
    if (bytes[i] == 0xC7 && bytes[i+1] == 0x04 && bytes[i+2] == 0x24 && bytes[i+3] == 0x54 &&
        bytes[i+4] == 0x0E && bytes[i+5] == 0x59 && bytes[i+6] == 0x00) {
      patchLocation1 = i+7;
      break;
    }
  }
  // found _tsc_init panic addres and patch it
  if (patchLocation1) {
    bytes[patchLocation1 + 0] = 0x90;
    bytes[patchLocation1 + 1] = 0x90;
    bytes[patchLocation1 + 2] = 0x90;
    bytes[patchLocation1 + 3] = 0x90;
    bytes[patchLocation1 + 4] = 0x90;
  }
  //first move panic code total 5 bytes, if patch cpuid fail still can boot with kernel
  bytes[patchLocation + 0] = 0x90;
  bytes[patchLocation + 1] = 0x90;
  bytes[patchLocation + 2] = 0x90;
  bytes[patchLocation + 3] = 0x90;
  bytes[patchLocation + 4] = 0x90;
  
  jumpaddr = patchLocation;
  
  for (i=0;i<500;i++) {
    if (bytes[jumpaddr-i-3] == 0x85 && bytes[jumpaddr-i-2] == 0xC0 &&
        bytes[jumpaddr-i-1] == 0x75 ) {
      jumpaddr -= i;
      bytes[jumpaddr-1] = 0x77;
      if (bytes[patchLocation - 17] == 0xC7) {
        bytes[jumpaddr] -=10;
      }
      break;
    }
  }
  
  if (jumpaddr == patchLocation) {
    return;  //can't find jump location
  }
  // patch info_p->cpufamily to CPUFAMILY_INTEL_MEROM
  if (bytes[patchLocation - 17] == 0xC7) {
    bytes[patchLocation - 11] = (CPUFAMILY_INTEL_YONAH & 0x000000FF) >>  0;
    bytes[patchLocation - 10] = (CPUFAMILY_INTEL_YONAH & 0x0000FF00) >>  8;
    bytes[patchLocation -  9] = (CPUFAMILY_INTEL_YONAH & 0x00FF0000) >> 16;
    bytes[patchLocation -  8] = (CPUFAMILY_INTEL_YONAH & 0xFF000000) >> 24;
  }
  //patch info->cpuid_cpufamily
  bytes[patchLocation -  7] = 0xC7;
  bytes[patchLocation -  6] = 0x05;
  bytes[patchLocation -  5] = bytes[jumpaddr + 3];
  bytes[patchLocation -  4] = bytes[jumpaddr + 4];
  bytes[patchLocation -  3] = bytes[jumpaddr + 5];
  bytes[patchLocation -  2] = bytes[jumpaddr + 6];
  
  bytes[patchLocation -  1] = CPUIDFAMILY_DEFAULT; //cpuid_family  need alway set 0x06
  bytes[patchLocation +  0] = CPUID_MODEL_YONAH;   //cpuid_model set CPUID_MODEL_MEROM
  bytes[patchLocation +  1] = 0x01;                //cpuid_extmodel alway set 0x01
  bytes[patchLocation +  2] = 0x00;                //cpuid_extfamily alway set 0x00
  bytes[patchLocation +  3] = 0x90;
  bytes[patchLocation +  4] = 0x90;
  
  if (KernVersion) {
    if (AsciiStrnCmp (KernVersion,"11",2) == 0) return;
    if (!SSSE3 && (AsciiStrnCmp (KernVersion,"10",2) == 0)) {
      Patcher_SSE3_6 ((VOID*) bytes);
    }
    if (!SSSE3 && (AsciiStrnCmp (KernVersion,"9",1) == 0)) {
      Patcher_SSE3_5 ((VOID*) bytes);
    }
  }
}

VOID
KernelLapicPatch_64 (
  VOID *kernelData
)
{
  // Credits to donovan6000 and sherlocks for providing the lapic kernel patch source used to build this function
  UINT8       *bytes = (UINT8*)kernelData;
  UINT32      patchLocation=0;
  UINT32      i;
  
  for (i=0; i<0x1000000; i++) {
    if (bytes[i+0]  == 0x65 && bytes[i+1]  == 0x8B && bytes[i+2]  == 0x04 && bytes[i+3]  == 0x25 &&
        bytes[i+4]  == 0x14 && bytes[i+5]  == 0x00 && bytes[i+6]  == 0x00 && bytes[i+7]  == 0x00 &&
        bytes[i+35] == 0x65 && bytes[i+36] == 0x8B && bytes[i+37] == 0x04 && bytes[i+38] == 0x25 &&
        bytes[i+39] == 0x14 && bytes[i+40] == 0x00 && bytes[i+41] == 0x00 && bytes[i+42] == 0x00) {
      patchLocation = i+30;
      break;
    } else if (bytes[i+0] == 0x65 && bytes[i+1] == 0x8B && bytes[i+2] == 0x04 && bytes[i+3] == 0x25 &&
               bytes[i+4] == 0x1C && bytes[i+5] == 0x00 && bytes[i+6] == 0x00 && bytes[i+7] == 0x00 &&
               bytes[i+36] == 0x65 && bytes[i+37] == 0x8B && bytes[i+38] == 0x04 && bytes[i+39] == 0x25 &&
               bytes[i+40] == 0x1C && bytes[i+41] == 0x00 && bytes[i+42] == 0x00 && bytes[i+43] == 0x00) {
      patchLocation = i+31;
      break;
      //rehabman: 10.10.DP1 lapic
    } else if (bytes[i+0] == 0x65 && bytes[i+1] == 0x8B && bytes[i+2] == 0x04 && bytes[i+3] == 0x25 &&
               bytes[i+4] == 0x1C && bytes[i+5] == 0x00 && bytes[i+6] == 0x00 && bytes[i+7] == 0x00 &&
               bytes[i+33] == 0x65 && bytes[i+34] == 0x8B && bytes[i+35] == 0x04 && bytes[i+36] == 0x25 &&
               bytes[i+37] == 0x1C && bytes[i+38] == 0x00 && bytes[i+39] == 0x00 && bytes[i+40] == 0x00) {
      patchLocation = i+28;
      break;
    }
  }
  if (!patchLocation) {
    return;
  }
  // Already patched?  May be running a non-vanilla kernel already?
  if ((bytes[patchLocation + 0] == 0x90 && bytes[patchLocation + 1] == 0x90 &&
      bytes[patchLocation + 2] == 0x90 && bytes[patchLocation + 3] == 0x90 &&
      bytes[patchLocation + 4] == 0x90) != TRUE) {
    bytes[patchLocation + 0] = 0x90;
    bytes[patchLocation + 1] = 0x90;
    bytes[patchLocation + 2] = 0x90;
    bytes[patchLocation + 3] = 0x90;
    bytes[patchLocation + 4] = 0x90;
  }
}

VOID
KernelLapicPatch_32 (
  VOID *kernelData
)
{
  // Credits to donovan6000 and sherlocks for providing the lapic kernel patch source used to build this function
  UINT8       *bytes = (UINT8*)kernelData;
  UINT32      patchLocation=0;
  UINT32      i;
  
  for (i=0; i<0x1000000; i++) {
    if (bytes[i+0]  == 0x65 && bytes[i+1]  == 0xA1 && bytes[i+2]  == 0x0C && bytes[i+3]  == 0x00 &&
        bytes[i+4]  == 0x00 && bytes[i+5]  == 0x00 &&
        bytes[i+30] == 0x65 && bytes[i+31] == 0xA1 && bytes[i+32] == 0x0C && bytes[i+33] == 0x00 &&
        bytes[i+34] == 0x00 && bytes[i+35] == 0x00) {
      patchLocation = i+25;
      break;
    }
  }
  if (!patchLocation) {
    return;
  }
  // Already patched?  May be running a non-vanilla kernel already?
  if ((bytes[patchLocation + 0] == 0x90 && bytes[patchLocation + 1] == 0x90 &&
      bytes[patchLocation + 2] == 0x90 && bytes[patchLocation + 3] == 0x90 &&
      bytes[patchLocation + 4] == 0x90) != TRUE) {
    bytes[patchLocation + 0] = 0x90;
    bytes[patchLocation + 1] = 0x90;
    bytes[patchLocation + 2] = 0x90;
    bytes[patchLocation + 3] = 0x90;
    bytes[patchLocation + 4] = 0x90;
  }
}
#endif

VOID
FindBootArgs (
  VOID
)
{
  UINT8           *ptr;
  UINT8           archMode;

  archMode = sizeof (UINTN) * 8;
  // start searching from 0x200000.
  ptr = (UINT8 *) (UINTN) 0x200000;
  
  while(TRUE) {
    // check bootargs for 10.7 and up
    bootArgs2 = (BootArgs2 *) ptr;
    
    if (bootArgs2->Version == 2 && bootArgs2->Revision == 0 &&
        bootArgs2->efiMode == archMode &&
        bootArgs2->kaddr == 0 && bootArgs2->ksize == 0 &&
        bootArgs2->efiSystemTable == 0) {
      // set vars
      dtRoot = (CHAR8 *) (UINTN) bootArgs2->deviceTreeP;
      KernelSlide = bootArgs2->kslide;
      DBG ("%a: bootArgs2->Revision 0x%x\n", __FUNCTION__, bootArgs2->Revision);
      DBG ("%a: bootArgs2->Version 0x%x\n", __FUNCTION__, bootArgs2->Version);
      DBG ("%a: bootArgs2->efiMode 0x%x\n", __FUNCTION__, bootArgs2->efiMode);
      DBG ("%a: bootArgs2->debugMode 0x%x\n", __FUNCTION__, bootArgs2->debugMode);
      DBG ("%a: bootArgs2->flags 0x%x\n", __FUNCTION__, bootArgs2->flags);
      DBG ("%a: bootArgs2->CommandLine %a\n", __FUNCTION__, bootArgs2->CommandLine);
      DBG ("%a: bootArgs2->MemoryMap 0x%x\n", __FUNCTION__, bootArgs2->MemoryMap);
      DBG ("%a: bootArgs2->MemoryMapSize 0x%x\n", __FUNCTION__, bootArgs2->MemoryMapSize);
      DBG ("%a: bootArgs2->MemoryMapDescriptorSize 0x%x\n", __FUNCTION__, bootArgs2->MemoryMapDescriptorSize);
      DBG ("%a: bootArgs2->MemoryMapDescriptorVersion 0x%x\n", __FUNCTION__, bootArgs2->MemoryMapDescriptorVersion);
      DBG ("%a: bootArgs2->deviceTreeP 0x%x\n", __FUNCTION__, bootArgs2->deviceTreeP);
      DBG ("%a: bootArgs2->deviceTreeLength 0x%x\n", __FUNCTION__, bootArgs2->deviceTreeLength);
      DBG ("%a: bootArgs2->kaddr 0x%x\n", __FUNCTION__, bootArgs2->kaddr);
      DBG ("%a: bootArgs2->ksize 0x%x\n", __FUNCTION__, bootArgs2->ksize);
      DBG ("%a: bootArgs2->efiRuntimeServicesPageStart 0x%x\n", __FUNCTION__, bootArgs2->efiRuntimeServicesPageStart);
      DBG ("%a: bootArgs2->efiRuntimeServicesPageCount 0x%x\n", __FUNCTION__, bootArgs2->efiRuntimeServicesPageCount);
      DBG ("%a: bootArgs2->efiRuntimeServicesVirtualPageStart 0x%x\n", __FUNCTION__, bootArgs2->efiRuntimeServicesVirtualPageStart);
      DBG ("%a: bootArgs2->efiSystemTable 0x%x\n", __FUNCTION__, bootArgs2->efiSystemTable);
      DBG ("%a: bootArgs2->kslide 0x%x\n", __FUNCTION__, bootArgs2->kslide);
      DBG ("%a: bootArgs2->performanceDataStart 0x%x\n", __FUNCTION__, bootArgs2->performanceDataStart);
      DBG ("%a: bootArgs2->performanceDataSize 0x%x\n", __FUNCTION__, bootArgs2->performanceDataSize);
      DBG ("%a: bootArgs2->keyStoreDataStart 0x%x\n", __FUNCTION__, bootArgs2->keyStoreDataStart);
      DBG ("%a: bootArgs2->keyStoreDataSize 0x%x\n", __FUNCTION__, bootArgs2->keyStoreDataSize);
      DBG ("%a: bootArgs2->bootMemStart 0x%x\n", __FUNCTION__, bootArgs2->bootMemStart);
      DBG ("%a: bootArgs2->bootMemSize 0x%x\n", __FUNCTION__, bootArgs2->bootMemSize);
      DBG ("%a: bootArgs2->PhysicalMemorySize 0x%x\n", __FUNCTION__, bootArgs2->PhysicalMemorySize);
      DBG ("%a: bootArgs2->FSBFrequency 0x%x\n", __FUNCTION__, bootArgs2->FSBFrequency);
      DBG ("%a: bootArgs2->pciConfigSpaceBaseAddress 0x%x\n", __FUNCTION__, bootArgs2->pciConfigSpaceBaseAddress);
      DBG ("%a: bootArgs2->pciConfigSpaceStartBusNumber 0x%x\n", __FUNCTION__, bootArgs2->pciConfigSpaceStartBusNumber);
      DBG ("%a: bootArgs2->pciConfigSpaceEndBusNumber 0x%x\n", __FUNCTION__, bootArgs2->pciConfigSpaceEndBusNumber);
#ifdef MEMMAP_DEBUG
      EFI_MEMORY_DESCRIPTOR       *MemoryMap;
      UINTN           MCount, i;

      MemoryMap = ((EFI_MEMORY_DESCRIPTOR *) (UINTN) bootArgs2->MemoryMap);
      MCount = (UINTN) DivU64x32 (bootArgs2->MemoryMapSize, bootArgs2->MemoryMapDescriptorSize);
      
      DBG ("   Type       Physical Start    Physical End      Number of Pages   Virtual Start     Attribute\n");
      for (i = 0; i < MCount; i++) {
        DBG ("%02d %a %016lx  %016lx  %016lx  %016lx  %016x\n",
             i,
             EfiMemoryTypeDesc[MemoryMap->Type],
             MemoryMap->PhysicalStart,
             MemoryMap->PhysicalStart + (((UINTN) MemoryMap->NumberOfPages * EFI_PAGE_SIZE)) - 1,
             MemoryMap->NumberOfPages,
             MemoryMap->VirtualStart,
             MemoryMap->Attribute);
        MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, bootArgs2->MemoryMapDescriptorSize);
      }
#endif
      // disable other pointer
      bootArgs1 = NULL;
      break;
    }
    
    // check bootargs for 10.4 - 10.6.x  
    bootArgs1 = (BootArgs1 *) ptr;
    
    if (bootArgs1->Version == 1 &&
        (bootArgs1->Revision == 6 || bootArgs1->Revision == 5 || bootArgs1->Revision == 4) &&
        bootArgs1->efiMode == archMode &&
        bootArgs1->kaddr == 0 && bootArgs1->ksize == 0 &&
        bootArgs1->efiSystemTable == 0) {
      // set vars
      dtRoot = (CHAR8 *) (UINTN) bootArgs1->deviceTreeP;
      DBG ("%a: bootArgs1->Revision 0x%x\n", __FUNCTION__, bootArgs1->Revision);
      DBG ("%a: bootArgs1->Version 0x%x\n", __FUNCTION__, bootArgs1->Version);
      DBG ("%a: bootArgs1->efiMode 0x%x\n", __FUNCTION__, bootArgs1->efiMode);
      DBG ("%a: bootArgs1->CommandLine %a\n", __FUNCTION__, bootArgs1->CommandLine);
      DBG ("%a: bootArgs1->MemoryMap 0x%x\n", __FUNCTION__, bootArgs1->MemoryMap);
      DBG ("%a: bootArgs1->MemoryMapSize 0x%x\n", __FUNCTION__, bootArgs1->MemoryMapSize);
      DBG ("%a: bootArgs1->MemoryMapDescriptorSize 0x%x\n", __FUNCTION__, bootArgs1->MemoryMapDescriptorSize);
      DBG ("%a: bootArgs1->MemoryMapDescriptorVersion 0x%x\n", __FUNCTION__, bootArgs1->MemoryMapDescriptorVersion);
      DBG ("%a: bootArgs1->deviceTreeP 0x%x\n", __FUNCTION__, bootArgs1->deviceTreeP);
      DBG ("%a: bootArgs1->deviceTreeLength 0x%x\n", __FUNCTION__, bootArgs1->deviceTreeLength);
      DBG ("%a: bootArgs1->kaddr 0x%x\n", __FUNCTION__, bootArgs1->kaddr);
      DBG ("%a: bootArgs1->ksize 0x%x\n", __FUNCTION__, bootArgs1->ksize);
      DBG ("%a: bootArgs1->efiRuntimeServicesPageStart 0x%x\n", __FUNCTION__, bootArgs1->efiRuntimeServicesPageStart);
      DBG ("%a: bootArgs1->efiRuntimeServicesPageCount 0x%x\n", __FUNCTION__, bootArgs1->efiRuntimeServicesPageCount);
      DBG ("%a: bootArgs1->efiSystemTable 0x%x\n", __FUNCTION__, bootArgs1->efiSystemTable);
      DBG ("%a: bootArgs1->performanceDataStart 0x%x\n", __FUNCTION__, bootArgs1->performanceDataStart);
      DBG ("%a: bootArgs1->performanceDataSize 0x%x\n", __FUNCTION__, bootArgs1->performanceDataSize);
      DBG ("%a: bootArgs1->efiRuntimeServicesVirtualPageStart 0x%x\n", __FUNCTION__, bootArgs1->efiRuntimeServicesVirtualPageStart);

#ifdef MEMMAP_DEBUG
      EFI_MEMORY_DESCRIPTOR       *MemoryMap;
      UINTN           MCount, i;

      MemoryMap = ((EFI_MEMORY_DESCRIPTOR *) (UINTN) bootArgs1->MemoryMap);
      MCount = (UINTN) DivU64x32 (bootArgs1->MemoryMapSize, bootArgs1->MemoryMapDescriptorSize);
      
      DBG ("   Type       Physical Start    Physical End      Number of Pages   Virtual Start     Attribute\n");
      for (i = 0; i < MCount; i++) {
        DBG ("%02d %a %016lx  %016lx  %016lx  %016lx  %016x\n",
             i,
             EfiMemoryTypeDesc[MemoryMap->Type],
             MemoryMap->PhysicalStart,
             MemoryMap->PhysicalStart + (((UINTN) MemoryMap->NumberOfPages * EFI_PAGE_SIZE)) - 1,
             MemoryMap->NumberOfPages,
             MemoryMap->VirtualStart,
             MemoryMap->Attribute);
        MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, bootArgs1->MemoryMapDescriptorSize);
      }
#endif
      // disable other pointer
      bootArgs2 = NULL;
      break;
    }

    ptr += 0x1000;
  }
}

VOID
KernelAndKextPatcherInit (
  VOID
)
{
  if (PatcherInited) {
    return;
  }
  
  // KernelRelocBase will normally be 0
  // but if OsxAptioFixDrv is used, then it will be > 0
  SetKernelRelocBase ();
  // Find bootArgs - we need then for proper detection
  // of kernel Mach-O header
  FindBootArgs ();

  if (bootArgs1 == NULL && bootArgs2 == NULL) {
    return;
  }
  
  // Find kernel Mach-O header:
  // for ML: bootArgs2->kslide + 0x00200000
  // for older versions: just 0x200000
  // for AptioFix booting - it's always at KernelRelocBase + 0x200000
  KernelData = (VOID*)(UINTN)(KernelSlide + KernelRelocBase + 0x00200000);
  
  // check that it is Mach-O header and detect architecture
  if (MACH_GET_MAGIC(KernelData) == MH_MAGIC || MACH_GET_MAGIC(KernelData) == MH_CIGAM) {
    is64BitKernel = FALSE;
  } else if (MACH_GET_MAGIC(KernelData) == MH_MAGIC_64 || MACH_GET_MAGIC(KernelData) == MH_CIGAM_64) {
    is64BitKernel = TRUE;
  } else {
    // not valid Mach-O header - exiting
    KernelData = NULL;
    return;
  }

  // find __PRELINK_TEXT and __PRELINK_INFO
  GetSection (kPrelinkTextSegment, kPrelinkTextSection, &PrelinkTextAddr, &PrelinkTextSize);
  GetSection (kPrelinkInfoSegment, kPrelinkInfoSection, &PrelinkInfoAddr, &PrelinkInfoSize);
#ifdef KERNEL_PATCH_DEBUG
  Print (L"%a: PrelinkTextAddr = 0x%x, PrelinkTextSize = 0x%x\n", __FUNCTION__, PrelinkTextAddr, PrelinkTextSize);
  Print (L"%a: PrelinkInfoAddr = 0x%x, PrelinkInfoSize = 0x%x\n", __FUNCTION__, PrelinkInfoAddr, PrelinkInfoSize);
#endif

  isKernelcache = PrelinkTextSize > 0 && PrelinkInfoSize > 0;

  DBG ("%a: OSVersion = %a\n", __FUNCTION__, OSVersion);
  KernVersion = GetKernelVersion ();
  DBG ("%a: KernVersion = %a\n", __FUNCTION__, KernVersion);
#ifdef KERNEL_PATCH_DEBUG
  Print (L"%a: OSVersion = %a\n", __FUNCTION__, OSVersion);
  Print (L"%a: KernVersion = %a\n", __FUNCTION__, KernVersion);
#endif

  PatcherInited = TRUE;
}

VOID
EFIAPI
AnyKernelPatch (
  IN UINT8 *Kernel
)
{
  UINTN   Num = 0, i;

  for (i = 0; i < gSettings.NrKernel; i++) {
    if (gSettings.AnyKernelData[i] > 0) {
      Num = SearchAndReplace (
              Kernel,
              KERNEL_MAX_SIZE,
              gSettings.AnyKernelData[i],
              gSettings.AnyKernelDataLen[i],
              gSettings.AnyKernelPatch[i],
              1
            );
    }
  }
}

VOID
KernelAndKextsPatcherStart (
  VOID
)
{
  UINT32      deviceTreeP;
  UINT32      deviceTreeLength;
  EFI_STATUS  Status;

  //
  // Kernel & Kexts patches
  //
  KernelAndKextPatcherInit ();

  if (KernelData == NULL) {
    DBG ("%a: kernel data is NULL.\n", __FUNCTION__);
#ifdef KERNEL_PATCH_DEBUG
    Print (L"%a: kernel data is NULL.\n", __FUNCTION__);
#endif
    return;
  }

  if (gSettings.PatchCPU) {
    if (is64BitKernel) {
      KernelPatcher_64 (KernelData);
    } else {
      KernelPatcher_32 (KernelData);
    }
  }

  // CPU power management patch for haswell with locked msr
  if (gSettings.PatchPM) {
    if (is64BitKernel) {
      KernelPatchPm (KernelData);
    }
  }

  if (gSettings.PatchLAPIC) {
    if (is64BitKernel) {
      KernelLapicPatch_64 (KernelData);
    } else {
      KernelLapicPatch_32 (KernelData);
    }
  }
  
  if (gSettings.KPKernelPatchesNeeded) {
    AnyKernelPatch (KernelData);
  }

  if (gSettings.KPKextPatchesNeeded) {
    KextPatcherStart ();
  }

  if (gSettings.CheckFakeSMC && PrelinkInfoAddr != 0) {
    if ((AsciiStrStr((CHAR8 *) (UINTN) PrelinkInfoAddr,
         "<string>org.netkas.driver.FakeSMC</string>") != NULL) ||
        (AsciiStrStr((CHAR8 *) (UINTN) PrelinkInfoAddr,
         "<string>org.netkas.FakeSMC</string>") != NULL)) {
      WithKexts = FALSE;
    }
  }

  if (WithKexts) {
    if (bootArgs1 != NULL) {
      deviceTreeP = bootArgs1->deviceTreeP;
      deviceTreeLength = bootArgs1->deviceTreeLength;
    } else if (bootArgs2 != NULL) {
      deviceTreeP = bootArgs2->deviceTreeP;
      deviceTreeLength = bootArgs2->deviceTreeLength;
    } else {
      DBG ("%a: Kext Injection - bootArgs not found.\n", __FUNCTION__);
#ifdef KEXT_INJECT_DEBUG
      Print (L"Kext Injection: bootArgs not found.\n");
#endif
      return;
    }

    Status = InjectKexts (deviceTreeP, &deviceTreeLength);

    if (!EFI_ERROR(Status)) {
      KernelBooterExtensionsPatch ();
    } else {
      DBG ("%a: InjectKexts error with status %r.\n", __FUNCTION__, Status);
#ifdef KEXT_INJECT_DEBUG
      Print (L"Kext Injection: InjectKexts error with status %r.\n", Status);
#endif
    }
  }
}
