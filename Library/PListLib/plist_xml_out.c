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

#define PLIST_INDENT "  "

char _plXmlHdr1[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
char _plXmlHdr2[] = "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
char _plXmlHdr3[] = "<plist version=\"1.0\">\n";

char _plXmlTlr1[] = "</plist>\n";

int _plNodeToXml(void* ipl, plbuf_t* obuf, int indent);

int
_plBufHasNum(plbuf_t* ibuf, unsigned int num) {
	return (ibuf->len - ibuf->pos >= num) ? 1 : 0;
}

/* Convert plist to xml */

int
_plAppendBytes(plbuf_t* obuf, char* datum, int dlen) {
	if (dlen < 1) { return 1; }
	if (_plBufHasNum(obuf, dlen)) {
		_plmemcpy(&obuf->dat[obuf->pos], datum, dlen);
		obuf->pos += dlen;
		return 1;
	}
	return 0;
}

int
_plIndentXml(plbuf_t* obuf, int indent) {
	int i;

	for (i = 0; i < indent; i++) {
		if (!_plAppendBytes(obuf, PLIST_INDENT, sizeof(PLIST_INDENT) - 1)) {
			return 0;
		}
	}
	return 1;
}

int
_plNodeTagToXml(plbuf_t* obuf, int indent, char* name, int nmlen, int nl, int tlr) {
	if (!_plIndentXml(obuf, indent)) { return 0; }
	if (!_plAppendBytes(obuf, "<", 1)) { return 0; }
	if (tlr && !_plAppendBytes(obuf, "/", 1)) { return 0; }
	if (!_plAppendBytes(obuf, name, nmlen)) { return 0; }
	if (!_plAppendBytes(obuf, ">", 1)) { return 0; }
	if (nl && !_plAppendBytes(obuf, "\n", 1)) { return 0; }
	return 1;
}

int
_plEmptyNodeTagToXml(plbuf_t* obuf, int indent, char* name, int nmlen, int nl) {
	if (!_plIndentXml(obuf, indent)) { return 0; }
	if (!_plAppendBytes(obuf, "<", 1)) { return 0; }
	if (!_plAppendBytes(obuf, name, nmlen)) { return 0; }
	if (!_plAppendBytes(obuf, "/>", 2)) { return 0; }
	if (nl && !_plAppendBytes(obuf, "\n", 1)) { return 0; }
	return 1;
}

int
_plBagToXml(void* ipl, char* name, int nlen, plbuf_t* obuf, int indent) {
	unsigned int bsize;
	unsigned int i;

	bsize = plGetSize(ipl);
	if (bsize == 0) {
		if (!_plEmptyNodeTagToXml(obuf, indent, name, nlen, 1)) { return 0; }
		return 1;
	}
	if (!_plNodeTagToXml(obuf, indent, name, nlen, 1, 0)) { return 0; }
	for (i = 0; i < bsize; i++) {
		void* spn;

		spn = plGetItem(ipl, i);
		if (spn != NULL && !_plNodeToXml(spn, obuf, indent + 1)) { return 0; }
	}
	if (!_plNodeTagToXml(obuf, indent, name, nlen, 1, 1)) { return 0; }
	return 1;
}

int
_plStrToXml(void* ipl, char* name, int nlen, plbuf_t* obuf, int indent) {
	char* bp;
	unsigned int bsize;

	bsize = plGetSize(ipl);
#if 0
	/* Parser cannot handle <string/> */
	if (bsize == 0) {
		if (!_plEmptyNodeTagToXml(obuf, indent, name, nlen, 1)) { return 0; }
		return 1;
	}
#endif
	if (!_plNodeTagToXml(obuf, indent, name, nlen, 0, 0)) { return 0; }
	if (bsize > 0) {
		bp = plGetBytes(ipl);
		if (bp == NULL || !_plAppendBytes(obuf, bp, bsize)) { return 0; }
	}
	if (!_plNodeTagToXml(obuf, 0, name, nlen, 1, 1)) { return 0; }
	return 1;
}

int
_plBytesToXml(void* ipl, char* name, int nlen, plbuf_t* obuf, int indent) {
	unsigned int bsize;
	unsigned int osize;
	char* odat;
	int rc;

	bsize = plGetSize(ipl);
#if 0
	/* Parser cannot handle <data/> */
	if (bsize == 0) {
		if (!_plEmptyNodeTagToXml(obuf, indent, name, nlen, 1)) { return 0; }
		return 1;
	}
#endif
	if (!_plNodeTagToXml(obuf, indent, name, nlen, 0, 0)) { return 0; }
	if (bsize > 0) {
		odat = _plb64encode(plGetBytes(ipl), bsize, &osize);
		if (odat == NULL) { return 0; }
		rc = _plAppendBytes(obuf, odat, osize);
		_plfree(odat);
		if (rc == 0) { return 0; }
	}
	if (!_plNodeTagToXml(obuf, 0, name, nlen, 1, 1)) { return 0; }
	return 1;
}

int
_plBoolToXml(void* ipl, plbuf_t* obuf, int indent) {
	if (plGetBool(ipl)) {
		if (!_plEmptyNodeTagToXml(obuf, indent, "true", 4, 1)) { return 0; }
	} else {
		if (!_plEmptyNodeTagToXml(obuf, indent, "false", 5, 1)) { return 0; }
	}
	return 1;
}

int
_plIntToXml(void* ipl, char* name, unsigned int nlen, plbuf_t* obuf, int indent) {
	char vbuf[32];
	int slen;

	if (!_plNodeTagToXml(obuf, indent, name, nlen, 0, 0)) { return 0; }
	slen = _plint2str(plGetIntValue(ipl), vbuf, sizeof(vbuf));
	if (!_plAppendBytes(obuf, vbuf, slen)) { return 0; }
	if (!_plNodeTagToXml(obuf, 0, name, nlen, 1, 1)) { return 0; }
	return 1;
}

int
_plDateToXml(void* ipl, char* name, unsigned int nlen, plbuf_t* obuf, int indent) {
	if (!_plNodeTagToXml(obuf, indent, name, nlen, 0, 0)) { return 0; }
	/* XXX date! */
	if (!_plNodeTagToXml(obuf, 0, name, nlen, 1, 1)) { return 0; }
	return 1;
}

int
_plNodeToXml(void* ipl, plbuf_t* obuf, int indent) {
	if (ipl == NULL) { return 1; }
	switch(plGetKind(ipl)) {
	case plKindDict:
		if (!_plBagToXml(ipl, "dict", 4, obuf, indent)) { return 0; }
		break;
	case plKindArray:
		if (!_plBagToXml(ipl, "array", 5, obuf, indent)) { return 0; }
		break;
	case plKindKey:
		if (!_plStrToXml(ipl, "key", 3, obuf, indent)) { return 0; }
   		if (!_plNodeToXml(plGetItem(ipl, 0), obuf, indent + 1)) { return 0; }
		break;
	case plKindString:
		if (!_plStrToXml(ipl, "string", 6, obuf, indent)) { return 0; }
		break;
	case plKindData:
		if (!_plBytesToXml(ipl, "data", 4, obuf, indent)) { return 0; }
		break;
	case plKindBool:
		if (!_plBoolToXml(ipl, obuf, indent)) { return 0; }
		break;
	case plKindInteger:
		if (!_plIntToXml(ipl, "integer", 7, obuf, indent)) { return 0; }
		break;
	case plKindDate:
		if (!_plDateToXml(ipl, "date", 4, obuf, indent)) { return 0; }
		break;
	default:
		return 0;
	}
	return 1;
}

int
plToXml(void* pl, plbuf_t* obuf) {
	if (!_plAppendBytes(obuf, _plXmlHdr1, sizeof(_plXmlHdr1) - 1)) { return 0; }
	if (!_plAppendBytes(obuf, _plXmlHdr2, sizeof(_plXmlHdr2) - 1)) { return 0; }
	if (!_plAppendBytes(obuf, _plXmlHdr3, sizeof(_plXmlHdr3) - 1)) { return 0; }
	if (!_plNodeToXml(pl, obuf, 1)) { return 0; }
	if (!_plAppendBytes(obuf, _plXmlTlr1, sizeof(_plXmlTlr1) - 1)) { return 0; }
	return 1;
}
