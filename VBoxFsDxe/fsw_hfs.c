/* $Id: fsw_hfs.c $ */

/** @file
 * fsw_hfs.c - HFS file system driver code, see
 *
 *   http://developer.apple.com/technotes/tn/tn1150.html
 *
 * Current limitations:
 *  - Doesn't support permissions
 *  - Complete Unicode case-insensitiveness disabled (large tables)
 *  - No links
 *  - Only supports pure HFS+ (i.e. no HFS, or HFS+ embedded to HFS)
 */

/*
 * Copyright (C) 2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "fsw_hfs.h"

#ifdef HOST_POSIX
#define DPRINT(x) printf(x)
#define DPRINT2(x,y) printf(x,y)
#endif

#ifdef HOST_MSWIN
#define DPRINT(x) printf(x)
#define DPRINT2(x,y) printf(x,y)
#endif

#ifndef DPRINT
#define CONCAT(x,y) x##y
#define DPRINT(x) Print(CONCAT(L,x))
#define DPRINT2(x,y) Print(CONCAT(L,x), y)
#endif

// functions

static fsw_status_t fsw_hfs_volume_mount (
  struct fsw_hfs_volume *vol
);
static void fsw_hfs_volume_free (
  struct fsw_hfs_volume *vol
);
static fsw_status_t fsw_hfs_volume_stat (
  struct fsw_hfs_volume *vol,
  struct fsw_volume_stat *sb
);

static fsw_status_t fsw_hfs_dnode_fill (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno
);
static void fsw_hfs_dnode_free (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno
);
static fsw_status_t fsw_hfs_dnode_stat (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_dnode_stat *sb
);
static fsw_status_t fsw_hfs_get_extent (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_extent *extent
);

static fsw_status_t fsw_hfs_dir_lookup (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_string *lookup_name,
  struct fsw_hfs_dnode **child_dno
);
static fsw_status_t fsw_hfs_dir_read (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_shandle *shand,
  struct fsw_hfs_dnode **child_dno
);

#if 0
static fsw_status_t fsw_hfs_read_dirrec (
  struct fsw_shandle *shand,
  struct hfs_dirrec_buffer *dirrec_buffer
);
#endif

static fsw_status_t fsw_hfs_readlink (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_string *link
);

//
// Dispatch Table
//

struct fsw_fstype_table FSW_FSTYPE_TABLE_NAME (
  hfs
) = {
  { FSW_STRING_TYPE_ISO88591, 4, 4, "hfs"},
  sizeof (struct fsw_hfs_volume),
  sizeof (struct fsw_hfs_dnode),
  fsw_hfs_volume_mount,
  fsw_hfs_volume_free,
  fsw_hfs_volume_stat,
  fsw_hfs_dnode_fill,
  fsw_hfs_dnode_free,
  fsw_hfs_dnode_stat,
  fsw_hfs_get_extent,
  fsw_hfs_dir_lookup,
  fsw_hfs_dir_read,
  fsw_hfs_readlink
};

static fsw_s32
fsw_hfs_read_block (
  struct fsw_hfs_dnode *dno,
  fsw_u32 log_bno,
  fsw_u32 off,
  fsw_s32 len,
  fsw_u8 * buf
)
{
  fsw_status_t status;
  struct fsw_extent extent;
  fsw_u32 phys_bno;
  fsw_u8 *buffer;

  extent.log_start = log_bno;
  status = fsw_hfs_get_extent (dno->g.vol, dno, &extent);
  if (status)
    return status;

  phys_bno = extent.phys_start;
  status = fsw_block_get (dno->g.vol, phys_bno, 0, (void **) &buffer);
  if (status)
    return status;

  fsw_memcpy (buf, buffer + off, len);

  fsw_block_release (dno->g.vol, phys_bno, buffer);

  return FSW_SUCCESS;
}

/* Read data from HFS file. */
static fsw_s32
fsw_hfs_read_file (
  struct fsw_hfs_dnode *dno,
  fsw_u64 pos,
  fsw_s32 len,
  fsw_u8 * buf
)
{
  fsw_status_t status;
  fsw_u32 log_bno;
  fsw_u32 block_size_bits = dno->g.vol->block_size_shift;
  fsw_u32 block_size = (1 << block_size_bits);
  fsw_u32 block_size_mask = block_size - 1;
  fsw_s32 read = 0;

  while (len > 0) {
    fsw_u32 off = (fsw_u32) (pos & block_size_mask);
    fsw_s32 next_len = len;

    log_bno = (fsw_u32) RShiftU64 (pos, block_size_bits);

    if (next_len >= 0 && (fsw_u32) next_len > block_size)
      next_len = block_size;
    status = fsw_hfs_read_block (dno, log_bno, off, next_len, buf);
    if (status)
      return -1;
    buf += next_len;
    pos += next_len;
    len -= next_len;
    read += next_len;
  }

  return read;
}

static fsw_s32
fsw_hfs_compute_shift (
  fsw_u32 size
)
{
  fsw_s32 i;

  for (i = 0; i < 32; i++) {
    if ((size >> i) == 0)
      return i - 1;
  }

  return 0;
}

