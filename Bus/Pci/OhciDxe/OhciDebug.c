/** @file
  This file provides the information dump support for OHCI when in debug mode.

Copyright(c) 2013 Intel Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
* Neither the name of Intel Corporation nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include "Ohci.h"
#include "OhciDebug.h"

/**
  Print the data of ED and the TDs attached to the ED

  @param  Ohc                   Pointer to OHCI private data
  @param  Ed                    Pointer to a ED to free
  @param  Td                    Pointer to the Td head

  @retval EFI_SUCCESS
**/

EFI_STATUS
OhciDumpEdTdInfo (
  IN USB_OHCI_HC_DEV      *Ohc,
  IN ED_DESCRIPTOR        *Ed,
  IN TD_DESCRIPTOR        *Td,
  BOOLEAN                 Stage
)
{
  UINT32                  Index;

  if (Stage) {
    DEBUG ((EFI_D_INFO, "Before executing command\n"));
  } else {
    DEBUG ((EFI_D_INFO, "After executing command\n"));
  }
  if (Ed != NULL) {
    DEBUG ((EFI_D_INFO, "ED Address: 0x%08x\n", (UINT32)(UINTN) Ed));
    DEBUG ((EFI_D_INFO, "DWord0  :TD Tail :TD Head :Next ED\n"));
    for (Index = 0; Index < sizeof (ED_DESCRIPTOR) / 4; Index++) {
      DEBUG ((EFI_D_INFO, "%08x ", *((UINT32*) (Ed) + Index)));
    }
  }
  while (Td != NULL) {
    if (Td->Word0.DirPID == TD_SETUP_PID) {
      DEBUG ((EFI_D_INFO, "\nSetup PID "));
    }else if (Td->Word0.DirPID == TD_OUT_PID) {
      DEBUG ((EFI_D_INFO, "\nOut PID "));
    }else if (Td->Word0.DirPID == TD_IN_PID) {
      DEBUG ((EFI_D_INFO, "\nIn PID "));
    }else if (Td->Word0.DirPID == TD_NODATA_PID) {
      DEBUG ((EFI_D_INFO, "\nNo data PID "));
    }
    DEBUG ((EFI_D_INFO, "TD Address: 0x%08x\n", (UINT32)(UINTN) Td));
    DEBUG ((EFI_D_INFO, "DWord0  :CuBuffer:Next TD :Buff End:Next TD :DataBuff:ActLength\n"));
    for (Index = 0; Index < sizeof (TD_DESCRIPTOR) / 4; Index++) {
      DEBUG ((EFI_D_INFO, "%08x ", *((UINT32*) (Td) + Index)));
    }
    DEBUG ((EFI_D_INFO, "\nCurrent TD Data buffer (size %d)\n", (UINT32) Td->ActualSendLength));
    for (Index = 0; Index < Td->ActualSendLength; Index++) {
      DEBUG ((EFI_D_INFO, "%02x ", *(UINT8 *)(UINTN) (Td->DataBuffer+Index)));
    }
  Td = (TD_DESCRIPTOR *)(UINTN) (Td->NextTDPointer);
  }
  DEBUG ((EFI_D_INFO, "\nTD buffer End\n", (UINT32)(UINTN) Td));

  return EFI_SUCCESS;
}

/**
  Print controller registers

  @param  Ohc                   Pointer to OHCI private data
**/

VOID
OhciDumpRegs (
  IN USB_OHCI_HC_DEV      *Ohc
)
{
  UINT32 i;

  DEBUG ((DEBUG_INFO, "%a: begin opregs dump for %p", __FUNCTION__, Ohc));
  for (i = 0; i <= 0x5C; i += 4) {
    if ((i % 0x10) == 0) {
      DEBUG ((DEBUG_INFO, "\n"));
    }
    DEBUG ((DEBUG_INFO, "  0x%02x 0x%08x", i, OhciGetOperationalReg (Ohc->PciIo, i)));
  }
  DEBUG ((DEBUG_INFO, "\n%a: end opregs dump for %p\n", __FUNCTION__, Ohc));
}

/**
  Print HCCA fields

  @param  Ohc                   Pointer to OHCI private data
**/

VOID
OhciDumpHCCA (
  IN USB_OHCI_HC_DEV      *Ohc
)
{
  DEBUG ((DEBUG_INFO, "%a: begin dump for %p\n", __FUNCTION__, Ohc));
  DEBUG ((DEBUG_INFO, "%a: end dump for %p\n", __FUNCTION__, Ohc));
}
