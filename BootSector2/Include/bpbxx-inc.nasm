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

%macro	bpb00	0
.oem			resb	8
.bytesPerSector		resw	1
.sectorsPerCluster	resb	1
.reservedSectors	resw	1
.fatCount		resb	1
.dirEntryCount		resw	1
.sectorCount		resw	1
.mediaType		resb	1
.sectorsPerFat		resw	1
.sectorsPerTrack	resw	1
.heads			resw	1
.hiddenSectors		resd	1
.sectorCountLarge	resd	1
%endmacro

struc	bpb16
			bpb00
.driveNumber		resb	1
.flags			resb	1
.signature		resb	1
.volumeId		resb	4
.volumeLabel		resb	11
.systemId		resb	8
endstruc

struc	bpb32
			bpb00
.sectorsPerFat32	resd	1
.flags1			resw	1
.version		resw	1
.rootCluster		resd	1
.fsInfoSector		resw	1
.bootBackupSector	resw	1
.reserved		resb	12
.driveNumber		resb	1
.flags2			resb	1
.signature		resb	1
.volumeId		resb	4
.volumeLabel		resb	11
.systemId		resb	8
endstruc