/**
 * Mount an HFS+ volume. Reads the superblock and constructs the
 * root directory dnode.
 */

static fsw_status_t
fsw_hfs_volume_mount (
  struct fsw_hfs_volume *vol
)
{
  fsw_status_t status, rv;
  void *buffer = NULL;
  HFSPlusVolumeHeader *voldesc;
  fsw_u32 blockno;
  struct fsw_string s;

  rv = FSW_UNSUPPORTED;

  vol->primary_voldesc = NULL;
  fsw_set_blocksize (vol, HFS_BLOCKSIZE, HFS_BLOCKSIZE);
  blockno = HFS_SUPERBLOCK_BLOCKNO;

#define CHECK(s)         \
        if (status)  {   \
            rv = status; \
            break;       \
        }

  vol->emb_block_off = 0;
  vol->hfs_kind = 0;
  do {
    fsw_u16 signature;
    BTHeaderRec tree_header;
    fsw_s32 r;
    fsw_u32 block_size;

    status = fsw_block_get (vol, blockno, 0, &buffer);
    CHECK (status);
    voldesc = (HFSPlusVolumeHeader *) buffer;
    signature = be16_to_cpu (voldesc->signature);

    if ((signature == kHFSPlusSigWord) || (signature == kHFSXSigWord)) {
      if (vol->hfs_kind == 0) {
        FSW_MSG_DEBUGV ((FSW_MSGSTR ("fsw_hfs: Found HFS+\n")));
        vol->hfs_kind = FSW_HFS_PLUS;
      }
    }
    else if (signature == kHFSSigWord) {
      HFSMasterDirectoryBlock *mdb = (HFSMasterDirectoryBlock *) buffer;

      if (be16_to_cpu (mdb->drEmbedSigWord) == kHFSPlusSigWord) {
        FSW_MSG_DEBUGV ((FSW_MSGSTR
                         ("fsw_hfs: Found HFS+ inside HFS, untested\n")));
        vol->hfs_kind = FSW_HFS_PLUS_EMB;
        vol->emb_block_off = be32_to_cpu (mdb->drEmbedExtent.startBlock);
        fsw_block_release (vol, blockno, buffer);
        blockno += vol->emb_block_off;
        /* retry */
        continue;
      }
      else {
        FSW_MSG_DEBUGV ((FSW_MSGSTR
                         ("fsw_hfs: Found plain HFS, unsupported\n")));
        vol->hfs_kind = FSW_HFS_PLAIN;
      }
      rv = FSW_UNSUPPORTED;
      break;
    }
    else {
      rv = FSW_UNSUPPORTED;
      break;
    }

    status =
      fsw_memdup ((void **) &vol->primary_voldesc, voldesc, sizeof (*voldesc));
    CHECK (status);

    block_size = be32_to_cpu (voldesc->blockSize);
    vol->block_size_shift = fsw_hfs_compute_shift (block_size);

    fsw_block_release (vol, blockno, buffer);
    buffer = NULL;
    voldesc = NULL;
    fsw_set_blocksize (vol, block_size, block_size);

    /* Set default volume name */
    s.type = FSW_STRING_TYPE_ISO88591;
    s.size = s.len = kHFSMaxVolumeNameChars;
    s.data = "HFS+ volume";
    status = fsw_strdup_coerce (&vol->g.label, vol->g.host_string_type, &s);
    CHECK (status);

    /* Setup catalog dnode */
    status =
      fsw_dnode_create_root (vol, kHFSCatalogFileID, &vol->catalog_tree.file);
    CHECK (status);
    fsw_memcpy (vol->catalog_tree.file->extents,
                vol->primary_voldesc->catalogFile.extents,
                sizeof vol->catalog_tree.file->extents);
    vol->catalog_tree.file->g.size =
      be64_to_cpu (vol->primary_voldesc->catalogFile.logicalSize);

    /* Setup extents overflow file */
    status =
      fsw_dnode_create_root (vol, kHFSExtentsFileID, &vol->extents_tree.file);
    CHECK (status);
    fsw_memcpy (vol->extents_tree.file->extents,
                vol->primary_voldesc->extentsFile.extents,
                sizeof vol->extents_tree.file->extents);
    vol->extents_tree.file->g.size =
      be64_to_cpu (vol->primary_voldesc->extentsFile.logicalSize);

    /* Setup the root dnode */
    status = fsw_dnode_create_root (vol, kHFSRootFolderID, &vol->g.root);
    CHECK (status);

    /*
     * Read catalog file, we know that first record is in the first node, right after
     * the node descriptor.
     */
    r =
      fsw_hfs_read_file (vol->catalog_tree.file, sizeof (BTNodeDescriptor),
                         sizeof (BTHeaderRec), (fsw_u8 *) & tree_header);
    if (r != sizeof (BTHeaderRec)) {
      rv = FSW_VOLUME_CORRUPTED;
      break;
    }
    vol->case_sensitive = (signature == kHFSXSigWord) &&
      (tree_header.keyCompareType == kHFSBinaryCompare);
    vol->catalog_tree.root_node = be32_to_cpu (tree_header.rootNode);
    vol->catalog_tree.node_size = be16_to_cpu (tree_header.nodeSize);

    /* Take Volume Name before tree_header overwritten */
    {
      fsw_u32 firstLeafNum;
      fsw_u64 catfOffset;
      fsw_u8 cbuff[sizeof (BTNodeDescriptor) + sizeof (HFSPlusCatalogKey)];

      firstLeafNum = be32_to_cpu (tree_header.firstLeafNode);
      catfOffset = firstLeafNum * vol->catalog_tree.node_size;

      r =
        fsw_hfs_read_file (vol->catalog_tree.file, catfOffset, sizeof (cbuff),
                           cbuff);

      if (r == sizeof (cbuff)) {
        BTNodeDescriptor *btnd;
        HFSPlusCatalogKey *ck;

        btnd = (BTNodeDescriptor *) cbuff;
        ck = (HFSPlusCatalogKey *) (cbuff + sizeof (BTNodeDescriptor));
        if (btnd->kind == kBTLeafNode &&
            be32_to_cpu (ck->parentID) == kHFSRootParentID) {
          struct fsw_string vn;

          vn.type = FSW_STRING_TYPE_UTF16_BE;
          vn.len = be16_to_cpu (ck->nodeName.length);
          vn.size = vn.len * sizeof (fsw_u16);
          vn.data = ck->nodeName.unicode;
          fsw_strfree (&vol->g.label);
          status =
            fsw_strdup_coerce (&vol->g.label, vol->g.host_string_type, &vn);
          CHECK (status);
        }
      }
    }

    /* Read extents overflow file */
    r =
      fsw_hfs_read_file (vol->extents_tree.file, sizeof (BTNodeDescriptor),
                         sizeof (BTHeaderRec), (fsw_u8 *) & tree_header);
    if (r != sizeof (BTHeaderRec)) {
      rv = FSW_VOLUME_CORRUPTED;
      break;
    }

    vol->extents_tree.root_node = be32_to_cpu (tree_header.rootNode);
    vol->extents_tree.node_size = be16_to_cpu (tree_header.nodeSize);

    rv = FSW_SUCCESS;
  } while (0);

#undef CHECK

  if (buffer != NULL)
    fsw_block_release (vol, blockno, buffer);

  return rv;
}

