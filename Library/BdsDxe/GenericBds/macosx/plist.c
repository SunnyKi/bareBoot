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

#include "macosx.h"

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
#define kXMLTagReference  "reference"
#define kXMLTagID         "ID="
#define kXMLTagIDREF      "IDREF="


SymbolPtr gSymbolsHead;
TagPtr    gTagsFree;

CHAR8* buffer_start = NULL;

// Forward declarations
EFI_STATUS ParseTagList (CHAR8* buffer, TagPtr * tag, UINT32 type, UINT32 empty, UINT32* lenPtr);
EFI_STATUS ParseTagKey (char * buffer, TagPtr * tag, UINT32* lenPtr);
EFI_STATUS ParseTagString (CHAR8* buffer, TagPtr * tag, UINT32* lenPtr);
EFI_STATUS ParseTagInteger (CHAR8* buffer, TagPtr * tag, UINT32* lenPtr);
EFI_STATUS ParseTagData (CHAR8* buffer, TagPtr * tag, UINT32* lenPtr);
EFI_STATUS ParseTagDate (CHAR8* buffer, TagPtr * tag, UINT32* lenPtr);
EFI_STATUS ParseTagBoolean (CHAR8* buffer, TagPtr * tag, UINT32 type, UINT32* lenPtr);
TagPtr     NewTag (VOID);
EFI_STATUS FixDataMatchingTag (CHAR8* buffer, CHAR8* tag, UINT32* lenPtr);
CHAR8*      NewSymbol (CHAR8* string);
VOID        FreeSymbol (CHAR8* string);
SymbolPtr   FindSymbol (char * string, SymbolPtr * prevSymbol);

/* Function for basic XML character entities parsing */

