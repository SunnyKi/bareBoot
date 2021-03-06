/*
;*   Copyright (c) 2016-2019, Nikolai Saoukh. All rights reserved.
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
 * Tool to stuff boot1 code to fat32 volume.
 * WARNING! Use this tool on UNMOUNTED fs ONLY!
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
	fprintf(stderr, "Usage: %s /dev/rdiskXsY bootcodefile\n", myname);
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
	fflush(f32p);
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
	unsigned short rsecs;	/* reserved sectors count */
	unsigned short fsinum;	/* FSinfo sector number */
	int tbsectors;		/* Total boot sectors (primary + backup) */

	unsigned char* bs0 = NULL;	/* incore copy of boot sector 0 (original) */
	unsigned char* fs0 = NULL;	/* incore copy of FSinfo sector */

	int rc = 0;

	if (argc != 3) {
		usage(argv[0]);
	}

	loadbootcode(argv[2]);

	f32open(argv[1]);

	bs0 = readsectors(0, 1);

	/* little paranoia ;-) */
	if (bs0[0x42] != 0x28) {
		if (memcmp(&bs0[0x52], "FAT32   ", 8) != 0) {
			fprintf(stderr, "%s does not look like fat32fs\n", argv[1]);
			rc = 1;
			goto finita;
		}
	}

	rsecs = getword(bs0, 0x0E);	/* Number of reserved sectors */
	fsinum = getword(bs0, 0x30);	/* FSinfo sector number */

	if (fsinum != 0x0000 && fsinum != 0xFFFF) {
		fs0 = readsectors(fsinum, 1);	/* make copy for relocation */
		rsecs--;			/* one sector for FSinfo out of total */
	}

	tbsectors = 2 * bcsectors;

	if (rsecs < tbsectors) {
		fprintf(stderr, "%s: not enough reserved sectors (%d available, need %d)\n",
				argv[1], rsecs, tbsectors);
		rc = 1;
		goto finita;
	}

	patchbc(bootcode, bs0);

	if (fs0 != NULL) {
		writesectors(fs0, tbsectors, 1);
		putword(tbsectors, bootcode, 0x30);
	}

	putword(bcsectors, bootcode, 0x32);
	writesectors(bootcode, 0, bcsectors);
	writesectors(bootcode, bcsectors, bcsectors);

finita:
	f32close();

	if (bs0 != NULL) { free (bs0); }
	if (fs0 != NULL) { free (fs0); }

	return rc;
}
