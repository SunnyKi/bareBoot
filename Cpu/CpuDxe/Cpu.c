/*++

 Copyright (c) 2006 - 2009, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 Module Name:
 Cpu.c

 Abstract:

--*/

#include "CpuDxe.h"

//
// Global Variables
//

EFI_LEGACY_8259_PROTOCOL             *gLegacy8259 = NULL;
THUNK_CONTEXT                        mThunkContext;
BOOLEAN                              mInterruptState = FALSE;
UINTN                                mTimerVector = 0;

volatile EFI_CPU_INTERRUPT_HANDLER   mTimerHandler = NULL;

extern UINT32                        mExceptionCodeSize;
//
// The Cpu Architectural Protocol that this Driver produces
//
EFI_HANDLE              mHandle = NULL;
EFI_CPU_ARCH_PROTOCOL   mCpu = {
  CpuFlushCpuDataCache,  //used in LightMemoryTest
  CpuEnableInterrupt,   //not-used in LegacyBoot
  CpuDisableInterrupt,
  CpuGetInterruptState,
  CpuInit,
  CpuRegisterInterruptHandler, //used in 8254Timer, HPETTimer, 
  CpuGetTimerValue,
  CpuSetMemoryAttributes,
  1,          // NumberOfTimers
  4,          // DmaBufferAlignment
};

EFI_STATUS
EFIAPI
CpuFlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL           *This,
  IN EFI_PHYSICAL_ADDRESS            Start,
  IN UINT64                          Length,
  IN EFI_CPU_FLUSH_TYPE              FlushType
  )
/*++

Routine Description:
  Flush CPU data cache. If the instruction cache is fully coherent
  with all DMA operations then function can just return EFI_SUCCESS.

Arguments:
  This                - Protocol instance structure
  Start               - Physical address to start flushing from.
  Length              - Number of bytes to flush. Round up to chipset 
                         granularity.
  FlushType           - Specifies the type of flush operation to perform.

Returns: 

  EFI_SUCCESS           - If cache was flushed
  EFI_UNSUPPORTED       - If flush type is not supported.
  EFI_DEVICE_ERROR      - If requested range could not be flushed.

--*/
{
  if (FlushType == EfiCpuFlushTypeWriteBackInvalidate) {
    AsmWbinvd ();
    return EFI_SUCCESS;
  } else if (FlushType == EfiCpuFlushTypeInvalidate) {
    AsmInvd ();
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
}


EFI_STATUS
EFIAPI
CpuEnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL          *This
  )
/*++

Routine Description:
  Enables CPU interrupts.

Arguments:
  This                - Protocol instance structure

Returns: 
  EFI_SUCCESS           - If interrupts were enabled in the CPU
  EFI_DEVICE_ERROR      - If interrupts could not be enabled on the CPU.

--*/
{
  EnableInterrupts (); 

  mInterruptState  = TRUE;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
CpuDisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL          *This
  )
