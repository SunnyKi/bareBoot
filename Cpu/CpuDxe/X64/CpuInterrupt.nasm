;------------------------------------------------------------------------------
;*
;*	Copyright 2006 - 2016, Intel Corporation
;*	All rights reserved. This program and the accompanying materials
;*	are licensed and made available under the terms and conditions of the BSD License
;*	which accompanies this distribution.	The full text of the license may be found at
;*	http://opensource.org/licenses/bsd-license.php
;*
;*	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;*	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;*
;*	CpuInterrupt.nasm
;*
;------------------------------------------------------------------------------

	BITS	64
	DEFAULT	REL
	SECTION	.text

extern	ASM_PFX(TimerHandler)
extern	ASM_PFX(ExceptionHandler)
extern	ASM_PFX(mTimerVector)

global	ASM_PFX(mExceptionCodeSize)
ASM_PFX(mExceptionCodeSize):
	dd 9

global	ASM_PFX(InitDescriptor)
ASM_PFX(InitDescriptor):
	lea	rax, [_GDT_BASE]	; RAX=PHYSICAL address of gdt
	mov	qword [gdtr + 2], rax	; Put address of gdt into the gdtr
	lgdt	[gdtr]

	mov	rax, 0x18
	mov	gs, rax
	mov	fs, rax

	lea	rax, [_IDT_BASE]	; RAX=PHYSICAL address of idt
	mov	qword [idtr + 2], rax	; Put address of idt into the idtr
	lidt	[idtr]
	ret

; VOID
; InstallInterruptHandler (
;	UINTN Vector,	// rcx
;	void	(*Handler)(void)	// rdx
;	)

global	ASM_PFX(InstallInterruptHandler)
ASM_PFX(InstallInterruptHandler):
	push	rbx
	pushfq	; save eflags
	cli	; turn off interrupts
	sub	rsp, 0x10	; open some space on the stack
	mov	rbx, rsp
	sidt	[rbx]	; get fword address of IDT
	mov	rbx, [rbx+2]	; move offset of IDT into RBX
	add	rsp, 0x10	; correct stack
	mov	rax, rcx	; Get vector number
	shl	rax, 4	; multiply by 16 to get offset
	add	rbx, rax	; add to IDT base to get entry
	mov	rax, rdx	; load new address into IDT entry
	mov	word [rbx], ax	; write bits 15..0 of offset
	shr	rax, 16	; use ax to copy 31..16 to descriptors
	mov	word [rbx+6], ax	; write bits 31..16 of offset
	shr	rax, 16	; use eax to copy 63..32 to descriptors
	mov	dword [rbx+8], eax	; write bits 63..32 of offset
	popfq	; restore flags (possible enabling interrupts)
	pop	rbx
	ret

	align	0x2

global	ASM_PFX(SystemExceptionHandler)
ASM_PFX(SystemExceptionHandler):

%macro	ihtentry	1
	push	0
	push	%1
	jmp	qword commonIhtEntry	; qword to keep the same entry length
%endmacro

%macro	ihtenerr	1
	nop
	nop
	push	%1
	jmp	qword commonIhtEntry	; qword to keep the same entry length
%endmacro

DEFAULT_HANDLER_SIZE 	qu	(INT1 - INT0)

INT0:
	ihtentry	0

INT1:
	ihtentry	1
	ihtentry	2
	ihtentry	3
	ihtentry	4
	ihtentry	5
	ihtentry	6
	ihtentry	7
	ihtenerr	8	; Double Fault
	ihtentry	9
	ihtenerr	10	; Invalid TSS
	ihtenerr	11	; Segment Not Present
	ihtenerr	12	; Stack Fault
	ihtenerr	13	; GP Fault
	ihtenerr	14	; Page Fault
	ihtentry	15
	ihtentry	16
	ihtenerr	17	; Alignment Check
	ihtentry	18
	ihtentry	19

INTUnknown:

; XXX: 9 is DEFAULT_HANDLER_SIZE. Keep in sync!

%rep	(32 - 20)
	ihtentry	(($ - INTUnknown - 2) / 9 + 20)