CHAR8*
XMLDecode (const CHAR8* src) {
  typedef const struct XMLEntity {
    const CHAR8* name;
    UINTN nameLen;
    CHAR8 value;
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
  UINTN len;
  const CHAR8 *s;
  CHAR8 *out, *o;

  len = AsciiStrLen (src);
  out = AllocateZeroPool (len + 1);

  if (src == NULL || len == 0 || out == NULL) {
    return NULL;
  }

  o = out;
  s = src;

  while (s <= src + len) { /* Make sure the terminator is also copied */
    if (*s == '&') {
      BOOLEAN entFound = FALSE;
      UINTN i;

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

//==========================================================================
// XMLFreeTag

VOID
FreeTag (TagPtr tag) {
  if (tag == NULL) {
    return;
  }

  if (tag->string) {
    FreeSymbol (tag->string);
  }

  FreeTag (tag->tag);
  FreeTag (tag->tagNext);
  // Clear and free the tag.
  tag->type = kTagTypeNone;
  tag->string = NULL;
  tag->tag = NULL;
  tag->offset = 0;
  tag->tagNext = gTagsFree;
  gTagsFree = tag;
}

//==========================================================================
// GetNextTag

EFI_STATUS
GetNextTag (UINT8* buffer, CHAR8** tag, UINT32* start, UINT32* length) {
  UINT32 cnt, cnt2;

  if (tag == 0) {
    return EFI_UNSUPPORTED;
  }

  // Find the start of the tag.
  cnt = 0;

  while ((buffer[cnt] != '\0') && (buffer[cnt] != '<')) {
    cnt++;
  }

  if (buffer[cnt] == '\0') {
    return EFI_UNSUPPORTED;
  }

  // Find the end of the tag.
  cnt2 = cnt + 1;

  while ((buffer[cnt2] != '\0') && (buffer[cnt2] != '>')) {
    cnt2++;
  }

  if (buffer[cnt2] == '\0') {
    return EFI_UNSUPPORTED;
  }

  // Fix the tag data.
  *tag = (CHAR8*) (buffer + cnt + 1);
  buffer[cnt2] = '\0';

  if (start) {
    *start = cnt;
  }

  *length = cnt2 + 1;
  return EFI_SUCCESS;
}

//==========================================================================
// ParseNextTag

EFI_STATUS
XMLParseNextTag (CHAR8* buffer, TagPtr * tag, UINT32* lenPtr) {
  EFI_STATUS  Status;
  UINT32    length;
  UINT32    pos;
  CHAR8*    tagName;

  length = 0;
  pos = 0;
  tagName = NULL;

  *lenPtr = 0;
  Status = GetNextTag ((UINT8*) buffer, &tagName, 0, &length);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  pos = length;

  if (!AsciiStrnCmp (tagName, kXMLTagPList, 6)) {
    length = 0;
    Status = EFI_SUCCESS;
  }
  /***** dict ****/
  else if (!AsciiStrCmp (tagName, kXMLTagDict)) {
    Status = ParseTagList (buffer + pos, tag, kTagTypeDict, 0, &length);
  } else if (!AsciiStrCmp (tagName, kXMLTagDict "/")) {
    Status = ParseTagList (buffer + pos, tag, kTagTypeDict, 1, &length);
  } else if (!AsciiStrCmp (tagName, kXMLTagDict " ")) {
    Status = ParseTagList (buffer + pos, tag, kTagTypeDict, 0, &length);
  }
  /***** key ****/
  else if (!AsciiStrCmp (tagName, kXMLTagKey)) {
    Status = ParseTagKey (buffer + pos, tag, &length);
  }
  /***** string ****/
  else if (!AsciiStrCmp (tagName, kXMLTagString)) {
    Status = ParseTagString (buffer + pos, tag, &length);
  }
  /***** integer ****/
  else if (!AsciiStrCmp (tagName, kXMLTagInteger)) {
    Status = ParseTagInteger (buffer + pos, tag, &length);
  } else if (!AsciiStrCmp (tagName, kXMLTagInteger " ")) {
    Status = ParseTagInteger (buffer + pos, tag, &length);
  }
  /***** data ****/
  else if (!AsciiStrCmp (tagName, kXMLTagData)) {
    Status = ParseTagData (buffer + pos, tag, &length);
  } else if (!AsciiStrCmp (tagName, kXMLTagData " ")) {
    Status = ParseTagData (buffer + pos, tag, &length);
  }
  /***** date ****/
  else if (!AsciiStrCmp (tagName, kXMLTagDate)) {
    Status = ParseTagDate (buffer + pos, tag, &length);
  }
  /***** FALSE ****/
  else if (!AsciiStrCmp (tagName, kXMLTagFalse)) {
    Status = ParseTagBoolean (buffer + pos, tag, kTagTypeFalse, &length);
  }
  /***** TRUE ****/
  else if (!AsciiStrCmp (tagName, kXMLTagTrue)) {
    Status = ParseTagBoolean (buffer + pos, tag, kTagTypeTrue, &length);
  }
  /***** array ****/
  else if (!AsciiStrCmp (tagName, kXMLTagArray)) {
    Status = ParseTagList (buffer + pos, tag, kTagTypeArray, 0, &length);
  } else if (!AsciiStrCmp (tagName, kXMLTagArray " ")) {
    Status = ParseTagList (buffer + pos, tag, kTagTypeArray, 0, &length);
  } else if (!AsciiStrCmp (tagName, kXMLTagArray "/")) {
    Status = ParseTagList (buffer + pos, tag, kTagTypeArray, 1, &length);
  }
  /***** unknown ****/
  else {
    *tag = NULL;
    length = 0;
  }

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  *lenPtr = pos + length;
  return EFI_SUCCESS;
}

// Expects to see one dictionary in the XML file, the final pos will be returned
// If the pos is not equal to the strlen, then there are multiple dicts
// Puts the first dictionary it finds in the
// tag pointer and returns the end of the dic, or returns -1 if not found.
//

EFI_STATUS
ParseXML (CONST CHAR8* buffer, TagPtr * dict) {
  EFI_STATUS  Status;
  UINT32    length;
  UINT32    pos;
  TagPtr    tag;
  CHAR8*    configBuffer;
  UINT32    bufferSize;

  length = 0;
  pos = 0;
  tag = NULL;
  configBuffer = NULL;
  bufferSize = (UINT32) AsciiStrLen (buffer) + 1;

  if (dict == NULL) {
    return EFI_UNSUPPORTED;
  }

  *dict = NULL;
  configBuffer = AllocateZeroPool (bufferSize);

  if (configBuffer == NULL) {
    return EFI_UNSUPPORTED;
  }

  CopyMem (configBuffer, buffer, bufferSize);
  buffer_start = configBuffer;

  while (TRUE) {
    Status = XMLParseNextTag (configBuffer + pos, &tag, &length);

    if (EFI_ERROR (Status)) {
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

  FreePool (configBuffer);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  *dict = tag;
  return EFI_SUCCESS;
}

//
// xml
//

#define DOFREE 1

//==========================================================================
// XMLGetProperty

TagPtr
GetProperty (TagPtr dict, const CHAR8* key) {
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

  while (tagList) {
    tag = tagList;
    tagList = tag->tagNext;

    if ((tag->type != kTagTypeKey) || (tag->string == NULL)) {
      continue;
    }

    if (!AsciiStrCmp (tag->string, key)) {
      return tag->tag;
    }
  }

  return NULL;
}

//==========================================================================
// ParseTagList

EFI_STATUS
ParseTagList (CHAR8* buffer, TagPtr * tag, UINT32 type, UINT32 empty, UINT32* lenPtr) {
  EFI_STATUS  Status;
  UINT32    pos;
  TagPtr    tagList;
  TagPtr    tmpTag;
  UINT32    length;

  Status = EFI_SUCCESS;
  length = 0;
  tagList = NULL;
  pos = 0;

  if (!empty) {
    while (TRUE) {
      Status = XMLParseNextTag (buffer + pos, &tmpTag, &length);

      if (EFI_ERROR (Status)) {
        break;
      }

      pos += length;

      if (tmpTag == NULL) {
        break;
      }

      tmpTag->tagNext = tagList;
      tagList = tmpTag;
    }

    if (EFI_ERROR (Status)) {
      FreeTag (tagList);
      return Status;
    }
  }

  tmpTag = NewTag();

  if (tmpTag == NULL) {
    FreeTag (tagList);
    return EFI_UNSUPPORTED;
  }

  tmpTag->type = type;
  tmpTag->string = 0;
  tmpTag->offset = (UINT32)(buffer_start ? buffer - buffer_start : 0);
  tmpTag->tag = tagList;
  tmpTag->tagNext = 0;
  *tag = tmpTag;
  *lenPtr = pos;

  return Status;
}

//==========================================================================
// ParseTagKey

EFI_STATUS
ParseTagKey (char * buffer, TagPtr * tag, UINT32* lenPtr) {
  EFI_STATUS  Status;
  UINT32    length;
  UINT32    length2;
  CHAR8*    string;
  TagPtr    tmpTag;
  TagPtr    subTag;

  Status = FixDataMatchingTag (buffer, kXMLTagKey, &length);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = XMLParseNextTag (buffer + length, &subTag, &length2);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  tmpTag = NewTag();

  if (tmpTag == NULL) {
    FreeTag (subTag);
    return EFI_UNSUPPORTED;
  }

  string = NewSymbol (buffer);

  if (string == NULL) {
    FreeTag (subTag);
    FreeTag (tmpTag);
    return EFI_UNSUPPORTED;
  }

  tmpTag->type = kTagTypeKey;
  tmpTag->string = string;
  tmpTag->tag = subTag;
  tmpTag->offset = (UINT32)(buffer_start ? buffer - buffer_start : 0);
  tmpTag->tagNext = 0;
  *tag = tmpTag;
  *lenPtr = length + length2;

  return EFI_SUCCESS;
}

//==========================================================================
// ParseTagString

EFI_STATUS
ParseTagString (CHAR8* buffer, TagPtr * tag, UINT32* lenPtr) {
  EFI_STATUS  Status;
  UINT32    length;
  CHAR8*    string;
  TagPtr    tmpTag;

  Status = FixDataMatchingTag (buffer, kXMLTagString, &length);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  tmpTag = NewTag();

  if (tmpTag == NULL) {
    return EFI_UNSUPPORTED;
  }

  string = NewSymbol (buffer);

  if (string == NULL) {
    FreeTag (tmpTag);
    return EFI_UNSUPPORTED;
  }

  tmpTag->type = kTagTypeString;
  tmpTag->string = string;
  tmpTag->tag = NULL;
  tmpTag->tagNext = NULL;
  tmpTag->offset = (UINT32)(buffer_start ? buffer - buffer_start : 0);
  *tag = tmpTag;
  *lenPtr = length;

  return EFI_SUCCESS;
}

//==========================================================================
// ParseTagInteger

EFI_STATUS
ParseTagInteger (CHAR8* buffer, TagPtr * tag, UINT32* lenPtr) {
  EFI_STATUS  Status;
  UINT32    length;
  INTN    integer;
  UINT32    size;
  BOOLEAN   negative;
  CHAR8*    val;
  TagPtr tmpTag;

  negative = FALSE;
  val = buffer;
  Status = FixDataMatchingTag (buffer, kXMLTagInteger, &length);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  tmpTag = NewTag();

  if (tmpTag == NULL) {
    return EFI_UNSUPPORTED;
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
    return EFI_SUCCESS;
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
#if 0
        getchar();
#endif
        return EFI_UNSUPPORTED;
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
#if 0
          getchar();
#endif
          return EFI_UNSUPPORTED;
        }

        integer = (integer * 10) + (*val++ - '0');
      }
    }

    if (negative) {
      integer = -integer;
    }
  }

  tmpTag->type = kTagTypeInteger;
  tmpTag->intval = (INT32)integer;
  tmpTag->tag = NULL;
  tmpTag->offset = (UINT32)(buffer_start ? buffer - buffer_start : 0);
  tmpTag->tagNext = NULL;
  *tag = tmpTag;
  *lenPtr = length;

  return EFI_SUCCESS;
}

//==========================================================================
// ParseTagData

EFI_STATUS
ParseTagData (CHAR8* buffer, TagPtr * tag, UINT32* lenPtr)
{
  EFI_STATUS  Status;
  UINT32    length;
  UINTN     len;
  TagPtr    tmpTag;
  CHAR8*    string;

  len = 0;
  length = 0;

  Status = FixDataMatchingTag (buffer, kXMLTagData, &length);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  tmpTag = NewTag();

  if (tmpTag == 0) {
    return EFI_UNSUPPORTED;
  }

//Slice - correction as Apple 2003
  string = NewSymbol (buffer);
  tmpTag->type = kTagTypeData;
  tmpTag->string = string;
	tmpTag->data = (UINT8 *) Base64Decode(tmpTag->string, &len);
  tmpTag->dataLen = len;
  tmpTag->tag = NULL;
  tmpTag->offset = (UINT32)(buffer_start ? buffer - buffer_start : 0);
  tmpTag->tagNext = NULL;
  *tag = tmpTag;
  *lenPtr = length;

  return EFI_SUCCESS;
}

//==========================================================================
// ParseTagDate

EFI_STATUS
ParseTagDate (CHAR8* buffer, TagPtr * tag, UINT32* lenPtr) {
  EFI_STATUS  Status;
  UINT32    length;
  TagPtr    tmpTag;

  Status = FixDataMatchingTag (buffer, kXMLTagDate, &length);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  tmpTag = NewTag();

  if (tmpTag == NULL) {
    return EFI_UNSUPPORTED;
  }

  tmpTag->type = kTagTypeDate;
  tmpTag->string = NULL;
  tmpTag->tag = NULL;
  tmpTag->tagNext = NULL;
  tmpTag->offset = (UINT32)(buffer_start ? buffer - buffer_start : 0);
  *tag = tmpTag;
  *lenPtr = length;

  return EFI_SUCCESS;
}

//==========================================================================
// ParseTagBoolean

EFI_STATUS
ParseTagBoolean (CHAR8* buffer, TagPtr * tag, UINT32 type, UINT32* lenPtr) {
  TagPtr tmpTag;

  tmpTag = NewTag();

  if (tmpTag == NULL) {
    return EFI_UNSUPPORTED;
  }

  tmpTag->type = type;
  tmpTag->string = NULL;
  tmpTag->tag = NULL;
  tmpTag->tagNext = NULL;
  tmpTag->offset = (UINT32)(buffer_start ? buffer - buffer_start : 0);
  *tag = tmpTag;
  *lenPtr = 0;
  return EFI_SUCCESS;
}

//==========================================================================
// FixDataMatchingTag
// Modifies 'buffer' to add a '\0' at the end of the tag matching 'tag'.
// Returns the length of the data found, counting the end tag,
// or -1 if the end tag was not found.

EFI_STATUS
FixDataMatchingTag (CHAR8* buffer, CHAR8* tag, UINT32* lenPtr) {
  EFI_STATUS  Status;
  UINT32    length;
  UINT32    start;
  UINT32    stop;
  CHAR8*    endTag;

  start = 0;

  while (1) {
    Status = GetNextTag (((UINT8 *) buffer) + start, &endTag, &stop, &length);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((*endTag == '/') && !AsciiStrCmp (endTag + 1, tag)) {
      break;
    }

    start += length;
  }

  buffer[start + stop] = '\0';
  *lenPtr = start + length;
  return EFI_SUCCESS;
}

//==========================================================================
// NewTag

TagPtr
NewTag (VOID) {
  UINT32  cnt;
  TagPtr  tag;

  if (gTagsFree == NULL) {
    tag = (TagPtr) AllocateZeroPool (0x1000 * sizeof (Tag));

    if (tag == NULL) {
      return NULL;
    }

    // Initalize the new tags.
    for (cnt = 0; cnt < 0x1000; cnt++) {
      tag[cnt].type = kTagTypeNone;
      tag[cnt].string = NULL;
      tag[cnt].tag = NULL;
      tag[cnt].tagNext = tag + cnt + 1;
    }

    tag[0x1000 - 1].tagNext = NULL;
    gTagsFree = tag;
  }

  tag = gTagsFree;
  gTagsFree = tag->tagNext;
  return tag;
}

CHAR8*
NewSymbol (CHAR8* string) {
  SymbolPtr symbol;
  SymbolPtr lastGuy;

  lastGuy = NULL;
  // Look for string in the list of symbols.
  symbol = FindSymbol (string, NULL);

  // Add the new symbol.
  if (symbol == NULL) {
    symbol = (SymbolPtr) AllocateZeroPool (sizeof (Symbol) + 1 + AsciiStrLen (string));

    if (symbol == NULL) {
      return NULL;
    }

    // Set the symbol's data.
    symbol->refCount = 0;
    AsciiStrCpy (symbol->string, string);
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

VOID
FreeSymbol (CHAR8* string) {
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
  FreePool (symbol);
}

//==========================================================================
// FindSymbol

SymbolPtr
FindSymbol (char * string, SymbolPtr * prevSymbol) {
  SymbolPtr symbol, prev;

  symbol = gSymbolsHead;
  prev = NULL;

  while (symbol != NULL) {
    if (!AsciiStrCmp (symbol->string, string)) {
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
