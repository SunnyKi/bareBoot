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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <b64/cencode.h>
#include <b64/cdecode.h>

#include "plist_helpers.h"

char*
_plstrcpy(char* dst, const char* src) {
	return strcpy(dst, src);
}

int
_plint2str(long val, char* vbuf, unsigned int bsz) {
	return snprintf(vbuf, bsz, "%ld", val);
}

int
_plstrcmp(const char* s1, const char* s2) {
	return strcmp(s1, s2);
}

unsigned int
_plstrlen(const char* str) {
	return strlen(str);
}

void
_plfree(void* ptr) {
	free(ptr);
}

int
_plmemcmp(const void* s1, const void* s2, unsigned int sz) {
	return memcmp(s1, s2, sz);
}

void*
_plmemcpy(void* dst, const void* src, unsigned int sz) {
	return memcpy(dst, src, sz);
}

void*
_plzalloc(unsigned int sz) {
	return (void*)calloc(sz, 1);
}

/* Following sources heavily inspired by SunnyKi */

char*
_plb64encode(unsigned char* idat, unsigned int ilen, unsigned int* olen) {
	unsigned int osiz;
	unsigned int csiz;
	unsigned int tsiz;
	unsigned char* odat;
	base64_encodestate b64state;

	if (idat == NULL || ilen == 0) { return NULL; }
	osiz = (((ilen + 2) / 3) + 1 ) * 4; 
	odat = _plzalloc(osiz);
	if (odat == NULL) { return NULL; }
	base64_init_encodestate(&b64state);
	csiz = base64_encode_block(idat, ilen, odat, &b64state);
	tsiz = base64_encode_blockend(&odat[csiz], &b64state);
	if (olen != NULL) { *olen = csiz + tsiz; }
	return odat;
}

unsigned char*
_plb64decode(char* idat, unsigned int ilen, unsigned int* olen) {
	unsigned int binsz;
	unsigned char* odat;
	base64_decodestate b64state;
	
	if (idat == NULL || ilen == 0) { return NULL; }
	// to simplify, we'll allocate the same size, although smaller size is needed
	odat = _plzalloc(ilen);
	if (odat == NULL) { return NULL; }
	base64_init_decodestate(&b64state);
	binsz = base64_decode_block(idat, (const int)ilen, (char*)odat, &b64state);
	if (olen != NULL) { *olen = binsz; }
	return odat;
}
