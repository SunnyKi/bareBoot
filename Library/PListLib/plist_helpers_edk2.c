/*
 * Copyright (c) 2013 Nikolai Saoukh. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#include <b64/cencode.h>
#include <b64/cdecode.h>

#include "plist.h"
#include "plist_helpers.h"

int
_plstrcpy (
  char *dst,
  unsigned int dsz,
  const char *src
)
{
  RETURN_STATUS sts;

  sts = AsciiStrCpyS (dst, dsz, src);
  if (sts) {
    return FALSE;
  }
  return TRUE;
}

int
_plint2str (
  vlong val,
  char *vbuf,
  unsigned int bsz
)
{
  RETURN_STATUS sts;

  sts = AsciiValueToStringS (vbuf, bsz, 0x00, val, bsz - 1);

  if (sts) {
    return 0;
  }

  return (int) _plstrlen (vbuf);
}

vlong
_plstr2vlong (
  char *vbuf,
  unsigned int bsz
)
{
  UINT64 rval;
  unsigned int wsize;
  char *wptr;
  int negative;
  char wbuffer[40];

  if (vbuf == NULL || bsz < 1) {
    return 0;
  }
  wsize = sizeof (wbuffer) - 1;
  if (bsz < wsize) {
    wsize = bsz;
  }
  _plmemcpy (wbuffer, vbuf, wsize);
  wbuffer[wsize] = '\0';
  wptr = ScanMem8 (wbuffer, wsize, '-');
  if (wptr != NULL) {
    negative = 1;
    wptr++;
  }
  else {
    negative = 0;
    wptr = wbuffer;
  }
  wsize -= (unsigned int) (wptr - wbuffer);
  if (ScanMem8 (wptr, wsize, 'x') != NULL ||
      ScanMem8 (wptr, wsize, 'X') != NULL) {
    rval = AsciiStrHexToUint64 (wptr);
  }
  else {
    rval = AsciiStrDecimalToUint64 (wptr);
  }
  if (negative) {
    return (vlong) (0 - (vlong) rval);
  }
  else {
    return (vlong) rval;
  }
}

int
_plstrcmp (
  const char *s1,
  const char *s2
)
{
  return (int) AsciiStrCmp (s1, s2);
}

unsigned int
_plstrlen (
  const char *str
)
{
  return (unsigned int) AsciiStrLen (str);
}

void
_plfree (
  void *ptr
)
{
  FreePool (ptr);
}

int
_plmemcmp (
  const void *s1,
  const void *s2,
  unsigned int sz
)
{
  return (int) CompareMem (s1, s2, sz);
}

void *
_plmemcpy (
  void *dst,
  const void *src,
  unsigned int sz
)
{
  return CopyMem (dst, src, sz);
}

void *
_plzalloc (
  unsigned int sz
)
{
  if (sz == 0) {
    return NULL;
  }
  return AllocateZeroPool (sz);
}

/* Following sources heavily inspired by SunnyKi ;-) */

char *
_plb64encode (
  char *idat,
  unsigned int ilen,
  unsigned int *olen
)
{
  unsigned int osiz;
  unsigned int csiz;
  unsigned int tsiz;
  char *odat;
  base64_encodestate b64state;

  if (idat == NULL || ilen == 0) {
    return NULL;
  }
  osiz = (((ilen + 2) / 3) + 1) * 4;
  odat = _plzalloc (osiz);
  if (odat == NULL) {
    return NULL;
  }
  base64_init_encodestate (&b64state);
  csiz = base64_encode_block (idat, ilen, odat, &b64state);
  tsiz = base64_encode_blockend (&odat[csiz], &b64state);
  if (olen != NULL) {
    *olen = csiz + tsiz;
  }
  return odat;
}

char *
_plb64decode (
  char *idat,
  unsigned int ilen,
  unsigned int *olen
)
{
  unsigned int binsz;
  char *odat;
  base64_decodestate b64state;

  if (idat == NULL || ilen == 0) {
    return NULL;
  }
  // to simplify, we'll allocate the same size, although smaller size is needed
  odat = _plzalloc (ilen);
  if (odat == NULL) {
    return NULL;
  }
  base64_init_decodestate (&b64state);
  binsz = base64_decode_block (idat, (const int) ilen, odat, &b64state);
  if (olen != NULL) {
    *olen = binsz;
  }
  return odat;
}
