/** @file
  Industry Standard Definitions of SMBIOS Table Specification v2.6.1

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

//Slice - why it separated from EDK? Because there is 2.7 while I want to have 2.6
//also Apple's definitions included

/*
 * Moved osx specifics to separate file (nms)
 */

#ifndef __OSXSMBIOS_H__
#define __OSXSMBIOS_H__

#pragma pack(1)

enum {
  FW_REGION_RESERVED   = 0,
  FW_REGION_RECOVERY   = 1,
  FW_REGION_MAIN       = 2,
  FW_REGION_NVRAM      = 3,
  FW_REGION_CONFIG     = 4,
  FW_REGION_DIAGVAULT  = 5,
  NUM_FLASHMAP_ENTRIES = 8
};

typedef struct {
  UINT32   StartAddress;
  UINT32   EndAddress;
} FW_REGION_INFO;

///
/// kSMBTypeFirmwareVolume
///
typedef struct {
  SMBIOS_STRUCTURE              Hdr;
  UINT8                         RegionCount;
  UINT8                         Reserved[3];
  UINT32                        FirmwareFeatures;
  UINT32                        FirmwareFeaturesMask;
  UINT8                         RegionType[NUM_FLASHMAP_ENTRIES];
  FW_REGION_INFO                FlashMap[NUM_FLASHMAP_ENTRIES];
} SMBIOS_TABLE_TYPE128;

///
/// kSMBTypeMemorySPD - as read 128 bytes from SMBus to memInfoData
///
typedef struct {
  SMBIOS_STRUCTURE              Hdr;
  UINT16                        Type17Handle;
  UINT16                        Offset;
  UINT16                        Size;
  UINT16                        Data[1];
} SMBIOS_TABLE_TYPE130;

///
/// kSMBTypeOemProcessorType
///
typedef struct {
  SMBIOS_STRUCTURE              Hdr;
  UINT16                        ProcessorType;
} SMBIOS_TABLE_TYPE131;

///
/// kSMBTypeOemProcessorBusSpeed
///
typedef struct { 
  SMBIOS_STRUCTURE              Hdr;
  UINT16                        ProcessorBusSpeed;
} SMBIOS_TABLE_TYPE132;

///
/// Union of all the possible OSXSMBIOS record types.
///

typedef union {
  SMBIOS_TABLE_TYPE128  *Type128;
  SMBIOS_TABLE_TYPE130  *Type130;
  SMBIOS_TABLE_TYPE131  *Type131;
  SMBIOS_TABLE_TYPE132  *Type132;
} OSXSMBIOS_STRUCTURE_POINTER;

#pragma pack()

#endif
