/* $Id: Console.c $ */
/** @file
 * Console.c - VirtualBox Console control emulation
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

#include "macosx.h"

BOOLEAN
EfiGrowBuffer (
  IN OUT EFI_STATUS   *Status,
  IN OUT VOID         **Buffer,
  IN UINTN            BufferSize
  );

extern  UINT64  TurboMsr;

CHAR8*  DefaultMemEntry = "N/A";
CHAR8*  DefaultSerial = "CT288GT9VT6";
CHAR8*  BiosVendor = "Apple Inc.";
CHAR8*  AppleManufacturer = "Apple Computer, Inc.";

CHAR8* AppleFirmwareVersion[24] = {
  "MB11.88Z.0061.B03.0809221748",
  "MB21.88Z.00A5.B07.0706270922",
  "MB41.88Z.0073.B00.0809221748",
  "MB52.88Z.0088.B05.0809221748",
  "MBP51.88Z.007E.B06.0906151647",
  "MBP81.88Z.0047.B27.1102071707",
  "MBP83.88Z.0047.B24.1110261426",
  "MBP91.88Z.00D3.B08.1205101904",
  "MBA31.88Z.0061.B07.0712201139",
  "MBA51.88Z.00EF.B01.1205221442",
  "MM21.88Z.009A.B00.0706281359",
  "MM51.88Z.0077.B10.1102291410",
  "MM61.88Z.0106.B00.1208091121",
  "IM81.88Z.00C1.B00.0803051705",
  "IM101.88Z.00CC.B00.0909031926",
  "IM111.88Z.0034.B02.1003171314",
  "IM112.88Z.0057.B01.0802091538",
  "IM113.88Z.0057.B01.1005031455",
  "IM121.88Z.0047.B1F.1201241648",
  "IM122.88Z.0047.B1F.1223021110",
  "IM131.88Z.010A.B00.1209042338",
  "MP31.88Z.006C.B05.0802291410",
  "MP41.88Z.0081.B04.0903051113",
  "MP51.88Z.007F.B00.1008031144"

};

CHAR8* AppleBoardID[24] = {
  "Mac-F4208CC8", //MB11 - yonah
  "Mac-F4208CA9",  //MB21 - merom 05/07
  "Mac-F22788A9",  //MB41 - penryn
  "Mac-F22788AA",  //MB52
  "Mac-F42D86C8",  //MBP51
  "Mac-94245B3640C91C81",  //MBP81 - i5 SB IntelHD3000
  "Mac-942459F5819B171B",  //MBP83 - i7 SB  ATI
  "Mac-6F01561E16C75D06",  //MBP92 - i5-3210M IvyBridge HD4000
  "Mac-942452F5819B1C1B",  //MBA31
  "Mac-2E6FAB96566FE58C",  //MBA52 - i5-3427U IVY BRIDGE IntelHD4000 did=166
  "Mac-F4208EAA",          //MM21 - merom GMA950 07/07
  "Mac-8ED6AF5B48C039E1",  //MM51 - Sandy + Intel 30000
  "Mac-F65AE981FFA204ED",  //MM62 - Ivy
  "Mac-F227BEC8",  //IM81 - merom 01/09
  "Mac-F2268CC8",  //IM101 - wolfdale? E7600 01/
  "Mac-F2268DAE",  //IM111 - Nehalem
  "Mac-F2238AC8",  //IM112 - Clarkdale
  "Mac-F2238BAE",  //IM113 - lynnfield
  "Mac-942B5BF58194151B",  //IM121 - i5-2500 - sandy
  "Mac-942B59F58194171B",  //IM122 - i7-2600
  "Mac-00BE6ED71E35EB86",  //IM131 - -i5-3470S -IVY
  "Mac-F2268DC8",  //MP31 - xeon quad 02/09 conroe
  "Mac-F4238CC8",  //MP41 - xeon wolfdale
  "Mac-F221BEC8"   //MP51 - Xeon Nehalem 4 cores
};

CHAR8* AppleReleaseDate[24] = {
  "09/22/08",  //mb11
  "06/27/07",
  "09/22/08",
  "01/21/09",
  "06/15/09",  //mbp51
  "02/07/11",
  "10/26/11",
  "05/10/2012", //MBP92
  "12/20/07",
  "05/22/2012", //mba52
  "08/07/07",  //mm21
  "02/29/11",  //MM51
  "08/09/2012", //MM62
  "03/05/08",
  "09/03/09",  //im101
  "03/17/10",
  "03/17/10",  //11,2
  "05/03/10",
  "01/24/12",  //121 120124
  "02/23/12",  //122
  "09/04/2012",  //131
  "02/29/08",
  "03/05/09",
  "08/03/10"
};

CHAR8* AppleProductName[24] = {
  "MacBook1,1",
  "MacBook2,1",
  "MacBook4,1",
  "MacBook5,2",
  "MacBookPro5,1",
  "MacBookPro8,1",
  "MacBookPro8,3",
  "MacBookPro9,2",
  "MacBookAir3,1",
  "MacBookAir5,2",
  "Macmini2,1",
  "Macmini5,1",
  "Macmini6,2",
  "iMac8,1",
  "iMac10,1",
  "iMac11,1",
  "iMac11,2",
  "iMac11,3",
  "iMac12,1",
  "iMac12,2",
  "iMac13,1",
  "MacPro3,1",
  "MacPro4,1",
  "MacPro5,1"
};

CHAR8* AppleFamilies[24] = {
  "MacBook",
  "MacBook",
  "MacBook",
  "MacBook",
  "MacBookPro",
  "MacBookPro",
  "MacBookPro",
  "MacBook Pro",
  "MacBookAir",
  "MacBook Air",
  "Macmini",
  "Mac mini",
  "Macmini",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "iMac",
  "MacPro",
  "MacPro",
  "MacPro"
};

CHAR8* AppleSystemVersion[24] = {
  "1.1",
  "1.2",
  "1.3",
  "1.3",
  "1.0",
  "1.0",
  "1.0",
  "1.0",
  "1.0",
  "1.0",
  "1.1",
  "1.0", //MM51
  "1.0",
  "1.3",
  "1.0",
  "1.0",
  "1.2",
  "1.0",
  "1.9",
  "1.9",
  "1.0",
  "1.3",
  "1.4",
  "1.2"
};

CHAR8* AppleSerialNumber[24] = {
  "W80A041AU9B", //MB11
  "W88A041AWGP", //MB21 - merom 05/07
  "W88A041A0P0", //MB41
  "W88AAAAA9GU", //MB52
  "W88439FE1G0", //MBP51
  "W89F9196DH2G", //MBP81 - i5 SB IntelHD3000
  "W88F9CDEDF93", //MBP83 -i7 SB  ATI
  "C02HA041DTY3", //MBP92 - i5 IvyBridge HD4000
  "W8649476DQX",  //MBA31
  "C02HA041DRVC", //MBA52 - IvyBridge
  "W88A56BYYL2",  //MM21 - merom GMA950 07/07
  "C07GA041DJD0", //MM51 - sandy
  "C07JD041DWYN", //MM62 - IVY
  "W89A00AAX88", //IM81 - merom 01/09
  "W80AA98A5PE", //IM101 - wolfdale? E7600 01/09
  "G8942B1V5PJ", //IM111 - Nehalem
  "W8034342DB7", //IM112 - Clarkdale
  "QP0312PBDNR", //IM113 - lynnfield
  "W80CF65ADHJF", //IM121 - i5-2500 - sandy
  "W88GG136DHJQ", //IM122 -i7-2600
  "C02JA041DNCT", //IM131 -i5-3470S -IVY
  "W88A77AA5J4", //MP31 - xeon quad 02/09
  "CT93051DK9Y", //MP41
  "CG154TB9WU3"  //MP51 C07J50F7F4MC
};

CHAR8* AppleChassisAsset[24] = {
  "MacBook-White",
  "MacBook-White",
  "MacBook-Black",
  "MacBook-Black",
  "MacBook-Aluminum",
  "MacBook-Aluminum",
  "MacBook-Aluminum",
  "MacBook-Aluminum",
  "Air-Enclosure",
  "Air-Enclosure",
  "Mini-Aluminum",
  "Mini-Aluminum",
  "Mini-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "iMac-Aluminum",
  "Pro-Enclosure",
  "Pro-Enclosure",
  "Pro-Enclosure"
};

CHAR8* AppleBoardSN = "C02032101R5DC771H";
CHAR8* AppleBoardLocation = "Part Component";
CHAR8* RuLang = "ru:0";

//---------------------------------------------------------------------------------

#if 0
VOID *
GetDataSetting (
  IN TagPtr dict,
  IN CHAR8 *propName,
  OUT UINTN *dataLen
  )
{
  TagPtr  prop;
  UINT8   *data = NULL;
  UINT32   len;
#if 0
  UINTN   i;
#endif
  prop = GetProperty (dict, propName);

  if (prop) {
    if (prop->data != NULL && prop->dataLen > 0) {
      // data property
      data = AllocateZeroPool (prop->dataLen);
      CopyMem (data, prop->data, prop->dataLen);

      if (dataLen != NULL) {
        *dataLen = prop->dataLen;
      }

#if 0
      DBG("Data: %p, Len: %d = ", data, prop->dataLen);
      for (i = 0; i < prop->dataLen; i++) DBG("%02x ", data[i]);
      DBG("\n");
#endif
    } else {
      // assume data in hex encoded string property
      len = (UINT32) (AsciiStrLen (prop->string) >> 1);       // 2 chars per byte
      data = AllocateZeroPool (len);
      len = hex2bin (prop->string, data, len);

      if (dataLen != NULL) {
        *dataLen = len;
      }

#if 0
      DBG("Data(str): %p, Len: %d = ", data, len);
      for (i = 0; i < len; i++) DBG("%02x ", data[i]);
      DBG("\n");
#endif
    }
  }

  return data;
}
#endif

EFI_STATUS
StrToBuf (
  OUT UINT8    *Buf,
  IN  UINTN    BufferLength,
  IN  CHAR16   *Str
)
{
  UINTN       Index;
  UINTN       StrLength;
  UINT8       Digit;
  UINT8       Byte;

  Digit = 0;
  //
  // Two hex char make up one byte
  //
  StrLength = BufferLength * sizeof (CHAR16);

  for (Index = 0; Index < StrLength; Index++, Str++) {
    if ((*Str >= L'a') && (*Str <= L'f')) {
      Digit = (UINT8) (*Str - L'a' + 0x0A);
    } else if ((*Str >= L'A') && (*Str <= L'F')) {
      Digit = (UINT8) (*Str - L'A' + 0x0A);
    } else if ((*Str >= L'0') && (*Str <= L'9')) {
      Digit = (UINT8) (*Str - L'0');
    } else {
      return EFI_INVALID_PARAMETER;
    }

    //
    // For odd characters, write the upper nibble for each buffer byte,
    // and for even characters, the lower nibble.
    //
    if ((Index & 1) == 0) {
      Byte = (UINT8) (Digit << 4);
    } else {
      Byte = Buf[Index / 2];
      Byte &= 0xF0;
      Byte = (UINT8) (Byte | Digit);
    }

    Buf[Index / 2] = Byte;
  }

  return EFI_SUCCESS;
}

/**
 Converts a string to GUID value.
 Guid Format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx

 @param Str              The registry format GUID string that contains the GUID value.
 @param Guid             A pointer to the converted GUID value.

 @retval EFI_SUCCESS     The GUID string was successfully converted to the GUID value.
 @retval EFI_UNSUPPORTED The input string is not in registry format.
 @return others          Some error occurred when converting part of GUID value.

 **/

