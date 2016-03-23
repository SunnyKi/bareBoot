;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2006 - 2016, Nikolai Saoukh. All rights reserved.<BR>
;*
;*   This program and the accompanying materials
;*   are licensed and made available under the terms and conditions of the BSD License
;*   which accompanies this distribution.  The full text of the license may be found at
;*   http://opensource.org/licenses/bsd-license.php
;*
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;------------------------------------------------------------------------------

	struc fatde
.nameext	resb	11
.attr		resb	1
.ntuse		resb	1
.ttime		resb	1
.ctime		resb	2
.cdate		resb	2
.adate		resb	2
.hcluster	resw	1
.mtime		resb	2
.mdate		resb	2
.lcluster	resw	1
.size		resd	1
	endstruc

nameext_size	equ	(fatde.attr - fatde.nameext)

FATDE_READ_ONLY	EQU	0x01
FATDE_HIDDEN	EQU	0x02
FATDE_SYSTEM	EQU	0x04
FATDE_VOL_ID	EQU	0x08
FATDE_DIRECTORY	EQU	0x10
FATDE_ARCHIVE	EQU	0x20

FATDE_LFN	EQU	0x0F
