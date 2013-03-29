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

#include "plist.h"
#include "plist_helpers.h"

#include "plist_xml_parser.h"

struct _plnode;

struct _plnode {
	struct _plnode* next;
	struct _plnode* last;
	struct _plnode* child;

	char* datval;
	vlong intval; /* int64 */
	unsigned int datlen;
	plkind_t kind;
};

typedef struct _plnode _plnode_t;

int _plGrowTree(TagPtr ipl, void* pn);

_plnode_t*
_plNewNode(plkind_t nk) {
	_plnode_t* node;

	node = (_plnode_t*)_plzalloc(sizeof(_plnode_t));
	if (node != NULL) {
		node->kind = nk;
	}
	return node;
}

void
plDeleteNode(void* node) {
	_plnode_t* pn;
	_plnode_t* wn;

	if (node == NULL) { return; }
	pn = (_plnode_t*)node;

	wn = pn->child;
	while (wn != NULL) {
		_plnode_t* nn;

		nn = wn->next;
		plDeleteNode(wn);
		wn = nn;
	}

	if (pn->datval != NULL) {
		_plfree(pn->datval);
	}

	_plfree(node);
}

int
plAdd(void* bag, void* node) {
	plkind_t bk;
	_plnode_t* bn;
	_plnode_t* dn;

	if (bag == NULL || node == NULL) {
		return 0;
	}
	bk = plGetKind(bag);
	dn = (_plnode_t*)node;
	/* We add to dict only well formed keys */
	if (bk == plKindDict && (plGetKind(node) != plKindKey || dn->child == NULL)) {
		return 0;
	}
	bn = (_plnode_t*)bag;
	switch(bk) {
	case plKindDict:
		/* FALLTHROUGH */
	case plKindArray:
		dn->next = NULL;
		if (bn->last == NULL) {
			bn->child = node;
		} else {
			bn->last->next = node;
		}
		bn->last = node;
		return 1;
	case plKindKey:
		if (bn->child != NULL) {
			return 0;
		}
		bn->child = node;
		return 1;
	default:
		return 0;
	}
}

void*
plNewDict(void) {
	_plnode_t* node;

	node = _plNewNode(plKindDict);
	return node;
}

void*
plNewArray(void) {
	_plnode_t* node;

	node = _plNewNode(plKindArray);
	return node;
}

void*
plNewData(char* datum, unsigned int dlen) {
	_plnode_t* node;

	if (datum == NULL && dlen == 0) { return NULL; }
	node = _plNewNode(plKindData);
	if (node == NULL) { return NULL; }
	node->datval = _plzalloc(dlen + 1);
	if (node->datval == NULL) {
		plDeleteNode(node);
		return NULL;
	}
	if (dlen > 0) { _plmemcpy(node->datval, datum, dlen); }
	node->datlen = dlen;
	return node;
}

void*
plNewKey(char* key, unsigned int klen, void* datum) {
	_plnode_t* node;

	node = plNewData(key, klen);
	if (node == NULL) { return NULL; }
	node->kind = plKindKey;
	node->child = datum;
	return node;
}

void*
plNewDate(char* datum, unsigned int dlen) {
	_plnode_t* node;

	node = plNewData(datum, dlen);
	if (node == NULL) { return NULL; }
	node->kind = plKindDate;
	return node;
}

void*
plNewString(char* datum, unsigned int dlen) {
	_plnode_t* node;

	node = plNewData(datum, dlen);
	if (node == NULL) { return NULL; }
	node->kind = plKindString;
	return node;
}

unsigned int
plGetSize(void* pn) {
	unsigned int sz;
	_plnode_t* wn;

	wn = (_plnode_t*) pn;
	switch(plGetKind(pn)) {
	case plKindArray:
	case plKindDict:
		for (sz = 0, wn = wn->child; wn != NULL; wn = wn->next) { sz++; }
		return sz;
	case plKindKey:
	case plKindData:
	case plKindString:
		return wn->datval != NULL ? wn->datlen : 0;
	default:
		break;
	}
	return 0;
}

char*
plGetBytes(void* pn) {
	switch (plGetKind(pn)) {
	case plKindKey:
	case plKindData:
	case plKindString:
		return ((_plnode_t*) pn)->datval;
	default:
		break;
	}
	return NULL;
}