EFI_STATUS
StrToGuidLE (
  IN  CHAR16   *Str,
  OUT EFI_GUID *Guid
)
{
  UINT8 GuidLE[16];

  StrToBuf (&GuidLE[0], 4, Str);

  while (!IS_HYPHEN (*Str) && !IS_NULL (*Str)) {
    Str ++;
  }

  if (IS_HYPHEN (*Str)) {
    Str++;
  } else {
    return EFI_UNSUPPORTED;
  }

  StrToBuf (&GuidLE[4], 2, Str);

  while (!IS_HYPHEN (*Str) && !IS_NULL (*Str)) {
    Str ++;
  }

  if (IS_HYPHEN (*Str)) {
    Str++;
  } else {
    return EFI_UNSUPPORTED;
  }

  StrToBuf (&GuidLE[6], 2, Str);

  while (!IS_HYPHEN (*Str) && !IS_NULL (*Str)) {
    Str ++;
  }

  if (IS_HYPHEN (*Str)) {
    Str++;
  } else {
    return EFI_UNSUPPORTED;
  }

  StrToBuf (&GuidLE[8], 2, Str);

  while (!IS_HYPHEN (*Str) && !IS_NULL (*Str)) {
    Str ++;
  }

  if (IS_HYPHEN (*Str)) {
    Str++;
  } else {
    return EFI_UNSUPPORTED;
  }

  StrToBuf (&GuidLE[10], 6, Str);
  CopyMem ((UINT8*) Guid, &GuidLE[0], 16);
  return EFI_SUCCESS;
}