/**
 * Free the volume data structure. Called by the core after an unmount or after
 * an unsuccessful mount to release the memory used by the file system type specific
 * part of the volume structure.
 */

static void
fsw_hfs_volume_free (
  struct fsw_hfs_volume *vol
)
{
  if (vol->primary_voldesc) {
    fsw_free (vol->primary_voldesc);
    vol->primary_voldesc = NULL;
  }
  if (vol->catalog_tree.file) {
    fsw_dnode_release ((struct fsw_dnode *) (vol->catalog_tree.file));
    vol->catalog_tree.file = NULL;
  }
  if (vol->extents_tree.file) {
    fsw_dnode_release ((struct fsw_dnode *) (vol->extents_tree.file));
    vol->extents_tree.file = NULL;
  }
}

/**
 * Get in-depth information on a volume.
 */

static fsw_status_t
fsw_hfs_volume_stat (
  struct fsw_hfs_volume *vol,
  struct fsw_volume_stat *sb
)
{
#ifdef HOST_EFI
  sb->total_bytes =
    FSW_U64_SHL (be32_to_cpu (vol->primary_voldesc->totalBlocks),
                 vol->block_size_shift);
  sb->free_bytes =
    FSW_U64_SHL (be32_to_cpu (vol->primary_voldesc->freeBlocks),
                 vol->block_size_shift);
#else
  sb->total_bytes =
    ((fsw_u64) be32_to_cpu (vol->primary_voldesc->totalBlocks)) << vol->block_size_shift;
  sb->free_bytes =
    ((fsw_u64) be32_to_cpu (vol->primary_voldesc->freeBlocks)) << vol->block_size_shift;
#endif
  return FSW_SUCCESS;
}

/**
 * Get full information on a dnode from disk. This function is called by the core
 * whenever it needs to access fields in the dnode structure that may not
 * be filled immediately upon creation of the dnode.
 */

static fsw_status_t
fsw_hfs_dnode_fill (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno
)
{
  return FSW_SUCCESS;
}

/**
 * Free the dnode data structure. Called by the core when deallocating a dnode
 * structure to release the memory used by the file system type specific part
 * of the dnode structure.
 */

static void
fsw_hfs_dnode_free (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno
)
{
}

static fsw_u32
mac_to_posix (
  fsw_u32 mac_time
)
{
  /* Mac time is 1904 year based */
  return mac_time ? mac_time - 2082844800 : 0;
}

/**
 * Get in-depth information on a dnode. The core makes sure that fsw_hfs_dnode_fill
 * has been called on the dnode before this function is called. Note that some
 * data is not directly stored into the structure, but passed to a host-specific
 * callback that converts it to the host-specific format.
 */

