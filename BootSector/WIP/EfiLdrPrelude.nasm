;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials
;*   are licensed and made available under the terms and conditions of the BSD License
;*   which accompanies this distribution.  The full text of the license may be found at
;*   http://opensource.org/licenses/bsd-license.php
;*
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;*
;*    efildrprelude.nasm
;*
;*   Abstract:
;*
;------------------------------------------------------------------------------
; nms was here

	BITS	16

Start:
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	sp, MyStack

; Gether memory msp

	xor	ebx, ebx
	lea	edi, [MemoryMap]

MemMapLoop:
	mov	eax, 0xE820
	mov	ecx, 20
	mov	edx, 'PAMS'
	int	0x15
	jc	MemMapDone
	add	edi, 20
	cmp	ebx, 0
	je	MemMapDone
	jmp	MemMapLoop

MemMapDone:
	lea	eax, [MemoryMap]
	sub	edi, eax			; Get the address of the memory map
	mov	dword [MemoryMapSize], edi	; Save the size of the memory map

	xor	ebx, ebx
	mov	bx, cs				; BX=segment
	shl	ebx, 4				; BX="linear" address of segment base
	lea	eax, [GDT_BASE + ebx]		; EAX=PHYSICAL address of gdt
	mov	dword [gdtr + 2], eax		; Put address of gdt into the gdtr
	lea	eax, [IDT_BASE + ebx]		; EAX=PHYSICAL address of idt
	mov	dword [idtr + 2], eax		; Put address of idt into the idtr
	lea	edx, [MemoryMapSize + ebx]	; Physical base address of the memory map

; Enable A20 Gate

	mov	ax, 0x2401	; Enable A20 Gate
	int	0x15
	jnc	A20GateEnabled	; Jump if it suceeded

DELAY_PORT		equ	0xED	; Port to use for 1uS delay
KBD_CONTROL_PORT	equ	0x60	; 8042 control port
KBD_STATUS_PORT		equ	0x64	; 8042 status port
WRITE_DATA_PORT_CMD	equ	0xD1	; 8042 command to write the data port
ENABLE_A20_CMD		equ	0xDF	; 8042 command to enable A20

; If INT 15 Function 2401 is not supported, then attempt to Enable A20 manually.

	call	Empty8042InputBuffer	; Empty the Input Buffer on the 8042 controller
	jnz	Timeout8042		; Jump if the 8042 timed out
	out	DELAY_PORT, ax		; Delay 1 uS
	mov	al, WRITE_DATA_PORT_CMD	; 8042 cmd to write output port
	out	KBD_STATUS_PORT, al	; Send command to the 8042
	call	Empty8042InputBuffer	; Empty the Input Buffer on the 8042 controller
	jnz	Timeout8042		; Jump if the 8042 timed out
	mov	al, ENABLE_A20_CMD	; gate address bit 20 on
	out	KBD_CONTROL_PORT, al	; Send command to thre 8042
	call	Empty8042InputBuffer	; Empty the Input Buffer on the 8042 controller
	mov	cx, 25			; Delay 25 uS for the command to complete on the 8042

Delay25uS:
	out	DELAY_PORT, ax		; Delay 1 uS
	loop	Delay25uS
	jmp	A20GateEnabled

Timeout8042:

%ifdef A20FAST
;
; attempt to Enable A20 (fast method).
;
	in	al, 0x92
	test	al, 2
	jnz	A20GateEnabled
	or	al, 2
	and	al, 0xFE
	out	0x92, al
%endif

A20GateEnabled:

; DISABLE INTERRUPTS - Entering Protected Mode

	cli

	lea	eax, [OffsetIn32BitProtectedMode]
	add	eax, 0x20000 + (In32BitProtectedMode - OffsetIn32BitProtectedMode)
	mov	dword [OffsetIn32BitProtectedMode], eax

	lea	eax, [OffsetInLongMode]
	add	eax, 0x20000 + (InLongMode - OffsetInLongMode)
	mov	dword [OffsetInLongMode], eax

	; load GDT

	o32
	lgdt	[gdtr]

	; Enable Protect Mode (set CR0.PE=1)

	mov	eax, cr0
	or	eax, 1		; Set PE=1
	mov	cr0, eax

	jmp	dword 0x0010:0x00000000	; will be modified by above code
