; Copyright (c) 1999-2003 Apple Computer, Inc. All rights reserved.
;
; @APPLE_LICENSE_HEADER_START@
;
; Portions Copyright (c) 1999-2003 Apple Computer, Inc. All Rights
; Reserved. This file contains Original Code and/or Modifications of
; Original Code as defined in and that are subject to the Apple Public
; Source License Version 2.0 (the "License"). You may not use this file
; except in compliance with the License. Please obtain a copy of the
; License at http://www.apple.com/publicsource and read it before using
; this file.
;
; The Original Code and all software distributed under the License are
; distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
; EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
; INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT. Please see the
; License for the specific language governing rights and limitations
; under the License.
;
; @APPLE_LICENSE_HEADER_END@
;------------------------------------------------------------------------------
;
; Partition Boot Loader: bootx132
;
; This program is designed to reside in sector 0 (+1) of a FAT32 partition.
; It expects that the MBR has left the drive number in DL
; and a pointer to the partition entry in SI.
;
; This version requires a BIOS with EBIOS (LBA) support.
;
; This code is written for the NASM assembler.
;
;	nasm -DBOOTSEL=1 -DVERBOSE=0 -DUSESIDL=1 -o $@ bootx132.nasm
;
; This version of bootx132 tries to find the file 'boot ' with efi stuff in the root folder.
;
; Written by mackerintel on 2009-01-26
; nms was here ;-) 20160323
;------------------------------------------------------------------------------

%include	"bootxdefs.nasm"
%include	"bpbxx-inc.nasm"
%include	"mbrstruct.nasm"
%include	"fatstruct.nasm"

%macro LogString 1
	mov	di, %1
	call	bLogString
%endmacro

;--------------------------------------------------------------------------

	BITS	16

	ORG	kBoot1LoadAddr	; 0x0000:0x7C00

Start0:
	jmp	Start1
	times	3 - ($ - $$) nop

;--------------------------------------------------------------------------

gDatum:
	istruc	bpb32	; will be filled by boot blocks stuffing machine ;-)
	iend

zFileName	db	"BOOT       "	; Mandatory first two bytes for boot0
					; boot file name too

zBIOSDriveNumber	equ	(gDatum + bpb32.driveNumber)
zNumFats		equ	(gDatum + bpb32.fatCount)
zPartLBAOffset		equ	(gDatum + bpb32.hiddenSectors)
zReservedSectors	equ	(gDatum + bpb32.reservedSectors)
zRootCluster		equ	(gDatum + bpb32.rootCluster)
zSPC			equ	(gDatum + bpb32.sectorsPerCluster)
zFatSectorsTotal	equ	(gDatum + bpb32.sectorsPerFat32)

;--------------------------------------------------------------------------

Start1:
	cli
	xor	eax, eax		; zero eax (needed below at gvars init)
	mov	ss, ax
	mov	sp, kBoot1StackAddr
	sti

	mov	ds, ax
	mov	es, ax

;--------------------------------------------------------------------------
; Initializing global variables

	mov	ax, word [zReservedSectors]
%if USESIDL
	mov	[zBIOSDriveNumber], dl
	add	eax, [si + mbrpe.lba]
%else
	add	eax, [zPartLBAOffset]
%endif
	mov	[zPartLBAOffset], eax
	xor	eax, eax
	mov	al, [zNumFats]
	mul	dword [zFatSectorsTotal]
	mov	[zFatSectorsTotal], eax

;--------------------------------------------------------------------------
; Find payload file in a FAT32 Volume's root directory & load it

%if BOOTSEL
selBootFile:
	mov	cx, 2000	; loop counter = max 2000 miliseconds in total

.loop:
	mov	ah, 0x01	; int 0x16, Func 0x01 - get keyboard status/preview key
	int	0x16
	jz	.wait		; no keypress - wait and loop again
	xor	ah, ah		; read the char from buffer to spend it
	int	0x16

	mov	BYTE [zFileName + 4], al	; put key to file name /boot<pressed key>
	jmp	SHORT bootFileSelected		; try to find & boot

