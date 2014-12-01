/*
 * fixDSDT
 *
 * Copyright (c) 2011-2012 Frank Peng. All rights reserved.
 *
 */

/*
 *  aml_generator.c
 *  Chameleon
 *
 *  Created by Mozodojo on 20/07/10.
 *  Copyright 2010 mozo. All rights reserved.
 *
 *  additions and corrections by Slice and pcj, 2012.
 */

#include <macosx.h>

UINT32
aml_write_size (
  UINT32 size,
  CHAR8 *buffer,
  UINT32 offset
)
{
  if (size <= 0x3f) { /* simple 1 byte length in 6 bits */
    buffer[offset++] = (CHAR8) size;
  }
  else if (size <= 0xfff) {
    buffer[offset++] = 0x40 | (size & 0xf); /* 0x40 is type, 0x0X is first nibble of length */
    buffer[offset++] = (size >> 4) & 0xff;  /* +1 bytes for rest length */
  }
  else if (size <= 0xfffff) {
    buffer[offset++] = 0x80 | (size & 0xf); /* 0x80 is type, 0x0X is first nibble of length */
    buffer[offset++] = (size >> 4) & 0xff;  /* +2 bytes for rest length */
    buffer[offset++] = (size >> 12) & 0xff;
  }
  else {
    buffer[offset++] = 0xc0 | (size & 0xf); /* 0xC0 is type, 0x0X is first nibble of length */
    buffer[offset++] = (size >> 4) & 0xff;  /* +3 bytes for rest length */
    buffer[offset++] = (size >> 12) & 0xff;
    buffer[offset++] = (size >> 20) & 0xff;
  }

  return offset;
}

UINT32
get_size (
  UINT8 *Buffer,
  UINT32 adr
)
{
  UINT32 temp;

  temp = Buffer[adr] & 0xF0;  //keep bits 0x30 to check if this is valid size field

  if (temp <= 0x30) { // 0
    temp = Buffer[adr];
  }
  else if (temp == 0x40) {  // 4
    temp = (Buffer[adr] - 0x40) << 0 | Buffer[adr + 1] << 4;
  }
  else if (temp == 0x80) {  // 8
    temp = (Buffer[adr] - 0x80) << 0 | Buffer[adr + 1] << 4 | Buffer[adr + 2] << 12;
  }
  else if (temp == 0xC0) {  // C
    temp = (Buffer[adr] - 0xC0) << 0 | Buffer[adr + 1] << 4 | Buffer[adr + 2] << 12 | Buffer[adr + 3] << 20;
  }
  else {
    //  DBG("wrong pointer to size field at %x\n", adr);
    return 0;
  }
  return temp;
}

BOOLEAN
CmpNum (
  UINT8 *dsdt,
  INT32 i,
  BOOLEAN Sure
)
{
  return ((Sure &&
           ((dsdt[i - 1] == 0x0A) || (dsdt[i - 2] == 0x0B) ||
            (dsdt[i - 4] == 0x0C))) || (!Sure && (((dsdt[i - 1] >= 0x0A) &&
                                                   (dsdt[i - 1] <= 0x0C)) ||
                                                  ((dsdt[i - 2] == 0x0B) ||
                                                   (dsdt[i - 2] == 0x0C)) ||
                                                  (dsdt[i - 4] == 0x0C))));
}

INT32
FindName (
  UINT8 *dsdt,
  INT32 len,
  CHAR8* name
)
{
  INT32 i;
  for (i = 0; i < len - 4; i++) {
    if ((dsdt[i] == 0x08) &&
        (dsdt[i+1] == name[0]) &&
        (dsdt[i+2] == name[1]) &&
        (dsdt[i+3] == name[2]) &&
        (dsdt[i+4] == name[3])) {
      return i+1;
    }
  }
  return 0;
}

BOOLEAN
GetName (
  UINT8 *dsdt,
  INT32 adr,
  CHAR8 *name,
  OUT INTN *shift
)
{
  INT32 i;
  INT32 j;  //now we accept \NAME

  j = (dsdt[adr] == 0x5C) ? 1 : 0;
  if (!name) {
    return FALSE;
  }
  for (i = adr + j; i < adr + j + 4; i++) {
    if ((dsdt[i] < 0x2F) ||
        ((dsdt[i] > 0x39) && (dsdt[i] < 0x41)) ||
        ((dsdt[i] > 0x5A) && (dsdt[i] != 0x5F))) {
      return FALSE;
    }
    name[i - adr - j] = dsdt[i];
  }
  name[4] = 0;
  if (shift) {
    *shift = j;
  }  
  return TRUE;
}

