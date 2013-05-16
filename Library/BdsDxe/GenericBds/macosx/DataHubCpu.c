/* $Id: Cpu.c $ */
/** @file
 */

#include "macosx.h"
#if 0
#include "DataHubRecords.h"
#else
#include <Guid/DataHubRecords.h>
#endif

EFI_GUID gEfiAppleFirmwarePasswordProtocolGuid  = {0x8FFEEB3A, 0x4C98, 0x4630, {0x80, 0x3F, 0x74, 0x0F, 0x95, 0x67, 0x09, 0x1D}};
EFI_GUID gEfiGlobalVarGuid                      = {0x8BE4DF61, 0x93CA, 0x11D2, {0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C}};
EFI_GUID AppleDevicePropertyProtocolGuid        = {0x91BD12FE, 0xF6C3, 0x44FB, {0xA5, 0xB7, 0x51, 0x22, 0xAB, 0x30, 0x3A, 0xE0}};
EFI_GUID gEfiAppleBootGuid                      = {0x7C436110, 0xAB2A, 0x4BBB, {0xA8, 0x80, 0xFE, 0x41, 0x99, 0x5C, 0x9F, 0x82}};
EFI_GUID gEfiAppleNvramGuid                     = {0x4D1EDE05, 0x38C7, 0x4A6A, {0x9C, 0xC6, 0x4B, 0xCC, 0xA8, 0xB3, 0x8C, 0x14}};
EFI_GUID gEfiAppleScreenInfoGuid                = {0xE316E100, 0x0751, 0x4C49, {0x90, 0x56, 0x48, 0x6C, 0x7E, 0x47, 0x29, 0x03}};
EFI_GUID AppleBootKeyPressProtocolGuid          = {0x5B213447, 0x6E73, 0x4901, {0xA4, 0xF1, 0xB8, 0x64, 0xF3, 0xB7, 0xA1, 0x72}};
EFI_GUID AppleNetBootProtocolGuid               = {0x78EE99FB, 0x6A5E, 0x4186, {0x97, 0xDE, 0xCD, 0x0A, 0xBA, 0x34, 0x5A, 0x74}};
EFI_GUID AppleImageCodecProtocolGuid            = {0x0DFCE9F6, 0xC4E3, 0x45EE, {0xA0, 0x6A, 0xA8, 0x61, 0x3B, 0x98, 0xA5, 0x07}};
EFI_GUID gEfiAppleVendorGuid                    = {0xAC39C713, 0x7E50, 0x423D, {0x88, 0x9D, 0x27, 0x8F, 0xCC, 0x34, 0x22, 0xB6}};
EFI_GUID gAppleEFINVRAMTRBSecureGuid            = {0xF68DA75E, 0x1B55, 0x4E70, {0xB4, 0x1B, 0xA7, 0xB7, 0xA5, 0xB7, 0x58, 0xEA}};
EFI_GUID gDataHubOptionsGuid                    = {0x0021001C, 0x3CE3, 0x41F8, {0x99, 0xC6, 0xEC, 0xF5, 0xDA, 0x75, 0x47, 0x31}};
EFI_GUID gNotifyMouseActivity                   = {0xF913C2C2, 0x5351, 0x4FDB, {0x93, 0x44, 0x70, 0xFF, 0xED, 0xB8, 0x42, 0x25}};
EFI_GUID gEfiDataHubProtocolGuid                = {0xae80d021, 0x618e, 0x11d4, {0xbc, 0xd7, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81}};
EFI_GUID gEfiMiscSubClassGuid                   = {0x772484B2, 0x7482, 0x4b91, {0x9F, 0x9A, 0xAD, 0x43, 0xF8, 0x1C, 0x58, 0x81}};
EFI_GUID gEfiProcessorSubClassGuid              = {0x26fdeb7e, 0xb8af, 0x4ccf, {0xaa, 0x97, 0x02, 0x63, 0x3c, 0xe4, 0x8c, 0xa7}};
EFI_GUID gEfiMemorySubClassGuid                 = {0x4E8F4EBB, 0x64B9, 0x4e05, {0x9B, 0x18, 0x4C, 0xFE, 0x49, 0x23, 0x50, 0x97}};
EFI_GUID gMsgLogProtocolGuid                    = {0x511CE018, 0x0018, 0x4002, {0x20, 0x12, 0x17, 0x38, 0x05, 0x01, 0x02, 0x03}};
EFI_GUID gEfiLegacy8259ProtocolGuid             = {0x38321dba, 0x4fe0, 0x4e17, {0x8a, 0xec, 0x41, 0x30, 0x55, 0xea, 0xed, 0xc1}};
EFI_GUID GPT_MSDOS_PARTITION                    = { 0xEBD0A0A2, 0xB9E5, 0x4433, { 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 }};
EFI_GUID GPT_HFSPLUS_PARTITION                  = { 0x48465300, 0x0000, 0x11AA, {0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC }};
EFI_GUID GPT_EMPTY_PARTITION                    = { 0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
EFI_GUID gDataHubPlatformGuid                   = {0x64517cc8, 0x6561, 0x4051, {0xb0, 0x3c, 0x59, 0x64, 0xb6, 0x0f, 0x4c, 0x7a}};

typedef union {
  EFI_CPU_DATA_RECORD *DataRecord;
  UINT8               *Raw;
} EFI_CPU_DATA_RECORD_BUFFER;

EFI_SUBCLASS_TYPE1_HEADER mCpuDataRecordHeader = {
  EFI_PROCESSOR_SUBCLASS_VERSION,       // Version
  sizeof (EFI_SUBCLASS_TYPE1_HEADER),   // Header Size
  0,                                    // Instance, Initialize later
  EFI_SUBCLASS_INSTANCE_NON_APPLICABLE, // SubInstance
  0                                     // RecordType, Initialize later
};

#pragma pack(1)
typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER   Hdr;      /* 0x48 */
  UINT32            NameLen;         /* 0x58 , in bytes */
  UINT32            ValLen;          /* 0x5c */
  UINT8           Data[1];         /* 0x60 Name Value */
} PLATFORM_DATA;
#pragma pack()