.wait:
	; waith for 1 ms: int 0x15, Func 0x86 (wait for cx:dx microseconds)

	push	cx	; save loop counter
	xor	cx, cx
	mov	dx, 1000
	mov	ah, 0x86
	int	0x15
	pop	cx
	loop	.loop

%endif

bootFileSelected:
%if VERBOSE
	LogString(init_str)
%endif
	mov	eax, [zRootCluster]

nextdirclus:
	mov	edx, kBoot1Dirbuf
	call	readCluster
	jc	error
	xor	si, si
	mov	bl, [zSPC]
	shl	bx, 9
	add	bx, si

nextdirent:
	mov	di, zFileName
	push	ds
	push	(kBoot1Dirbuf >> 4)
	pop	ds
	mov	cl, [si]
	test	cl, cl
	jz	dserror
	mov	cx, nameext_size
	repe	cmpsb
	jz	direntfound

falsealert:
	pop	ds
	add	cl, (fatde_size - nameext_size) ; next entry, please
	add	si, cx
	cmp	si, bx
	jz	nextdirclus
	jmp	nextdirent

direntfound:
	lodsb				; fatde.attr to AL
	test	al, FATDE_VOL_ID | FATDE_DIRECTORY
	jnz	falsealert
	sub	si, (nameext_size + 1)	; move back to the start of the entry
	push	word [si + fatde.hcluster]
	push	word [si + fatde.lcluster]
	pop	eax
	pop	ds
	push	si			; save dir entry offset

;--------------------------------------------------------------------------

	mov	edx, kPayloadHighAddr

readFileDatum:
	push	edx
	mov	edx, kPayloadLowAddr	; reuse low memory for buffer
	call	readCluster
	pop	edx
	jc	nextStage

	xor	ebx, ebx
	mov	bl, [zSPC]
	shl	ebx, 9

	push	eax
	mov	eax, kPayloadLowAddr
	push	ebx
	call	copyHigh
	pop	ebx
	pop	eax

	add	edx, ebx
	jmp	readFileDatum

;--------------------------------------------------------------------------
; Payload in high memory. Lets play with it!

nextStage:
	push	(kBoot1Dirbuf >> 4)
	pop	es
	pop	si				; recover dir entry offset
	mov	ebx, [es:si + fatde.size]	; total payload size
	cmp	ebx, kPayloadLowMax		; does the payload fits low memory?
	jle	.notbig
	mov	ebx, kPayloadLowHdr		; nope, copy just prelude

.notbig:
	mov	eax, kPayloadHighAddr
	mov	edx, kPayloadLowAddr
	call	copyHigh
	mov	dl, [zBIOSDriveNumber]	; load BIOS drive number
	jmp	(kPayloadLowAddr >> 4):(kPayloadLowAddr & 0xFFFF)

;--------------------------------------------------------------------------
; Directory search error

dserror:
	pop ds

error:
%if VERBOSE
	LogString(error_str)
%endif

hang:
	hlt
	jmp	hang

;--------------------------------------------------------------------------
; readCluster - Reads cluster EAX to (EDX), updates EAX to next cluster

cachedFatSector	dd	0xFFFFFFEE	; some bizzare value

readCluster:
	cmp	eax, 0x0FFFFFF8	; bad cluster or end of the chain?
	jb	.do_read

	stc
	ret

.do_read:
	push	eax
	xor	ecx, ecx
	dec	eax
	dec	eax
	mov	cl, [zSPC]
	push	edx
	mul	ecx
	pop	edx
	add	eax, [zFatSectorsTotal]
	mov	ecx, eax
	xor	ah, ah
	mov	al, [zSPC]
	call	readSectors
	jc	.clusend

	pop	ecx
	push	cx
	shr	ecx, 7
	cmp	ecx, [cachedFatSector]
	je	.inmemory

	push	ecx
	xor	ax, ax
	inc	ax
	mov	edx, kBoot1Fatbuf
	call	readSectors
	pop	ecx
	jc	.clusend

	mov	[cachedFatSector], ecx

.inmemory:
	pop	si
	and	si, 0x7F
	shl	si, 2
	mov	eax, [si + kBoot1Fatbuf]
	and	eax, 0x0FFFFFFF
	clc
	ret

.clusend:
	pop	eax
	ret