//                start => move data start address
//                offset => data move how many byte 
//                len => initial length of the buffer
// return final length of the buffer
// we suppose that buffer allocation is more then len+offset

UINT32
move_data (
  UINT32 start,
  UINT8 *buffer,
  UINT32 len,
  INT32 offset
)
{
  UINT32 i;

  if (offset < 0) {
    for (i = start; i < len + offset; i++) {
      buffer[i] = buffer[i - offset];
    }
  }
  else {  // data move to back        
    for (i = len - 1; i >= start; i--) {
      buffer[i + offset] = buffer[i];
    }
  }
  return len + offset;
}

//the procedure can find BIN array UNSIGNED CHAR8 sizeof N inside part of large array "dsdt" size of len

INT32
FindBin (
  UINT8 *dsdt,
  UINT32 len,
  UINT8 *bin,
  UINTN N
)
{
  UINT32 i, j;
  BOOLEAN eq;

  for (i = 0; i < len - N; i++) {
    eq = TRUE;
    for (j = 0; j < N; j++) {
      if (dsdt[i + j] != bin[j]) {
        eq = FALSE;
        break;
      }
    }
    if (eq) {
      return (INT32) i;
    }
  }
  return -1;
}

/*
  adr - a place to write new size. Size of some object.
  buffer - the binary aml codes array
  len - its length
  sizeoffset - how much the object increased in size
  return address shift from original  +/- n from outers
 When we increase the object size there is a chance that new size field +1
 so out devices should also be corrected +1 and this may lead to new shift 
*/

//Slice - I excluded check (oldsize <= 0x0fffff && size > 0x0fffff)
//because I think size of DSDT will never be 1Mb

INT32
write_size (
  UINT32 adr,
  UINT8 *buffer,
  UINT32 len,
  INT32 sizeoffset
)
{
  UINT32 size, oldsize;
  INT32 offset = 0;

  oldsize = get_size (buffer, adr);

  if (!oldsize) {
    return 0; //wrong address, will not write here
  }
  size = oldsize + sizeoffset;
  // data move to back
  if (oldsize <= 0x3f && size > 0x0fff) {
    offset = 2;
  }
  else if ((oldsize <= 0x3f && size > 0x3f) ||
           (oldsize <= 0x0fff && size > 0x0fff)) {
    offset = 1;
  } // data move to front
  else if ((size <= 0x3f && oldsize > 0x3f) ||
           (size <= 0x0fff && oldsize > 0x0fff)) {
    offset = -1;
  }
  else if (oldsize > 0x0fff && size <= 0x3f) {
    offset = -2;
  }

  (void) move_data (adr, buffer, len, offset);

  size += offset;
  aml_write_size (size, (CHAR8 *) buffer, adr); //reuse existing codes

  return offset;
}

//this procedure corrects size of outer method. Embedded methods is not proposed
// adr - a place of changes
// shift - a size of changes

UINT32
CorrectOuterMethod (
  UINT8 *dsdt,
  UINT32 len,
  UINT32 adr,
  INT32 shift
)
{
  INT32 i, k;
  UINT32 size = 0;
  INT32 offset = 0;
  CHAR8 Name[5];

  if (shift == 0) {
    return len;
  }
  i = adr;  //usually adr = @5B - 1 = sizefield - 3
  while (i-- > 0x20) {  //find method that previous to adr
    k = i + 1;
    if ((dsdt[i] == 0x14) && !CmpNum (dsdt, i, FALSE)) {  //method candidate
      size = get_size (dsdt, k);
      if (!size) {
        continue;
      }
      if (((size <= 0x3F) && !GetName (dsdt, k + 1, &Name[0], NULL)) ||
          ((size > 0x3F) && (size <= 0xFFF) && !GetName (dsdt, k + 2, &Name[0], NULL))
          || ((size > 0xFFF) && !GetName (dsdt, k + 3, &Name[0], NULL))) {
        DBG ("method found, size=0x%x but name is not\n", size);
        continue;
      }
      if ((k + size) > adr + 4) { //Yes - it is outer
        DBG ("found outer method %a begin=%x end=%x\n", Name, k, k + size);
        offset = write_size (k, dsdt, len, shift);  //size corrected to sizeoffset at address j
        //       shift += offset;
        len += offset;
      } //else not an outer method
      break;
    }
  }
  return len;
}

