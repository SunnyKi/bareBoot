/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 *  plist.c - plist parsing functions
 *
 *  Copyright (c) 2000-2005 Apple Computer, Inc.
 *
 *  DRI: Josh de Cesare
 *  code split out from drivers.c by Soren Spies, 2005
 */

//Slice - rewrite for UEFI with more functions like Copyright (c) 2003 Apple Computer

// nms -- made less environment dependent

#include "plist.h"
#include "plist_xml_parser.h"
#include "plist_helpers.h"

#define kXMLTagPList      "plist"
#define kXMLTagDict       "dict"
#define kXMLTagKey        "key"
#define kXMLTagString     "string"
#define kXMLTagInteger    "integer"
#define kXMLTagData       "data"
#define kXMLTagDate       "date"
#define kXMLTagFalse      "false/"
#define kXMLTagTrue       "true/"
#define kXMLTagArray      "array"

typedef struct _Symbol Symbol, *SymbolPtr;

struct _Symbol {
  unsigned int    refCount;
  struct _Symbol* next;
  char            string[1];
};

SymbolPtr gPListXMLSymbolsHead;
TagPtr    gPListXMLTagsFree;
void*    gPListXMLTagsArena;

static char* buffer_start = NULL;

// Forward declarations

char*      PListXMLNewSymbol (char* string);

int PListXMLFixDataMatchingTag (char* buffer, char* tag, unsigned int* lenPtr);
int PListXMLParseTagBoolean (char* buffer, TagPtr * tag, unsigned int type, unsigned int* lenPtr);
int PListXMLParseTagData (char* buffer, TagPtr * tag, unsigned int* lenPtr);
int PListXMLParseTagDate (char* buffer, TagPtr * tag, unsigned int* lenPtr);
int PListXMLParseTagInteger (char* buffer, TagPtr * tag, unsigned int* lenPtr);
int PListXMLParseTagKey (char * buffer, TagPtr * tag, unsigned int* lenPtr);
int PListXMLParseTagList (char* buffer, TagPtr * tag, unsigned int type, unsigned int empty, unsigned int* lenPtr);
int PListXMLParseTagString (char* buffer, TagPtr * tag, unsigned int* lenPtr);

SymbolPtr   PListXMLFindSymbol (char * string, SymbolPtr * prevSymbol);

TagPtr     PListXMLNewTag (void);

void        PListXMLFreeSymbol (char* string);

#if 0
typedef const struct XMLEntity {
    const char* name;
    unsigned int nameLen;
    char value;
} XMLEntity;

/* This is ugly, but better than specifying the lengths by hand */

#define _e(str,c) {str,sizeof(str)-1,c}

XMLEntity ents[] = {
    _e ("quot;", '"'),
    _e ("apos;", '\''),
    _e ("lt;",  '<'),
    _e ("gt;",  '>'),
    _e ("amp;", '&')
};

/* Function for basic XML character entities parsing */

char*
PListXMLDecode (const char* src) {
  unsigned int len;
  const char *s;
  char *out, *o;

  len = _plstrlen (src);
  out = _plzalloc (len + 1);

  if (src == NULL || len == 0 || out == NULL) {
    return NULL;
  }

  o = out;
  s = src;

  while (s <= src + len) { /* Make sure the terminator is also copied */
    if (*s == '&') {
      int entFound = FALSE;
      unsigned int i;

      s++;

      for (i = 0; i < sizeof (ents) / sizeof (XMLEntity); i++) {
        if (AsciiStrnCmp (s, ents[i].name, ents[i].nameLen) == 0) {
          entFound = TRUE;
          break;
        }
      }

      if (entFound) {
        *o++ = ents[i].value;
        s += ents[i].nameLen;
        continue;
      }
    }

    *o++ = *s++;
  }

  return out;
}
#endif

//==========================================================================
// PListXMLGetNextTag

int
PListXMLGetNextTag (unsigned char* buffer, char** tag, unsigned int* start, unsigned int* length) {
  unsigned int cnt, cnt2;

  if (tag == NULL) {
    return (-1);
  }

  // Find the start of the tag.
  cnt = 0;

  while ((buffer[cnt] != '\0') && (buffer[cnt] != '<')) {
    cnt++;
  }

  if (buffer[cnt] == '\0') {
    return (-1);
  }

  // Find the end of the tag.
  cnt2 = cnt + 1;

  while ((buffer[cnt2] != '\0') && (buffer[cnt2] != '>')) {
    cnt2++;
  }

  if (buffer[cnt2] == '\0') {
    return (-1);
  }

  // Fix the tag data.
  *tag = (char*) (buffer + cnt + 1);
  buffer[cnt2] = '\0';

  if (start) {
    *start = cnt;
  }

  *length = cnt2 + 1;
  return 0;
}

