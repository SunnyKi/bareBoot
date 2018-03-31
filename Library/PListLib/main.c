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

#include <stdlib.h>
#include <stdio.h>

#include "plist.h"
#include "plist_xml_parser.h"

#define PLBUFSIZE (2 * 1024 * 1024)

char ibig[PLBUFSIZE];
char obig[PLBUFSIZE];

plbuf_t ibuf = {ibig, 0, 0 };
plbuf_t obuf = {obig, sizeof(obig), 0 };

int main(int argc, char* argv[]) {
	void* pl;
#ifdef DEBUG
	void* dp;
	void* kp;
	int rc;
#else
	FILE* ifp;

	if (argc > 1) {
		ifp = fopen(argv[1], "rb");

		if (ifp == NULL)
			return 1;
		ibuf.len = fread(ibig, 1, sizeof(ibig), ifp);
		fclose(ifp);

		if (ibuf.len < 1)
			return 2;

		pl = plXmlToNode(&ibuf);

		if (pl != NULL) {
			(void) plNodeToXml(pl, &obuf);
			fwrite(obig, 1, obuf.pos, stdout);
		}
		plNodeDelete(pl);
		obuf.pos = 0;
	}
#endif

#ifdef DEBUG
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	fflush(stdout);
	obuf.pos = 0;

	pl = plDictNew();

	dp = plDataNew("7890", 4);
	kp = plKeyNew("dkey", 4, dp);

	rc = plNodeAdd(pl, kp);

	dp = plDataNew("0123", 4);
	kp = plKeyNew("zkey", 4, dp);

	rc = plNodeAdd(pl, kp);

	(void) plNodeToXml(pl, &obuf);

	plNodeDelete(pl);

	fwrite(obig, 1, obuf.pos, stdout);
#endif
	fclose(stdout);
	return 0;
}
