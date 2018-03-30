/*
 * Copyright (c) 2013-2018 Nikolai Saoukh. All rights reserved.
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
	int refcnt;
	plkind_t kind;

	struct _plnode* keyval;
	char* datval;
	unsigned int datlen;
	vlong intval; /* int64 */
	struct _plnode** amap;	/* array map to values */
	unsigned int acap;	/* array capacity size */
	unsigned int asiz;	/* number of items in array */
};

typedef struct _plnode _plnode_t;

int _plGrowTree(TagPtr ipl, void* pn);

/* Property List stuff */

_plnode_t*
_plNodeNew(plkind_t nk) {
	_plnode_t* node;

	node = (_plnode_t*)_plzalloc(sizeof(_plnode_t));
	if (node != NULL) { node->kind = nk; }
	return node;
}

void
plNodeDelete(void* node) {
	_plnode_t* pn;

	if (node == NULL) { return; }
	pn = (_plnode_t*)node;

	pn->refcnt--;
	if (pn->refcnt > 0) { return; }

	if (pn->datval != NULL) { _plfree(pn->datval); }
	if (pn->keyval != NULL) { plNodeDelete(pn->keyval); }
	if (pn->amap != NULL) {
		unsigned int i;
		for (i = 0; i < pn->asiz; i++) { plNodeDelete(pn->amap[i]); }
		_plfree(pn->amap);
	}
	_plfree(node);
}

int
plNodeAdd(void* bag, void* node) {
	plkind_t bk;
	_plnode_t* bn;
	_plnode_t* dn;

	if (bag == NULL || node == NULL) {
		return 0;
	}
	bk = plNodeGetKind(bag);
	dn = (_plnode_t*)node;
	bn = (_plnode_t*)bag;
	switch(bk) {
	case plKindDict:
		/* We add to dict only well formed keys */
		if (plNodeGetKind(node) != plKindKey || plNodeGetItem(node, 0) == NULL) { return 0; }
#ifdef PLISTLIB_NO_DUPS
		/* No duplicates, please! */
		if (plDictFind(bag, plNodeGetBytes(node), plNodeGetSize(node), plKindAny) != NULL) { return 0; }
#endif
		/* FALLTHROUGH */
	case plKindArray:
		if (bn->asiz >= bn->acap) {
			unsigned int ncap;
			unsigned int nsz;
			unsigned int osz;
			_plnode_t** nmap;

			ncap = bn->acap + (bn->acap >> 2) + 1;
			osz = bn->acap * sizeof (*bn->amap);
			nsz = ncap * sizeof (*bn->amap);
			nmap = _plrealloc(bn->amap, osz, nsz);
			if (nmap == NULL) { return 0; }
			bn->acap = ncap;
			bn->amap = nmap;
		}
		bn->amap[bn->asiz] = dn;
		bn->asiz++;
		dn->refcnt++;
		return 1;
	case plKindKey:
		if (plNodeGetItem(bag, 0) != NULL) { return 0; }
		bn->keyval = dn;
		dn->refcnt++;
		return 1;
	default:
		return 0;
	}
}

void*
plDictNew(void) {
	_plnode_t* node;

	node = _plNodeNew(plKindDict);
	return node;
}

void*
plArrayNew(void) {
	_plnode_t* node;

	node = _plNodeNew(plKindArray);
	return node;
}

void*
plDataNew(char* datum, unsigned int dlen) {
	_plnode_t* node;

	if (datum == NULL && dlen == 0) { return NULL; }
	node = _plNodeNew(plKindData);
	if (node == NULL) { return NULL; }
	node->datval = _plzalloc(dlen + 1);
	if (node->datval == NULL) {
		plNodeDelete(node);
		return NULL;
	}
	if (dlen > 0) { _plmemcpy(node->datval, datum, dlen); }
	node->datlen = dlen;
	return node;
}

void*
plKeyNew(char* key, unsigned int klen, void* datum) {
	_plnode_t* node;

	node = plDataNew(key, klen);
	if (node == NULL) { return NULL; }
	node->kind = plKindKey;
	if (datum != NULL) { (void) plNodeAdd(node, datum); }
	return node;
}

void*
plDateNew(char* datum, unsigned int dlen) {
	_plnode_t* node;

	node = plDataNew(datum, dlen);
	if (node == NULL) { return NULL; }
	node->kind = plKindDate;
	return node;
}

void*
plStringNew(char* datum, unsigned int dlen) {
	_plnode_t* node;

	node = plDataNew(datum, dlen);
	if (node == NULL) { return NULL; }
	node->kind = plKindString;
	return node;
}

unsigned int
plNodeGetSize(void* pn) {
	_plnode_t* wn;

	wn = (_plnode_t*) pn;
	switch(plNodeGetKind(pn)) {
	case plKindDict:
		/* FALLTHROUGH */
	case plKindArray:
		return wn->asiz;
	case plKindKey:
		/* FALLTHROUGH */
	case plKindData:
		/* FALLTHROUGH */
	case plKindString:
		return wn->datval != NULL ? wn->datlen : 0;
	default:
		break;
	}
	return 0;
}

