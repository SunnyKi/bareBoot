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

/*
 * All strings are not terminated by '\0'.
 * Ask for size
 */

typedef enum _plkind {
  plKindAny = 0,
  /* bag nodes */
  plKindArray,
  plKindDict,
  plKindKey, /* Node with ascii key and datum child node */
  /* leaf nodes */
  plKindBool,
  plKindData, /* byte array with any content */
  plKindDate,
  plKindInteger,
  plKindString /* string? */
} plkind_t;

typedef struct _plbuf {
	unsigned char* dat;
	unsigned int len; 
	unsigned int pos;
} plbuf_t;

void* plFromXml(plbuf_t*);

int plToXml(void*, plbuf_t*);

plkind_t plGetKind(void*);

unsigned int plGetSize(void*); /* Applied to any node where size has meaning */
unsigned char* plGetBytes(void*);

void* plNewBool(int);
int plGetBool(void*);

void* plNewInteger(long);
long plGetIntValue(void*);

void* plNewDate(unsigned char*, unsigned int);

int plAdd(void* bag, void* elem);

void* plNewArray(void);
void* plNewDict(void);
void* plGetItem(void*, unsigned int);
void* plNewKey(unsigned char*, unsigned int, void*);
void* plFind(void* dict, unsigned char* key, unsigned int klen, plkind_t kind); /* kind can be Any */

void* plNewData(unsigned char*, unsigned int);

void* plNewString(unsigned char*, unsigned int);

void plDeleteNode(void*);
