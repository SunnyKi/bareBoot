;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
;*
;*   This program and the accompanying materials
;*   are licensed and made available under the terms and conditions of the BSD License
;*   which accompanies this distribution.  The full text of the license may be found at
;*   http://opensource.org/licenses/bsd-license.php
;*
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;------------------------------------------------------------------------------

kSectorBytes		equ	512	; sector size in bytes
kSectorShift		equ	9

kBootSignature		equ	0xAA55	; boot sector signature

kEXFATSignature		equ	'EX'	; exFAT volume signature
kFAT32BootCodeOffset	equ	0x5A	; offset of boot code in FAT32 boot sector
kFAT32BootSignature	equ	'BO'	; Magic string to detect our boot1f32 code
kHFSPCaseSignature	equ	'HX'	; HFS+ volume case-sensitive signature
kHFSPSignature		equ	'H+'	; HFS+ volume signature

kBoot0LoadAddr		equ	0x7C00	; boot0 load address
kBoot0RelocAddr		equ	0xE000
kBoot0Segment		equ	0x0000
kBoot0StackAddr		equ	0xFFF0	; boot0 stack pointer

kBoot1LoadAddr		equ	0x7C00	; boot1 load address (no relocation)
kBoot1Sectors		equ	2	; boot1 size (current)
kBoot1Segment		equ	0x0000
kBoot1StackAddr		equ	0xFFF0	; XXX: boot1 stack pointer (use boot0 one?)

kBoot1Dirbuf		equ	0x00010000	; XXX fat directory buffer (cluster long, segment aligned)
kBoot1Fatbuf		equ	0x00006000	; XXX FAT buffer (1 sector long)

kPayloadHighAddr	equ	(1 * 1024 * 1024)
kPayloadLowAddr		equ	0x00020000
kPayloadLowHdr		equ	(2 * 64 * 1024)		; when payload size exceeds LowMax, copy to low memory only that amount
kPayloadLowMax		equ	(480 * 1024)
