; Format of MBR partition entry
;
; The symbol 'parte_size' is automatically defined as an `EQU'
; giving the size of the structure.

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
