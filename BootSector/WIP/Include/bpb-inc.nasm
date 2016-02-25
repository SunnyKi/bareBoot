; Bios Parameter Block Structures

BOOTABLE_SIGNATURE      equ     0xAA55
FAT_SIGNATURE_1         equ     0x28
FAT_SIGNATURE_2         equ     0x29
MEDIA_TYPE_FIXED_DISC   equ     0xF8

struc bpb
    .oem                resb    8
    .bytesPerSector     resw    1
    .sectorsPerCluster  resb    1
    .reservedSectors    resw    1
    .fatCount           resb    1
    .dirEntryCount      resw    1
    .sectorCount        resw    1
    .mediaType          resb    1
    .sectorsPerFat      resw    1
    .sectorsPerTrack    resw    1
    .heads              resw    1
    .hiddenSectors      resd    1
    .sectorCountLarge   resd    1
endstruc

struc ebpb16
    .bpb                resb    bpb_size
    .driveNumber        resb    1
    .flags              resb    1
    .signature          resb    1
    .volumeId           resb    4
    .volumeLabel        resb    11
    .systemId           resb    8
endstruc

struc ebpb32
    .bpb                resb    bpb_size
    .sectorsPerFat32    resd    1
    .flags1             resw    1
    .version            resw    1
    .rootCluster        resd    1
    .fsInfoSector       resw    1
    .bootBackupSector   resw    1
    .reserved           resb    12
    .driveNumber        resb    1
    .flags2             resb    1
    .signature          resb    1
    .volumeId           resb    4
    .volumeLabel        resb    11
    .systemId           resb    8
endstruc
