/*
;*   Copyright (c) 2016, Nikolai Saoukh. All rights reserved.<BR>
;*
;*   This program and the accompanying materials
;*   are licensed and made available under the terms and conditions of the BSD License
;*   which accompanies this distribution.  The full text of the license may be found at
;*   http://opensource.org/licenses/bsd-license.php
;*
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

/*
 * Tool to stuff oversized boot1 code to fat32 volume.
 * Damn foolproofness is not included.
 *
 * compile as
 *     cc -o f32b1tool f32b1tool.c
 */

#include	<stdlib.h>
#include	<stdio.h>
#include	<memory.h>

void
usage(char* myname) {
	fprintf(stderr, "Usage: %s special-file-with-fat32-fs bootcodefile\n", myname);
	exit(2);
}

long
filesize(FILE* fp) {
	long cp;
	long flen;

	cp = ftell(fp);
	if (fseek(fp, 0L, SEEK_END) != 0) {
		return 0;
	}
	flen = ftell(fp);
	(void)fseek(fp, cp, SEEK_SET);
	return flen;
}

unsigned char*	bootcode;
int	bcsectors;	/* length of the bootcode in sectors */

void
loadbootcode(char* bcfilename) {
	FILE* bfp;
	long bcsize;

	if ((bfp = fopen(bcfilename, "rb")) == NULL) {
		fprintf(stderr, "Can not open bootcode file %s for reading\n", bcfilename);
		exit(1);
	}
	bcsize = filesize(bfp);
	if (bcsize < 512 || (bcsize & 511) != 0) {
		fclose(bfp);
		fprintf(stderr, "Bootcode size %ld of %s is not multilple of 512\n", bcsize, bcfilename);
		bcsectors = 0;
		exit(1);
	}
	bootcode = malloc(bcsize);
	if (fread(bootcode, bcsize, 1, bfp) != 1) {
		fclose(bfp);
		fprintf(stderr, "Can not read bootcode from %s\n", bcfilename);
		exit(1);
	}
	fclose(bfp);
	bcsectors = (int) (bcsize >> 9);
}

FILE*	f32p;
long	f32size = 0;	/* XXX: beware huge file systems */

void
f32open(char* f32name) {
	if ((f32p = fopen(f32name, "r+b")) == NULL) {
		fprintf(stderr, "Can not open fat32fs file %s for modifications\n", f32name);
		exit(1);
	}
	if (fseek(f32p, f32size, SEEK_END) != 0) {
		fclose(f32p);
		fprintf(stderr, "Can not seek to the end of %s\n", f32name);
		exit(1);
	}
	f32size = ftell(f32p);
	rewind(f32p);
}

void
f32close(void) {
	fseek(f32p, f32size, SEEK_SET);
	fclose(f32p);
}


unsigned char*
readsectors(int sn, int sc) {
	unsigned char* od;

	if (fseek(f32p, sn << 9, SEEK_SET) != 0) {
		f32close();
		fprintf(stderr, "Can not seek to %d sector\n", sn);
		exit(1);
	}

	if ((od = malloc(sc << 9)) == NULL) {
		f32close();
		fprintf(stderr, "No memory for %d sectors\n", sc);
		exit(1);
	}

	if (fread(od, 512, sc, f32p) != sc) {
		free(od);
		f32close();
		fprintf(stderr, "Can not read %d sectors (%d first)\n", sc, sn);
		exit(1);
	}
	return od;
}

void
writesectors (unsigned char* sv, int sn, int sc) {
	if (fseek(f32p, sn << 9, SEEK_SET) != 0) {
		f32close();
		fprintf(stderr, "Can not seek to %d sector\n", sn);
		exit(1);
	}

	if (fwrite(sv, 512, sc, f32p) != sc) {
		f32close();
		fprintf(stderr, "Can not write %d sectors (%d first)\n", sc, sn);
		exit(1);
	}
}

unsigned short
getword(unsigned char* bv, int fb) {
	unsigned short w;

	w = (bv[fb + 1] << 8) | bv[fb];
	return w;
}

void
putword(unsigned short w, unsigned char* bv, int fb) {
	bv[fb] = (unsigned char) w;
	bv[fb + 1] = (unsigned char) (w >> 8);
}

void
patchbc(unsigned char* bc, unsigned char* src) {
	memcpy(&bc[3], &src[3], 87);
	putword(0xAA55, bc, 510);
}

int
main(int argc, char* argv[]) {
	unsigned short bkps;	/* bootcode backup sector number */
	unsigned short fsis;	/* FSinfo sector number */
	unsigned short ressec;	/* reserved sector count */
	unsigned char* s0;
	unsigned char* fs0 = NULL;
	int minsectors;

	if (argc != 3) {
		usage(argv[0]);
	}

	loadbootcode(argv[2]);

	f32open(argv[1]);

	s0 = readsectors(0, 1);

	/* little paranoia ;-) */
	if (s0[0x42] != 0x28) {
			if (memcmp(&s0[0x52], "FAT32   ", 8) != 0) {
			fprintf(stderr, "%s does not look like fat32fs\n", argv[1]);
			f32close();
			return 1;
		}
	}

	ressec = getword(s0, 0x0E);	/* count of reserved sectors */
	fsis = getword(s0, 0x30);	/* FSinfo sector number */

	if (fsis == 0x0000 || fsis == 0xFFFF) {
		fsis = 0x0000;
	} else {
		fs0 = readsectors(fsis, 1);
		fsis = 2 * bcsectors;	/* next after bootcode backup */
	}

	minsectors = 2 * bcsectors + (fsis ? 1 : 0);
	if (ressec < minsectors) {
		fprintf(stderr, "%s: not enough reserved sectors (%d), need %d\n",
				argv[1], ressec, minsectors);
		f32close();
		return 1;
	}

	bkps = bcsectors;
	if (fsis != 0x0000) {
		bkps++;
	}

	patchbc(bootcode, s0);

	putword(fsis, bootcode, 0x30);
	putword(bkps, bootcode, 0x32);

	writesectors(bootcode, 0, bcsectors);
	writesectors(bootcode, bcsectors, bcsectors);

	if (fsis) {
		writesectors(fs0, fsis, 1);
	}

	f32close();

	if (fs0 != NULL) {
		free (fs0);
	}

	if (s0 != NULL) {
		free (s0);
	}
	return 0;
}
