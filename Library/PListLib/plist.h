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

#include <Library/DebugLib.h>

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

typedef long long vlong;

typedef struct _plbuf {
	char* dat;
	unsigned int len; 
	unsigned int pos;
} plbuf_t;

char* plNodeGetBytes(void*);

int plBoolGet(void*);
int plNodeAdd(void* bag, void* elem);
int plNodeToXml(void*, plbuf_t*);

plkind_t plNodeGetKind(void*);

unsigned int plNodeGetSize(void*); /* Applied to any node where size has meaning */

vlong plIntegerGet(void*);

void plNodeDelete(void*);

void* plArrayNew(void);
void* plBoolNew(int);
void* plDataNew(char*, unsigned int);
void* plDateNew(char*, unsigned int);
void* plDictFind(void* dict, char* key, unsigned int klen, plkind_t kind); /* kind can be Any */
void* plDictNew(void);
void* plIntegerNew(vlong);
void* plKeyNew(char*, unsigned int, void*);
void* plNodeGetItem(void*, unsigned int);
void* plStringNew(char*, unsigned int);
void* plXmlToNode(plbuf_t*);