static fsw_status_t
fsw_hfs_dnode_stat (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_dnode_stat *sb
)
{
  sb->used_bytes = dno->used_bytes;
  sb->store_time_posix (sb, FSW_DNODE_STAT_CTIME, mac_to_posix (dno->ctime));
  sb->store_time_posix (sb, FSW_DNODE_STAT_MTIME, mac_to_posix (dno->mtime));
  sb->store_time_posix (sb, FSW_DNODE_STAT_ATIME, 0);
  sb->store_attr_posix (sb, 0700);

  return FSW_SUCCESS;
}

static int
fsw_hfs_find_block (
  HFSPlusExtentRecord * exts,
  fsw_u32 * lbno,
  fsw_u32 * pbno
)
{
  int i;
  fsw_u32 cur_lbno = *lbno;

  for (i = 0; i < 8; i++) {
    fsw_u32 start = be32_to_cpu ((*exts)[i].startBlock);
    fsw_u32 count = be32_to_cpu ((*exts)[i].blockCount);

    if (cur_lbno < count) {
      *pbno = start + cur_lbno;
      return 1;
    }

    cur_lbno -= count;
  }

  *lbno = cur_lbno;

  return 0;
}

/* Find record offset, numbering starts from the end */
static fsw_u32
fsw_hfs_btree_recoffset (
  struct fsw_hfs_btree *btree,
  BTNodeDescriptor * node,
  fsw_u32 index
)
{
  fsw_u8 *cnode = (fsw_u8 *) node;
  fsw_u16 *recptr;

  recptr = (fsw_u16 *) (cnode + btree->node_size - index * 2 - 2);
  return be16_to_cpu (*recptr);
}

/* Pointer to the key inside node */
static BTreeKey *
fsw_hfs_btree_rec (
  struct fsw_hfs_btree *btree,
  BTNodeDescriptor * node,
  fsw_u32 index
)
{
  fsw_u8 *cnode = (fsw_u8 *) node;
  fsw_u32 offset;

  offset = fsw_hfs_btree_recoffset (btree, node, index);
  return (BTreeKey *) (cnode + offset);
}

static fsw_u32
fsw_hfs_btree_next_node (
  BTreeKey *currkey
)
{
        fsw_u32 *pointer;

        pointer = (fsw_u32 *) ((char *) currkey + be16_to_cpu (currkey->length16) + 2);
        return be32_to_cpu (*pointer);
}

static fsw_status_t
fsw_hfs_btree_search (
  struct fsw_hfs_btree *btree,
  BTreeKey * key,
  int (*compare_keys) (BTreeKey * key1,
                       BTreeKey * key2),
  BTNodeDescriptor ** result,
  fsw_u32 * key_offset
)
{
  fsw_status_t status;
  fsw_u8 *buffer = NULL;
  BTNodeDescriptor *node;
  fsw_u32 currnode;
  fsw_u32 recnum;
#ifdef VBOXHFS_BTREE_BINSEARCH
  fsw_u32 lower, upper;
#endif

  currnode = btree->root_node;
  status = fsw_alloc (btree->node_size, &buffer);
  if (status) {
    if (buffer != NULL)
      fsw_free(buffer);
    return status;
  }
  node = (BTNodeDescriptor *) buffer;

  for (;;) {
    fsw_s32 cmp = 0;
    int match;
    fsw_u32 count;
    BTreeKey *currkey;

    match = 0;
    /* Read a node */
    if ((fsw_u32) fsw_hfs_read_file
        (btree->file, (fsw_u64) currnode * btree->node_size, btree->node_size,
         buffer) != btree->node_size) {
      status = FSW_VOLUME_CORRUPTED;
      break;
    }

    if (be16_to_cpu (*(fsw_u16 *) (buffer + btree->node_size - 2)) !=
        sizeof (BTNodeDescriptor)) {
      status = FSW_VOLUME_CORRUPTED;
      break;
    }

    count = be16_to_cpu (node->numRecords);

#ifndef VBOXHFS_BTREE_BINSEARCH
    for (recnum = 0; recnum < count; recnum++) {

      currkey = fsw_hfs_btree_rec (btree, node, recnum);
      cmp = compare_keys (currkey, key);
#if 0
      FSW_MSG_DEBUGV ((FSW_MSGSTR (__FUNCTION__ ": currnode %d recnum=%d/%d cmp=%d kind=%d\n"),
			      currnode, recnum, count, cmp, node->kind));
#endif

      /* Leaf node */
      if (node->kind == kBTLeafNode) {
        if (cmp == 0) {
          /* Found! */
          *result = node;
          *key_offset = recnum;

          return FSW_SUCCESS;
        }
      }
      else if (node->kind == kBTIndexNode) {
        if (cmp > 0)
          break;
        currnode = fsw_hfs_btree_next_node (currkey);
#if 0
      FSW_MSG_DEBUGV ((FSW_MSGSTR (__FUNCTION__ ": candidate for the next currnode is %d\n"), currnode));
#endif
        match = 1;
      }
    }

    if (node->kind == kBTLeafNode && cmp < 0 && node->fLink) {
      currnode = be32_to_cpu (node->fLink);
      continue;
    }
    else if (!match) {
      status = FSW_NOT_FOUND;
      break;
    }
#else
    /* Perform binary search */
    lower = 0;
    upper = count - 1;
    currkey = NULL;

    if (count == 0) {
      status = FSW_NOT_FOUND;
      goto done;
    }

    while (lower <= upper) {
      recnum = (lower + upper) / 2;

      currkey = fsw_hfs_btree_rec (btree, node, recnum);

      cmp = 0 - compare_keys (currkey, key);
#if 0
      FSW_MSG_DEBUGV ((FSW_MSGSTR (__FUNCTION__ ": currnode %d lower/recnum/upper %d/%d/%d (%d) cmp=%d kind=%d\n"),
			      currnode, lower, recnum, upper, count, cmp, node->kind));
#endif
      if (cmp < 0)
        upper = recnum - 1;
      if (cmp > 0)
        lower = recnum + 1;
      if (cmp == 0) {
        /* Found! */
        *result = node;
        *key_offset = recnum;

        return FSW_SUCCESS;
      }
    }

    if (cmp < 0)
      currkey = fsw_hfs_btree_rec (btree, node, upper);

    if (node->kind == kBTIndexNode && currkey != NULL) {
      currnode = fsw_hfs_btree_next_node (currkey);
#if 0
      FSW_MSG_DEBUGV ((FSW_MSGSTR (__FUNCTION__ ": candidate for the next currnode is %d\n"), currnode));
#endif
    }
    else {
      status = FSW_NOT_FOUND;
      break;
    }
#endif
  }

#ifdef VBOXHFS_BTREE_BINSEARCH
done:
#endif
  if (buffer != NULL && status != FSW_SUCCESS)
    fsw_free (buffer);

  return status;
}

