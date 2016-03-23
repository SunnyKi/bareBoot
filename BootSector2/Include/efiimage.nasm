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

struc	MsDosStub
.noise		resb	0x3c
.e_lfanew	resd	1
endstruc

struc	PeHeader
.Magic			resd	1
.Machine		resw	1
.NumberOfSections	resw	1
.TimeDateStamp		resd	1
.PointerToSymbolTable	resd	1
.NumberOfSymbols	resd	1
.SizeOfOptionalHeader	resw	1
.Characteristics	resw	1	; BIT8 (0x0100) -- 32 bit machine
endstruc

struc	Pe32OptionalHeader
.Magic				resw	1	; 0x010B -- IA32 header, 0x020B -- X64 header
.MajorLinkerVersion		resb	1
.MinorLinkerVersion		resb	1
.SizeOfCode			resd	1
.SizeOfInitializedData		resd	1
.SizeOfUninitializedData	resd	1
.AddressOfEntryPoint		resd	1
.BaseOfCode			resd	1
.BaseOfData			resd	1	; Not present in pe64 header
.ImageBase			resd	1
.SectionAlignment		resd	1
.FileAlignment			resd	1
.MajorOperatingSystemVersion	resw	1
.MinorOperatingSystemVersion	resw	1
.MajorImageVersion		resw	1
.MinorImageVersion		resw	1
.MajorSubsystemVersion		resw	1
.MinorSubsystemVersion		resw	1
.Win32VersionValue		resd	1
.SizeOfImage			resd	1
.SizeOfHeaders			resd	1
.CheckSum			resd	1
.Subsystem			resw	1
.DllCharacteristics		resw	1
.SizeOfStackReserve		resd	1
.SizeOfStackCommit		resd	1
.SizeOfHeapReserve		resd	1
.SizeOfHeapCommit		resd	1
.LoaderFlags			resd	1
.NumberOfRvaAndSizes		resd	1
endstruc

struc	Pe64OptionalHeader
.Magic				resw	1	; 0x010B -- IA32 header, 0x020B -- X64 header
.MajorLinkerVersion		resb	1
.MinorLinkerVersion		resb	1
.SizeOfCode			resd	1
.SizeOfInitializedData		resd	1
.SizeOfUninitializedData	resd	1
.AddressOfEntryPoint		resd	1
.BaseOfCode			resd	1
.ImageBase			resd	1
.SectionAlignment		resd	1
.FileAlignment			resd	1
.MajorOperatingSystemVersion	resw	1
.MinorOperatingSystemVersion	resw	1
.MajorImageVersion		resw	1
.MinorImageVersion		resw	1
.MajorSubsystemVersion		resw	1
.MinorSubsystemVersion		resw	1
.Win32VersionValue		resd	1
.SizeOfImage			resd	1
.SizeOfHeaders			resd	1
.CheckSum			resd	1
.Subsystem			resw	1
.DllCharacteristics		resw	1
.SizeOfStackReserve		resd	1
.SizeOfStackCommit		resd	1
.SizeOfHeapReserve		resd	1
.SizeOfHeapCommit		resd	1
.LoaderFlags			resd	1
.NumberOfRvaAndSizes		resd	1
endstruc

struc	PeSectionHeader
.Name			resb	8
.VirtualSize		resd	1
.VirtualAddress		resd	1
.SizeOfRawData		resd	1
.PointerToRawData	resd	1
.PointerToRealocations	resd	1
.PointerToLinenumbers	resd	1
.NumberOfRealocations	resw	1
.NumberOfLinenumbers	resw	1
.Characteristics	resd	1
endstruc