//==========================================================================
// PListXMLParseNextTag

int
PListXMLParseNextTag (char* buffer, TagPtr * tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  unsigned int    pos;
  char*    tagName;

  length = 0;
  tagName = NULL;

  *lenPtr = 0;
  Status = PListXMLGetNextTag ((unsigned char*) buffer, &tagName, 0, &length);

  if (Status != 0) {
    return Status;
  }

  pos = length;

  if (!_plstrcmp (tagName, kXMLTagPList)) {
    length = 0;
    Status = 0;
  }
  /***** dict ****/
  else if (!_plstrcmp (tagName, kXMLTagDict)) {
    Status = PListXMLParseTagList (buffer + pos, tag, kTagTypeDict, 0, &length);
  } else if (!_plstrcmp (tagName, kXMLTagDict "/")) {
    Status = PListXMLParseTagList (buffer + pos, tag, kTagTypeDict, 1, &length);
  } else if (!_plstrcmp (tagName, kXMLTagDict " ")) {
    Status = PListXMLParseTagList (buffer + pos, tag, kTagTypeDict, 0, &length);
  }
  /***** key ****/
  else if (!_plstrcmp (tagName, kXMLTagKey)) {
    Status = PListXMLParseTagKey (buffer + pos, tag, &length);
  }
  /***** string ****/
  else if (!_plstrcmp (tagName, kXMLTagString)) {
    Status = PListXMLParseTagString (buffer + pos, tag, &length);
  }
  /***** integer ****/
  else if (!_plstrcmp (tagName, kXMLTagInteger)) {
    Status = PListXMLParseTagInteger (buffer + pos, tag, &length);
  } else if (!_plstrcmp (tagName, kXMLTagInteger " ")) {
    Status = PListXMLParseTagInteger (buffer + pos, tag, &length);
  }
  /***** data ****/
  else if (!_plstrcmp (tagName, kXMLTagData)) {
    Status = PListXMLParseTagData (buffer + pos, tag, &length);
  } else if (!_plstrcmp (tagName, kXMLTagData " ")) {
    Status = PListXMLParseTagData (buffer + pos, tag, &length);
  }
  /***** date ****/
  else if (!_plstrcmp (tagName, kXMLTagDate)) {
    Status = PListXMLParseTagDate (buffer + pos, tag, &length);
  }
  /***** FALSE ****/
  else if (!_plstrcmp (tagName, kXMLTagFalse)) {
    Status = PListXMLParseTagBoolean (buffer + pos, tag, kTagTypeFalse, &length);
  }
  /***** TRUE ****/
  else if (!_plstrcmp (tagName, kXMLTagTrue)) {
    Status = PListXMLParseTagBoolean (buffer + pos, tag, kTagTypeTrue, &length);
  }
  /***** array ****/
  else if (!_plstrcmp (tagName, kXMLTagArray)) {
    Status = PListXMLParseTagList (buffer + pos, tag, kTagTypeArray, 0, &length);
  } else if (!_plstrcmp (tagName, kXMLTagArray " ")) {
    Status = PListXMLParseTagList (buffer + pos, tag, kTagTypeArray, 0, &length);
  } else if (!_plstrcmp (tagName, kXMLTagArray "/")) {
    Status = PListXMLParseTagList (buffer + pos, tag, kTagTypeArray, 1, &length);
  }
  /***** unknown ****/
  else {
    *tag = NULL;
    length = 0;
  }

  if (Status != 0) {
    return (-1);
  }

  *lenPtr = pos + length;
  return 0;
}

// Expects to see one dictionary in the XML file, the final pos will be returned
// If the pos is not equal to the strlen, then there are multiple dicts
// Puts the first dictionary it finds in the
// tag pointer and returns the end of the dic, or returns -1 if not found.
//

int
PListXMLParse (const char* buffer, unsigned int bSize, TagPtr* dict) {
  int  Status;
  unsigned int    length;
  unsigned int    pos;
  TagPtr    tag;
  char*    configBuffer;
  unsigned int    bufferSize;

  length = 0;
  pos = 0;
  tag = NULL;
  configBuffer = NULL;
  bufferSize = bSize + 1;

  if (dict == NULL) {
    return (-1);
  }

  *dict = NULL;
  configBuffer = _plzalloc (bufferSize);

  if (configBuffer == NULL) {
    return (-1);
  }

  _plmemcpy (configBuffer, buffer, bufferSize);
  buffer_start = configBuffer;

  for (;;) {
    Status = PListXMLParseNextTag (configBuffer + pos, &tag, &length);

    if (Status != 0) {
      break;
    }

    pos += length;

    if (tag == NULL) {
      continue;
    }

    if (tag->type == kTagTypeDict) {
      break;
    }
    PListXMLFreeTag (tag);
  }

  _plfree (configBuffer);

  if (Status != 0) {
    return Status;
  }

  *dict = tag;
  return 0;
}