typedef struct {
  fsw_u32 id;
  fsw_u32 type;
  struct fsw_string *name;
  fsw_u64 size;
  fsw_u64 used;
  fsw_u32 ctime;
  fsw_u32 mtime;
  HFSPlusExtentRecord extents;
} file_info_t;

typedef struct {
  fsw_u32 cur_pos;              /* current position */
  fsw_u32 parent;
  struct fsw_hfs_volume *vol;

  struct fsw_shandle *shandle;  /* this one track iterator's state */
  file_info_t file_info;
} visitor_parameter_t;

static int
fsw_hfs_btree_visit_node (
  BTreeKey * record,
  void *param
)
{
  visitor_parameter_t *vp = (visitor_parameter_t *) param;
  fsw_u8 *base = (fsw_u8 *) record->rawData + be16_to_cpu (record->length16) + 2;
  fsw_u16 rec_type = be16_to_cpu (*(fsw_u16 *) base);
  struct HFSPlusCatalogKey *cat_key = (HFSPlusCatalogKey *) record;
  fsw_u16 name_len;
  fsw_u16 *name_ptr;
  fsw_u32 i;
  struct fsw_string *file_name;

  if (be32_to_cpu (cat_key->parentID) != vp->parent)
    return -1;

  /* not smth we care about */
  if (vp->shandle->pos != vp->cur_pos++)
    return 0;

  switch (rec_type) {
  case kHFSPlusFolderRecord:
    {
      HFSPlusCatalogFolder *folder_info = (HFSPlusCatalogFolder *) base;

      vp->file_info.id = be32_to_cpu (folder_info->folderID);
      vp->file_info.type = FSW_DNODE_TYPE_DIR;
      vp->file_info.size = be32_to_cpu (folder_info->valence);
      vp->file_info.used = be32_to_cpu (folder_info->valence);
      vp->file_info.ctime = be32_to_cpu (folder_info->createDate);
      vp->file_info.mtime = be32_to_cpu (folder_info->contentModDate);
      break;
    }
  case kHFSPlusFileRecord:
    {
      HFSPlusCatalogFile *file_info = (HFSPlusCatalogFile *) base;

      vp->file_info.id = be32_to_cpu (file_info->fileID);
      vp->file_info.type = FSW_DNODE_TYPE_FILE;
      vp->file_info.size = be64_to_cpu (file_info->dataFork.logicalSize);
      vp->file_info.used =
        LShiftU64 (be32_to_cpu (file_info->dataFork.totalBlocks),
                   vp->vol->block_size_shift);
      vp->file_info.ctime = be32_to_cpu (file_info->createDate);
      vp->file_info.mtime = be32_to_cpu (file_info->contentModDate);
      fsw_memcpy (&vp->file_info.extents, &file_info->dataFork.extents,
                  sizeof vp->file_info.extents);
      break;
    }
  case kHFSPlusFolderThreadRecord:
  case kHFSPlusFileThreadRecord:
    {
      vp->shandle->pos++;
      return 0;
    }
  default:
    vp->file_info.type = FSW_DNODE_TYPE_UNKNOWN;
    break;
  }

  name_len = be16_to_cpu (cat_key->nodeName.length);

  file_name = vp->file_info.name;
  file_name->len = name_len;
  fsw_memdup (&file_name->data, &cat_key->nodeName.unicode[0], 2 * name_len);
  file_name->size = 2 * name_len;
  file_name->type = FSW_STRING_TYPE_UTF16;
  name_ptr = (fsw_u16 *) file_name->data;
  for (i = 0; i < name_len; i++) {
    name_ptr[i] = be16_to_cpu (name_ptr[i]);
  }
  vp->shandle->pos++;

  return 1;
}