OffsetIn32BitProtectedMode	equ $ - 6

In32BitProtectedMode:

	BITS	32

; Entering Long Mode

	mov	ax, LINEAR_SEL
	mov	ds, ax
	mov	es, ax
	mov	ss, ax

	;
	; Enable the 64-bit page-translation-table entries by
	; setting CR4.PAE=1 (this is _required_ before activating
	; long mode). Paging is not enabled until after long mode
	; is enabled.

	mov	eax, cr4
	bts	eax, 5
	mov	cr4, eax

	; This is the Trampoline Page Tables that are guaranted
	; under 4GB.
	;
	; Address Map:
	;    10000 ~    12000 - efildr (loaded)
	;    20000 ~    21000 - start64.com
	;    21000 ~    22000 - efi64.com
	;    22000 ~    90000 - efildr + stuff
	;    90000 ~    96000 - 4G pagetable (will be reload later)

	mov	eax, 0x90000	; XXX: !!!
	mov	cr3, eax

	; Enable long mode (set EFER.LME=1).

	mov	ecx, 0xC0000080	; EFER MSR number.
	rdmsr
	bts	eax, 8		; Set LME=1.
	wrmsr

	; Enable paging to activate long mode (set CR0.PG=1)
 
	mov	eax, cr0
	bts	eax, 31		; Set PG=1.
	mov	cr0, eax
	jmp	GoToLongMode

GoToLongMode:

	db	0x67	; XXX: ???
	jmp	dword 0x0038:0x00000000	; Selector is Our Code Selector (38h)
OffsetInLongMode	equ $ - 6

	BITS	64

InLongMode:
	mov	ax, SYS_DATA64_SEL
	mov	ds, ax

	mov	ax, SYS_DATA_SEL
	mov	es, ax
	mov	ss, ax
	mov	ds, ax

	mov	ebp, 0x00400000	; XXX: Destination of EFILDR32
	mov	ebx, 0x00070000	; Length of copy

	; load idt later

	xor	rax, rax
	mov	ax, idtr
	add	rax, 0x20000	; XXX

	lidt	[rax]

	mov	rax, dword 0x21000	; XXX
	push	rax
	ret

	BITS	16

Empty8042InputBuffer:
	xor	cx, cx
Empty8042Loop:
	out	DELAY_PORT, ax		; Delay 1us
	in	al, KBD_STATUS_PORT	; Read the 8042 Status Port
	and	al, 0x02		; Check the Input Buffer Full Flag
	loopnz	Empty8042Loop		; Loop until the input buffer is empty or a timout of 65536 uS
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	align 2

gdtr:
	dw	GDT_END - GDT_BASE - 1	; GDT limit
	dd	0			; (GDT base gets set above)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   global descriptor table (GDT)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%macro	gdtentry	4
%[%1]_SEL	equ	$ - GDT_BASE
	dw	%2
	dw	0
	db	0
	db	%3
	db	%4
	db	0
%endmacro

	align 2

GDT_BASE:

	gdtentry	NULL, 0, 0, 0
	gdtentry	LINEAR, 0xFFFF, 0x92, 0xCF
	gdtentry	LINEAR_CODE, 0xFFFF, 0x9A, 0xCF
	gdtentry	SYS_DATA, 0xFFFF, 0x92, 0xCF
	gdtentry	SYS_CODE, 0xFFFF, 0x9A, 0xCF
	gdtentry	SPARE3, 0, 0, 0
	gdtentry	SYS_DATA64, 0xFFFF, 0x92, 0xCF
	gdtentry	SYS_CODE64, 0xFFFF, 0x9A, 0xAF
	gdtentry	SPARE4, 0, 0, 0

GDT_END:

	align 2

idtr:
	dw	IDT_END - IDT_BASE - 1	; IDT limit
	dd	0			; (IDT base gets set above)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   interrupt descriptor table (IDT)