%endrep

;------------------------------------------------------------------------------

global	ASM_PFX(SystemTimerHandler)
ASM_PFX(SystemTimerHandler):

	ihtentry	0 ; Actual vector number will be patched from mTimerVector

commonIhtEntry:
; +---------------------+ <-- 16-byte aligned ensured by processor
; +	Old SS	+
; +---------------------+
; +	Old RSP	+
; +---------------------+
; +	RFlags	+
; +---------------------+
; +	CS	+
; +---------------------+
; +	RIP	+
; +---------------------+
; +	Error Code	+
; +---------------------+
; +	Vector Number	+
; +---------------------+
; +	RBP	+
; +---------------------+ <-- RBP, 16-byte aligned

	cli
	push	rbp
	mov	rbp, rsp

; Since here the stack pointer is 16-byte aligned, so
; EFI_FX_SAVE_STATE_X64 of EFI_SYSTEM_CONTEXT_x64
; is 16-byte aligned

;; UINT64 Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;; UINT64 R8, R9, R10, R11, R12, R13, R14, R15;
	push	r15
	push	r14
	push	r13
	push	r12
	push	r11
	push	r10
	push	r9
	push	r8
	push	rax
	push	rcx
	push	rdx
	push	rbx
	push	qword [rbp + 6 * 8]	; RSP
	push	qword [rbp]		; RBP
	push	rsi
	push	rdi

;; UINT64 Gs, Fs, Es, Ds, Cs, Ss;	insure high 16 bits of each is zero
	movzx	rax, word [rbp + 7 * 8]
	push	rax	; for ss
	movzx	rax, word [rbp + 4 * 8]
	push	rax	; for cs
	mov	rax, ds
	push	rax
	mov	rax, es
	push	rax
	mov	rax, fs
	push	rax
	mov	rax, gs
	push	rax

;; UINT64 Rip;
	push	qword [rbp + 3 * 8]

;; UINT64 Gdtr[2], Idtr[2];
	sub	rsp, 16
	sidt	[rsp]
	sub	rsp, 16
	sgdt	[rsp]

;; UINT64 Ldtr, Tr;
	xor	rax, rax
	str	ax
	push	rax
	sldt	ax
	push	rax

;; UINT64 RFlags;
	push	qword [rbp + 5 * 8]

;; UINT64 Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
	mov	rax, cr8
	push	rax
	mov	rax, cr4
	or	rax, 0x208
	mov	cr4, rax
	push	rax
	mov	rax, cr3
	push	rax
	mov	rax, cr2
	push	rax
	xor	rax, rax
	push	rax
	mov	rax, cr0
	push	rax

;; UINT64 Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
	mov	rax, dr7
	push	rax
;; clear Dr7 while executing debugger itself
	xor	rax, rax
	mov	dr7, rax

	mov	rax, dr6
	push	rax
;; insure all status bits in dr6 are clear...
	xor	rax, rax
	mov	dr6, rax

	mov	rax, dr3
	push	rax
	mov	rax, dr2
	push	rax
	mov	rax, dr1
	push	rax
	mov	rax, dr0
	push	rax

;; FX_SAVE_STATE_X64 FxSaveState;

	sub	rsp, 512
	mov	rdi, rsp
	fxsave	[rdi]

;; UINT32 ExceptionData;
	push	qword [rbp + 2 * 8]

;; call into exception handler
;; Prepare parameter and call
	mov	rcx, qword [rbp + 1 * 8]
	mov	rdx, rsp
	;
	; Per X64 calling convention, allocate maximum parameter stack space
	; and make sure RSP is 16-byte aligned
	;
	sub	rsp, 4 * 8 + 8
	cmp	rcx, 32
	jb	CallException

	call	ASM_PFX(TimerHandler)
	jmp	ExceptionDone

CallException:
	call	ASM_PFX(ExceptionHandler)

ExceptionDone:
	add	rsp, 4 * 8 + 8

	cli
;; UINT64 ExceptionData;
	add	rsp, 8

;; FX_SAVE_STATE_X64 FxSaveState;

	mov	rsi, rsp
	fxrstor	[rsi]
	add	rsp, 512

