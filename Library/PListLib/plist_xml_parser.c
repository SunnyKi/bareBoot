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

SymbolPtr gSymbolsHead;
TagPtr    gTagsFree;

char* buffer_start = NULL;

// Forward declarations

char*      NewSymbol (char* string);

int FixDataMatchingTag (char* buffer, char* tag, unsigned int* lenPtr);
int ParseTagBoolean (char* buffer, TagPtr * tag, unsigned int type, unsigned int* lenPtr);
int ParseTagData (char* buffer, TagPtr * tag, unsigned int* lenPtr);
int ParseTagDate (char* buffer, TagPtr * tag, unsigned int* lenPtr);
int ParseTagInteger (char* buffer, TagPtr * tag, unsigned int* lenPtr);
int ParseTagKey (char * buffer, TagPtr * tag, unsigned int* lenPtr);
int ParseTagList (char* buffer, TagPtr * tag, unsigned int type, unsigned int empty, unsigned int* lenPtr);
int ParseTagString (char* buffer, TagPtr * tag, unsigned int* lenPtr);

SymbolPtr   FindSymbol (char * string, SymbolPtr * prevSymbol);

TagPtr     NewTag (void);

void        FreeSymbol (char* string);

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
XMLDecode (const char* src) {
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
// XMLFreeTag

void
FreeTag (TagPtr tag) {
  if (tag == NULL) {
    return;
  }

  if (tag->string != NULL) {
    FreeSymbol (tag->string);
  }

  FreeTag (tag->tag);
  FreeTag (tag->tagNext);
  // Clear and free the tag.
  tag->type = kTagTypeNone;
  tag->string = NULL;
  tag->dataLen = 0;
  tag->tag = NULL;
  tag->offset = 0;
  tag->tagNext = gTagsFree;
  gTagsFree = tag;
}

//==========================================================================
// GetNextTag

int
GetNextTag (unsigned char* buffer, char** tag, unsigned int* start, unsigned int* length) {
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
// ParseNextTag

int
XMLParseNextTag (char* buffer, TagPtr * tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  unsigned int    pos;
  char*    tagName;

  length = 0;
  pos = 0;
  tagName = NULL;

  *lenPtr = 0;
  Status = GetNextTag ((unsigned char*) buffer, &tagName, 0, &length);

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
    Status = ParseTagList (buffer + pos, tag, kTagTypeDict, 0, &length);
  } else if (!_plstrcmp (tagName, kXMLTagDict "/")) {
    Status = ParseTagList (buffer + pos, tag, kTagTypeDict, 1, &length);
  } else if (!_plstrcmp (tagName, kXMLTagDict " ")) {
    Status = ParseTagList (buffer + pos, tag, kTagTypeDict, 0, &length);
  }
  /***** key ****/
  else if (!_plstrcmp (tagName, kXMLTagKey)) {
    Status = ParseTagKey (buffer + pos, tag, &length);
  }
  /***** string ****/
  else if (!_plstrcmp (tagName, kXMLTagString)) {
    Status = ParseTagString (buffer + pos, tag, &length);
  }
  /***** integer ****/
  else if (!_plstrcmp (tagName, kXMLTagInteger)) {
    Status = ParseTagInteger (buffer + pos, tag, &length);
  } else if (!_plstrcmp (tagName, kXMLTagInteger " ")) {
    Status = ParseTagInteger (buffer + pos, tag, &length);
  }
  /***** data ****/
  else if (!_plstrcmp (tagName, kXMLTagData)) {
    Status = ParseTagData (buffer + pos, tag, &length);
  } else if (!_plstrcmp (tagName, kXMLTagData " ")) {
    Status = ParseTagData (buffer + pos, tag, &length);
  }
  /***** date ****/
  else if (!_plstrcmp (tagName, kXMLTagDate)) {
    Status = ParseTagDate (buffer + pos, tag, &length);
  }
  /***** FALSE ****/
  else if (!_plstrcmp (tagName, kXMLTagFalse)) {
    Status = ParseTagBoolean (buffer + pos, tag, kTagTypeFalse, &length);
  }
  /***** TRUE ****/
  else if (!_plstrcmp (tagName, kXMLTagTrue)) {
    Status = ParseTagBoolean (buffer + pos, tag, kTagTypeTrue, &length);
  }
  /***** array ****/
  else if (!_plstrcmp (tagName, kXMLTagArray)) {
    Status = ParseTagList (buffer + pos, tag, kTagTypeArray, 0, &length);
  } else if (!_plstrcmp (tagName, kXMLTagArray " ")) {
    Status = ParseTagList (buffer + pos, tag, kTagTypeArray, 0, &length);
  } else if (!_plstrcmp (tagName, kXMLTagArray "/")) {
    Status = ParseTagList (buffer + pos, tag, kTagTypeArray, 1, &length);
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
PListParseXML (const char* buffer, unsigned int bSize, TagPtr* dict) {
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
    Status = XMLParseNextTag (configBuffer + pos, &tag, &length);

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

    FreeTag (tag);
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

#define DOFREE 1

//==========================================================================
// XMLGetProperty

TagPtr
PListGetProperty (TagPtr dict, const char* key) {
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
// ParseTagList

int
ParseTagList (char* buffer, TagPtr * tag, unsigned int type, unsigned int empty, unsigned int* lenPtr) {
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

  if (!empty) {
    for (;;) {
      Status = XMLParseNextTag (buffer + pos, &tmpTag, &length);

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
      FreeTag (tagList);
      return Status;
    }
  }

  tmpTag = NewTag();

  if (tmpTag == NULL) {
    FreeTag (tagList);
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
// ParseTagKey

int
ParseTagKey (char* buffer, TagPtr* tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  unsigned int    length2;
  char*    string;
  TagPtr    tmpTag;
  TagPtr    subTag;

  Status = FixDataMatchingTag (buffer, kXMLTagKey, &length);

  if (Status != 0) {
    return Status;
  }

  Status = XMLParseNextTag (buffer + length, &subTag, &length2);

  if (Status != 0) {
    return Status;
  }

  tmpTag = NewTag();

  if (tmpTag == NULL) {
    FreeTag (subTag);
    return (-1);
  }

  string = NewSymbol (buffer);

  if (string == NULL) {
    FreeTag (subTag);
    FreeTag (tmpTag);
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
// ParseTagString

int
ParseTagString (char* buffer, TagPtr * tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  char*    string;
  TagPtr    tmpTag;

  Status = FixDataMatchingTag (buffer, kXMLTagString, &length);

  if (Status != 0) {
    return Status;
  }

  tmpTag = NewTag();

  if (tmpTag == NULL) {
    return (-1);
  }

  string = NewSymbol (buffer);

  if (string == NULL) {
    FreeTag (tmpTag);
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
// ParseTagInteger

int
ParseTagInteger (char* buffer, TagPtr * tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  long    integer;
  unsigned int    size;
  int   negative;
  char*    val;
  TagPtr tmpTag;

  negative = FALSE;
  val = buffer;
  Status = FixDataMatchingTag (buffer, kXMLTagInteger, &length);

  if (Status != 0) {
    return Status;
  }

  tmpTag = NewTag();

  if (tmpTag == NULL) {
    return (-1);
  }

  size = length;
  integer = 0;

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

  if (size > 1 && (val[1] == 'x' || val[1] == 'X')) { // Hex value
    val += 2;

    while (*val) {
      if ((*val >= '0' && *val <= '9')) { // 0 - 9
        integer = (integer * 16) + (*val++ - '0');
      } else if ((*val >= 'a' && *val <= 'f')) { // a - f
        integer = (integer * 16) + (*val++ - 'a' + 10);
      } else if ((*val >= 'A' && *val <= 'F')) { // A - F
        integer = (integer * 16) + (*val++ - 'a' + 10);
      } else {
        return (-1);
      }
    }
  } else if (size) { // Decimal value
    if (*val == '-') {
      negative = TRUE;
      val++;
      size--;
    }

    for (integer = 0; size > 0; size--) {
      if (*val) { // UGLY HACK, fix me.
        if (*val < '0' || *val > '9') {
          return (-1);
        }

        integer = (integer * 10) + (*val++ - '0');
      }
    }

    if (negative) {
      integer = -integer;
    }
  }

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
// ParseTagData

int
ParseTagData (char* buffer, TagPtr * tag, unsigned int* lenPtr)
{
  int  Status;
  unsigned int    length;
  unsigned int     len;
  TagPtr    tmpTag;
  char*    string;

  len = 0;
  length = 0;

  Status = FixDataMatchingTag (buffer, kXMLTagData, &length);

  if (Status != 0) {
    return Status;
  }

  tmpTag = NewTag();

  if (tmpTag == 0) {
    return (-1);
  }

//Slice - correction as Apple 2003
  string = NewSymbol (buffer);
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
// ParseTagDate

int
ParseTagDate (char* buffer, TagPtr * tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  TagPtr    tmpTag;

  Status = FixDataMatchingTag (buffer, kXMLTagDate, &length);

  if (Status != 0) {
    return Status;
  }

  tmpTag = NewTag();

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
// ParseTagBoolean

int
ParseTagBoolean (char* buffer, TagPtr * tag, unsigned int type, unsigned int* lenPtr) {
  TagPtr tmpTag;

  tmpTag = NewTag();

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
// FixDataMatchingTag
// Modifies 'buffer' to add a '\0' at the end of the tag matching 'tag'.
// Returns the length of the data found, counting the end tag,
// or -1 if the end tag was not found.

int
FixDataMatchingTag (char* buffer, char* tag, unsigned int* lenPtr) {
  int  Status;
  unsigned int    length;
  unsigned int    start;
  unsigned int    stop;
  char*    endTag;

  start = 0;

  for (;;) {
    Status = GetNextTag (((unsigned char *) buffer) + start, &endTag, &stop, &length);

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
// NewTag

TagPtr
NewTag (void) {
  unsigned int i;
  TagPtr       tag;

  if (gTagsFree == NULL) {
    tag = (TagPtr) _plzalloc (0x1000 * sizeof (Tag));

    if (tag == NULL) {
      return NULL;
    }

    // Initalize the new tags.
    for (i = 0; i < 0x1000; i++) {
      tag[i].type = kTagTypeNone;
      tag[i].string = NULL;
      tag[i].tag = NULL;
      tag[i].tagNext = tag + i + 1;
    }

    tag[0x1000 - 1].tagNext = NULL;
    gTagsFree = tag;
  }

  tag = gTagsFree;
  gTagsFree = tag->tagNext;
  return tag;
}

char*
NewSymbol (char* string) {
  SymbolPtr symbol;
  SymbolPtr lastGuy;

  lastGuy = NULL;
  // Look for string in the list of symbols.
  symbol = FindSymbol (string, NULL);

  // Add the new symbol.
  if (symbol == NULL) {
    symbol = (SymbolPtr) _plzalloc (sizeof (Symbol) + 1 + _plstrlen (string));

    if (symbol == NULL) {
      return NULL;
    }

    // Set the symbol's data.
    symbol->refCount = 0;
    _plstrcpy (symbol->string, string);
    // Add the symbol to the list.
    symbol->next = gSymbolsHead;
    gSymbolsHead = symbol;
  }

  // Update the refCount and return the string.
  symbol->refCount++;

  if (lastGuy && lastGuy->next != NULL) {
    return NULL;
  }

  return symbol->string;
}

//==========================================================================
// FreeSymbol

void
FreeSymbol (char* string) {
  SymbolPtr symbol, prev;

  // Look for string in the list of symbols.
  symbol = FindSymbol (string, &prev);

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
    gSymbolsHead = symbol->next;
  }

  // Free the symbol's memory.
  _plfree (symbol);
}

//==========================================================================
// FindSymbol

SymbolPtr
FindSymbol (char * string, SymbolPtr * prevSymbol) {
  SymbolPtr symbol, prev;

  symbol = gSymbolsHead;
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