/*++

Routine Description:
  Disables CPU interrupts.

Arguments:
  This                - Protocol instance structure

Returns: 
  EFI_SUCCESS           - If interrupts were disabled in the CPU.
  EFI_DEVICE_ERROR      - If interrupts could not be disabled on the CPU.

--*/
{
  DisableInterrupts ();
  
  mInterruptState = FALSE;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
CpuGetInterruptState (
  IN  EFI_CPU_ARCH_PROTOCOL         *This,
  OUT BOOLEAN                       *State
  )
/*++

Routine Description:
  Return the state of interrupts.

Arguments:
  This                - Protocol instance structure
  State               - Pointer to the CPU's current interrupt state

Returns: 
  EFI_SUCCESS           - If interrupts were disabled in the CPU.
  EFI_INVALID_PARAMETER - State is NULL.
  
--*/
{
  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *State = mInterruptState;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
CpuInit (
  IN EFI_CPU_ARCH_PROTOCOL           *This,
  IN EFI_CPU_INIT_TYPE               InitType
  )

/*++

Routine Description:
  Generates an INIT to the CPU

Arguments:
  This                - Protocol instance structure
  InitType            - Type of CPU INIT to perform

Returns: 
  EFI_SUCCESS           - If CPU INIT occurred. This value should never be
        seen.
  EFI_DEVICE_ERROR      - If CPU INIT failed.
  EFI_NOT_SUPPORTED     - Requested type of CPU INIT not supported.

--*/
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
CpuRegisterInterruptHandler (
  IN EFI_CPU_ARCH_PROTOCOL          *This,
  IN EFI_EXCEPTION_TYPE             InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER      InterruptHandler
  )
/*++

Routine Description:
  Registers a function to be called from the CPU interrupt handler.

Arguments:
  This                - Protocol instance structure
  InterruptType       - Defines which interrupt to hook. IA-32 valid range
                         is 0x00 through 0xFF
  InterruptHandler    - A pointer to a function of type 
      EFI_CPU_INTERRUPT_HANDLER that is called when a
      processor interrupt occurs. A null pointer
      is an error condition.

Returns: 
  EFI_SUCCESS           - If handler installed or uninstalled.
  EFI_ALREADY_STARTED   - InterruptHandler is not NULL, and a handler for 
        InterruptType was previously installed
  EFI_INVALID_PARAMETER - InterruptHandler is NULL, and a handler for 
        InterruptType was not previously installed.
  EFI_UNSUPPORTED       - The interrupt specified by InterruptType is not
        supported.

--*/
{
  if ((InterruptType < 0) || (InterruptType >= INTERRUPT_VECTOR_NUMBER)) {
    return EFI_UNSUPPORTED;
  }
  if ((UINTN)(UINT32)InterruptType != mTimerVector) {
    return EFI_UNSUPPORTED;
  }
  if ((mTimerHandler == NULL) && (InterruptHandler == NULL)) {
    return EFI_INVALID_PARAMETER;
  } else if ((mTimerHandler != NULL) && (InterruptHandler != NULL)) {
    return EFI_ALREADY_STARTED;
  }
  mTimerHandler = InterruptHandler;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CpuGetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL          *This,
  IN  UINT32                         TimerIndex,
  OUT UINT64                         *TimerValue,
  OUT UINT64                         *TimerPeriod   OPTIONAL
  )
/*++

Routine Description:
  Returns a timer value from one of the CPU's internal timers. There is no
  inherent time interval between ticks but is a function of the CPU 
  frequency.

Arguments:
  This                - Protocol instance structure
  TimerIndex          - Specifies which CPU timer ie requested
  TimerValue          - Pointer to the returned timer value
  TimerPeriod         - 

Returns: 
  EFI_SUCCESS           - If the CPU timer count was returned.
  EFI_UNSUPPORTED       - If the CPU does not have any readable timers
  EFI_DEVICE_ERROR      - If an error occurred reading the timer.
  EFI_INVALID_PARAMETER - TimerIndex is not valid

--*/
{  
  if (TimerValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (TimerIndex == 0) {
    *TimerValue = AsmReadTsc ();
    if (TimerPeriod != NULL) {
      //
      // BugBug: Hard coded. Don't know how to do this generically
      //
      *TimerPeriod = 1000000000;
    }
    return EFI_SUCCESS;
  }
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
CpuSetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes
  )
/*++

Routine Description:
  Set memory cacheability attributes for given range of memeory

Arguments:
  This                - Protocol instance structure
  BaseAddress         - Specifies the start address of the memory range
  Length              - Specifies the length of the memory range
  Attributes          - The memory cacheability for the memory range

Returns: 
  EFI_SUCCESS           - If the cacheability of that memory range is set successfully
  EFI_UNSUPPORTED       - If the desired operation cannot be done
  EFI_INVALID_PARAMETER - The input parameter is not correct, such as Length = 0

--*/
{
  return EFI_UNSUPPORTED;
}

STATIC
VOID
DumpExceptionDataDebugOut (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  UINT32        ErrorCodeFlag;

  ErrorCodeFlag = 0x00027d00;

#ifdef MDE_CPU_IA32
  DEBUG ((
    EFI_D_ERROR,
    "!!!! IA32 Exception Type - %08x !!!!\n",
    InterruptType
    ));
  DEBUG ((
    EFI_D_ERROR,
    "EIP - %08x, CS - %08x, EFLAGS - %08x\n",
    SystemContext.SystemContextIa32->Eip,
    SystemContext.SystemContextIa32->Cs,
    SystemContext.SystemContextIa32->Eflags
    ));
  if (ErrorCodeFlag & (1 << InterruptType)) {
    DEBUG ((
      EFI_D_ERROR,
      "ExceptionData - %08x\n",
      SystemContext.SystemContextIa32->ExceptionData
      ));
  }
  DEBUG ((
    EFI_D_ERROR,
    "EAX - %08x, ECX - %08x, EDX - %08x, EBX - %08x\n",
    SystemContext.SystemContextIa32->Eax,
    SystemContext.SystemContextIa32->Ecx,
    SystemContext.SystemContextIa32->Edx,
    SystemContext.SystemContextIa32->Ebx
    ));
  DEBUG ((
    EFI_D_ERROR,
    "ESP - %08x, EBP - %08x, ESI - %08x, EDI - %08x\n",
    SystemContext.SystemContextIa32->Esp,
    SystemContext.SystemContextIa32->Ebp,
    SystemContext.SystemContextIa32->Esi,
    SystemContext.SystemContextIa32->Edi
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DS - %08x, ES - %08x, FS - %08x, GS - %08x, SS - %08x\n",
    SystemContext.SystemContextIa32->Ds,
    SystemContext.SystemContextIa32->Es,
    SystemContext.SystemContextIa32->Fs,
    SystemContext.SystemContextIa32->Gs,
    SystemContext.SystemContextIa32->Ss
    ));
  DEBUG ((
    EFI_D_ERROR,
    "GDTR - %08x %08x, IDTR - %08x %08x\n",
    SystemContext.SystemContextIa32->Gdtr[0],
    SystemContext.SystemContextIa32->Gdtr[1],
    SystemContext.SystemContextIa32->Idtr[0],
    SystemContext.SystemContextIa32->Idtr[1]
    ));
  DEBUG ((
    EFI_D_ERROR,
    "LDTR - %08x, TR - %08x\n",
    SystemContext.SystemContextIa32->Ldtr,
    SystemContext.SystemContextIa32->Tr
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR0 - %08x, CR2 - %08x, CR3 - %08x, CR4 - %08x\n",
    SystemContext.SystemContextIa32->Cr0,
    SystemContext.SystemContextIa32->Cr2,
    SystemContext.SystemContextIa32->Cr3,
    SystemContext.SystemContextIa32->Cr4
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR0 - %08x, DR1 - %08x, DR2 - %08x, DR3 - %08x\n",
    SystemContext.SystemContextIa32->Dr0,
    SystemContext.SystemContextIa32->Dr1,
    SystemContext.SystemContextIa32->Dr2,
    SystemContext.SystemContextIa32->Dr3
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR6 - %08x, DR7 - %08x\n",
    SystemContext.SystemContextIa32->Dr6,
    SystemContext.SystemContextIa32->Dr7
    ));
#else
  DEBUG ((
    EFI_D_ERROR,
    "!!!! X64 Exception Type - %016lx !!!!\n",
    (UINT64)InterruptType
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RIP - %016lx, CS - %016lx, RFLAGS - %016lx\n",
    SystemContext.SystemContextX64->Rip,
    SystemContext.SystemContextX64->Cs,
    SystemContext.SystemContextX64->Rflags
    ));
  if (ErrorCodeFlag & (1 << InterruptType)) {
    DEBUG ((
      EFI_D_ERROR,
      "ExceptionData - %016lx\n",
      SystemContext.SystemContextX64->ExceptionData
      ));
  }
  DEBUG ((
    EFI_D_ERROR,
    "RAX - %016lx, RCX - %016lx, RDX - %016lx\n",
    SystemContext.SystemContextX64->Rax,
    SystemContext.SystemContextX64->Rcx,
    SystemContext.SystemContextX64->Rdx
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RBX - %016lx, RSP - %016lx, RBP - %016lx\n",
    SystemContext.SystemContextX64->Rbx,
    SystemContext.SystemContextX64->Rsp,
    SystemContext.SystemContextX64->Rbp
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RSI - %016lx, RDI - %016lx\n",
    SystemContext.SystemContextX64->Rsi,
    SystemContext.SystemContextX64->Rdi
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R8 - %016lx, R9 - %016lx, R10 - %016lx\n",
    SystemContext.SystemContextX64->R8,
    SystemContext.SystemContextX64->R9,
    SystemContext.SystemContextX64->R10
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R11 - %016lx, R12 - %016lx, R13 - %016lx\n",
    SystemContext.SystemContextX64->R11,
    SystemContext.SystemContextX64->R12,
    SystemContext.SystemContextX64->R13
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R14 - %016lx, R15 - %016lx\n",
    SystemContext.SystemContextX64->R14,
    SystemContext.SystemContextX64->R15
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DS - %016lx, ES - %016lx, FS - %016lx\n",
    SystemContext.SystemContextX64->Ds,
    SystemContext.SystemContextX64->Es,
    SystemContext.SystemContextX64->Fs
    ));
  DEBUG ((
    EFI_D_ERROR,
    "GS - %016lx, SS - %016lx\n",
    SystemContext.SystemContextX64->Gs,
    SystemContext.SystemContextX64->Ss
    ));
  DEBUG ((
    EFI_D_ERROR,
    "GDTR - %016lx %016lx, LDTR - %016lx\n",
    SystemContext.SystemContextX64->Gdtr[0],
    SystemContext.SystemContextX64->Gdtr[1],
    SystemContext.SystemContextX64->Ldtr
    ));
  DEBUG ((
    EFI_D_ERROR,
    "IDTR - %016lx %016lx, TR - %016lx\n",
    SystemContext.SystemContextX64->Idtr[0],
    SystemContext.SystemContextX64->Idtr[1],
    SystemContext.SystemContextX64->Tr
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR0 - %016lx, CR2 - %016lx, CR3 - %016lx\n",
    SystemContext.SystemContextX64->Cr0,
    SystemContext.SystemContextX64->Cr2,
    SystemContext.SystemContextX64->Cr3
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR4 - %016lx, CR8 - %016lx\n",
    SystemContext.SystemContextX64->Cr4,
    SystemContext.SystemContextX64->Cr8
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR0 - %016lx, DR1 - %016lx, DR2 - %016lx\n",
    SystemContext.SystemContextX64->Dr0,
    SystemContext.SystemContextX64->Dr1,
    SystemContext.SystemContextX64->Dr2
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR3 - %016lx, DR6 - %016lx, DR7 - %016lx\n",
    SystemContext.SystemContextX64->Dr3,
    SystemContext.SystemContextX64->Dr6,
    SystemContext.SystemContextX64->Dr7
    ));
#endif
  return ;
}

VOID
EFIAPI
ExceptionHandler (
  IN EFI_EXCEPTION_TYPE    InterruptType,
  IN EFI_SYSTEM_CONTEXT    SystemContext
  )
{
  DumpExceptionDataDebugOut (InterruptType, SystemContext);
  //
  // Use this macro to hang so that the compiler does not optimize out
  // the following RET instructions. This allows us to return if we
  // have a debugger attached.
  //
  CpuDeadLoop ();

  return ;
}

VOID
EFIAPI
TimerHandler (
  IN EFI_EXCEPTION_TYPE    InterruptType,
  IN EFI_SYSTEM_CONTEXT    SystemContext
  )
{
  if (mTimerHandler != NULL) {
    mTimerHandler (InterruptType, SystemContext);
  }
}

EFI_STATUS
EFIAPI
InitializeCpu (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:
  Initialize the state information for the CPU Architectural Protocol

Arguments:
  ImageHandle of the loaded driver
  Pointer to the System Table

Returns:
  EFI_SUCCESS           - thread can be successfully created
  EFI_OUT_OF_RESOURCES  - cannot allocate protocol data structure
  EFI_DEVICE_ERROR      - cannot create the thread

--*/
{
  EFI_STATUS                  Status;
  EFI_8259_IRQ                Irq;
  UINT32                      InterruptVector;

  //
  // Find the Legacy8259 protocol.
  //
  Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &gLegacy8259);
  ASSERT_EFI_ERROR (Status);

  //
  // Get the interrupt vector number corresponding to IRQ0 from the 8259 driver
  //
  Status = gLegacy8259->GetVector (gLegacy8259, Efi8259Irq0, (UINT8 *) &mTimerVector);
  ASSERT_EFI_ERROR (Status);

  //
  // Reload GDT, IDT
  //
  InitDescriptor ();

  //
  // Install Exception Handler (0x00 ~ 0x1F)
  //
  for (InterruptVector = 0; InterruptVector < 0x20; InterruptVector++) {
    InstallInterruptHandler (
      InterruptVector,
      (VOID (*)(VOID))(UINTN)((UINTN)SystemExceptionHandler + mExceptionCodeSize * InterruptVector)
      );
  }

  //
  // XXX: Patch System Timer Handler (push ...) with actual timer vector number
  //
  ((UINT8 *)(UINTN)&SystemTimerHandler)[3] = (UINT8) mTimerVector;

  //
  // Install Timer Handler
  //
  InstallInterruptHandler (mTimerVector, SystemTimerHandler);

  //
  // BUGBUG: We add all other interrupt vector
  //
  for (Irq = Efi8259Irq1; Irq <= Efi8259Irq15; Irq++) {
    InterruptVector = 0;
    Status = gLegacy8259->GetVector (gLegacy8259, Irq, (UINT8 *) &InterruptVector);
    ASSERT_EFI_ERROR (Status);
    InstallInterruptHandler (InterruptVector, SystemTimerHandler);
  }

  //
  // Install CPU Architectural Protocol and the thunk protocol
  //
  mHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEfiCpuArchProtocolGuid,
                  &mCpu,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