//return final length of dsdt
UINT32
CorrectOuters (
  UINT8 *dsdt,
  UINT32 len,
  UINT32 adr,
  INT32 shift
)
{
  INT32 i, j, k;
  UINT32 size = 0;
  INT32 offset = 0;
  UINT32 SBSIZE = 0, SBADR = 0;
  BOOLEAN SBFound;

  SBFound = FALSE;
  if (shift == 0) {
    return len;
  }
  i = adr;  //usually adr = @5B - 1 = sizefield - 3
  while (i > 0x20) {  //find devices that previous to adr
    //check device
    k = i + 2;
    if ((dsdt[i] == 0x5B) && (dsdt[i + 1] == 0x82) && !CmpNum (dsdt, i, TRUE)) {  //device candidate      
      size = get_size (dsdt, k);
      if (size) {
        if ((k + size) > adr + 4) { //Yes - it is outer
          //          DBG("found outer device begin=%x end=%x\n", k, k+size);
          offset = write_size (k, dsdt, len, shift);  //size corrected to sizeoffset at address j
          shift += offset;
          len += offset;
        } //else not an outer device          
      } //else wrong size field - not a device
    } //else not a device
    // check scope
    SBSIZE = 0;
    if (dsdt[i] == '_' && dsdt[i + 1] == 'S' && dsdt[i + 2] == 'B' && dsdt[i + 3] == '_') {
      for (j = 0; j < 10; j++) {
        if (dsdt[i-j] != 0x10) {
          continue;
        }
        if (!CmpNum (dsdt, i - j, TRUE)) {
          SBADR = i - j + 1;
          SBSIZE = get_size (dsdt, SBADR);
          if ((SBSIZE != 0) && (SBSIZE < len)) {  //if zero or too large then search more
            //if found
            k = SBADR - 6;
            if ((SBADR + SBSIZE) > adr + 4) { //Yes - it is outer
              offset = write_size (SBADR, dsdt, len, shift);
              shift += offset;
              len += offset;
              SBFound = TRUE;
              break;  //SB found
            } //else not an outer scope
          }
        }
      }
    } //else not a scope
    if (SBFound) {
      break;
    }
    i = k - 3;  //if found then search again from found
  }
  return len;
}

UINT32
FixAny (
  UINT8 *dsdt,
  UINT32 len,
  UINT8 *ToFind,
  UINT32 LenTF,
  UINT8 *ToReplace,
  UINT32 LenTR
)
{
  INT32 sizeoffset, adr;
  UINT32 i;
  BOOLEAN found = FALSE;

  if (!ToFind) {
    return len;
  }
  if ((LenTF + 20) > len) {
    DBG ("FixAny:  the patch is too large!\n");
    return len;
  }
  sizeoffset = LenTR - LenTF;

  for (i = 20; i < len; i++) {
    adr = FindBin (dsdt + i, len, ToFind, LenTF);
    if (adr < 0) {
      if (!found) {
        DBG ("FixAny:  bin not found\n");
      }
      return len;
    }
    DBG ("FixAny:  patched at %x\n", (i + adr));
    found = TRUE;
    i += adr;
    len = move_data (i, dsdt, len, sizeoffset);
    if ((LenTR > 0) && (ToReplace != NULL)) {
      CopyMem (dsdt + i, ToReplace, LenTR);
    }
#if 0
    len = CorrectOuterMethod (dsdt, len, i - 2, sizeoffset);
    len = CorrectOuters (dsdt, len, i - 3, sizeoffset);
#endif
    i += LenTR;
  }

  return len;
}

