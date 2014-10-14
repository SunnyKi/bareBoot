/**
 * \file tstmain.c
 * Test program for the POSIX user space environment.
 */

/*-
 * Copyright (c) 2006 Christoph Pfisterer
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsw_mswin.h"

#ifndef ITERATIONS
#define ITERATIONS 1
#endif

#if 0
extern struct fsw_fstype_table FSW_FSTYPE_TABLE_NAME(ext2);
extern struct fsw_fstype_table FSW_FSTYPE_TABLE_NAME(reiserfs);
#endif

extern struct fsw_fstype_table FSW_FSTYPE_TABLE_NAME(FSTYPE);

static struct fsw_fstype_table *fstypes[] = {
#if 0
    &FSW_FSTYPE_TABLE_NAME(ext2),
    &FSW_FSTYPE_TABLE_NAME(reiserfs),
#endif
    &FSW_FSTYPE_TABLE_NAME(FSTYPE),
    NULL
};

FILE* outfile = NULL;

static int
catfile(struct fsw_mswin_volume *vol, char *path)
{
    struct fsw_mswin_file *file;
    int r;
    char buf[4096];

    file = fsw_mswin_open(vol, path, 0, 0);
    if (file == NULL) {
        fprintf(stderr, "open(%s) call failed.\n", path);
        return 1;
    }
    while ((r = fsw_mswin_read(file, buf, sizeof(buf))) > 0)
    {
        if (outfile != NULL)
	    (void) fwrite(buf, r, 1, outfile);
    }
    fsw_mswin_close(file);

    return 0;
}

static int
viewdir(struct fsw_mswin_volume *vol, char *path, int level, int rflag, int docat)
{
    struct fsw_mswin_dir *dir;
    struct dirent *dent;
    int i;
    char subpath[4096];

    dir = fsw_mswin_opendir(vol, path);
    if (dir == NULL) {
        fprintf(stderr, "opendir(%s) call failed.\n", path);
        return 1;
    }
    while ((dent = fsw_mswin_readdir(dir)) != NULL) {
        if (outfile != NULL) {
            for (i = 0; i < level*2; i++)
                fputc(' ', outfile);
            fprintf(outfile, "0x%04x %4d %s\n", dent->d_type, dent->d_reclen, dent->d_name);
	}

        if (rflag && dent->d_type == DT_DIR) {
            _snprintf(subpath, sizeof(subpath) - 1, "%s%s/", path, dent->d_name);
            viewdir(vol, subpath, level + 1, rflag, docat);
        } else if (docat && dent->d_type == DT_REG) {
            _snprintf(subpath, sizeof(subpath) - 1, "%s%s", path, dent->d_name);
            catfile(vol, subpath);
	}
    }
    fsw_mswin_closedir(dir);

    return 0;
}

void
usage(char* pname)
{
    fprintf(stderr, "Usage: %s <file/device> l|c[r] dir|file\n", pname);
    exit(1);
}

int
main(int argc, char **argv)
{
    int lflag, cflag, rflag;
    struct fsw_mswin_volume *vol = NULL;
    int i;
    char* cp;

    if (argc != 4) {
        usage(argv[0]);
    }

    lflag = cflag = rflag = 0;

    for (cp = argv[2]; *cp != '\0'; cp++) {
	switch (*cp) {
        case 'l':
	    lflag = 1;
	    break;
        case 'c':
	    cflag = 1;
	    break;
        case 'r':
	    rflag = 1;
	    break;
        default:
	    usage(argv[0]);
	}
    }

    for (i = 0; fstypes[i] != NULL; i++) {
        vol = fsw_mswin_mount(argv[1], fstypes[i]);
        if (vol != NULL) {
            fprintf(stdout, "Mounted as '%s'.\n", (char*)(fstypes[i]->name.data));
            break;
        }
    }
    if (vol == NULL) {
        fprintf(stderr, "Mounting failed.\n");
        return 1;
    }

#if 0
    listdir(vol, "/System/Library/Extensions/udf.kext/", 0);
    listdir(vol, "/System/Library/Extensions/AppleACPIPlatform.kext/", 0);
    listdir(vol, "/System/Library/Extensions/", 0);
    catfile(vol, "/System/Library/Extensions/AppleHPET.kext/Contents/Info.plist");
#endif

    outfile = stdout;

    for (i = 0; i < ITERATIONS; i++) {
        if (lflag) {
            viewdir(vol, argv[3], 0, rflag, cflag);
        } else if (cflag) {
            catfile(vol, argv[3]);
        }
    }

    fsw_mswin_unmount(vol);

    if (outfile != NULL) {
        (void) fclose (outfile);
    }
    (void) fclose (stdout);
    (void) fclose (stderr);
    return 0;
}

// EOF