char*
plNodeGetBytes(void* pn) {
	switch (plNodeGetKind(pn)) {
	case plKindKey:
		/* FALLTHROUGH */
	case plKindData:
		/* FALLTHROUGH */
	case plKindString:
		return ((_plnode_t*) pn)->datval;
	default:
		break;
	}
	return NULL;
}

void*
plNodeGetItem(void* bag, unsigned int inum) {
	_plnode_t* bn;

	if (bag == NULL) { return NULL; }
	bn = (_plnode_t*) bag;
	switch (plNodeGetKind(bag)) {
	case plKindArray:
		/* FALLTHROUGH */
	case plKindDict:
		if (inum < bn->asiz) { return bn->amap[inum]; }
		break;
	case plKindKey:
		return bn->keyval;
	default:
		break;
	}
	return NULL;
}

void*
plBoolNew(int bval) {
	_plnode_t* node;

	node = _plNodeNew(plKindBool);
	node->intval = (bval != 0);
	return node;
}

int
plBoolGet(void* pn) {
	_plnode_t* wn;

	wn = (_plnode_t*) pn;
	return (wn->intval != 0 ? 1 : 0);
}

void*
plIntegerNew(vlong val) {
	_plnode_t* node;

	node = _plNodeNew(plKindInteger);
	node->intval = val;
	return node;
}

vlong
plIntegerGet(void* pn) {
	_plnode_t* wn;

	wn = (_plnode_t*) pn;
	return wn->intval;
}

plkind_t
plNodeGetKind(void* node) {
	_plnode_t* pn;

	if(node == NULL) { return plKindAny; }
	pn = (_plnode_t*)node;
	return pn->kind;
}

void*
plDictFind(void* dict, char* key, unsigned int klen, plkind_t kind) {
	unsigned int dsz;
	unsigned int i;

	if (plNodeGetKind(dict) != plKindDict || key == NULL || klen < 1) { return NULL; }
	dsz = plNodeGetSize(dict);
	for (i = 0; i < dsz; i++) {
		void* dent;
		void* kval;

		dent = plNodeGetItem(dict, i);
		if (dent == NULL || plNodeGetKind(dent) != plKindKey || plNodeGetSize(dent) != klen) { continue; }
		if (_plmemcmp(plNodeGetBytes(dent), key, klen) != 0) { continue; }
		kval = plNodeGetItem(dent, 0);
		if (kind == plKindAny || plNodeGetKind(kval) == kind) {
			return kval;
		} else {
			break;
		}
	}
	return NULL;
}

int
_plGrowBag(TagPtr ipl, void* parn, void* bagn) {
	if (bagn == NULL) { return 0; }
	if (!_plGrowTree(ipl->tag, bagn) || !plNodeAdd(parn, bagn)) {
		plNodeDelete(bagn);
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
			wn = plArrayNew();
			if (!_plGrowBag(ipl, pn, wn)) { return 0; }
			break;
		case kTagTypeDict:
			wn = plDictNew();
			if (!_plGrowBag(ipl, pn, wn)) { return 0; }
			break;
		case kTagTypeKey:
			wn = plKeyNew(ipl->string, ipl->dataLen, NULL);
			if (!_plGrowBag(ipl, pn, wn)) { return 0; }
			break;
		case kTagTypeString:
			wn = plStringNew(ipl->string, ipl->dataLen);
			if (wn == NULL || !plNodeAdd(pn, wn)) { return 0; }
			break;
		case kTagTypeData:
			wn = plDataNew((char*)(ipl->data), ipl->dataLen);
			if (wn == NULL || !plNodeAdd(pn, wn)) { return 0; }
			break;
		case kTagTypeTrue:
			wn = plBoolNew(1);
			if (wn == NULL || !plNodeAdd(pn, wn)) { return 0; }
			break;
		case kTagTypeFalse:
			wn = plBoolNew(0);
			if (wn == NULL || !plNodeAdd(pn, wn)) { return 0; }
			break;
		case kTagTypeInteger:
			wn = plIntegerNew(ipl->intval);
			if (wn == NULL || !plNodeAdd(pn, wn)) { return 0; }
			break;
		case kTagTypeDate:
			wn = plDateNew(ipl->string, ipl->dataLen);
			if (wn == NULL || !plNodeAdd(pn, wn)) { return 0; }
			break;
		default:
			return 0;
		}
		ipl = ipl->tagNext; /* next sibling */
	}

	return 1;
}

void*
plXmlToNode(plbuf_t* ibuf) {
	TagPtr plist;
	void* pn;
	int rc;

	pn = NULL;

	rc = PListXMLParse(ibuf->dat, ibuf->len, &plist);
	if(rc == 0) {
		switch(plist->type) {
		case kTagTypeDict:
			pn = plDictNew();
			rc = _plGrowTree(plist->tag, pn);
			if (rc == 0) { plNodeDelete(pn); }
			break;
		default:
			rc = 0;
			break;
		}
		PListXMLFreeTag(plist);
	}
	PListXMLCleanup();
	return  rc == 0 ? NULL : pn;
}
