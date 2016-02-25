%macro fatXXhdr 1
%if (%1 != 12) && (%1 != 16) && (%1 != 32)
%error Unknown FAT kind (%1)
%endif
OemId			db	"INTEL	"	; OemId - 8 bytes
SectorSize		dw	0		; Sector Size - 16 bits
SectorsPerCluster	db	0		; Sector Per Cluster - 8 bits
ReservedSectors		dw	0		; Reserved Sectors - 16 bits
NumFats			db	0		; Number of FATs - 8 bits
RootEntries		dw	0		; Root Entries - 16 bits
Sectors			dw	0		; Number of Sectors - 16 bits
Media			db	0		; Media - 8 bits 
SectorsPerFat		dw	0		; Sectors Per FAT - 16 bits
SectorsPerTrack		dw	0		; Sectors Per Track - 16 bits
Heads			dw	0		; Heads - 16 bits
HiddenSectors		dd	0		; Hidden Sectors - 32 bits
LargeSectors		dd	0		; Large Sectors - 32 bits
%if (%1 == 32)
;******************************************************************************
;
;The structure for FAT32 starting at offset 36 of the boot sector. (At this point,
;the BPB/boot sector for FAT12 and FAT16 differs from the BPB/boot sector for FAT32.)
;
;******************************************************************************

SectorsPerFat32	dd	0		; Sectors Per FAT for FAT32 - 4 bytes
ExtFlags	dw	0		; Mirror Flag - 2 bytes
FSVersion	dw	0		; File System Version - 2 bytes
RootCluster	dd	0		; 1st Cluster Number of Root Dir - 4 bytes
FSInfo		dw	0		; Sector Number of FSINFO - 2 bytes
BkBootSector	dw	0		; Sector Number of Bk BootSector - 2 bytes
Reserved	times 12 db 0		; Reserved Field - 12 bytes
PhysicalDrive	db	0		; Physical Drive Number - 1 byte
Reserved1	db	0		; Reserved Field - 1 byte
%else
PhysicalDrive	db	0		; PhysicalDriveNumber - 8 bits 
CurrentHead	db	0		; Current Head - 8 bits
%endif
Signature	db	0		; Extended Boot Signature - 1 byte
VolId		db	"	"	; Volume Serial Number - 4 bytes
FatLabel	db	"	" 	; Volume Label - 11 bytes
FileSystemType	db	"FAT"		; File System Type - 8 bytes
; XXX: nasm manual
%define	fatstr(x)	db	"%1"
		fatstr(FAT)
%undef	fatstr
	db	"	"
%endmacro

FAT_BLOCK_MASK			EQU	01FFh
FAT_BLOCK_SHIFT			EQU	9
FAT_BLOCK_SIZE			EQU	0200h
FAT_DIRECTORY_ENTRY_SHIFT	EQU	5
FAT_DIRECTORY_ENTRY_SIZE	EQU	0020h