//
// xml
//

//==========================================================================
// PListXMLGetProperty

TagPtr
PListXMLGetProperty (TagPtr dict, const char* key) {
  TagPtr tagList, tag;

  // No more shots to leg ;-)
  if (dict == NULL) {
    return NULL;
  }

  if (dict->type != kTagTypeDict) {
    return NULL;
  }

  tag = NULL;
  tagList = dict->tag;

  while (tagList != NULL) {
    tag = tagList;
    tagList = tag->tagNext;

    if ((tag->type != kTagTypeKey) || (tag->string == NULL)) {
      continue;
    }

    if (!_plstrcmp (tag->string, key)) {
      return tag->tag;
    }
  }

  return NULL;
}

//==========================================================================
// PListXMLParseTagList

int
PListXMLParseTagList (char* buffer, TagPtr * tag, unsigned int type, unsigned int empty, unsigned int* lenPtr) {
  int          Status;
  unsigned int pos;
  unsigned int length;
  TagPtr       tagList;
  TagPtr       lastTag;
  TagPtr       tmpTag;

  Status = 0;
  length = 0;
  pos = 0;
  tagList = NULL;
  lastTag = NULL;
  tmpTag = NULL;

  if (!empty) {
    for (;;) {
      Status = PListXMLParseNextTag (buffer + pos, &tmpTag, &length);

      if (Status != 0) {
        break;
      }

      pos += length;

      if (tmpTag == NULL) {
        break;
      }

      tmpTag->tagNext = NULL;
      if (lastTag == NULL) {
        tagList = tmpTag;
      } else {
        lastTag->tagNext = tmpTag;
      }
      lastTag = tmpTag;
    }

    if (Status != 0) {
      PListXMLFreeTag (tagList);
      return Status;
    }
  }

  tmpTag = PListXMLNewTag();

  if (tmpTag == NULL) {
    PListXMLFreeTag (tagList);
    return (-1);
  }

  tmpTag->type = type;
  tmpTag->string = NULL;
  tmpTag->dataLen = 0;
  tmpTag->offset = (unsigned int)(buffer_start ? buffer - buffer_start : 0);
  tmpTag->tag = tagList;
  tmpTag->tagNext = NULL;
  *tag = tmpTag;
  *lenPtr = pos;

  return Status;
}

//==========================================================================
// PListXMLParseTagKey

int
PListXMLParseTagKey (char* buffer, TagPtr* tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  unsigned int    length2;
  char*    string;
  TagPtr    tmpTag;
  TagPtr    subTag;

  Status = PListXMLFixDataMatchingTag (buffer, kXMLTagKey, &length);

  if (Status != 0) {
    return Status;
  }

  subTag = NULL;
  Status = PListXMLParseNextTag (buffer + length, &subTag, &length2);

  if (Status != 0) {
    return Status;
  }

  tmpTag = PListXMLNewTag();

  if (tmpTag == NULL) {
    PListXMLFreeTag (subTag);
    return (-1);
  }

  string = PListXMLNewSymbol (buffer);

  if (string == NULL) {
    PListXMLFreeTag (subTag);
    PListXMLFreeTag (tmpTag);
    return (-1);
  }

  tmpTag->type = kTagTypeKey;
  tmpTag->string = string;
  tmpTag->dataLen = _plstrlen (buffer);
  tmpTag->tag = subTag;
  tmpTag->offset = (unsigned int)(buffer_start ? buffer - buffer_start : 0);
  tmpTag->tagNext = NULL;
  *tag = tmpTag;
  *lenPtr = length + length2;

  return 0;
}

//==========================================================================
// PListXMLParseTagString

int
PListXMLParseTagString (char* buffer, TagPtr * tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  char*    string;
  TagPtr    tmpTag;

  Status = PListXMLFixDataMatchingTag (buffer, kXMLTagString, &length);

  if (Status != 0) {
    return Status;
  }

  tmpTag = PListXMLNewTag();

  if (tmpTag == NULL) {
    return (-1);
  }

  string = PListXMLNewSymbol (buffer);

  if (string == NULL) {
    PListXMLFreeTag (tmpTag);
    return (-1);
  }

  tmpTag->type = kTagTypeString;
  tmpTag->string = string;
  tmpTag->dataLen = _plstrlen (buffer);
  tmpTag->tag = NULL;
  tmpTag->tagNext = NULL;
  tmpTag->offset = (unsigned int)(buffer_start ? buffer - buffer_start : 0);
  *tag = tmpTag;
  *lenPtr = length;

  return 0;
}

