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
#include <stdio.h>

#include "plist.h"

char ibig[32768];
char obig[32768];

plbuf_t ibuf = {ibig, 0, 0 };
plbuf_t obuf = {obig, sizeof(obig), 0 };

int main(int argc, char* argv[]) {
	FILE* ifp;
	void* pl;
	void* ap;

	if (argc > 1) {
		ifp = fopen(argv[1], "rb");
		if (ifp == NULL)
			return 1;
		ibuf.len = fread(ibig, 1, sizeof(ibig), ifp);
		fclose(ifp);
		if (ibuf.len < 1)
			return 2;
		pl = plFromXml(&ibuf);
		if (pl != NULL) {
			(void) plToXml(pl, &obuf);
			fwrite(obig, 1, obuf.pos, stdout);
		}
		plDeleteNode(pl);
	}
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	fflush(stdout);
	obuf.pos = 0;

	pl = plNewDict();

	plAdd(pl, plNewKey("dkey", 4, plNewData("7890", 4)));

	ap = plNewArray();
	plAdd(ap, plNewInteger(4));
	plAdd(ap, plNewInteger(5));
	plAdd(ap, plNewBool(0));
	plAdd(ap, plNewBool(1));
	plAdd(ap, plNewString("abcdef", 6));

	plAdd(pl, plNewKey("akey", 4, ap));

	(void) plToXml(pl, &obuf);

	plDeleteNode(pl);

	fwrite(obig, 1, obuf.pos, stdout);
	fflush(stdout);
	return 0;
}
