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

FATDE_READ_ONLY	EQU	0x01
FATDE_HIDDEN	EQU	0x02
FATDE_SYSTEM	EQU	0x04
FATDE_VOL_ID	EQU	0x08
FATDE_DIRECTORY	EQU	0x10
FATDE_ARCHIVE	EQU	0x20

FATDE_LFN	EQU	0x0F