static fsw_status_t
fsw_hfs_btree_iterate_node (
  struct fsw_hfs_btree *btree,
  BTNodeDescriptor * first_node,
  fsw_u32 first_rec,
  int (*callback) (BTreeKey * record,
                   void *param),
  void *param
)
{
  fsw_status_t status;

  /* We modify node, so make a copy */
  BTNodeDescriptor *node = first_node;
  fsw_u8 *buffer = NULL;

  status = fsw_alloc (btree->node_size, &buffer);
  if (status) {
    if (buffer != NULL)
      fsw_free(buffer);
    return status;
  }

  for (;;) {
    fsw_u32 i;
    fsw_u32 count = be16_to_cpu (node->numRecords);
    fsw_u32 next_node;

    /* Iterate over all records in this node */
    for (i = first_rec; i < count; i++) {
      int rv = callback (fsw_hfs_btree_rec (btree, node, i), param);

      switch (rv) {
      case 1:
        status = FSW_SUCCESS;
        goto done;
      case -1:
        status = FSW_NOT_FOUND;
        goto done;
      }
      /* if callback returned 0 - continue */
    }

    next_node = be32_to_cpu (node->fLink);

    if (!next_node) {
      status = FSW_NOT_FOUND;
      break;
    }

    if ((fsw_u32) fsw_hfs_read_file
        (btree->file, next_node * btree->node_size, btree->node_size,
         buffer) != btree->node_size) {
      status = FSW_VOLUME_CORRUPTED;
      break;
    }

    node = (BTNodeDescriptor *) buffer;
    first_rec = 0;
  }
done:
  if (buffer)
    fsw_free (buffer);

  return status;
}

static int
fsw_hfs_cmp_extkey (
  BTreeKey * key1,
  BTreeKey * key2
)
{
  HFSPlusExtentKey *ekey1 = (HFSPlusExtentKey *) key1;
  HFSPlusExtentKey *ekey2 = (HFSPlusExtentKey *) key2;
  int result;

  /* First key is read from the FS data, second is in-memory in CPU endianess */
  result = be32_to_cpu (ekey1->fileID) - ekey2->fileID;

  if (result)
    return result;

  result = ekey1->forkType - ekey2->forkType;

  if (result)
    return result;

  result = be32_to_cpu (ekey1->startBlock) - ekey2->startBlock;
  return result;
}

static int
fsw_hfs_cmp_catkey (
  BTreeKey * key1,
  BTreeKey * key2
)
{
  HFSPlusCatalogKey *ckey1 = (HFSPlusCatalogKey *) key1;
  HFSPlusCatalogKey *ckey2 = (HFSPlusCatalogKey *) key2;

  int apos, bpos, lc;
  fsw_u16 ac, bc;
  fsw_u32 parentId1;
  int key1Len;
  fsw_u16 *p1;
  fsw_u16 *p2;

  parentId1 = be32_to_cpu (ckey1->parentID);

  if (parentId1 > ckey2->parentID)
    return 1;
  if (parentId1 < ckey2->parentID)
    return -1;

  p1 = &ckey1->nodeName.unicode[0];
  p2 = &ckey2->nodeName.unicode[0];
  key1Len = be16_to_cpu (ckey1->nodeName.length);
  apos = bpos = 0;

  for (;;) {
    /* get next valid character from ckey1 */
    for (lc = 0; lc == 0 && apos < key1Len; apos++) {
      ac = be16_to_cpu (p1[apos]);
      lc = ac;
    };
    ac = (fsw_u16) lc;

    /* get next valid character from ckey2 */
    for (lc = 0; lc == 0 && bpos < ckey2->nodeName.length; bpos++) {
      bc = p2[bpos];
      lc = bc;
    };
    bc = (fsw_u16) lc;

    if (ac != bc || (ac == 0 && bc == 0))
      return ac - bc;
  }
}

