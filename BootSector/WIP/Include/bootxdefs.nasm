kSectorBytes		EQU	512	; sector size in bytes

kBootSignature		EQU	0xAA55	; boot sector signature

kEXFATSignature		EQU	'EX'	; exFAT volume signature
kFAT32BootCodeOffset	EQU	0x5A	; offset of boot code in FAT32 boot sector
kFAT32BootSignature	EQU	'BO'	; Magic string to detect our boot1f32 code
kHFSPCaseSignature	EQU	'HX'	; HFS+ volume case-sensitive signature
kHFSPSignature		EQU	'H+'	; HFS+ volume signature

kBoot0LoadAddr		EQU	0x7C00	; boot0 load address
kBoot0RelocAddr		EQU	0xE000	; boot0 relocated address
kBoot0Segment		EQU	0x0000
kBoot0StackAddr		EQU	0xFFF0	; boot0 stack pointer

kBoot1LoadAddr		EQU	0x7C00	; boot1 load address
kBoot1RelocAddr		EQU	0xE000	; boot1 relocated address
kBoot1Segment		EQU	0x0000
kBoot1StackAddr		EQU	0xFFF0	; boot1 stack pointer

kBoot2LoadAddr		EQU	0x0200					; boot2 load address
kBoot2Segment		EQU	0x2000					; boot2 load segment
kBoot2Sectors		EQU	(480 * 1024 - 512) / kSectorBytes	; max size of 'boot' file in sectors