EFI_DATA_HUB_PROTOCOL         *mDataHub;

UINT32
CopyRecord (
  PLATFORM_DATA* Rec,
  CONST CHAR16* Name,
  VOID* Val,
  UINT32 ValLen
)
{
  CopyMem (&Rec->Hdr, &mCpuDataRecordHeader, sizeof (EFI_SUBCLASS_TYPE1_HEADER));
  Rec->NameLen = (UINT32) StrLen (Name) * sizeof (CHAR16);
  Rec->ValLen = ValLen;
  CopyMem (Rec->Data, Name, Rec->NameLen);
  CopyMem (Rec->Data + Rec->NameLen, Val, ValLen);
  return (sizeof (EFI_SUBCLASS_TYPE1_HEADER) + 8 + Rec->NameLen + Rec->ValLen);
}

EFI_STATUS
EFIAPI
LogDataHub (
  EFI_GUID         *TypeGuid,
  CHAR16           *Name,
  VOID             *Data,
  UINT32           DataSize
)
{
  UINT32                      RecordSize;
  EFI_STATUS                  Status;
  PLATFORM_DATA               *PlatformData;

  PlatformData = (PLATFORM_DATA*) AllocatePool (sizeof (PLATFORM_DATA) + DataSize + EFI_CPU_DATA_MAXIMUM_LENGTH);

  if (PlatformData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  RecordSize = CopyRecord (PlatformData, Name, Data, DataSize);
  Status = mDataHub->LogData (
             mDataHub,
             TypeGuid,                /* DataRecordGuid */
             &gDataHubPlatformGuid,   /* ProducerName */  
             EFI_DATA_RECORD_CLASS_DATA,
             PlatformData,
             RecordSize
           );
  
  ASSERT_EFI_ERROR (Status);
  FreePool (PlatformData);
  return Status;
}

EFI_STATUS
SetVariablesForOSX (
  VOID
)
{
  EFI_STATUS  Status;
  UINT32      FwFeatures;
  UINT32      FwFeaturesMask;
#if 0
  UINT16      *BootNext;
  UINT32      BackgroundClear;
  CHAR8       *None;
#endif

  FwFeatures      = 0xc0007417;
  FwFeaturesMask  = 0xc0007fff;
#if 0
  BootNext = NULL; //it already presents in EFI FW. First GetVariable ?
  BackgroundClear = 0x00000000;
  None = "none";

  Status = gRS->SetVariable(L"ROM", &gEfiAppleNvramGuid, 0, 0, NULL);

  Status = gRS->SetVariable (
                  L"BootNext",
                  &gEfiAppleNvramGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (BootNext),
                  &BootNext
                );
#endif

  Status = gRS->SetVariable (
                  L"FirmwareFeatures",
                  &gEfiAppleNvramGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (FwFeatures),
                  &FwFeatures
                );

  Status = gRS->SetVariable (
                  L"FirmwareFeaturesMask",
                  &gEfiAppleNvramGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (FwFeaturesMask),
                  &FwFeaturesMask
                );

  Status = gRS->SetVariable (
                  L"MLB",
                  &gEfiAppleNvramGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  AsciiStrLen (gSettings.BoardSerialNumber),
                  &gSettings.BoardSerialNumber
                );

  Status = gRS->SetVariable (
                  L"ROM",
                  &gEfiAppleNvramGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  gSettings.MacAddrLen,
                  gSettings.EthMacAddr
                );

  if (gSettings.Language[0] != '\0') {

    Status = gRS->SetVariable (
                    L"prev-lang:kbd",
                    &gEfiAppleBootGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrLen (gSettings.Language),
                    &gSettings.Language
                  );
  }

  if (EFI_ERROR (SystemIDStatus) &&
      !EFI_ERROR (PlatformUuidStatus)) {
    Status = gRS->SetVariable (
                    L"platform-uuid",
                    &gEfiAppleBootGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS |  EFI_VARIABLE_RUNTIME_ACCESS,
                    16,
                    &gPlatformUuid
                  );
  }

  Status = gRS->SetVariable (
                  L"boot-args",
                  &gEfiAppleBootGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  AsciiStrLen (gSettings.BootArgs),
                  &gSettings.BootArgs
                );
#if 0
  Status = gRS->SetVariable (
                  L"BackgroundClear",
                  &gEfiAppleNvramGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (BackgroundClear),
                  &BackgroundClear
                );

  Status = gRS->SetVariable (
                  L"security-mode",
                  &gEfiAppleBootGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  AsciiStrLen (None),
                  (VOID*) None
                );
#endif

  return Status;
}

EFI_STATUS
SetupDataForOSX (
  VOID
)
{
  EFI_STATUS      Status;
  UINT32          devPathSupportedVal;
#if 0
  UINT64          FrontSideBus;
  UINT64          CpuSpeed;
  UINT64          TSCFrequency;
#endif
  CHAR16*         productName;
  CHAR16*         serialNumber;

  productName     = AllocateZeroPool (64);
  serialNumber    = AllocateZeroPool (64);

  Status = gBS->LocateProtocol (&gEfiDataHubProtocolGuid, NULL, (VOID**) &mDataHub);

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

#if 0
  FrontSideBus    = gCPUStructure.FSBFrequency;
  CpuSpeed        = gCPUStructure.CPUFrequency;
  TSCFrequency    = gCPUStructure.TSCFrequency;
#endif

  devPathSupportedVal = 1;
  AsciiStrToUnicodeStr (gSettings.ProductName, productName);
  AsciiStrToUnicodeStr (gSettings.SerialNr, serialNumber);

  Status =  LogDataHub (&gEfiProcessorSubClassGuid, L"FSBFrequency", &gCPUStructure.FSBFrequency, sizeof (gCPUStructure.FSBFrequency));
#if 0
  Status =  LogDataHub (&gEfiProcessorSubClassGuid, L"TSCFrequency", &TSCFrequency, sizeof (UINT64));
  Status =  LogDataHub (&gEfiProcessorSubClassGuid, L"CPUFrequency", &CpuSpeed, sizeof (UINT64));
#endif
  Status =  LogDataHub (&gEfiMiscSubClassGuid, L"DevicePathsSupported", &devPathSupportedVal, sizeof (UINT32));
  Status =  LogDataHub (&gEfiMiscSubClassGuid, L"Model", productName, (UINT32) StrSize (productName));
  Status =  LogDataHub (&gEfiMiscSubClassGuid, L"SystemSerialNumber", serialNumber, (UINT32) StrSize (serialNumber));
  if (!EFI_ERROR (SystemIDStatus)) {
    Status =  LogDataHub(&gEfiMiscSubClassGuid, L"system-id", &gSystemID, sizeof(EFI_GUID));
    Status =  LogDataHub(&gEfiMiscSubClassGuid, L"system-id", &gSystemID, sizeof(EFI_GUID));
  } else {
    if (EFI_ERROR (PlatformUuidStatus)) {
      Status =  LogDataHub(&gEfiMiscSubClassGuid, L"system-id", &gUuid, sizeof(EFI_GUID));
      Status =  LogDataHub(&gEfiMiscSubClassGuid, L"system-id", &gUuid, sizeof(EFI_GUID));
    }
  }
  return Status;
}
