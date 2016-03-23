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

; Format of MBR partition entry

	struc mbrpe
.bootid		resb	1	; bootable or not
.shead		resb	1	; starting head, sector, cylinder
.sssect		resb	1	;
.scyl		resb	1	;
.type		resb	1	; partition type
.ehead		resb	1	; ending head, sector, cylinder
.esect		resb	1	;
.ecyl		resb	1	;
.lba		resd	1	; starting lba
.sectors	resd	1	; size in sectors
	endstruc
