/** @file
    Provides simple log services to memory buffer.
**/

#ifndef __LEGACYBIOSTHUNK_LIB_H__
#define __LEGACYBIOSTHUNK_LIB_H__

#include <Protocol/Legacy8259.h>

#define EFI_SEGMENT(_Adr)     (UINT16) ((UINT16) (((UINTN) (_Adr)) >> 4) & 0xf000)
#define EFI_OFFSET(_Adr)      (UINT16) (((UINT16) ((UINTN) (_Adr))) & 0xffff)


/**
 Initialize legacy environment for BIOS INI caller.

 @param ThunkContext   the instance pointer of THUNK_CONTEXT
 **/
VOID
InitializeBiosIntCaller (
  THUNK_CONTEXT     *ThunkContext
);

/**
 Initialize interrupt redirection code and entries, because
 IDT Vectors 0x68-0x6f must be redirected to IDT Vectors 0x08-0x0f.
 Or the interrupt will lost when we do thunk.
 NOTE: We do not reset 8259 vector base, because it will cause pending
 interrupt lost.

 @param Legacy8259  Instance pointer for EFI_LEGACY_8259_PROTOCOL.

 **/
VOID
InitializeInterruptRedirection (
  IN  EFI_LEGACY_8259_PROTOCOL  *Legacy8259
);

/**
 Thunk to 16-bit real mode and execute a software interrupt with a vector
 of BiosInt. Regs will contain the 16-bit register context on entry and
 exit.

 @param  This    Protocol instance pointer.
 @param  BiosInt Processor interrupt vector to invoke
 @param  Reg     Register contexted passed into (and returned) from thunk to 16-bit mode

 @retval TRUE   Thunk completed, and there were no BIOS errors in the target code.
 See Regs for status.
 @retval FALSE  There was a BIOS erro in the target code.
 **/
BOOLEAN
EFIAPI
LegacyBiosInt86 (
  IN  EFI_LEGACY_8259_PROTOCOL *Legacy8259,
  IN  THUNK_CONTEXT            *ThunkContext,
  IN  UINT8                    BiosInt,
  IN  IA32_REGISTER_SET        *Regs
);
#endif // __LEGACYBIOSTHUNK_LIB_H__
