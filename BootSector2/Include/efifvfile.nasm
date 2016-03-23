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

struc	EfiLdrHeader
.Signature	resd	1
.HeaderCheckSum	resd	1
.FileLength	resd	1
.NumberOfImages	resd	1
endstruc

struc	EfiLdrImage
.CheckSum	resd	1
.Offset		resd	1
.Length		resd	1
.FileName	resb	52
endstruc