;; UINT64 Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
	pop	rax
	mov	dr0, rax
	pop	rax
	mov	dr1, rax
	pop	rax
	mov	dr2, rax
	pop	rax
	mov	dr3, rax
;; skip restore of dr6.	We cleared dr6 during the context save.
	add	rsp, 8
	pop	rax
	mov	dr7, rax

;; UINT64 Cr0, Cr1, Cr2, Cr3, Cr4, Cr8;
	pop	rax
	mov	cr0, rax
	add	rsp, 8	; not for Cr1
	pop	rax
	mov	cr2, rax
	pop	rax
	mov	cr3, rax
	pop	rax
	mov	cr4, rax
	pop	rax
	mov	cr8, rax

;; UINT64 RFlags;
	pop	qword [rbp + 5 * 8]

;; UINT64 Ldtr, Tr;
;; UINT64 Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
	add	rsp, 48

;; UINT64 Rip;
	pop	qword [rbp + 3 * 8]

;; UINT64 Gs, Fs, Es, Ds, Cs, Ss;
	pop	rax
	; mov	gs, rax ; not for gs
	pop	rax
	; mov	fs, rax ; not for fs
	; (X64 will not use fs and gs, so we do not restore it)
	pop	rax
	mov	es, rax
	pop	rax
	mov	ds, rax
	pop	qword [rbp + 4 * 8]	; for cs
	pop	qword [rbp + 7 * 8]	; for ss

;; UINT64 Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax;
;; UINT64 R8, R9, R10, R11, R12, R13, R14, R15;
	pop	rdi
	pop	rsi
	add	rsp, 8	; not for rbp
	pop	qword [rbp + 6 * 8] ; for rsp
	pop	rbx
	pop	rdx
	pop	rcx
	pop	rax
	pop	r8
	pop	r9
	pop	r10
	pop	r11
	pop	r12
	pop	r13
	pop	r14
	pop	r15

	mov	rsp, rbp
	pop	rbp
	add	rsp, 16
	iretq

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; global descriptor table (GDT)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%macro	gdtentry	4
%[%1]_SEL	equ	$ - _GDT_BASE
	dw	%2	; limit low bits (0..15)
	dw	0	; base low bits (0..15)
	db	0	; base middle bits (16..23)
	db	%3	; access
	db	%4	; limit high bits (16..19) & flags
	db	0	; base high bits (24..31)
%endmacro

;------------------------------------------------------------------------------

	align 2

gdtr:
	dw	GDT_END - _GDT_BASE - 1	; GDT limit
	dd	_GDT_BASE		; will be adjusted at runtime

;------------------------------------------------------------------------------

	align 2

global _GDT_BASE
_GDT_BASE:
	gdtentry	DUMMY, 0, 0, 0			; selector [0x00]
	gdtentry	LINEAR, 0xFFFF, 0x92, 0xCF	; selector [0x08]
	gdtentry	LINEAR_CODE, 0xFFFF, 0x9A, 0xCF	; selector [0x10]
	gdtentry	SYS_DATA, 0xFFFF, 0x92, 0xCF	; selector [0x18]
	gdtentry	SYS_CODE, 0xFFFF, 0x9A, 0xCF	; selector [0x20]
	gdtentry	SPARE3, 0, 0, 0			; selector [0x28]
	gdtentry	SYS_DATA64, 0xFFFF, 0x92, 0xCF	; selector [0x30]
	gdtentry	SYS_CODE64, 0xFFFF, 0x9A, 0xAF	; selector [0x38]
	gdtentry	SPARE4, 0, 0, 0			; selector [0x40]

GDT_END:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; interrupt descriptor table (IDT)
;
; Note: The hardware IRQ's specified in this table are the normal PC/AT IRQ
; mappings. This implementation only uses the system timer and all other
; IRQs will remain masked. The descriptors for vectors 33+ are provided
; for convenience.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

struc	idtdescr
	.loffset	resw	1	; offset low bits (0..15)
	.selector	resw	1	; code segment selector
	.zero1		resb	1	; zero
	.tattr		resb	1	; type and attributes
	.moffset	resw	1	; offset middle bits (16..31)
	.hoffset	resd	1	; offset high bits (32..63)
	.zero2		resd	1	; zero