;
;   Note: The hardware IRQ's specified in this table are the normal PC/AT IRQ
;       mappings.  This implementation only uses the system timer and all other
;       IRQs will remain masked.  The descriptors for vectors 33 + are provided
;       for convenience.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	align 2

IDT_BASE:

; divide by zero (INT 0)
DIV_ZERO_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; debug exception (INT 1)
DEBUG_EXCEPT_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; NMI (INT 2)
NMI_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; soft breakpoint (INT 3)
BREAKPOINT_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; overflow (INT 4)
OVERFLOW_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; bounds check (INT 5)
BOUNDS_CHECK_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; invalid opcode (INT 6)
INVALID_OPCODE_SEL  equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; device not available (INT 7)
DEV_NOT_AVAIL_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; double fault (INT 8)
DOUBLE_FAULT_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; Coprocessor segment overrun - reserved (INT 9)
RSVD_INTR_SEL1	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; invalid TSS (INT 0ah)
INVALID_TSS_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; segment not present (INT 0bh)
SEG_NOT_PRESENT_SEL equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; stack fault (INT 0ch)
STACK_FAULT_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; general protection (INT 0dh)
GP_FAULT_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; page fault (INT 0eh)
PAGE_FAULT_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; Intel reserved - do not use (INT 0fh)
RSVD_INTR_SEL2	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; floating point error (INT 10h)
FLT_POINT_ERR_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; alignment check (INT 11h)
ALIGNMENT_CHECK_SEL equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; machine check (INT 12h)
MACHINE_CHECK_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; SIMD floating-point exception (INT 13h)
SIMD_EXCEPTION_SEL  equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; 85 unspecified descriptors, First 12 of them are reserved, the rest are avail
	times (85 * 16) db 0

; IRQ 0 (System timer) - (INT 68h)
IRQ0_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 1 (8042 Keyboard controller) - (INT 69h)
IRQ1_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; Reserved - IRQ 2 redirect (IRQ 2) - DO NOT USE!!! - (INT 6ah)
IRQ2_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 3 (COM 2) - (INT 6bh)
IRQ3_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 4 (COM 1) - (INT 6ch)
IRQ4_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 5 (LPT 2) - (INT 6dh)
IRQ5_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 6 (Floppy controller) - (INT 6eh)
IRQ6_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 7 (LPT 1) - (INT 6fh)
IRQ7_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 8 (RTC Alarm) - (INT 70h)
IRQ8_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 9 - (INT 71h)
IRQ9_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 10 - (INT 72h)
IRQ10_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 11 - (INT 73h)
IRQ11_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 12 (PS/2 mouse) - (INT 74h)
IRQ12_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 13 (Floating point error) - (INT 75h)
IRQ13_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 14 (Secondary IDE) - (INT 76h)
IRQ14_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

; IRQ 15 (Primary IDE) - (INT 77h)
IRQ15_SEL	equ $-IDT_BASE
	dw 0	; offset 15:0
	dw SYS_CODE64_SEL	; selector 15:0
	db 0	; 0 for interrupt gate
	db 0eh | 80h	; (10001110)type = 386 interrupt gate, present
	dw 0	; offset 31:16
	dd 0	; offset 63:32
	dd 0	; 0 for reserved

IDT_END:

	align 2

MemoryMapSize:
	dd  0

MemoryMap:
	times (0x0FE0 - ($ - $$)) db 0

MyStack:
	; below is the pieces of the IVT that is used to redirect INT 68h - 6fh
	;    back to INT 08h - 0fh  when in real mode...  It is 'org'ed to a
	;    known low address (20f00) so it can be set up by PlMapIrqToVect in
	;    8259.c

	int 8
	iret

	int 9
	iret

	int 10
	iret

	int 11
	iret

	int 12
	iret

	int 13
	iret

	int 14
	iret

	int 15
	iret

DEFAULT_HANDLER_SIZE equ INT1 - INT0

Start2:
	mov     esp, 0x001FFFE8	; make final stack aligned

	; set OSFXSR and OSXMMEXCPT because some code will use XMM register

	mov eax, cr4
	bts eax, 9
	bts eax, 10
	mov cr4, eax

	; Populate IDT with meaningful offsets for exception handlers...

	lea	eax, [idtr]
	sidt    [eax]		; get fword address of IDT

	lea	eax, [Halt]
	mov     ebx, eax	; use bx to copy 15..0 to descriptors
	shr     eax, 16		; use ax to copy 31..16 to descriptors
				; 63..32 of descriptors are 0
	mov     ecx, 0x78	; 0x78 IDT entries to initialize with unique entry points (exceptions)
	mov	esi, [idtr + 2]
	mov	edi, [esi]

	; loop through all IDT entries exception handlers and initialize to default handler

.1:
	mov     word [edi + 0], bx		; write bits 15..0 of offset
	mov     word [edi + 2], SYS_CODE64_SEL
	mov     word [edi + 4], 0x0E00 | 0x8000	; type = 386 interrupt gate, present
	mov     word [edi + 6], ax		; write bits 31..16 of offset
	mov     dword [edi + 8], 0		; write bits 63..32 of offset
	add     edi, 16				; move up to next descriptor
	add     ebx, DEFAULT_HANDLER_SIZE	; move to next entry point
	loop    .1

	lea	esi, [EfiLdrCode]
	mov     eax, [esi + 014h]	; eax = [22014]
	add     esi, eax		; esi = 22000 + [22014] = Base of EFILDR.C
	mov     ebp, [esi + 03Ch]	; ebp = [22000 + [22014] + 3c] = NT Image Header for EFILDR.C
	add	ebp, esi
	mov     edi, [ebp + 030h]	; edi = [[22000 + [22014] + 3c] + 2c] = ImageBase (63..32 is zero, ignore)
	mov     eax, [ebp + 028h]	; eax = [[22000 + [22014] + 3c] + 24] = EntryPoint
	add     eax, edi		; eax = ImageBase + EntryPoint

	mov	[EfiLdrEntry], eax

	mov     bx, word [ebp + 6]	; bx = Number of sections
	mov     ax, word [ebp + 014h]	; ax = Optional Header Size
	add	ebp, eax
	add     ebp, 018h	; ebp = Start of 1st Section

SectionLoop:
	push    esi	; Save Base of EFILDR.C
	push    edi	; Save ImageBase
	add     esi, [ebp + 014h]	; esi = Base of EFILDR.C + PointerToRawData
	add     edi, [ebp + 00Ch]	; edi = ImageBase + VirtualAddress
	mov     ecx, [ebp + 010h]	; ecs = SizeOfRawData

	cld
	shr	ecx, 2
	rep	movsd

	pop     edi	; Restore ImageBase
	pop     esi	; Restore Base of EFILDR.C

	add     ebp, 0x28	; ebp = ebp + 028h = Pointer to next section record
	dec	ebx
	cmp	ebx, 0
	jne	SectionLoop

	lea	edx, [idtr]
	movzx   eax, word [edx]	; get size of IDT
	inc	eax
	add     eax, [edx + 2]	; add to base of IDT to get location of memory map...
	mov     ecx, eax	; put argument to RCX

	mov	eax, [EfiLdrEntry]
	push	eax
	ret

	align	2

EfiLdrEntry:
	dd 0
	dd 0

; Interrupt handling

%macro intentry 1
	push 0
	push %1
	jmp qword commonIdtEntry	; qword to keep the same entry length
%endmacro

%macro intenerr 1
	nop
	nop
	push %1
	jmp qword commonIdtEntry	; qword to keep the same entry length
%endmacro

	align	2

	BITS	64

Halt:

INT0:
	intentry 0
INT1:
	intentry 1
	intentry 2
	intentry 3
	intentry 4
	intentry 5
	intentry 6
	intentry 7
	intenerr 8	; Double Fault
	intentry 9
	intenerr 10	; Invalid TSS
	intenerr 11	; Segment Not Present
	intenerr 12	; Stack Fault
	intenerr 13	; GP Fault
	intenerr 14	; Page Fault
	intentry 15
	intentry 16
	intenerr 17	; Alignment Check
	intentry 18
	intentry 19

INTUnknown:

%rep (0x78 - 20)
	intentry ($ - INTUnknown - 2) / 9 + 20	; vector number
%endrep