//==========================================================================
// PListXMLParseTagInteger

int
PListXMLParseTagInteger (char* buffer, TagPtr * tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  vlong    integer;
  TagPtr tmpTag;

  Status = PListXMLFixDataMatchingTag (buffer, kXMLTagInteger, &length);

  if (Status != 0) {
    return Status;
  }

  tmpTag = PListXMLNewTag();

  if (tmpTag == NULL) {
    return (-1);
  }

  if (buffer[0] == '<') {
    tmpTag->type = kTagTypeInteger;
    tmpTag->string = NULL;
    tmpTag->intval = 0;
    tmpTag->tag = NULL;
    tmpTag->offset =  0;
    tmpTag->tagNext = NULL;
    *tag = tmpTag;
    length = 0;
    return 0;
  }

  integer = _plstr2vlong(buffer, length);

  tmpTag->type = kTagTypeInteger;
  tmpTag->intval = integer;
  tmpTag->tag = NULL;
  tmpTag->offset = (unsigned int)(buffer_start ? buffer - buffer_start : 0);
  tmpTag->tagNext = NULL;
  *tag = tmpTag;
  *lenPtr = length;

  return 0;
}

//==========================================================================
// PListXMLParseTagData

int
PListXMLParseTagData (char* buffer, TagPtr * tag, unsigned int* lenPtr)
{
  int  Status;
  unsigned int    length;
  unsigned int     len;
  TagPtr    tmpTag;
  char*    string;

  len = 0;
  length = 0;

  Status = PListXMLFixDataMatchingTag (buffer, kXMLTagData, &length);

  if (Status != 0) {
    return Status;
  }

  tmpTag = PListXMLNewTag();

  if (tmpTag == 0) {
    return (-1);
  }

//Slice - correction as Apple 2003
  string = PListXMLNewSymbol (buffer);
  tmpTag->type = kTagTypeData;
  tmpTag->string = string;
  tmpTag->data = (unsigned char*)_plb64decode(tmpTag->string, length, &len);
  tmpTag->dataLen = len;
  tmpTag->tag = NULL;
  tmpTag->offset = (unsigned int)(buffer_start ? buffer - buffer_start : 0);
  tmpTag->tagNext = NULL;
  *tag = tmpTag;
  *lenPtr = length;

  return 0;
}

//==========================================================================
// PListXMLParseTagDate

int
PListXMLParseTagDate (char* buffer, TagPtr * tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  TagPtr    tmpTag;

  Status = PListXMLFixDataMatchingTag (buffer, kXMLTagDate, &length);

  if (Status != 0) {
    return Status;
  }

  tmpTag = PListXMLNewTag();

  if (tmpTag == NULL) {
    return (-1);
  }

  tmpTag->type = kTagTypeDate;
  tmpTag->string = NULL;
  tmpTag->tag = NULL;
  tmpTag->tagNext = NULL;
  tmpTag->offset = (unsigned int)(buffer_start ? buffer - buffer_start : 0);
  *tag = tmpTag;
  *lenPtr = length;

  return 0;
}

//==========================================================================
// PListXMLParseTagBoolean

int
PListXMLParseTagBoolean (char* buffer, TagPtr * tag, unsigned int type, unsigned int* lenPtr) {
  TagPtr tmpTag;

  tmpTag = PListXMLNewTag();

  if (tmpTag == NULL) {
    return (-1);
  }

  tmpTag->type = type;
  tmpTag->string = NULL;
  tmpTag->dataLen = 0;
  tmpTag->tag = NULL;
  tmpTag->tagNext = NULL;
  tmpTag->offset = (unsigned int)(buffer_start ? buffer - buffer_start : 0);
  *tag = tmpTag;
  *lenPtr = 0;
  return 0;
}

//==========================================================================
// PListXMLFixDataMatchingTag
// Modifies 'buffer' to add a '\0' at the end of the tag matching 'tag'.
// Returns the length of the data found, counting the end tag,
// or -1 if the end tag was not found.