endstruc

%macro	idtentry	1
%[%1]_SEL	equ	$ - _IDT_BASE
	istruc	idtdescr
		at	idtdescr.selector
			dw SYS_CODE64_SEL
		at	idtdescr.tattr
			db 0x8E	 ; 386 32-bit interrupt gate, present
	iend
%endmacro

;------------------------------------------------------------------------------

	align 2

idtr:
	dw	IDT_END - _IDT_BASE - 1	; IDT limit
	dq	_IDT_BASE		; will be adjusted at runtime

;------------------------------------------------------------------------------

	align 2

_IDT_BASE:
	idtentry	DIV_ZERO	; divide by zero (INT 0)
	idtentry	DEBUG_EXCEPT	; debug exception (INT 1)
	idtentry	NMI		; NMI (INT 2)
	idtentry	BREAKPOINT	; soft breakpoint (INT 3)
	idtentry	OVERFLOW	; overflow (INT 4)
	idtentry	BOUNDS_CHECK	; bounds check (INT 5)
	idtentry	INVALID_OPCODE	; invalid opcode (INT 6)
	idtentry	DEV_NOT_AVAIL	; device not available (INT 7)
	idtentry	DOUBLE_FAULT	; double fault (INT 8)
	idtentry	RSVD_INTR1	; Coprocessor segment overrun - reserved (INT 9)
	idtentry	INVALID_TSS	; invalid TSS (INT 0x0A)
	idtentry	SEG_NOT_PRESENT	; segment not present (INT 0x0B)
	idtentry	STACK_FAULT	; stack fault (INT 0x0C)
	idtentry	GP_FAULT	; general protection (INT 0x0D)
	idtentry	PAGE_FAULT	; page fault (INT 0x0E)
	idtentry	RSVD_INTR2	; Intel reserved - do not use (INT 0x0F)
	idtentry	FLT_POINT_ERR	; floating point error (INT 0x10)
	idtentry	ALIGNMENT_CHECK	; alignment check (INT 0x11)
	idtentry	MACHINE_CHECK	; machine check (INT 0x12)
	idtentry	SIMD_EXCEPTION	; SIMD floating-point exception (INT 0x13)

	; 84 unspecified descriptors, First 12 of them are reserved, the rest are avail

%assign	i	0x14
%rep	84
	idtentry	UNSPEC%+i
%assign	i	(i + 1)
%endrep

	idtentry	IRQ0	; IRQ 0 (System timer) - (INT 0x68)
	idtentry	IRQ1	; IRQ 1 (8042 Keyboard controller) - (INT 0x69)
	idtentry	IRQ2	; Reserved - IRQ 2 redirect (IRQ 2) - DO NOT USE!!! - (INT 0x6A)
	idtentry	IRQ3	; IRQ 3 (COM 2) - (INT 0x6B)
	idtentry	IRQ4	; IRQ 4 (COM 1) - (INT 0x6C)
	idtentry	IRQ5	; IRQ 5 (LPT 2) - (INT 0x6D)
	idtentry	IRQ6	; IRQ 6 (Floppy controller) - (INT 0x6E)
	idtentry	IRQ7	; IRQ 7 (LPT 1) - (INT 0x6F)
	idtentry	IRQ8	; IRQ 8 (RTC Alarm) - (INT 0x70)
	idtentry	IRQ9	; IRQ 9 - (INT 0x71)
	idtentry	IRQ10	; IRQ 10 - (INT 0x72)
	idtentry	IRQ11	; IRQ 10 - (INT 0x73)
	idtentry	IRQ12	; IRQ 12 (PS/2 mouse) - (INT 0x74)
	idtentry	IRQ13	; IRQ 13 (Floating point error) - (INT 0x75)
	idtentry	IRQ14	; IRQ 14 (Secondary IDE) - (INT 0x76)
	idtentry	IRQ15	; IRQ 15 (Primary IDE) - (INT 0x77)

IDT_END:
