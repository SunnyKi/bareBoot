/**
 *  Module for work with runtime (RT, NVRAM) vars,
 *  determining default boot volume (Startup disk)
 *  and (kid of) persistent RT support with nvram.plist on CloverEFI.
 *  dmazar, 2012
 */

#include <macosx.h>
#include <Library/plist.h>

// for saving nvram.plist and it's data
VOID                          *gNvramDict;

/** returns given time as miliseconds.
 *  assumes 31 days per month, so it's not correct,
 *  but is enough for basic checks.
 */
UINT64
GetEfiTimeInMs (
  IN EFI_TIME *T
)
{
  UINT64              TimeMs;
  
  TimeMs = T->Year - 1900;
  // is 64bit multiply workign in 32 bit?
  TimeMs = MultU64x32(TimeMs, 12) + T->Month;
  TimeMs = MultU64x32(TimeMs, 31) + T->Day; // counting with 31 day
  TimeMs = MultU64x32(TimeMs, 24) + T->Hour;
  TimeMs = MultU64x32(TimeMs, 60) + T->Minute;
  TimeMs = MultU64x32(TimeMs, 60) + T->Second;
  TimeMs = MultU64x32(TimeMs, 1000) + DivU64x32(T->Nanosecond, 1000000);
  
  return TimeMs;
}

/** Searches all volumes for the most recent nvram.plist and loads it into gNvramDict. */
EFI_STATUS
LoadLatestNvramPlist (
  VOID
)
{
#if 0
  EFI_GUID                        *Guid;
#endif
  CHAR16                          *DPString;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  EFI_FILE_HANDLE                 FHandle;
  EFI_FILE_HANDLE                 FileHandle;
  EFI_FILE_HANDLE                 File;
  EFI_FILE_INFO                   *FileInfo;
  UINT64                          LastModifTimeMs;
  UINT64                          ModifTimeMs;
  UINTN                           NumberFileSystemHandles;
  EFI_HANDLE                      *FileSystemHandles;
  UINTN                           Index;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
  EFI_STATUS                      Status;
  
  DPString = NULL;
  FHandle = NULL;
  LastModifTimeMs = 0;
  
  DBG ("Searching volumes for latest nvram.plist ...");
  //
  // skip loading if already loaded
  //
  if (gNvramDict != NULL) {
    DBG (" already loaded\n");
    return EFI_SUCCESS;
  }
  DBG ("\n");
  
  // find latest nvram.plist
  // search all volumes
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL,
         &NumberFileSystemHandles,
         &FileSystemHandles
         );
  
  for (Index = 0; Index < NumberFileSystemHandles; Index++) {
    DevicePath  = DevicePathFromHandle (FileSystemHandles[Index]);
#if 0
    Guid = FindGPTPartitionGuidInDevicePath (DevicePath);
    DBG (" %2d. Volume '%s', GUID = %g", Index, Volume->VolName, Guid);
    if (Guid == NULL) {
      // not a GUID partition
      DBG(" - not GPT");
    }
    Print (L"%a: Device = %s\n",__FUNCTION__, ConvertDevicePathToText (DevicePath, FALSE, FALSE));
    Print (L"%a: DevicePath->Type = 0x%x, DevicePath->SubType = 0x%x\n",
           __FUNCTION__, DevicePath->Type, DevicePath->SubType);
#endif
    Status = gBS->HandleProtocol (
                    FileSystemHandles[Index],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID *) &Volume
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }
    Status = Volume->OpenVolume (Volume, &FileHandle);
    if (EFI_ERROR (Status)) {
      continue;
    }
    Status = FileHandle->Open (
                           FileHandle,
                           &File,
                           L"nvram.plist",
                           EFI_FILE_MODE_READ,
                           0
                           );
    if (!EFI_ERROR(Status)) {
      // get nvram.plist modification date
      FileInfo = EfiLibFileInfo (File);
      File->Close (File);
      if (FileInfo == NULL) {
        DBG (" - no nvram.plist file info - skipping!\n");
        FileHandle->Close (FileHandle);
        continue;
      }
      DBG(" Modified = ");
      ModifTimeMs = GetEfiTimeInMs (&FileInfo->ModificationTime);
      DBG ("%d-%d-%d %d:%d:%d (%ld ms)",
           FileInfo->ModificationTime.Year, FileInfo->ModificationTime.Month, FileInfo->ModificationTime.Day,
           FileInfo->ModificationTime.Hour, FileInfo->ModificationTime.Minute, FileInfo->ModificationTime.Second,
           ModifTimeMs);
      FreePool (FileInfo);
      // check if newer
      if (LastModifTimeMs < ModifTimeMs) {
        DBG (" - newer - will use this one\n");
        DPString = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
        FHandle = FileHandle;
        LastModifTimeMs = ModifTimeMs;
      } else {
        DBG (" - older - skipping!\n");
        FileHandle->Close (FileHandle);
        continue;
      }
    } else {
      FileHandle->Close (FileHandle);
    }
  }
  
  //
  // if we have nvram.plist - load it
  //
  if (FHandle != NULL) {
    DBG (" loading nvram.plist from Vol '%s'\n", DPString);
    gNvramDict = LoadPListFile (FHandle, L"nvram.plist");
    FHandle->Close (FHandle);
    Status = EFI_SUCCESS;
  } else {
    DBG (" nvram.plist not found!\n");
    Status = EFI_NOT_FOUND;
  }
  
  return Status;
}


/** Puts all vars from nvram.plist to RT vars. Should be used in CloverEFI only
 *  or if some UEFI boot uses EmuRuntimeDxe driver.
 */
VOID
PutNvramPlistToRtVars (
  VOID
)
{
  EFI_STATUS      Status;
  UINTN           i, DictSize;
  VOID            *prop;
  VOID            *val;
  CHAR8           *akey;
  UINT32          len;
  CHAR16          KeyBuf[128];
  CHAR8           *Data;

  Data = NULL;
  
  if (gNvramDict == NULL) {
    Status = LoadLatestNvramPlist ();
    if (gNvramDict == NULL) {
      DBG ("PutNvramPlistToRtVars: nvram.plist not found\n");
      return;
    }
  }
  
  DBG ("PutNvramPlistToRtVars ...\n");
  // iterate over dict elements
  DictSize = plNodeGetSize (gNvramDict);
  for (i = 0; i < DictSize; i++) {
    prop = plNodeGetItem (gNvramDict, i);
    len = plNodeGetSize(prop);
    akey = plNodeGetBytes (prop);
    
    if (akey == NULL) {
      continue;
    }
    if (AsciiStrCmp (akey, "OsxAptioFixDrv-RelocBase") == 0) {
      DBG (" skipping %a\n", akey);
      continue;
    }
    if (AsciiStrCmp (akey, "OsxAptioFixDrv-ErrorExitingBootServices") == 0) {
      DBG (" skipping %a\n", akey);
      continue;
    }
    if (AsciiStrCmp (akey, "EmuVariableUefiPresent") == 0) {
      DBG (" skipping %a\n", akey);
      continue;
    }
    if (AsciiStrCmp (akey, "boot-args") == 0) {
      DBG (" skipping %a\n", akey);
      continue;
    }
    if (AsciiStrCmp (akey, "prev-lang:kbd") == 0) {
      DBG (" skipping %a\n", akey);
      continue;
    }
    if (len > (sizeof (KeyBuf) / 2 - 2)) {
      DBG (" ERROR: skipping too large key %a\n", akey);
      continue;
    }
    AsciiStrToUnicodeStr(akey, KeyBuf);
    DBG (" adding key: %s\n", KeyBuf);

    val = plNodeGetItem(prop, 0);
    switch (plNodeGetKind (val)) {
      case plKindData:
      case plKindString:
        len = plNodeGetSize (val);
        Data = AllocateCopyPool (len, plNodeGetBytes (val));
        break;
#if 0
      case plKindString:
        len = plNodeGetSize (val);
        len++;
        Data = AllocateZeroPool (len);
        CopyMem (Data, plNodeGetBytes (val), (len - 1));
        break;
#endif
      default:
        DBG ("ERROR: unsupported tag type\n");
        break;
    }
    Status = gRS->SetVariable (
                    KeyBuf,
                    &gEfiAppleBootGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    len,
                    Data
                    );
    FreePool (Data);
  }
}