VOID
GetDefaultSettings (
  VOID
)
{
  MACHINE_TYPES   Model;

  Model             = GetDefaultModel();
  gSettings.CpuType = GetAdvancedCpuType();
  gSettings.PMProfile = 0;
  gSettings.DefaultBoot[0] = 0;
  gSettings.BusSpeed = 0;
  gSettings.CpuFreqMHz = 0; //Hz ->MHz
  gSettings.ProcessorInterconnectSpeed = 0;
  AsciiStrCpy (gSettings.VendorName,             BiosVendor);
  AsciiStrCpy (gSettings.RomVersion,             AppleFirmwareVersion[Model]);
  AsciiStrCpy (gSettings.ReleaseDate,            AppleReleaseDate[Model]);
  AsciiStrCpy (gSettings.ManufactureName,        BiosVendor);
  AsciiStrCpy (gSettings.ProductName,            AppleProductName[Model]);
  AsciiStrCpy (gSettings.VersionNr,              AppleSystemVersion[Model]);
  AsciiStrCpy (gSettings.SerialNr,               AppleSerialNumber[Model]);
  AsciiStrCpy (gSettings.FamilyName,             AppleFamilies[Model]);
  AsciiStrCpy (gSettings.BoardManufactureName,   BiosVendor);
  AsciiStrCpy (gSettings.BoardSerialNumber,      AppleBoardSN);
  AsciiStrCpy (gSettings.BoardNumber,            AppleBoardID[Model]);
  AsciiStrCpy (gSettings.BoardVersion,           AppleSystemVersion[Model]);
  AsciiStrCpy (gSettings.LocationInChassis,      AppleBoardLocation);
  AsciiStrCpy (gSettings.ChassisManufacturer,    BiosVendor);
  AsciiStrCpy (gSettings.ChassisAssetTag,        AppleChassisAsset[Model]);
}