commonIdtEntry:
	push	rax
	push	rcx
	push	rdx
	push	rbx
	push	rsp
	push	rbp
	push	rsi
	push	rdi
	push	r8
	push	r9
	push	r10
	push	r11
	push	r12
	push	r13
	push	r14
	push	r15
	mov	rbp, rsp

;;  At this point the stack looks like this:
;;
;;      Calling SS
;;      Calling RSP
;;      rflags
;;      Calling CS
;;      Calling RIP
;;      Error code or 0
;;      Int num or 0xFF for unknown int num
;;      rax
;;      rcx
;;      rdx
;;      rbx
;;      rsp
;;      rbp
;;      rsi
;;      rdi
;;      r8
;;      r9
;;      r10
;;      r11
;;      r12
;;      r13
;;      r14
;;      r15 <------- RSP, RBP

%macro PrintRegister 2
	mov	rsi, %1
	call	PrintString
	mov	rax, [rbp + %2 * 8]
	call	PrintQword
%endmacro

	call	ClearScreen

	mov	rsi, String1
	call	PrintString
	mov     rax, [rbp + 16 * 8]	; move Int number into RAX
	cmp	rax, 18
	ja	PrintDefaultString

PrintExceptionString:
	shl     rax, 3			; multiply by 8 to get from StringTable to actual string address
	add	rax, StringTable
	mov	rsi, [rax]
	jmp	PrintTheString

PrintDefaultString:
	mov	rsi, IntUnkString
	mov     rdx, rax		; patch Int number
	call	A2C
	mov	[rsi + 1], al
	mov	rax, rdx
	shr	rax, 4
	call	A2C
	mov	[rsi], al

PrintTheString:
	call	PrintString

	PrintRegister String2, 19	; CS

	mov	al, ':'
	mov	byte [rdi], al
	add	rdi, 2
	mov     rax, [rbp + 18 * 8]	; RIP
	call	PrintQword
	mov	rsi, String3
	call	PrintString

	mov	rdi, 0x000B8140

	PrintRegister StringRax, 15
	PrintRegister StringRcx, 14
	PrintRegister StringRdx, 13

	mov	rdi, 0x000B81E0

	PrintRegister StringRbx, 12
	PrintRegister StringRsp, 21
	PrintRegister StringRbp, 10

	mov	rdi, 0x000B8280

	PrintRegister StringRsi, 9
	PrintRegister StringRdi, 8
	PrintRegister StringEcode, 17

	mov	rdi, 0x000B8320

	PrintRegister StringR8, 7
	PrintRegister StringR9, 6
	PrintRegister StringR10, 5

	mov	rdi, 0x000B83C0

	PrintRegister StringR11, 4
	PrintRegister StringR12, 3
	PrintRegister StringR13, 2

	mov	rdi, 0x000B8460

	PrintRegister StringR14, 1
	PrintRegister StringR15, 0
	PrintRegister StringSs, 22

	mov	rdi, 0x000B8500

	PrintRegister StringRflags, 20

	mov	rdi, 0x000B8640

	mov	rsi, rbp
	add	rsi, 23 * 8
	mov	rcx, 4


OuterLoop:
	push	rcx
	mov	rcx, 4
	mov	rdx, rdi

InnerLoop:
	mov	rax, [rsi]
	call	PrintQword
	add	rsi, 8
	mov	al, ' '
	mov	[rdi], al
	add	rdi, 2
	loop	InnerLoop

	pop	rcx
	add	rdx, 160
	mov	rdi, rdx
	loop	OuterLoop

	mov	rdi, 0x000B8960

	mov     rax, [rbp + 18 * 8]	; RIP
	sub	rax, 8 * 8
	mov     rsi, rax		; esi = rip - 8 QWORD linear (total 16 QWORD)

	mov	rcx, 4

OuterLoop1:
	push	rcx
	mov	rcx, 4
	mov	rdx, rdi

InnerLoop1:
	mov	rax, [rsi]
	call	PrintQword
	add	rsi, 8
	mov	al, ' '
	mov	[rdi], al
	add	rdi, 2
	loop	InnerLoop1

	pop	rcx
	add	rdx, 160
	mov	rdi, rdx
	loop	OuterLoop1