;--------------------------------------------------------------------------
; Align to 512 bytes for the mandatory boot sector signature.

	times	510 - ($ - $$) db 0
	dw	kBootSignature

;--------------------------------------------------------------------------
; readSectors - Reads more than 127 sectors using LBA addressing.
;
; Arguments:
;	AX = number of 512-byte sectors to read (valid from 1-1280).
;	EDX = pointer to where the sectors should be stored.
;	ECX = sector offset in partition
;
; Returns:
;	CF = 0	success
;	1 error

maxSectorCount	EQU	64	; maximum sector count we can handle at once

readSectors:
	pushad
	mov	bx, ax

.loop:
	xor	eax, eax		; EAX = 0
	mov	al, bl			; assume we reached the last block.
	cmp	bx, maxSectorCount	; check if we really reached the last block
	jb	.readBlock		; yes, BX < MaxSectorCount
	mov	al, maxSectorCount	; no, read MaxSectorCount

.readBlock:
	call	readLBA
	sub	bx, ax		; decrease remaning sectors with the read amount
	jz	.exit		; exit if no more sectors left to be loaded
	add	ecx, eax	; adjust LBA sector offset
	shl	ax, 9		; convert sectors to bytes
	add	edx, eax	; adjust target memory location
	jmp	.loop		; read remaining sectors

.exit:
	popad
	ret

;--------------------------------------------------------------------------
; readLBA - Read sectors from a partition using LBA addressing.
;
; Arguments:
;	AL = number of 512-byte sectors to read (valid from 1-127).
;	EDX = pointer to where the sectors should be stored.
;	ECX = sector offset in partition
;	[bios_drive_number] = drive number (0x80 + unit number)
;
; Returns:
;	CF = 0	success
;	1 error

readLBA:
	pushad		; save all registers
	push	es	; save ES
	mov	bp, sp	; save current SP


	; Convert EDX to segment:offset model and set ES:BX
	;
	; Some BIOSes do not like offset to be negative while reading
	; from hard drives. This usually leads to "boot1: error" when trying
	; to boot from hard drive, while booting normally from USB flash.
	; The routines, responsible for this are apparently different.
	; Thus we split linear address slightly differently for these
	; capricious BIOSes to make sure offset is always positive.

	mov	bx, dx		; save offset to BX
	and	bh, 0x0F	; keep low 12 bits
	shr	edx, 4		; adjust linear address to segment base
	xor	dl, dl		; mask low 8 bits
	mov	es, dx		; save segment to ES

	; Create the Disk Address Packet structure for the
	; INT13/F42 (Extended Read Sectors) on the stack.

	; push	dword 0		; offset 12, upper 32-bit LBA
	push	ds		; For sake of saving memory,
	push	ds		; push DS register, which is 0.
	add	ecx, [zPartLBAOffset]	; offset 8, lower 32-bit LBA
	push	ecx
	push	es		; offset 6, memory segment
	push	bx		; offset 4, memory offset
	xor	ah, ah		; offset 3, must be 0
	push	ax		; offset 2, number of sectors
	push	word 16		; offset 0-1, packet size


	; INT13 Func 42 - Extended Read Sectors
	;
	; Arguments:
	;	AH	= 0x42
	;	[bios_drive_number] = drive number (0x80 + unit number)
	;	DS:SI = pointer to Disk Address Packet
	;
	; Returns:
	;	AH	= return status (sucess is 0)
	;	carry = 0 success
	;	1 error
	;
	; Packet offset 2 indicates the number of sectors read
	; successfully.

	mov	dl, [zBIOSDriveNumber]	; load BIOS drive number
	mov	si, sp
	mov	ah, 0x42
	int	0x13
	jc	error

%if 0
	; Issue a disk reset on error.
	; Should this be changed to Func 0xD to skip the diskette controller
	; reset?

	xor	ax, ax	; Func 0
	int	0x13	; INT 13
	stc		; set carry to indicate error
%endif

.exit:
	mov	sp, bp	; restore SP
	pop	es	; restore ES
	popad
	ret

;------------------------------------------------------------------------------
; High memory copy
; Arguments:
;	EAX = source address
;	EBX = even number of bytes to copy
;	EDX = destination address