int
PListXMLFixDataMatchingTag (char* buffer, char* tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  unsigned int    start;
  unsigned int    stop;
  char*    endTag;

  start = 0;

  for (;;) {
    Status = PListXMLGetNextTag (((unsigned char *) buffer) + start, &endTag, &stop, &length);

    if (Status != 0) {
      return Status;
    }

    if ((*endTag == '/') && !_plstrcmp (endTag + 1, tag)) {
      break;
    }

    start += length;
  }

  buffer[start + stop] = '\0';
  *lenPtr = start + length;
  return 0;
}

//==========================================================================
// PListXMLNewSymbol

char*
PListXMLNewSymbol (char* string) {
  SymbolPtr symbol;
  SymbolPtr lastGuy;

  lastGuy = NULL;
  // Look for string in the list of symbols.
  symbol = PListXMLFindSymbol (string, NULL);

  // Add the new symbol.
  if (symbol == NULL) {
    symbol = (SymbolPtr) _plzalloc (sizeof (Symbol) + 1 + _plstrlen (string));

    if (symbol == NULL) {
      return NULL;
    }

    // Set the symbol's data.
    if (!_plstrcpy (symbol->string, _plstrlen(string) + 1, string)) {
      _plfree (symbol);
      return NULL;
    }
    symbol->refCount = 0;
    // Add the symbol to the list.
    symbol->next = gPListXMLSymbolsHead;
    gPListXMLSymbolsHead = symbol;
  }

  // Update the refCount and return the string.
  symbol->refCount++;

  if (lastGuy && lastGuy->next != NULL) {
    return NULL;
  }

  return symbol->string;
}

//==========================================================================
// PListXMLFreeSymbol

void
PListXMLFreeSymbol (char* string) {
  SymbolPtr symbol, prev;

  // Look for string in the list of symbols.
  symbol = PListXMLFindSymbol (string, &prev);

  if (symbol == NULL) {
    return;
  }

  // Update the refCount.
  symbol->refCount--;

  if (symbol->refCount != 0) {
    return;
  }

  // Remove the symbol from the list.
  if (prev != NULL) {
    prev->next = symbol->next;
  } else {
    gPListXMLSymbolsHead = symbol->next;
  }

  // Free the symbol's memory.
  _plfree (symbol);
}

//==========================================================================
// PListXMLFindSymbol

SymbolPtr
PListXMLFindSymbol (char * string, SymbolPtr * prevSymbol) {
  SymbolPtr symbol, prev;

  if (string == NULL) {
    return NULL;
  }
  symbol = gPListXMLSymbolsHead;
  prev = NULL;

  while (symbol != NULL) {
    if (!_plstrcmp (symbol->string, string)) {
      break;
    }

    prev = symbol;
    symbol = symbol->next;
  }

  if ((symbol != NULL) && (prevSymbol != NULL)) {
    *prevSymbol = prev;
  }

  return symbol;
}

//==========================================================================
// PListXMLNewTag

#define TAGS_CACHE_SIZE 0x0100

TagPtr
PListXMLNewTag (void) {
  unsigned int i;
  TagPtr       tag;

  if (gPListXMLTagsFree == NULL) {
    tag = (TagPtr) _plzalloc (TAGS_CACHE_SIZE * sizeof (Tag));

    if (tag == NULL) {
      return NULL;
    }

    // Initalize the new tags.
    for (i = 0; i < TAGS_CACHE_SIZE; i++) {
      tag[i].tagNext = tag + i + 1;
    }

    i--;
    tag[i].tagNext = NULL;
    gPListXMLTagsFree = tag;
    gPListXMLTagsArena = tag;
  }

  tag = gPListXMLTagsFree;
  gPListXMLTagsFree = tag->tagNext;
  return tag;
}

#undef TAGS_CACHE_SIZE

//==========================================================================
// PListXMLFreeTag

void
PListXMLFreeTag (TagPtr tag) {
  if (tag == NULL) {
    return;
  }

  if (tag->string != NULL) {
    PListXMLFreeSymbol (tag->string);
  }

  PListXMLFreeTag (tag->tag);
  PListXMLFreeTag (tag->tagNext);
  // Clear and free the tag.
  tag->type = kTagTypeNone;
  tag->string = NULL;
  tag->dataLen = 0;
  tag->tag = NULL;
  tag->offset = 0;
  tag->tagNext = gPListXMLTagsFree;
  gPListXMLTagsFree = tag;
}

//==========================================================================
// PListXMLCleanup

void
PListXMLCleanup (void) {
  SymbolPtr symbol;

  symbol = gPListXMLSymbolsHead;
  while (symbol != NULL) {
    SymbolPtr next;

    next = symbol->next;
    _plfree (symbol);
    symbol = next;
  }
  _plfree (gPListXMLTagsArena);
  gPListXMLTagsFree = NULL;
}