static int
fsw_hfs_cmpi_catkey (
  BTreeKey * key1,
  BTreeKey * key2
)
{
  HFSPlusCatalogKey *ckey1 = (HFSPlusCatalogKey *) key1;
  HFSPlusCatalogKey *ckey2 = (HFSPlusCatalogKey *) key2;

  int apos, bpos, lc;
  fsw_u16 ac, bc;
  fsw_u32 parentId1;
  int key1Len;
  fsw_u16 *p1;
  fsw_u16 *p2;

  parentId1 = be32_to_cpu (ckey1->parentID);

  if (parentId1 > ckey2->parentID)
    return 1;
  if (parentId1 < ckey2->parentID)
    return -1;

  key1Len = be16_to_cpu (ckey1->nodeName.length);

  if (key1Len == 0 && ckey2->nodeName.length == 0)
    return 0;

  p1 = &ckey1->nodeName.unicode[0];
  p2 = &ckey2->nodeName.unicode[0];

  apos = bpos = 0;

  for (;;) {
    /* get next valid character from ckey1 */
    for (lc = 0; lc == 0 && apos < key1Len; apos++) {
      ac = be16_to_cpu (p1[apos]);
      lc = fsw_to_lower (ac);
    }
    ac = (fsw_u16) lc;

    /* get next valid character from ckey2 */
    for (lc = 0; lc == 0 && bpos < ckey2->nodeName.length; bpos++) {
      bc = p2[bpos];
      lc = fsw_to_lower (bc);
    }
    bc = (fsw_u16) lc;

    if (ac != bc || (ac == 0 && bc == 0))
      return ac - bc;
  }
}

/**
 * Retrieve file data mapping information. This function is called by the core when
 * fsw_shandle_read needs to know where on the disk the required piece of the file's
 * data can be found. The core makes sure that fsw_hfs_dnode_fill has been called
 * on the dnode before. Our task here is to get the physical disk block number for
 * the requested logical block number.
 */

static fsw_status_t
fsw_hfs_get_extent (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_extent *extent
)
{
  fsw_status_t status;
  fsw_u32 lbno;
  HFSPlusExtentRecord *exts;
  BTNodeDescriptor *node = NULL;

  extent->type = FSW_EXTENT_TYPE_PHYSBLOCK;
  extent->log_count = 1;
  lbno = extent->log_start;

  /* we only care about data forks atm, do we? */
  exts = &dno->extents;

  for (;;) {
    struct HFSPlusExtentKey *key;
    struct HFSPlusExtentKey overflowkey;
    fsw_u32 ptr;
    fsw_u32 phys_bno;

    if (fsw_hfs_find_block (exts, &lbno, &phys_bno)) {
      extent->phys_start = phys_bno + vol->emb_block_off;
      status = FSW_SUCCESS;
      break;
    }

    /* Find appropriate overflow record */
    overflowkey.fileID = dno->g.dnode_id;
    overflowkey.startBlock = extent->log_start - lbno;

    if (node != NULL) {
      fsw_free (node);
      node = NULL;
    }

    status =
      fsw_hfs_btree_search (&vol->extents_tree, (BTreeKey *) & overflowkey,
                            fsw_hfs_cmp_extkey, &node, &ptr);
    if (status)
      break;

    key = (struct HFSPlusExtentKey *)
      fsw_hfs_btree_rec (&vol->extents_tree, node, ptr);
    exts = (HFSPlusExtentRecord *) (key + 1);
  }

  if (node != NULL)
    fsw_free (node);

  return status;
}

static fsw_status_t
create_hfs_dnode (
  struct fsw_hfs_dnode *dno,
  file_info_t * file_info,
  struct fsw_hfs_dnode **child_dno_out
)
{
  fsw_status_t status;
  struct fsw_hfs_dnode *baby;

  status =
    fsw_dnode_create (dno, file_info->id, file_info->type, file_info->name,
                      &baby);
  if (status)
    return status;

  baby->g.size = file_info->size;
  baby->used_bytes = file_info->used;
  baby->ctime = file_info->ctime;
  baby->mtime = file_info->mtime;

  /* Fill-in extents info */
  if (file_info->type == FSW_DNODE_TYPE_FILE) {
    fsw_memcpy (baby->extents, &file_info->extents, sizeof file_info->extents);
  }

  *child_dno_out = baby;

  return FSW_SUCCESS;
}

/**
 * Lookup a directory's child dnode by name. This function is called on a directory
 * to retrieve the directory entry with the given name. A dnode is constructed for
 * this entry and returned. The core makes sure that fsw_hfs_dnode_fill has been called
 * and the dnode is actually a directory.
 */