copyHigh:
	push	eax
	push	ebx
	push	ecx
	push	edx

	mov	ecx, ebx

.checklimit:
	cmp	ebx, 0x10000	; biggest chunk int15-87 can handle
	jle	.docopy

	mov	ebx, 0x10000

.docopy:
	call	copyChunkHigh
	add	eax, ebx	; advance pointers
	add	edx, ebx
	sub	ecx, ebx	; adjust size
	jz	.copydone

	mov	ebx, ecx
	jmp	.checklimit

.copydone:
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	ret

struc	copymemdscr
.res1	resb	8
.res2	resb	8
.slimit	resw	1
.p24src	resb	3
.sright	resw	1
.shigh	resb	1
.dlimit	resw	1
.p24dst	resb	3
.dright	resw	1
.dhigh	resb	1
.res4	resb	16
endstruc

copyChunkHigh:
	push	ds
	push	esi
	push	edi
	push	eax
	push	ecx

	xor	ax, ax
	mov	cx, copymemdscr_size
	sub	sp, cx	; allocate gdt buffer from stack
	mov	di, sp	; es:di = gdt address
	push	es
	push	ss
	pop	es
	rep	stosb	; zero gdt
	pop	es
	mov	si, sp	; es:si = gdt address
	push	ss
	pop	ds

	mov	dword [si + copymemdscr.p24src], eax	; int 15-87: source address bits 0-23
	bswap	eax					; al is source address bits 24-31
	mov	byte [si + copymemdscr.shigh], al	; int 15-87: source address bits 25-31

	mov	eax, edx				; eax is 32-bit destination address
	mov	dword [si + copymemdscr.p24dst], eax	; int 15-87: dest address bits 0-23
	bswap	eax					; al is dest address bits 24-31
	mov	byte [si + copymemdscr.dhigh], al	; int 15-87: dest address bits 25-31

	mov	cx, 0xCF93				; access rights byte, lim 16-19 (max), gran=1, big=1
	mov	word [si + copymemdscr.sright], cx	; int 15-87: source segment limit (max)
	mov	word [si + copymemdscr.dright], cx	; int 15-87: dest segment limit (max)

	mov	cx, 0xFFFF				; segment limit (max)
	mov	word [si + copymemdscr.slimit], cx	; int 15-87: source segment limit (max)
	mov	word [si + copymemdscr.dlimit], cx	; int 15-87: dest segment limit (max)

	mov	ah, 0x87				; int 15h code for copy
	push	es					; save es before 15-87 use
	push	ss
	pop	es
	mov	ecx, ebx
	shr	ecx, 1	; cx is word count
	int	0x15	; do the copy operation
	jc	$	; XXX: debug: stop if error

	pop	es			; restore es after 15-87 use
	add	sp, copymemdscr_size	; deallocate gdt
	pop	ecx			; restore registers
	pop	eax
	pop	edi
	pop	esi
	pop	ds

	ret

%if VERBOSE
;--------------------------------------------------------------------------
; Write a string with our prefix to the console.
;
; Arguments:
;	DS:DI	pointer to a NUL terminated string.
;
; Clobber list:
;	DI

bLogString:
	pushad
	push	di
	mov	si, log_title_str
	call	bPrintString
	pop	si
	call	bPrintString
	popad
	ret

;-------------------------------------------------------------------------
; Write a string to the console
;
; Arguments:
;	DS:SI	pointer to a NUL terminated string.
;
; Clobber list:
;	AX, BX, SI

bPrintString:
	mov	bx, 1	; BH=0, BL=1 (blue)

.1:
	lodsb		; load a byte from DS:SI into AL
	cmp	al, NUL	; End of string?
	je	.2	; yes, all done
	mov	ah, 0x0E
	int	0x10	; display byte in tty mode
	jmp	.1

.2:
	ret

;--------------------------------------------------------------------------
; Static data

CR		EQU	0x0D
LF		EQU	0x0A
NUL		EQU	0x00

log_title_str	db	CR, LF, 'bootx1: ', NUL
init_str	db	'init', NUL
error_str	db	'error', NUL
%endif	; VERBOSE

;------------------------------------------------------------------------------

	align	512, db 0

; This is the end