VOID
FixRegions (
  UINT8 *dsdt,
  UINT32 len
)
{
  UINTN         i, j;
  INTN          shift;
  CHAR8         Name[8];
  CHAR8         NameAdr[8];
  OPER_REGION   *p;

  //  OperationRegion (GNVS, SystemMemory, 0xDE2E9E18, 0x01CD)
  //  5B 80 47 4E 56 53 00  0C 18 9E 2E DE  0B CD 01
  //or
  //  Name (RAMB, 0xDD991188)
  //  OperationRegion (\RAMW, SystemMemory, RAMB, 0x00010000)
  //  08 52 41 4D 42   0C 88 11 99 DD 
  //  5B 80 52 41 4D 57 00   52 41 4D 42   0C 00 00 01 00 
  
  if (!gRegions) {
    return;
  }
  for (i = 0x20; i < len - 15; i++) {
    if ((dsdt[i] == 0x5B) && (dsdt[i+1] == 0x80) && GetName(dsdt, (INT32)(i+2), &Name[0], &shift)) {
      //this is region. Compare to bios tables
      p = gRegions;
      while (p)  {
        if (AsciiStrStr(p->Name, Name) != NULL) {
          //apply patch
          if (p->Address != 0) {
            if (dsdt[i+7+shift] == 0x0C) {
              CopyMem(&dsdt[i+8+shift], &p->Address, 4);
            } else if (dsdt[i+7+shift] == 0x0B) {
              CopyMem(&dsdt[i+8+shift], &p->Address, 2);
            } else {
              //propose this is indirect name
              if (GetName(dsdt, (INT32)(i+7+shift), &NameAdr[0], NULL)) {
                j = FindName(dsdt, len, &NameAdr[0]);
                if (j > 0) {
                  DBG(" FixRegions: indirect name = %a\n", NameAdr);
                  if (dsdt[j+4] == 0x0C) {
                    CopyMem(&dsdt[j+5], &p->Address, 4);
                  } else if (dsdt[j+4] == 0x0B) {
                    CopyMem(&dsdt[j+5], &p->Address, 2);
                  } else {
                    DBG(" FixRegions: ... value not defined\n");
                  }
                }
              }
            }
            DBG(" FixRegions: OperationRegion (%a...) corrected to addr = 0x%x\n", Name, p->Address);
          }
          break;
        }
        p = p->next;
      }
    }
  }
}

VOID
GetBiosRegions (
  UINT8* buffer
)
{
  EFI_ACPI_DESCRIPTION_HEADER *TableHeader;
  UINT32                      bufferLen = 0;
  UINTN                       i, j;
  INTN                        shift;
  OPER_REGION                 *tmpRegion;
  CHAR8                       Name[8];
  CHAR8                       NameAdr[8];
  
  gRegions = NULL;
  TableHeader = (EFI_ACPI_DESCRIPTION_HEADER*) buffer;
  bufferLen = TableHeader->Length;
  
  for (i=0x24; i<bufferLen-15; i++) {
    if ((buffer[i] == 0x5B) && (buffer[i+1] == 0x80) &&
        GetName(buffer, (INT32)(i+2), &Name[0], &shift)) {
      if (buffer[i+6+shift] == 0) {
        //this is SystemMemory region. Write to bios regions tables
        tmpRegion = gRegions;
        gRegions = AllocateZeroPool(sizeof(OPER_REGION));
        CopyMem(&gRegions->Name[0], &buffer[i+2+shift], 4);
        gRegions->Name[4] = 0;
        if (buffer[i+7+shift] == 0x0C) {
          CopyMem(&gRegions->Address, &buffer[i+8+shift], 4);
        } else if (buffer[i+7+shift] == 0x0B) {
          CopyMem(&gRegions->Address, &buffer[i+8+shift], 2);
        } else {
          if (GetName(buffer, (INT32)(i+7+shift), &NameAdr[0], NULL)) {
            DBG (" GetBiosRegions:  name = %a indirect to %a\n", Name, NameAdr);
            j = FindName(buffer, bufferLen, &NameAdr[0]);
            DBG (" GetBiosRegions:  indirect name = 0x08%a found at 0x%x\n", NameAdr, j);
            if (j > 0) {
              if (buffer[j+4] == 0x0C) {
                CopyMem(&gRegions->Address, &buffer[j+5], 4);
              } else if (buffer[j+4] == 0x0B) {
                CopyMem(&gRegions->Address, &buffer[j+5], 2);
              }          
              DBG (" GetBiosRegions:  indirect addr = %x\n", gRegions->Address);
            }
          }
        }
        DBG (" GetBiosRegions: Found OperationRegion(%a, SystemMemory, %x, ...)\n",
             gRegions->Name,
             gRegions->Address);
        gRegions->next = tmpRegion;
      }      
    }
  }
}