static fsw_status_t
fsw_hfs_dir_lookup (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_string *lookup_name,
  struct fsw_hfs_dnode **child_dno_out
)
{
  fsw_status_t status;
  struct HFSPlusCatalogKey catkey;
  fsw_u32 ptr;
  fsw_u16 rec_type;
  BTNodeDescriptor *node = NULL;
  struct fsw_string rec_name;
  int free_data = 0;
  HFSPlusCatalogKey *file_key;
  file_info_t file_info;
  fsw_u8 *base;

  fsw_memzero (&file_info, sizeof file_info);
  file_info.name = &rec_name;

  catkey.parentID = dno->g.dnode_id;
  catkey.nodeName.length = (fsw_u16) lookup_name->len;

  /* no need to allocate anything */
  if (lookup_name->type == FSW_STRING_TYPE_UTF16) {
    fsw_memcpy (catkey.nodeName.unicode, lookup_name->data, lookup_name->size);
    rec_name = *lookup_name;
  }
  else {
    status = fsw_strdup_coerce (&rec_name, FSW_STRING_TYPE_UTF16, lookup_name);
    /* nothing allocated so far */
    if (status)
      goto done;
    free_data = 1;
    fsw_memcpy (catkey.nodeName.unicode, rec_name.data, rec_name.size);
  }

  catkey.keyLength = (fsw_u16) (6 + rec_name.size);

  status =
    fsw_hfs_btree_search (&vol->catalog_tree, (BTreeKey *) & catkey,
                          vol->
                          case_sensitive ? fsw_hfs_cmp_catkey :
                          fsw_hfs_cmpi_catkey, &node, &ptr);
  if (status)
    goto done;

  file_key =
    (HFSPlusCatalogKey *) fsw_hfs_btree_rec (&vol->catalog_tree, node, ptr);
  /* for plain HFS "-(keySize & 1)" would be needed */
  base = (fsw_u8 *) file_key + be16_to_cpu (file_key->keyLength) + 2;
  rec_type = be16_to_cpu (*(fsw_u16 *) base);

    /** @todo: read additional info */
  switch (rec_type) {
  case kHFSPlusFolderRecord:
    {
      HFSPlusCatalogFolder *info = (HFSPlusCatalogFolder *) base;

      file_info.id = be32_to_cpu (info->folderID);
      file_info.type = FSW_DNODE_TYPE_DIR;
      /* @todo: return number of elements, maybe use smth else */
      file_info.size = be32_to_cpu (info->valence);
      file_info.used = be32_to_cpu (info->valence);
      file_info.ctime = be32_to_cpu (info->createDate);
      file_info.mtime = be32_to_cpu (info->contentModDate);
      break;
    }
  case kHFSPlusFileRecord:
    {
      HFSPlusCatalogFile *info = (HFSPlusCatalogFile *) base;

      file_info.id = be32_to_cpu (info->fileID);
      file_info.type = FSW_DNODE_TYPE_FILE;
      file_info.size = be64_to_cpu (info->dataFork.logicalSize);
      file_info.used =
        LShiftU64 (be32_to_cpu (info->dataFork.totalBlocks),
                   vol->block_size_shift);
      file_info.ctime = be32_to_cpu (info->createDate);
      file_info.mtime = be32_to_cpu (info->contentModDate);
      fsw_memcpy (&file_info.extents, &info->dataFork.extents,
                  sizeof file_info.extents);
      break;
    }
  default:
    file_info.type = FSW_DNODE_TYPE_UNKNOWN;

    break;
  }

  status = create_hfs_dnode (dno, &file_info, child_dno_out);
  if (status)
    goto done;

done:

  if (node != NULL)
    fsw_free (node);

  if (free_data)
    fsw_strfree (&rec_name);

  return status;
}

/**
 * Get the next directory entry when reading a directory. This function is called during
 * directory iteration to retrieve the next directory entry. A dnode is constructed for
 * the entry and returned. The core makes sure that fsw_hfs_dnode_fill has been called
 * and the dnode is actually a directory. The shandle provided by the caller is used to
 * record the position in the directory between calls.
 */

static fsw_status_t
fsw_hfs_dir_read (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_shandle *shand,
  struct fsw_hfs_dnode **child_dno_out
)
{
  fsw_status_t status;
  struct HFSPlusCatalogKey catkey;
  fsw_u32 ptr;
  BTNodeDescriptor *node = NULL;

  visitor_parameter_t param;
  struct fsw_string rec_name;

  catkey.parentID = dno->g.dnode_id;
  catkey.nodeName.length = 0;

  fsw_memzero (&param, sizeof (param));

  rec_name.type = FSW_STRING_TYPE_EMPTY;
  param.file_info.name = &rec_name;

  status =
    fsw_hfs_btree_search (&vol->catalog_tree, (BTreeKey *) & catkey,
                          vol->
                          case_sensitive ? fsw_hfs_cmp_catkey :
                          fsw_hfs_cmpi_catkey, &node, &ptr);
  if (status)
    goto done;

  /* Iterator updates shand state */
  param.vol = vol;
  param.shandle = shand;
  param.parent = dno->g.dnode_id;
  param.cur_pos = 0;
  status =
    fsw_hfs_btree_iterate_node (&vol->catalog_tree, node, ptr,
                                fsw_hfs_btree_visit_node, &param);
  if (status)
    goto done;

  status = create_hfs_dnode (dno, &param.file_info, child_dno_out);

done:
  if (node)
    fsw_free (node);
  fsw_strfree (&rec_name);

  return status;
}

/**
 * Get the target path of a symbolic link. This function is called when a symbolic
 * link needs to be resolved. The core makes sure that the fsw_hfs_dnode_fill has been
 * called on the dnode and that it really is a symlink.
 *
 */
static fsw_status_t
fsw_hfs_readlink (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_string *link_target
)
{
  return FSW_UNSUPPORTED;
}

// EOF