@2:
	jmp	@2

; return
	mov	rsp, rbp
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	r11
	pop	r10
	pop	r9
	pop	r8
	pop	rdi
	pop	rsi
	pop	rbp
	pop    rax	; esp
	pop	rbx
	pop	rdx
	pop	rcx
	pop	rax

	add    rsp, 16	; error code and INT number
	iretq

PrintString:
	push	rax
@3:
	mov	al, byte [rsi]
	cmp	al, 0
	je	@4
	mov	byte [rdi], al
	inc	rsi
	add	rdi, 2
	jmp	@3
@4:
	pop	rax
	ret

;; RAX contains qword to print
;; RDI contains memory location (screen location) to print it to

PrintQword:
	push	rcx
	push	rbx
	push	rax

	mov	rcx, dword 16
looptop:
	rol	rax, 4
	mov	bl, al
	and	bl, 0x0F
	add	bl, '0'
	cmp	bl, '9'
	jle	@5
	add	bl, 7
@5:
	mov	byte [rdi], bl
	add	rdi, 2
	loop	looptop

	pop	rax
	pop	rbx
	pop	rcx
	ret

ClearScreen:
	push	rax
	push	rcx

	mov	al, ' '
	mov	ah, 0x0C
	mov	rdi, 0x000B8000
	mov	rcx, 80 * 24
@6:
	mov	word [rdi], ax
	add	rdi, 2
	loop	@6
	mov	rdi, 0x000B8000

	pop	rcx
	pop	rax
	ret

A2C:
	and	al, 0x0F
	add	al, '0'
	cmp	al, '9'
	jle	@7
	add	al, 7
@7:
	ret

Int0String	db	"00h Divide by 0 -", 0
Int1String	db	"01h Debug exception -", 0
Int2String	db	"02h NMI -", 0
Int3String	db	"03h Breakpoint -", 0
Int4String	db	"04h Overflow -", 0
Int5String	db	"05h Bound -", 0
Int6String	db	"06h Invalid opcode -", 0
Int7String	db	"07h Device not available -", 0
Int8String	db	"08h Double fault -", 0
Int9String	db	"09h Coprocessor seg overrun (reserved) -", 0
Int10String	db	"0Ah Invalid TSS -", 0
Int11String	db	"0Bh Segment not present -", 0
Int12String	db	"0Ch Stack fault -", 0
Int13String	db	"0Dh General protection fault -", 0
Int14String	db	"0Eh Page fault -", 0
Int15String	db	"0Fh (Intel reserved) -", 0
Int16String	db	"10h Floating point error -", 0
Int17String	db	"11h Alignment check -", 0
Int18String	db	"12h Machine check -", 0
Int19String	db	"13h SIMD Floating-Point Exception -", 0

IntUnkString	db	"??h Unknown interrupt -", 0

StringTable:
	dq	Int0String, Int1String, Int2String, Int3String
	dq	Int4String, Int5String, Int6String, Int7String
	dq	Int8String, Int9String, Int10String, Int11String
	dq	Int12String, Int13String, Int14String, Int15String
	dq	Int16String, Int17String, Int18String, Int19String

String1		db	"*** INT ", 0
String2		db	" HALT!! *** (", 0
String3		db	")", 0

StringRax	db	"RAX=", 0
StringRcx	db	" RCX=", 0
StringRdx	db	" RDX=", 0
StringRbx	db	"RBX=", 0
StringRsp	db	" RSP=", 0
StringRbp	db	" RBP=", 0
StringRsi	db	"RSI=", 0
StringRdi	db	" RDI=", 0
StringEcode	db	" ECODE=", 0
StringR8	db	"R8 =", 0
StringR9	db	" R9 =", 0
StringR10	db	" R10=", 0
StringR11	db	"R11=", 0
StringR12	db	" R12=", 0
StringR13	db	" R13=", 0
StringR14	db	"R14=", 0
StringR15	db	" R15=", 0
StringSs	db	" SS =", 0
StringRflags	db	"RFLAGS=", 0

	align	16

EfiLdrCode:
