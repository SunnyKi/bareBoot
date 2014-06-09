/* $Id: macosx.h $ */
/** @file
 * macosx.h
 */
#ifndef _MACOSX_H_
#define _MACOSX_H_

#include <Uefi.h>
#include <FrameworkDxe.h>

#include <Guid/Acpi.h>
#include <Guid/EventGroup.h>
#include <Guid/SmBios.h>
#include <Guid/Mps.h>
#include <Guid/FileInfo.h>

#include <Protocol/Cpu.h>
#include <Protocol/CpuIo.h>
#include <Protocol/Smbios.h>
#include <Protocol/DataHub.h>
#include <Protocol/PciIo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/UnicodeCollation.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/HighPrecisionEventTimerTable.h>
#include <IndustryStandard/Acpi10.h>
#include <IndustryStandard/Acpi20.h>
#if 0
#include <IndustryStandard/Acpi30.h>
#include <IndustryStandard/Acpi40.h>
#endif

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>

#include <debug.h>

#define offsetof(st, m) ((UINTN) ( (UINT8 *)&((st *)(0))->m - (UINT8 *)0 ))
#define MAX_NUM_DEVICES 64

#define IS_COMMA(a)         ((a) == L',')
#define IS_HYPHEN(a)        ((a) == L'-')
#define IS_DOT(a)           ((a) == L'.')
#define IS_LEFT_PARENTH(a)  ((a) == L'(')
#define IS_RIGHT_PARENTH(a) ((a) == L')')
#define IS_SLASH(a)         ((a) == L'/')
#define IS_NULL(a)          ((a) == L'\0')

#define IS_DIGIT(a)         ((a) >= '0' && (a) <= '9')
#define IS_HEX(a)           ((a) >= 'a' && (a) <= 'f')
#define IS_LOWER(a)         ((a) >= 'a' && (a) <= 'z')
#define IS_UPPER(a)         ((a) >= 'A' && (a) <= 'Z')
#define IS_ALPHA(x)         (IS_LOWER(x) || IS_UPPER(x))
#define IS_ASCII(x)         ((x) >= 0x20 && (x) <= 0x7F)
#define IS_PUNCT(x)         ((x) == '.' || (x) == '-')

#define EFI_SYSTEM_TABLE_MAX_ADDRESS 0xFFFFFFFF
#define ROUND_PAGE(x)  ((((unsigned)(x)) + EFI_PAGE_SIZE - 1) & ~(EFI_PAGE_SIZE - 1))

#define CPUID_MAX 9
#define EFI_CPU_DATA_MAXIMUM_LENGTH 0x100
//
#define MAX_RAM_SLOTS 8
#define MAX_FILE_SIZE (1024*1024*1024)
/* CPU Cache */
#define MAX_CACHE_COUNT  4
#define CPU_CACHE_LEVEL  3
//
/* PCI */
#define PCI_BASE_ADDRESS_0          0x10    /* 32 bits */
#define PCI_BASE_ADDRESS_1          0x14    /* 32 bits [htype 0,1 only] */
#define PCI_BASE_ADDRESS_2          0x18    /* 32 bits [htype 0 only] */
#define PCI_BASE_ADDRESS_3          0x1c    /* 32 bits */
#define PCI_BASE_ADDRESS_4          0x20    /* 32 bits */
#define PCI_BASE_ADDRESS_5          0x24    /* 32 bits */

#define PCI_CLASS_MEDIA_HDA         0x03

#define GEN_PMCON_1                 0xA0

#define PCIADDR(bus, dev, func) ((1 << 31) | ((bus) << 16) | ((dev) << 11) | ((func) << 8))

#define REG8(base, reg)  ((volatile UINT8 *)(UINTN)base)[(reg)]
#define REG16(base, reg)  ((volatile UINT16 *)(UINTN)base)[(reg) >> 1]
#define REG32(base, reg)  ((volatile UINT32 *)(UINTN)base)[(reg) >> 2]
#define WRITEREG32(base, reg, value) REG32(base, reg) = value

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER   Header;
  UINT32            Entry;
} RSDT_TABLE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER   Header;
  UINT64            Entry;
} XSDT_TABLE;

#pragma pack()

typedef struct {
  EFI_STATUS          LastStatus;
  EFI_FILE            *DirHandle;
  BOOLEAN             CloseDirHandle;
  EFI_FILE_INFO       *LastFileInfo;
} DIR_ITER;

typedef struct {
  UINT8   Type;
  UINT32  ModuleSize;
  UINT32  Frequency;
  CHAR8*  Vendor;
  CHAR8*  PartNo;
  CHAR8*  SerialNo;
  UINT8   *spd;
  BOOLEAN InUse;
  UINT16  SpdSize;
} RAM_SLOT_INFO;

typedef struct {
  BOOLEAN   SpdDetected;
  UINT8     MaxMemorySlots;         // number of memory slots polulated by SMBIOS
  UINT16    MemoryModules;          // number of memory modules installed
  UINT16    Map[MAX_RAM_SLOTS];     // Information and SPD mapping for each slot
  RAM_SLOT_INFO DIMM[MAX_RAM_SLOTS];
} MEM_STRUCTURE;

typedef struct {          //gSettings.cMemDevice
  BOOLEAN      InUse;
  UINT8        MemoryType;
  UINT16       Speed;
  UINT16       Size;
  CHAR8*       DeviceLocator;
  CHAR8*       BankLocator;
  CHAR8*       Manufacturer;
  CHAR8*       SerialNumber;
  CHAR8*       PartNumber;
} CUSTOM_SMBIOS_TYPE17;

typedef struct ACPI_DROP_TABLE ACPI_DROP_TABLE;
struct ACPI_DROP_TABLE
{
  ACPI_DROP_TABLE *Next;
  UINT32          Signature;
  UINT32          align;
  UINT64          TableId;
};

typedef struct {
  // SMBIOS TYPE0
  CHAR8 VendorName[64];
  CHAR8 RomVersion[64];
  CHAR8 ReleaseDate[64];
  // SMBIOS TYPE1
  CHAR8 ManufactureName[64];
  CHAR8 ProductName[64];
  CHAR8 VersionNr[64];
  CHAR8 SerialNr[64];
  CHAR8 FamilyName[64];
#if 0
  CHAR8 OEMProduct[64];
#endif
  // SMBIOS TYPE2
  CHAR8 BoardManufactureName[64];
  CHAR8 BoardSerialNumber[64];
  CHAR8 BoardNumber[64]; //Board-ID
  CHAR8 LocationInChassis[64];
  CHAR8 BoardVersion[64];
  // SMBIOS TYPE3
  BOOLEAN Mobile;
  CHAR8 ChassisManufacturer[64];
  CHAR8 ChassisAssetTag[64];
  // SMBIOS TYPE4
  UINT64  CPUFrequency;
  UINT64  FSBFrequency; 
  // SMBIOS TYPE131
  UINT16  CpuType;
  UINT32  ProcessorInterconnectSpeed; //Hz
  // OS parameters
  CHAR8   Language[10];
  CHAR8   BootArgs[256];
  CHAR16  DefaultBoot[40];
  UINT16  BootTimeout;
  UINT32  ScreenMode;
  BOOLEAN CheckFakeSMC;
  //ACPI
  UINT64  ResetAddr;
  UINT8   ResetVal;
  BOOLEAN DropSSDT;
  BOOLEAN DropDMAR;
  UINT8   PMProfile;
  BOOLEAN FixRegions;
  // Graphics
  BOOLEAN GraphicsInjector;
  BOOLEAN LoadVBios;
  CHAR16  FBName[16];
  UINT16  VideoPorts;
  UINT16  DualLink;
  UINT64  VRAM;
  UINT8   Dcfg[8];
  UINT8   NVCAP[20];
  UINT8   *CustomEDID;
  // PCI injections
  UINTN   MacAddrLen;
  UINT16  PCIRootUID;
  UINT16  HDALayoutId;
  UINT8   *EthMacAddr;
  BOOLEAN ETHInjection;
  BOOLEAN USBInjection;
  BOOLEAN SPDScan;
  // Kexts Patches
  BOOLEAN KPKernelPatchesNeeded;
  UINTN   AnyKernelDataLen[100];
  CHAR8   *AnyKernelData[100];
  CHAR8   *AnyKernelPatch[100];
  UINT32   NrKernel;
  // Kernel Patches
  BOOLEAN KPKextPatchesNeeded;
  CHAR8   *AnyKext[100];
  BOOLEAN AnyKextInfoPlistPatch[100];
  UINTN   AnyKextDataLen[100];
  CHAR8   *AnyKextData[100];
  CHAR8   *AnyKextPatch[100];
  UINT32   NrKexts;
  //Patch DSDT arbitrary
  UINT32 PatchDsdtNum;
  UINT8  **PatchDsdtFind;
  UINT32 *LenToFind;
  UINT8  **PatchDsdtReplace;
  UINT32 *LenToReplace;
  BOOLEAN SavePatchedDsdt;
  //Custom Memory Device (Type 17).
  CUSTOM_SMBIOS_TYPE17 cMemDevice[MAX_RAM_SLOTS];
  // Table dropping
  ACPI_DROP_TABLE *ACPIDropTables;
} SETTINGS_DATA;

typedef enum {
  Unknown,
  Ati,
  Intel,
  Nvidia
} GFX_MANUFACTERER;

typedef struct {
  CHAR8   BrandString[48];
  UINT32  Vendor;
  UINT32  Family;
  UINT32  Extfamily;
  UINT32  Model;
  UINT32  Extmodel;
  UINT32  Stepping;
  UINT32  Type;
  UINT32  Signature;
  UINT64  Features;
  UINT64  ExtFeatures;
  UINT64  MicroCode;
  UINT64  ProcessorFlag;
  UINT8   Mobile;

  UINT8   Cores;
  UINT8   Threads;
#if 0
  UINT8   ThreadsPerCache;
  BOOLEAN HTTEnabled;
#endif
  UINT32  CoresPerPackage;
  UINT32  LogicalPerPackage;

  UINT32  SubDivider;
  UINT32  MaxRatio;
  UINT64  FSBFrequency; //Hz
  UINT64  CPUFrequency; //Hz
  UINT64  TSCFrequency; //Hz
  UINT64  ProcessorInterconnectSpeed; //Hz
  UINT64  TurboMsr; //1 Core

  UINT32  CPUID[CPUID_MAX][4];

} CPU_STRUCTURE;

typedef enum {
  MacBook11,
  MacBook21,
  MacBook41,
  MacBook52,
  MacBookPro51,
  MacBookPro81,
  MacBookPro83,
  MacBookPro92,
  MacBookAir31,
  MacBookAir52,
  MacMini21,
  MacMini51,
  MacMini62,
  iMac81,
  iMac101,
  iMac111,
  iMac112,
  iMac113,
  iMac121,
  iMac122,
  iMac131,
  MacPro31,
  MacPro41,
  MacPro51,
} MACHINE_TYPES;

typedef struct {
  GFX_MANUFACTERER  Vendor;
  UINT16            DeviceID;
  UINT16            Width;
  UINT16            Height;
} GFX_PROPERTIES;

struct _oper_region {
  CHAR8 Name[8];
  UINT32 Address;
  struct _oper_region *next;
};
typedef struct _oper_region OPER_REGION;

MEM_STRUCTURE                   *gRAM;
EFI_RUNTIME_SERVICES            *gRS;
EFI_FILE_HANDLE                 gRootFHandle;
SETTINGS_DATA                   gSettings;
EFI_GUID                        gUuid;
EFI_GUID                        gSystemID;
EFI_GUID                        gPlatformUuid;
EFI_STATUS                      SystemIDStatus;
EFI_STATUS                      PlatformUuidStatus;
GFX_PROPERTIES                  gGraphics;
CPU_STRUCTURE                   gCPUStructure;
CHAR8                           *AddBootArgs;
CHAR8                           *OSVersion;
LIST_ENTRY                      gBootOptionList;
CHAR16                          *gProductNameDir2;
CHAR16                          *gProductNameDir;
CHAR16                          *gPNConfigPlist;
CHAR16                          *gPNAcpiDir;
BOOLEAN                         gPNDirExists;
BOOLEAN                         WithKexts;
VOID                            *gConfigPlist;
BOOLEAN                         gFronPage;
OPER_REGION                     *gRegions;

extern CHAR8                    *gDevProp;
extern CHAR8                    *cDevProp;
extern EFI_GUID                 gEfiAppleNvramGuid;
extern EFI_GUID                 gEfiAppleBootGuid;
extern EFI_GUID                 gEfiMiscSubClassGuid;
extern BOOLEAN                  SSSE3;
extern EFI_UNICODE_COLLATION_PROTOCOL  *gUnicodeCollation;

// ----------------------------------------
UINT32
hex2bin (
  IN CHAR8 *hex,
  OUT UINT8 *bin,
  INT32 len
);

#if 0
UINT8
hexstrtouint8 (
  CHAR8* buf
);
#endif

EFI_STATUS
StrToGuidLE (
  IN  CHAR16   *Str,
  OUT EFI_GUID *Guid
);

EFI_STATUS
EFIAPI
InitializeConsoleSim (
  IN EFI_HANDLE           ImageHandle
);

VOID
Pause (
  IN CHAR16* Message
);

BOOLEAN
FileExists (
  IN EFI_FILE *Root, IN CHAR16 *RelativePath
);

EFI_STATUS
PatchACPI (
  IN EFI_FILE *FHandle
);

EFI_FILE_INFO *
EfiLibFileInfo (
  IN EFI_FILE_HANDLE      FHand
);

EFI_STATUS
egLoadFile (
  IN EFI_FILE_HANDLE BaseDir,
  IN CHAR16 *FileName,
  OUT UINT8 **FileData,
  OUT UINTN *FileDataLength
);

EFI_STATUS
egSaveFile (
  IN EFI_FILE_HANDLE BaseDir OPTIONAL,
  IN CHAR16   *FileName,
  IN UINT8    *FileData,
  IN UINTN    FileDataLength
);

EFI_STATUS
SaveBooterLog (
  IN EFI_FILE_HANDLE BaseDir,
  IN CHAR16 *FileName
);

EFI_STATUS
GetBootDefault(
  IN EFI_FILE *RootFileHandle
);

VOID
GetDefaultSettings (
  VOID
);

EFI_STATUS
GetUserSettings (
  VOID
);

VOID
GetCpuProps (
  VOID
);

VOID
SetDevices (
  VOID
);

VOID
PatchSmbios (
  VOID
);

VOID
ScanSPD (
  VOID
);

EFI_STATUS
SetupDataForOSX (
  VOID
);

EFI_STATUS
SetVariablesForOSX (
  VOID
);

EFI_STATUS
EFIAPI
SetPrivateVarProto (
  VOID
);

EFI_STATUS
EFIAPI
EventsInitialize (
  VOID
  );

EFI_STATUS
GetOSVersion(
  IN EFI_FILE *FileHandle
);

EFI_STATUS
GetOptionalStringByIndex (
  IN      CHAR8                   *OptionalStrStart,
  IN      UINT8                   Index,
  OUT     CHAR16                  **String
);

VOID*
LoadPListFile (
  IN EFI_FILE *RootFileHandle,
  IN CHAR16* XmlPlistPath
);

BOOLEAN
GetUnicodeProperty (
  VOID* dict,
  CHAR8* key,
  CHAR16* uptr
);

EFI_STATUS
InitializeUnicodeCollationProtocol (
  VOID
);

VOID
DirIterOpen (
  IN EFI_FILE *BaseDir,
  IN CHAR16 *RelativePath OPTIONAL,
  OUT DIR_ITER *DirIter
);

BOOLEAN
DirIterNext (
  IN OUT DIR_ITER *DirIter,
  IN UINTN FilterMode,
  IN CHAR16 *FilePattern OPTIONAL,
  OUT EFI_FILE_INFO **DirEntry
);

EFI_STATUS
DirIterClose (
  IN OUT DIR_ITER *DirIter
);

EFI_STATUS
EFIAPI
LogDataHub (
  EFI_GUID         *TypeGuid,
  CHAR16           *Name,
  VOID             *Data,
  UINT32           DataSize
);

BOOLEAN
LoadKexts (
  VOID
);

VOID
EFIAPI
KernelBooterExtensionsPatch (
  IN UINT8 *Kernel
);

EFI_STATUS
InjectKexts (
  IN UINT32 deviceTreeP,
  IN UINT32* deviceTreeLength
);

VOID
DumpGcdMemoryMap (
  VOID
);

UINT32
FixAny (
  UINT8* dsdt,
  UINT32 len,
  UINT8* ToFind,
  UINT32 LenTF,
  UINT8* ToReplace,
  UINT32 LenTR
);

VOID
GetBiosRegions (
  UINT8* buffer
);

VOID
FixRegions (
  UINT8 *dsdt,
  UINT32 len
);

#endif