BOOLEAN
IsHexDigit (
  CHAR8 c
)
{
  return (IS_DIGIT (c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) ? TRUE : FALSE;
}

UINT8
hexstrtouint8 (
  CHAR8* buf
)
{
  INT8 i;

  if (IS_DIGIT (buf[0])) {
    i = buf[0] - '0';
  } else if (IS_HEX (buf[0])) {
    i = buf[0] - 'a' + 10;
  } else {
    i = buf[0] - 'A' + 10;
  }

  if (AsciiStrLen (buf) == 1) {
    return i;
  }

  i <<= 4;

  if (IS_DIGIT (buf[1])) {
    i += buf[1] - '0';
  } else if (IS_HEX (buf[1])) {
    i += buf[1] - 'a' + 10;
  } else {
    i += buf[1] - 'A' + 10;  //no error checking
  }

  return i;
}

BOOLEAN
hex2bin (
  IN CHAR8 *hex,
  OUT UINT8 *bin,
  INT32 len
)
{
  CHAR8 *p;
  INT32 i;
  CHAR8 buf[3];

  if (hex == NULL || bin == NULL || len <= 0 || (INT32) AsciiStrLen (hex) != len * 2) {
    return FALSE;
  }

  buf[2] = '\0';
  p = (CHAR8 *) hex;

  for (i = 0; i < len; i++) {
    if (!IsHexDigit (p[0]) || !IsHexDigit (p[1])) {
      return FALSE;
    }

    buf[0] = *p++;
    buf[1] = *p++;
    bin[i] = hexstrtouint8 (buf);
  }

  return TRUE;
}

VOID
Pause (
  IN CHAR16* Message
)
{
  if (Message) {
    Print (L"%s", Message);
  }

  gBS->Stall (4000000);
}

BOOLEAN
FileExists (
  IN EFI_FILE *RootFileHandle,
  IN CHAR16   *RelativePath
)
{
  EFI_STATUS  Status;
  EFI_FILE    *TestFile;

  Status = RootFileHandle->Open (RootFileHandle, &TestFile, RelativePath, EFI_FILE_MODE_READ, 0);

  if (Status == EFI_SUCCESS) {
    TestFile->Close (TestFile);
    return TRUE;
  }

  return FALSE;
}

EFI_STATUS
egLoadFile (
  IN EFI_FILE_HANDLE BaseDir,
  IN CHAR16 *FileName,
  OUT UINT8 **FileData,
  OUT UINTN *FileDataLength
)
{
  EFI_STATUS          Status;
  EFI_FILE_HANDLE     FileHandle;
  EFI_FILE_INFO       *FileInfo;
  UINT64              ReadSize;
  UINTN               BufferSize;
  UINT8               *Buffer;

  Status = BaseDir->Open (BaseDir, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  FileInfo = EfiLibFileInfo (FileHandle);

  if (FileInfo == NULL) {
    FileHandle->Close (FileHandle);
    return EFI_NOT_FOUND;
  }

  ReadSize = FileInfo->FileSize;

  if (ReadSize > MAX_FILE_SIZE) {
    ReadSize = MAX_FILE_SIZE;
  }

  FreePool (FileInfo);
  BufferSize = (UINTN) ReadSize;   // was limited to 1 GB above, so this is safe
  Buffer = (UINT8 *) AllocateAlignedPages (EFI_SIZE_TO_PAGES (BufferSize), 16);

  if (Buffer == NULL) {
    FileHandle->Close (FileHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = FileHandle->Read (FileHandle, &BufferSize, Buffer);
  FileHandle->Close (FileHandle);

  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return Status;
  }

  *FileData = Buffer;
  *FileDataLength = BufferSize;
  return EFI_SUCCESS;
}

VOID
WaitForSts (
  VOID
)
{
  UINT32 inline_timeout;

  inline_timeout = 100000;

  while (AsmReadMsr64 (MSR_IA32_PERF_STATUS) & (1 << 21)) {
    if (!inline_timeout--) {
      break;
    }
  }
}

EFI_STATUS
SaveSettings (
  VOID
)
{
  UINT64  msr;

  gMobile = gSettings.Mobile;

  if ((gSettings.BusSpeed != 0) &&
       (gSettings.BusSpeed > 10 * kilo) &&
       (gSettings.BusSpeed < 500 * kilo)) {
    gCPUStructure.ExternalClock = gSettings.BusSpeed;
    gCPUStructure.FSBFrequency = MultU64x32 (kilo, gSettings.BusSpeed); //kHz -> Hz
  }

  if ((gSettings.CpuFreqMHz != 0) &&
       (gSettings.CpuFreqMHz > 100) &&
       (gSettings.CpuFreqMHz < 20000)) {
    gCPUStructure.CurrentSpeed = gSettings.CpuFreqMHz;
  }

  if (gSettings.Turbo) {
    if (gCPUStructure.Turbo4) {
      gCPUStructure.CPUFrequency = DivU64x32 (MultU64x32 (gCPUStructure.FSBFrequency, gCPUStructure.Turbo4), 10);
    }

    if (TurboMsr != 0) {
      AsmWriteMsr64 (MSR_IA32_PERF_CONTROL, TurboMsr);
      gBS->Stall (100);
      WaitForSts();
    }

    msr = AsmReadMsr64 (MSR_IA32_PERF_STATUS);
  }

  return EFI_SUCCESS;
}

// ----============================----

UINTN
AsciiStr2Uintn (
  CHAR8* ps
)
{
  UINTN val;

  if ((ps[0] == '0')  && (ps[1] == 'x' || ps[1] == 'X')) {
    val = AsciiStrHexToUintn (ps);
  } else {
    val = AsciiStrDecimalToUintn (ps);
  }
  return val;
}

UINTN
GetNumProperty (
  TagPtr dict,
  CHAR8* key,
  UINTN def
)
{
  TagPtr dentry;

  dentry = GetProperty (dict, key);
  if (dentry != NULL) {
    def = AsciiStr2Uintn(dentry->string);
  }
  return def;
}

// Bizzare property -- FALSE or NUMERIC

typedef enum {
  xOther,
  xFalse,
  xTrue
} xBool;

xBool
AsciiStr2xBool (
  CHAR8* ps
)
{
  if (ps[0] == 'y' || ps[0] == 'Y') {
    return xTrue;
  }
  if (ps[0] == 'n' || ps[0] == 'N') {
    return xFalse;
  }
  return xOther;
}

BOOLEAN
GetBoolProperty (
  TagPtr dict,
  CHAR8* key,
  BOOLEAN def
)
{
  TagPtr dentry;

  dentry = GetProperty (dict, key);
  if (dentry != NULL) {
    switch (AsciiStr2xBool(dentry->string)) {
      case xFalse:
        return FALSE;
      case xTrue:
        return TRUE;
      default:
        return def;
    }
  }
  return def;
}

VOID
GetAsciiProperty (
  TagPtr dict,
  CHAR8* key,
  CHAR8* aptr
)
{
  TagPtr dentry;

  dentry = GetProperty (dict, key);
  if (dentry != NULL) {
    AsciiStrCpy (aptr, dentry->string);
  }
}

VOID
GetUnicodeProperty (
  TagPtr dict,
  CHAR8* key,
  CHAR16* uptr
)
{
  TagPtr dentry;

  dentry = GetProperty (dict, key);
  if (dentry != NULL) {
    AsciiStrToUnicodeStr (dentry->string, uptr);
  }
}

// ----============================----

EFI_STATUS
GetBootDefault (
  IN EFI_FILE *RootFileHandle,
  IN CHAR16* ConfigPlistPath
)
{
  EFI_STATUS  Status;
  CHAR8*      gConfigPtr;
  TagPtr      dict;
  TagPtr      dictPointer;
  UINTN       size;

  Status = EFI_NOT_FOUND;
  gConfigPtr = NULL;

  if ((RootFileHandle != NULL) && FileExists (RootFileHandle, ConfigPlistPath)) {
    Status = egLoadFile (RootFileHandle, ConfigPlistPath, (UINT8**) &gConfigPtr, &size);
  }

  if (EFI_ERROR (Status)) {
    Print (L"Error loading config.plist!\r\n");
    return Status;
  }

  dict = NULL;
  if (gConfigPtr) {
    if (ParseXML ((const CHAR8*) gConfigPtr, &dict) != EFI_SUCCESS) {
      Print (L"config error\n");
      return EFI_UNSUPPORTED;
    }
  }

  dictPointer = GetProperty (dict, "SystemParameters");

  if (dictPointer) {
    gSettings.BootTimeout = (UINT16) GetNumProperty (dictPointer, "Timeout", 0);
    GetUnicodeProperty (dictPointer, "DefaultBootVolume", gSettings.DefaultBoot);
  }

  return Status;
}

EFI_STATUS
GetUserSettings (
  IN EFI_FILE *RootFileHandle,
  IN CHAR16* ConfigPlistPath
)
{
  EFI_STATUS  Status;
  UINTN       size;
  CHAR8*      gConfigPtr;
  TagPtr      dict;
  TagPtr      dictPointer;
  TagPtr      prop;
  CHAR16      UStr[64];
  CHAR8       *BA;
  CHAR16      SystemID[40];

  Status = EFI_NOT_FOUND;
  gConfigPtr = NULL;

  if ((RootFileHandle != NULL) && FileExists (RootFileHandle, ConfigPlistPath)) {
    Status = egLoadFile (RootFileHandle, ConfigPlistPath, (UINT8**) &gConfigPtr, &size);
  }

  if (EFI_ERROR (Status)) {
    Print (L"Error loading config.plist!\r\n");
    return Status;
  }

  if (gConfigPtr) {
    if (ParseXML ((const CHAR8*) gConfigPtr, &dict) != EFI_SUCCESS) {
      Print (L"config error\n");
      return EFI_UNSUPPORTED;
    }

    ZeroMem (gSettings.Language, 10);
    ZeroMem (gSettings.BootArgs, 120);
    dictPointer = GetProperty (dict, "SystemParameters");

    if (dictPointer) {
      AsciiStrCpy (gSettings.Language, RuLang);
      GetAsciiProperty (dictPointer, "prev-lang", gSettings.Language);
      prop = GetProperty (dictPointer, "boot-args");

      if (prop) {
        AsciiStrCpy (gSettings.BootArgs, prop->string);
        BA = &gSettings.BootArgs[119];
        bootArgsLen = 120;

        while ((*BA == ' ') || (*BA == 0)) {
          BA--;
          bootArgsLen--;
        }
      }

      GetUnicodeProperty (dictPointer, "DefaultBootVolume", gSettings.DefaultBoot);
      prop = GetProperty (dictPointer, "CustomUUID");

      if (prop) {
        AsciiStrToUnicodeStr (prop->string, SystemID);
        Status = StrToGuidLE (SystemID, &gUuid);
        //else value from SMBIOS
      }
    }

    ZeroMem (gSettings.SerialNr, 64);
    dictPointer = GetProperty (dict, "Graphics");

    if (dictPointer) {
      gSettings.GraphicsInjector = GetBoolProperty (dictPointer, "GraphicsInjector", TRUE);

      prop = GetProperty (dictPointer, "VRAM");

      if (prop) {
        AsciiStrToUnicodeStr (prop->string, (CHAR16*) &UStr[0]);
        gSettings.VRAM = LShiftU64 (StrDecimalToUintn ((CHAR16*) &UStr[0]), 20);      //bytes
      }

      gSettings.LoadVBios = GetBoolProperty (dictPointer, "LoadVBios", FALSE);
      prop = GetProperty (dictPointer, "VideoPorts");

      if (prop) {
        AsciiStrToUnicodeStr (prop->string, (CHAR16*) &UStr[0]);
        gSettings.VideoPorts = (UINT16) StrDecimalToUintn ((CHAR16*) &UStr[0]);
      }

      GetUnicodeProperty (dictPointer, "FBName", gSettings.FBName);
      prop = GetProperty (dictPointer, "NVCAP");

      if (prop) {
        hex2bin (prop->string, (UINT8*) &gSettings.NVCAP[0], 20);
      }

      prop = GetProperty (dictPointer, "DisplayCfg");

      if (prop) {
        hex2bin (prop->string, (UINT8*) &gSettings.Dcfg[0], 8);
      }

#if 0
      prop = GetProperty (dictPointer, "CustomEDID");

      if (prop) {
        UINTN j = 128;
        gSettings.CustomEDID = GetDataSetting (dictPointer, "CustomEDID", &j);
      }
#endif
    }

    dictPointer = GetProperty (dict, "PCI");

    if (dictPointer) {
#if 0
      gSettings.PCIRootUID = 0;
      prop = GetProperty (dictPointer, "PCIRootUID");

      if (prop) {
        AsciiStrToUnicodeStr (prop->string, (CHAR16*) &UStr[0]);
        gSettings.PCIRootUID = (UINT16) StrDecimalToUintn ((CHAR16*) &UStr[0]);
      }
#endif
      gSettings.PCIRootUID = (UINT16) GetNumProperty (dictPointer, "PCIRootUID", 0);

      prop = GetProperty (dictPointer, "DeviceProperties");

      if (prop) {
        cDevProp = AllocateZeroPool (AsciiStrLen (prop->string) + 1);
        AsciiStrCpy (cDevProp, prop->string);
      }

#if 0
      gSettings.CustomDevProp = GetBoolProperty (dictPointer, "CustomDevProp", FALSE);
#endif

      gSettings.ETHInjection = GetBoolProperty (dictPointer, "ETHInjection", FALSE);
      gSettings.USBInjection = GetBoolProperty (dictPointer, "USBInjection", FALSE);
#if 0
      gSettings.HDAInjection = FALSE;
      gSettings.HDALayoutId = 0;
#endif
      gSettings.HDALayoutId = (UINT16) GetNumProperty (dictPointer, "HDAInjection", 0);
#if 0
      prop = GetProperty (dictPointer, "HDAInjection");
      if (prop != NULL) {
        switch (AsciiStr2xBool (prop->string)) {
          case xOther:
            gSettings.HDAInjection = TRUE;
            gSettings.HDALayoutId = AsciiStr2Uintn (prop->string);
            break;
            
          default:
            break;
        }
      }
#endif
    }

    dictPointer = GetProperty (dict, "ACPI");

    if (dictPointer) {
      gSettings.DropSSDT = GetBoolProperty (dictPointer, "DropOemSSDT", FALSE);
#if 0
      gSettings.GeneratePStates = GetBoolProperty (dictPointer, "GeneratePStates", FALSE);
      gSettings.GenerateCStates = GetBoolProperty (dictPointer, "GenerateCStates", FALSE);
#endif
      gSettings.ResetAddr = (UINT64) GetNumProperty (dictPointer, "ResetAddress", 0);
      gSettings.ResetVal = (UINT8) GetNumProperty (dictPointer, "ResetValue", 0);
      // other known pair is 0x0[C/2]F9/0x06
#if 0
      gSettings.EnableC2 = GetBoolProperty (dictPointer, "EnableC2", FALSE);
      gSettings.EnableC4 = GetBoolProperty (dictPointer, "EnableC4", FALSE);
      gSettings.EnableC6 = GetBoolProperty (dictPointer, "EnableC6", FALSE);
      gSettings.EnableISS = GetBoolProperty (dictPointer, "EnableISS", FALSE);
#endif
      gSettings.PMProfile = (UINT8) GetNumProperty (dictPointer, "PMProfile", 0);
    }

    dictPointer = GetProperty (dict, "SMBIOS");

    if (dictPointer) {
      gSettings.Mobile = GetBoolProperty (dictPointer, "Mobile", gMobile);

      GetAsciiProperty (dictPointer, "BiosVendor", gSettings.VendorName);
      GetAsciiProperty (dictPointer, "BiosVersion", gSettings.RomVersion);
      GetAsciiProperty (dictPointer, "BiosReleaseDate", gSettings.ReleaseDate);
      GetAsciiProperty (dictPointer, "Manufacturer", gSettings.ManufactureName);
      GetAsciiProperty (dictPointer, "ProductName", gSettings.ProductName);
      GetAsciiProperty (dictPointer, "Version", gSettings.VersionNr);
      GetAsciiProperty (dictPointer, "Family", gSettings.FamilyName);
      GetAsciiProperty (dictPointer, "SerialNumber", gSettings.SerialNr);
      GetAsciiProperty (dictPointer, "BoardManufacturer", gSettings.BoardManufactureName);
      GetAsciiProperty (dictPointer, "BoardSerialNumber", gSettings.BoardSerialNumber);
      GetAsciiProperty (dictPointer, "Board-ID", gSettings.BoardNumber);
      GetAsciiProperty (dictPointer, "BoardVersion", gSettings.BoardVersion);
      GetAsciiProperty (dictPointer, "LocationInChassis", gSettings.LocationInChassis);
      GetAsciiProperty (dictPointer, "ChassisManufacturer", gSettings.ChassisManufacturer);
      GetAsciiProperty (dictPointer, "ChassisAssetTag", gSettings.ChassisAssetTag);
    }

    dictPointer = GetProperty (dict, "CPU");

    if (dictPointer) {
      gSettings.Turbo = GetBoolProperty (dictPointer, "Turbo", FALSE);
      gSettings.CpuType = (UINT16) GetNumProperty (dictPointer, "ProcessorType", GetAdvancedCpuType());
      gSettings.CpuFreqMHz = (UINT16) GetNumProperty (dictPointer, "CpuFrequencyMHz", 0);
      gSettings.BusSpeed = (UINT32) GetNumProperty (dictPointer, "BusSpeedkHz", 0);
      gSettings.ProcessorInterconnectSpeed = (UINT32) GetNumProperty (dictPointer, "QPI", 0);
    }

    SaveSettings();
  }

  return Status;
}

#if 0
EFI_STATUS
GetOSVersion (
  IN EFI_FILE *RootFileHandle,
  OUT CHAR16  *OsName
)
{
  EFI_STATUS        Status = EFI_NOT_FOUND;
  CHAR8*            plistBuffer = 0;
  UINTN             plistLen;
  TagPtr            dict = NULL;
  TagPtr            prop = NULL;
  CHAR16*           SystemPlist = L"System\\Library\\CoreServices\\SystemVersion.plist";
  CHAR16*           ServerPlist = L"System\\Library\\CoreServices\\ServerVersion.plist";

  if (!RootFileHandle) {
    return EFI_NOT_FOUND;
  }

  OsName = NULL;

  // Mac OS X
  if (FileExists (RootFileHandle, SystemPlist)) {
    Status = egLoadFile (RootFileHandle, SystemPlist, (UINT8 **) &plistBuffer, &plistLen);
  }
  // Mac OS X Server
  else if (FileExists (RootFileHandle, ServerPlist)) {
    Status = egLoadFile (RootFileHandle, ServerPlist, (UINT8 **) &plistBuffer, &plistLen);
  }

  if (!EFI_ERROR (Status)) {
    if (ParseXML (plistBuffer, &dict) != EFI_SUCCESS) {
      FreePool (plistBuffer);
      return EFI_NOT_FOUND;
    }

    prop = GetProperty (dict, "ProductVersion");

    if (prop != NULL) {
      // Tiger
      if (AsciiStrStr (prop->string, "10.4") != 0) {
        OsName = L"tiger";
        Status = EFI_SUCCESS;
      } else

        // Leopard
        if (AsciiStrStr (prop->string, "10.5") != 0) {
          OsName = L"leo";
          Status = EFI_SUCCESS;
        } else

          // Snow Leopard
          if (AsciiStrStr (prop->string, "10.6") != 0) {
            OsName = L"snow";
            Status = EFI_SUCCESS;
          } else

            // Lion
            if (AsciiStrStr (prop->string, "10.7") != 0) {
              OsName = L"lion";
              Status = EFI_SUCCESS;
            } else

              // Mountain Lion
              if (AsciiStrStr (prop->string, "10.8") != 0) {
                OsName = L"cougar";
                Status = EFI_SUCCESS;
              }
    }
  }

  return Status;
}
#endif