void*
plGetItem(void* bag, unsigned int inum) {
	_plnode_t* wn;
	unsigned int i;

	if (bag == NULL) { return NULL; }
	wn = ((_plnode_t*) bag)->child;
	switch (plGetKind(bag)) {
	case plKindArray:
	case plKindDict:
		if (inum >= plGetSize(bag)) { return NULL; }
		for (i = 0; i < inum; i++) {
			wn = wn->next;
		}
		/* FALLTHROUGH */
	case plKindKey:
		return wn;
	default:
		break;
	}
	return NULL;
}

void*
plNewBool(int bval) {
	_plnode_t* node;

	node = _plNewNode(plKindBool);
	node->intval = (bval != 0);
	return node;
}

int
plGetBool(void* pn) {
	_plnode_t* wn;

	wn = (_plnode_t*) pn;
	return (wn->intval != 0 ? 1 : 0);
}

void*
plNewInteger(vlong val) {
	_plnode_t* node;

	node = _plNewNode(plKindInteger);
	node->intval = val;
	return node;
}

vlong
plGetIntValue(void* pn) {
	_plnode_t* wn;

	wn = (_plnode_t*) pn;
	return wn->intval;
}

plkind_t
plGetKind(void* node) {
	_plnode_t* pn;

	if(node == NULL) { return plKindAny; }
	pn = (_plnode_t*)node;
	return pn->kind;
}

void*
plFind(void* dict, char* key, unsigned int klen, plkind_t kind) {
	unsigned int dsz;
	unsigned int i;

	if (plGetKind(dict) != plKindDict || key == NULL || klen < 1) { return NULL; }
	dsz = plGetSize(dict);
	for (i = 0; i < dsz; i++) {
		void* dent;
		void* kval;

		dent = plGetItem(dict, i);
		if (dent == NULL || plGetKind(dent) != plKindKey || plGetSize(dent) != klen) { continue; }
		if (_plmemcmp(plGetBytes(dent), key, klen) != 0) { continue; }
		kval = plGetItem(dent, 0);
		if (kind == plKindAny || plGetKind(kval) == kind) { return kval; }
	}
	return NULL;
}

int
_plGrowBag(TagPtr ipl, void* parn, void* bagn) {
	if (bagn == NULL) { return 0; }
	if (!_plGrowTree(ipl->tag, bagn) || !plAdd(parn, bagn)) {
		plDeleteNode(bagn);
		return 0;
	}
	return 1;
}

int
_plGrowTree(TagPtr ipl, void* pn) {
	void* wn;

	while(ipl != NULL) {
		wn = NULL;
		switch (ipl->type) {
		case kTagTypeArray:
			wn = plNewArray();
			if (!_plGrowBag(ipl, pn, wn)) { return 0; }
			break;
		case kTagTypeDict:
			wn = plNewDict();
			if (!_plGrowBag(ipl, pn, wn)) { return 0; }
			break;
		case kTagTypeKey:
			wn = plNewKey(ipl->string, ipl->dataLen, NULL);
			if (!_plGrowBag(ipl, pn, wn)) { return 0; }
			break;
		case kTagTypeString:
			wn = plNewString(ipl->string, ipl->dataLen);
			if (wn == NULL || !plAdd(pn, wn)) { return 0; }
			break;
		case kTagTypeData:
			wn = plNewData((char*)(ipl->data), ipl->dataLen);
			if (wn == NULL || !plAdd(pn, wn)) { return 0; }
			break;
		case kTagTypeTrue:
			wn = plNewBool(1);
			if (wn == NULL || !plAdd(pn, wn)) { return 0; }
			break;
		case kTagTypeFalse:
			wn = plNewBool(0);
			if (wn == NULL || !plAdd(pn, wn)) { return 0; }
			break;
		case kTagTypeInteger:
			wn = plNewInteger(ipl->intval);
			if (wn == NULL || !plAdd(pn, wn)) { return 0; }
			break;
		case kTagTypeDate:
			wn = plNewDate(ipl->string, ipl->dataLen);
			if (wn == NULL || !plAdd(pn, wn)) { return 0; }
			break;
		default:
			return 0;
		}
		ipl = ipl->tagNext; /* next sibling */
	}

	return 1;
}

void*
plFromXml(plbuf_t* ibuf) {
	TagPtr plist;
	void* pn;
	int rc;

	rc = 0;
	pn = NULL;

	rc = PListParseXML(ibuf->dat, ibuf->len, &plist);
	if(rc == 0) {
		switch(plist->type) {
		case kTagTypeDict:
			pn = plNewDict();
			rc = _plGrowTree(plist->tag, pn);
			if (rc == 0) { plDeleteNode(pn); }
			break;
		default:
			rc = 0;
			break;
		}
		FreeTag(plist);
	}
	return  rc == 0 ? NULL : pn;
}
