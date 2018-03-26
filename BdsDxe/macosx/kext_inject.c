
#include <macosx.h>
#include <Library/plist.h>

#include "kext_inject.h"

////////////////////
// globals
////////////////////
LIST_ENTRY gKextList = INITIALIZE_LIST_HEAD_VARIABLE (gKextList);

////////////////////
// before booting
////////////////////
EFI_STATUS
EFIAPI
ThinFatFile (
  IN OUT UINT8 **binary,
  IN OUT UINTN *length,
  IN cpu_type_t archCpuType
)
{
  BOOLEAN swapped;
  FAT_ARCH *fap;
  FAT_HEADER *fhp;
  UINT32 fapoffset;
  UINT32 fapsize;
  UINT32 nfat, size;
  cpu_type_t fapcputype;

  fap = (FAT_ARCH *) (*binary + sizeof (FAT_HEADER));
  fhp = (FAT_HEADER *) *binary;
  size = 0;
  swapped = FALSE;

  switch (fhp->magic) {
    case FAT_MAGIC:
      nfat = fhp->nfat_arch;
      break;

    case FAT_CIGAM:
      nfat = SwapBytes32 (fhp->nfat_arch);
      swapped = TRUE;
      break;

    case THIN_X64:
      return (archCpuType == CPU_TYPE_X86_64 ? EFI_SUCCESS : EFI_NOT_FOUND);

    case THIN_IA32:
      return (archCpuType == CPU_TYPE_I386 ? EFI_SUCCESS : EFI_NOT_FOUND);

    default:
      return EFI_NOT_FOUND;
  }

  for (; nfat > 0; nfat--, fap++) {
    if (swapped) {
      fapcputype = SwapBytes32 (fap->cputype);
      fapoffset = SwapBytes32 (fap->offset);
      fapsize = SwapBytes32 (fap->size);
    } else {
      fapcputype = fap->cputype;
      fapoffset = fap->offset;
      fapsize = fap->size;
    }

    if (fapcputype == archCpuType) {
      *binary += fapoffset;
      size = fapsize;
      break;
    }
  }

  if (length != NULL) {
    *length = size;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LoadKext (
  IN CHAR16 *FileName,
  IN cpu_type_t archCpuType,
  IN OUT _DeviceTreeBuffer * kext
)
{
  _BooterKextFileInfo *infoAddr = NULL;
  BOOLEAN NoContents = FALSE;
  CHAR8 *bundlePathBuffer = NULL;
  EFI_STATUS Status;
  UINT8 *executableBuffer = NULL;
  UINT8 *executableFatBuffer = NULL;
  UINT8 *infoDictBuffer = NULL;
  UINTN bundlePathBufferLength = 0;
  UINTN executableBufferLength = 0;
  UINTN executableFatBufferLength = 0;
  UINTN infoDictBufferLength = 0;
  VOID *plist;
  CHAR16 Executable[256];
  CHAR16 TempName[256];

  UnicodeSPrint (TempName, sizeof (TempName), L"%s\\%s", FileName, L"Contents\\Info.plist");
  plist = LoadPListFile (gRootFHandle, TempName);

  if (plist == NULL) {
    UnicodeSPrint (TempName, sizeof (TempName), L"%s\\%s", FileName, L"Info.plist");
    plist = LoadPListFile (gRootFHandle, TempName);

    if (plist == NULL) {
      DBG ("%a: Error loading kext %s plist!\n", __FUNCTION__, FileName);
      return EFI_NOT_FOUND;
    }
    NoContents = TRUE;
  }

  Status =
    egLoadFile (gRootFHandle, TempName, &infoDictBuffer, &infoDictBufferLength);
  if (EFI_ERROR (Status)) {
    DBG ("%a: Error loading kext %s raw plist! (%r)\n", __FUNCTION__, FileName, Status);
    plNodeDelete (plist);
    return EFI_NOT_FOUND;
  }

  if (GetUnicodeProperty (plist, "CFBundleExecutable", Executable, ARRAY_SIZE (Executable))) {
    if (NoContents) {
      UnicodeSPrint (TempName, sizeof (TempName), L"%s\\%s", FileName, Executable);
    } else {
      UnicodeSPrint (TempName, sizeof (TempName), L"%s\\%s\\%s", FileName, L"Contents\\MacOS",
                     Executable);
    }
    Status =
      egLoadFile (gRootFHandle, TempName, &executableFatBuffer,
                  &executableFatBufferLength);
    if (EFI_ERROR (Status)) {
      DBG ("%a: Failed to load extra kext: %s (%r)\n", __FUNCTION__, FileName, Status);
      FreeAlignedPages (infoDictBuffer, EFI_SIZE_TO_PAGES (infoDictBufferLength));
      plNodeDelete (plist);
      return EFI_NOT_FOUND;
    }
    executableBuffer = executableFatBuffer;
    executableBufferLength = executableFatBufferLength;
    Status = ThinFatFile (&executableBuffer, &executableBufferLength, archCpuType);
    if (EFI_ERROR (Status)) {
      FreeAlignedPages (infoDictBuffer, EFI_SIZE_TO_PAGES (infoDictBufferLength));
      FreeAlignedPages (executableFatBuffer, EFI_SIZE_TO_PAGES (executableFatBufferLength));
      DBG ("%a: Thinning failed %s (%r)\n", __FUNCTION__, FileName, Status);
      plNodeDelete (plist);
      return EFI_NOT_FOUND;
    }
  }

  plNodeDelete (plist);

  bundlePathBufferLength = StrLen (FileName) + 1;
  bundlePathBuffer = AllocateZeroPool (bundlePathBufferLength);
  UnicodeStrToAsciiStrS (FileName, bundlePathBuffer, bundlePathBufferLength);

  kext->length =
    (UINT32) (sizeof (_BooterKextFileInfo) + infoDictBufferLength +
              executableBufferLength + bundlePathBufferLength);
  infoAddr = (_BooterKextFileInfo *) AllocatePool (kext->length);
  infoAddr->infoDictPhysAddr = sizeof (_BooterKextFileInfo);
  infoAddr->infoDictLength = (UINT32) infoDictBufferLength;
  infoAddr->executablePhysAddr =
    (UINT32) (sizeof (_BooterKextFileInfo) + infoDictBufferLength);
  infoAddr->executableLength = (UINT32) executableBufferLength;
  infoAddr->bundlePathPhysAddr =
    (UINT32) (sizeof (_BooterKextFileInfo) + infoDictBufferLength +
              executableBufferLength);
  infoAddr->bundlePathLength = (UINT32) bundlePathBufferLength;
  kext->paddr = (UINT32) (UINTN) infoAddr;

  CopyMem ((CHAR8 *) infoAddr + sizeof (_BooterKextFileInfo), infoDictBuffer,
           infoDictBufferLength);

  if (executableBufferLength > 0) {
    CopyMem ((CHAR8 *) infoAddr + sizeof (_BooterKextFileInfo) +
             infoDictBufferLength, executableBuffer, executableBufferLength);
  }

  CopyMem ((CHAR8 *) infoAddr + sizeof (_BooterKextFileInfo) +
           infoDictBufferLength + executableBufferLength, bundlePathBuffer,
           bundlePathBufferLength);

  FreeAlignedPages (infoDictBuffer, EFI_SIZE_TO_PAGES (infoDictBufferLength));

  if (executableFatBufferLength > 0) {
    FreeAlignedPages (executableFatBuffer, EFI_SIZE_TO_PAGES (executableFatBufferLength));
  }

  FreePool (bundlePathBuffer);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
AddKext (
  IN CHAR16 *FileName,
  IN cpu_type_t archCpuType
)
{
  EFI_STATUS Status;
  KEXT_ENTRY *KextEntry;

  KextEntry = AllocatePool (sizeof (KEXT_ENTRY));
  KextEntry->Signature = KEXT_SIGNATURE;
  Status = LoadKext (FileName, archCpuType, &KextEntry->kext);
  if (EFI_ERROR (Status)) {
    DBG ("%a: load kext %s failed\n", __FUNCTION__, FileName);
    FreePool (KextEntry);
  } else {
    DBG ("%a: load kext %s successful\n", __FUNCTION__, FileName);
    InsertTailList (&gKextList, &KextEntry->Link);
  }

  return Status;
}

UINT16
GetListCount (
  LIST_ENTRY const *List
)
{
  LIST_ENTRY *Link;
  UINT16 Count = 0;

  if (!IsListEmpty (List)) {
    for (Link = List->ForwardLink; Link != List; Link = Link->ForwardLink)
      Count++;
  } else {
    DBG ("%a: Kext Inject - list is empty.\n",__FUNCTION__);
#ifdef KEXT_INJECT_DEBUG
    Print (L"%a: Get List Count - list is empty.\n", __FUNCTION__);
#endif
  }

  return Count;
}

UINT16
GetKextCount (
  VOID
)
{
  return GetListCount (&gKextList);
}

UINT32
GetKextsSize (
  VOID
)
{
  LIST_ENTRY *Link;
  KEXT_ENTRY *KextEntry;
  UINT32 kextsSize = 0;

  if (!IsListEmpty (&gKextList)) {
    for (Link = gKextList.ForwardLink; Link != &gKextList;
         Link = Link->ForwardLink) {
      KextEntry = CR (Link, KEXT_ENTRY, Link, KEXT_SIGNATURE);
      kextsSize += RoundPage (KextEntry->kext.length);
    }
  }

  return kextsSize;
}

CHAR16 *
GetExtraKextsDir (
  VOID
)
{
  CHAR16 *OSTypeStr;
  CHAR16 *KextsDir;
  CHAR8 *s;
  UINTN tmpLen;

  OSTypeStr = NULL;
  KextsDir = NULL;
  s = NULL;

  if (OSVersion != NULL) {
    tmpLen = AsciiStrSize (OSVersion);
    s = AllocatePool (tmpLen + 1);
    OSTypeStr = AllocatePool (tmpLen * sizeof (CHAR16));
    if (AsciiStrnCmp (OSVersion, "10.1", 4) != 0) {
      (VOID) AsciiStrnCpyS (s, tmpLen + 1, OSVersion, 4);
      AsciiStrToUnicodeStrS (s, OSTypeStr, tmpLen);
    } else {
      (VOID) AsciiStrnCpyS (s, tmpLen + 1, OSVersion, 5);
      AsciiStrToUnicodeStrS (s, OSTypeStr, tmpLen);
    }
    FreePool (s);
  } else {
    OSTypeStr = L"other";
  }
  // find source injection folder with kexts
  // note: we are just checking for existance of particular folder, not checking if it is empty or not
  // check OEM subfolders: version speciffic or default to Other

  KextsDir = AllocatePool (200 * sizeof (CHAR16));
  (VOID) UnicodeSPrint (KextsDir, 200 * sizeof (CHAR16), L"%skexts\\%s",
                        gPNDirExists ? gProductNameDir : BB_HOME_DIR, OSTypeStr);
  FreePool (OSTypeStr);

  DBG ("%a: expected extra kexts dir is %s\n", __FUNCTION__, KextsDir);

  if (!FileExists (gRootFHandle, KextsDir)) {
    FreePool (KextsDir);
    return NULL;
  }

  return KextsDir;
}

BOOLEAN
LoadKexts (
  VOID
)
{
  BOOLEAN CommonKextsDir = TRUE;
  CHAR16 *KextsDir = NULL;
  DIR_ITER KextIter;
  EFI_FILE_INFO *KextFile;
  DIR_ITER PlugInIter;
  EFI_FILE_INFO *PlugInFile;
  CHAR16 FileName[256];
  CHAR16 PlugIns[256];
  UINTN mm_extra_size;
  VOID *mm_extra;
  UINTN extra_size;
  VOID *extra;
  UINT16 KextCount;

#if 0
#if defined(MDE_CPU_X64)
  cpu_type_t archCpuType = CPU_TYPE_X86_64;
#else
  cpu_type_t archCpuType = CPU_TYPE_I386;
#endif

  if (AsciiStrStr (gSettings.BootArgs, "arch=x86_64") != NULL)
    archCpuType = CPU_TYPE_X86_64;
  else if (AsciiStrStr (gSettings.BootArgs, "arch=i386") != NULL)
    archCpuType = CPU_TYPE_I386;
  else if (OSVersion != NULL) {
    if (AsciiStrLen (OSVersion) >= 5) {
      if (AsciiStrnCmp (OSVersion, "10.10", 5) == 0)
        archCpuType = CPU_TYPE_X86_64;
      if (AsciiStrnCmp (OSVersion, "10.11", 5) == 0)
        archCpuType = CPU_TYPE_X86_64;
    }
    else if (AsciiStrnCmp (OSVersion, "10.9", 4) == 0)
      archCpuType = CPU_TYPE_X86_64;
    else if (AsciiStrnCmp (OSVersion, "10.8", 4) == 0)
      archCpuType = CPU_TYPE_X86_64;
    else if (AsciiStrnCmp (OSVersion, "10.7", 4) != 0)
      archCpuType = CPU_TYPE_I386;
  }
#else
  cpu_type_t archCpuType = CPU_TYPE_X86_64;

  if (OSVersion != NULL) {
    if (AsciiStrnCmp (OSVersion, "10.1", 4) != 0 && // Puma is not for Intel, assume it's 10.10 and greater
        AsciiStrnCmp (OSVersion, "10.8", 4) < 0) {
      if (AsciiStrnCmp (OSVersion, "10.5", 4) <= 0 ||
          (AsciiStrnCmp (OSVersion, "10.6", 4) == 0 && AsciiStrStr (gSettings.BootArgs, "arch=x86_64") == NULL) ||
          (AsciiStrnCmp (OSVersion, "10.7", 4) == 0 && AsciiStrStr (gSettings.BootArgs, "arch=i386") == NULL)) {
          archCpuType = CPU_TYPE_I386;
      }
    }
  } else {
    if (AsciiStrStr (gSettings.BootArgs, "arch=i386") != NULL)
      archCpuType = CPU_TYPE_I386;
  }
#endif

  InitializeUnicodeCollationProtocol ();

  KextsDir = AllocatePool (200 * sizeof (CHAR16));
  (VOID) UnicodeSPrint (KextsDir, 200 * sizeof (CHAR16), L"%skexts\\common",
                        gPNDirExists ? gProductNameDir : BB_HOME_DIR);

  if (!FileExists (gRootFHandle, KextsDir)) {
    FreePool (KextsDir);
    CommonKextsDir = FALSE;

    DBG ("%a: No common extra kexts.\n", __FUNCTION__);

    KextsDir = GetExtraKextsDir ();
    if (KextsDir == NULL) {
      DBG ("%a: No extra kexts.\n", __FUNCTION__);
      return FALSE;
    }
  }

  while (KextsDir != NULL) {
    // look through contents of the directory
    DBG ("%a: extra kexts dir is %s\n", __FUNCTION__, KextsDir);

    DirIterOpen (gRootFHandle, KextsDir, &KextIter);
    while (DirIterNext (&KextIter, 1, L"*.kext", &KextFile)) {
      if (KextFile->FileName[0] == '.' ||
          StrStr (KextFile->FileName, L".kext") == NULL)
        continue; // skip this
      DBG ("%a: KextFile->FileName = %s\n", __FUNCTION__, KextFile->FileName);

      UnicodeSPrint (FileName, sizeof (FileName), L"%s\\%s", KextsDir, KextFile->FileName);
      AddKext (FileName, archCpuType);

      UnicodeSPrint (PlugIns, sizeof (PlugIns), L"%s\\%s", FileName, L"Contents\\PlugIns");
      DirIterOpen (gRootFHandle, PlugIns, &PlugInIter);
      while (DirIterNext (&PlugInIter, 1, L"*.kext", &PlugInFile)) {
        if (PlugInFile->FileName[0] == '.' ||
            StrStr (PlugInFile->FileName, L".kext") == NULL)
          continue; // skip this
        DBG ("%a:  PlugInFile->FileName = %s\n", __FUNCTION__, PlugInFile->FileName);

        UnicodeSPrint (FileName, sizeof (FileName), L"%s\\%s", PlugIns, PlugInFile->FileName);
        AddKext (FileName, archCpuType);
      }
      DirIterClose (&PlugInIter);
    }
    DirIterClose (&KextIter);

    if (CommonKextsDir) {
      FreePool (KextsDir);
      CommonKextsDir = FALSE;
      KextsDir = GetExtraKextsDir ();
    } else {
      FreePool (KextsDir);
      KextsDir = NULL;
    }
  }

  KextCount = GetKextCount ();
  DBG ("%a:  KextCount = %d\n", __FUNCTION__, KextCount);
  if (KextCount > 0) {
    mm_extra_size =
      KextCount * (sizeof (DeviceTreeNodeProperty) +
                   sizeof (_DeviceTreeBuffer));
    mm_extra =
      AllocateZeroPool (mm_extra_size - sizeof (DeviceTreeNodeProperty));
      (void) LogDataHub (&gEfiMiscSubClassGuid, L"mm_extra", mm_extra,
                  (UINT32) (mm_extra_size - sizeof (DeviceTreeNodeProperty))
      );

    extra_size = GetKextsSize ();
    extra =
      AllocateZeroPool (extra_size - sizeof (DeviceTreeNodeProperty) +
                        EFI_PAGE_SIZE);
      (void) LogDataHub (&gEfiMiscSubClassGuid, L"extra", extra,
                  (UINT32) (extra_size - sizeof (DeviceTreeNodeProperty) +
                            EFI_PAGE_SIZE)
      );
    return TRUE;
  }

  return FALSE;
}

////////////////////
// OnExitBootServices
////////////////////
EFI_STATUS
InjectKexts (
  IN UINT32 deviceTreeP,
  IN UINT32 *deviceTreeLength
)
{
  UINT8 *dtEntry = (UINT8 *) (UINTN) deviceTreeP;
  UINTN dtLength = (UINTN) *deviceTreeLength;
  DTEntry platformEntry = NULL;
  DTEntry memmapEntry = NULL;
  CHAR8 *ptr;
  UINT8 *infoPtr = 0;
  UINT8 *extraPtr = 0;
  UINT8 *drvPtr = 0;
  UINTN offset = 0;
  UINTN Index;

  LIST_ENTRY *Link;
  KEXT_ENTRY *KextEntry;
  UINTN KextBase = 0;

  struct OpaqueDTPropertyIterator OPropIter;
  DTPropertyIterator iter = &OPropIter;
  DeviceTreeNodeProperty *prop = NULL;
  _DeviceTreeBuffer *mm;
  _BooterKextFileInfo *drvinfo;

  if (GetKextCount () == 0) {
    DBG ("%a: Kext Inject - extra kexts not found.\n",__FUNCTION__);
#ifdef KEXT_INJECT_DEBUG
    Print (L"%a: extra kexts not found.\n", __FUNCTION__);
#endif
    return EFI_NOT_FOUND;
  }

  // kextsBase = Desc->PhysicalStart + (((UINTN) Desc->NumberOfPages) * EFI_PAGE_SIZE);
  // kextsPages = EFI_SIZE_TO_PAGES(kext.length);
  // Status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, kextsPages, &kextsBase);
  // if (EFI_ERROR(Status)) { MsgLog("Kext inject: could not allocate memory\n"); return Status; }
  // Desc->NumberOfPages += kextsPages;
  // CopyMem((VOID*)kextsBase, (VOID*)(UINTN)kext.paddr, kext.length);
  // drvinfo = (_BooterKextFileInfo*) kextsBase;
  // drvinfo->infoDictPhysAddr += (UINT32)kextsBase;
  // drvinfo->executablePhysAddr += (UINT32)kextsBase;
  // drvinfo->bundlePathPhysAddr += (UINT32)kextsBase;

  DTInit (dtEntry);
  if (DTLookupEntry (NULL, "/chosen/memory-map", &memmapEntry) == kSuccess) {
    if (DTCreatePropertyIteratorNoAlloc (memmapEntry, iter) == kSuccess) {
      while (DTIterateProperties (iter, &ptr) == kSuccess) {
        prop = iter->currentProperty;
        drvPtr = (UINT8 *) prop;
        if (AsciiStrnCmp (prop->name, "Driver-", 7) == 0 ||
            AsciiStrnCmp (prop->name, "DriversPackage-", 15) == 0) {
          break;
        }
      }
    }
  }

  if (DTLookupEntry (NULL, "/efi/platform", &platformEntry) == kSuccess) {
    if (DTCreatePropertyIteratorNoAlloc (platformEntry, iter) == kSuccess) {
      while (DTIterateProperties (iter, &ptr) == kSuccess) {
        prop = iter->currentProperty;
        if (AsciiStrCmp (prop->name, "mm_extra") == 0) {
          infoPtr = (UINT8 *) prop;
        }
        if (AsciiStrCmp (prop->name, "extra") == 0) {
          extraPtr = (UINT8 *) prop;
        }
      }
    }
  }

  if (drvPtr == 0 || infoPtr == 0 || extraPtr == 0 || drvPtr > infoPtr ||
      drvPtr > extraPtr || infoPtr > extraPtr) {
    DBG ("%a: Kext Inject - invalid device tree for kext injection.\n",__FUNCTION__);
#ifdef KEXT_INJECT_DEBUG
    Print (L"%a: Invalid device tree for kext injection\n", __FUNCTION__);
#endif
    return EFI_INVALID_PARAMETER;
  }

  // make space for memory map entries
  platformEntry->nProperties -= 2;
  offset =
    sizeof (DeviceTreeNodeProperty) +
    ((DeviceTreeNodeProperty *) infoPtr)->length;
  CopyMem (drvPtr + offset, drvPtr, infoPtr - drvPtr);

  // make space behind device tree
  // platformEntry->nProperties--;
  offset =
    sizeof (DeviceTreeNodeProperty) +
    ((DeviceTreeNodeProperty *) extraPtr)->length;
  CopyMem (extraPtr, extraPtr + offset,
           dtLength - (UINTN) (extraPtr - dtEntry) - offset);
  *deviceTreeLength -= (UINT32) offset;

  KextBase = RoundPage (dtEntry + *deviceTreeLength);
  if (!IsListEmpty (&gKextList)) {
    Index = 1;
    for (Link = gKextList.ForwardLink; Link != &gKextList;
         Link = Link->ForwardLink) {
      KextEntry = CR (Link, KEXT_ENTRY, Link, KEXT_SIGNATURE);

      CopyMem ((VOID *) KextBase, (VOID *) (UINTN) KextEntry->kext.paddr,
               KextEntry->kext.length);
      drvinfo = (_BooterKextFileInfo *) KextBase;
      drvinfo->infoDictPhysAddr += (UINT32) KextBase;
      drvinfo->executablePhysAddr += (UINT32) KextBase;
      drvinfo->bundlePathPhysAddr += (UINT32) KextBase;

      memmapEntry->nProperties++;
      prop = ((DeviceTreeNodeProperty *) drvPtr);
      prop->length = sizeof (_DeviceTreeBuffer);
      mm =
        (_DeviceTreeBuffer *) (((UINT8 *) prop) +
                               sizeof (DeviceTreeNodeProperty));
      mm->paddr = (UINT32) KextBase;
      mm->length = KextEntry->kext.length;
      AsciiSPrint (prop->name, sizeof (prop->name) - 1, "Driver-%x", KextBase);

      drvPtr += sizeof (DeviceTreeNodeProperty) + sizeof (_DeviceTreeBuffer);
      KextBase = RoundPage (KextBase + KextEntry->kext.length);
      Index++;
    }
  } else {
    DBG ("%a: Kext Inject - kext list is empty.\n",__FUNCTION__);
#ifdef KEXT_INJECT_DEBUG
    Print (L"%a: kext list is empty.\n", __FUNCTION__);
#endif
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
#if 0
////////////////////////////////////
//
// KernelBooterExtensionsPatch to load extra kexts besides kernelcache
//
// 

UINT8 KBELionSearch_X64[] =
  { 0xE8, 0x0C, 0xFD, 0xFF, 0xFF, 0xEB, 0x08, 0x48, 0x89, 0xDF };
UINT8 KBELionReplace_X64[] =
  { 0xE8, 0x0C, 0xFD, 0xFF, 0xFF, 0x90, 0x90, 0x48, 0x89, 0xDF };

UINT8 KBELionSearch_i386[] =
  { 0xE8, 0xAA, 0xFB, 0xFF, 0xFF, 0xEB, 0x08, 0x89, 0x34, 0x24 };
UINT8 KBELionReplace_i386[] =
  { 0xE8, 0xAA, 0xFB, 0xFF, 0xFF, 0x90, 0x90, 0x89, 0x34, 0x24 };

UINT8 KBEMLSearch[] =
  { 0xC6, 0xE8, 0x30, 0x00, 0x00, 0x00, 0xEB, 0x08, 0x48, 0x89, 0xDF };
UINT8 KBEMLReplace[] =
  { 0xC6, 0xE8, 0x30, 0x00, 0x00, 0x00, 0x90, 0x90, 0x48, 0x89, 0xDF };

UINT8 KBEYSearch[] =
  { 0xC6, 0xE8, 0x25, 0x00, 0x00, 0x00, 0xEB, 0x05, 0xE8, 0xCE, 0x02 };
UINT8 KBEYReplace[] =
  { 0xC6, 0xE8, 0x25, 0x00, 0x00, 0x00, 0x90, 0x90, 0xE8, 0xCE, 0x02 };

//
// We can not rely on OSVersion global variable for OS version detection,
// since in some cases it is not correct (install of ML from Lion, for example).
// So, we'll use "brute-force" method - just try to pacth.
// Actually, we'll at least check that if we can find only one instance of code that
// we are planning to patch.
//

#define KERNEL_MAX_SIZE 40000000

VOID
  EFIAPI
KernelBooterExtensionsPatch (
  IN UINT8 *Kernel
)
{

  UINTN Num = 0;
  UINTN NumLion_X64 = 0;
  UINTN NumLion_i386 = 0;
  UINTN NumML = 0;
  UINTN NumY = 0;

  if (is64BitKernel) {
    NumLion_X64 =
      SearchAndCount (Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBELionSearch_X64,
                      sizeof (KBELionSearch_X64));
    NumML =
      SearchAndCount (Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBEMLSearch,
                      sizeof (KBEMLSearch));

    NumY =
      SearchAndCount (Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBEYSearch,
                      sizeof (KBEYSearch));
  }
  else {
    NumLion_i386 =
      SearchAndCount (Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBELionSearch_i386,
                      sizeof (KBELionSearch_i386));
  }

  if (NumLion_X64 + NumLion_i386 + NumML + NumY > 1) {
    // more then one pattern found - we do not know what to do with it
    // and we'll skipp it
    DBG ("%a: Kext Inject - more then one pattern found and it's no good.\n",__FUNCTION__);
#ifdef KEXT_INJECT_DEBUG
    Print (L"%a: more then one pattern found and it's no good.\n", __FUNCTION__);
#endif
    return;
  }

  if (NumLion_X64 == 1) {
    Num =
      SearchAndReplace (Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBELionSearch_X64,
                        sizeof (KBELionSearch_X64),
                        (CHAR8 *) KBELionReplace_X64, 1);
  }
  else if (NumLion_i386 == 1) {
    Num =
      SearchAndReplace (Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBELionSearch_i386,
                        sizeof (KBELionSearch_i386),
                        (CHAR8 *) KBELionReplace_i386, 1);
  }
  else if (NumML == 1) {
    Num =
      SearchAndReplace (Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBEMLSearch,
                        sizeof (KBEMLSearch), (CHAR8 *) KBEMLReplace, 1);
  }
  else if (NumY == 1) {
    Num =
      SearchAndReplace (Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBEYSearch,
                        sizeof (KBEMLSearch), (CHAR8 *) KBEYReplace, 1);
  }
  DBG ("%a: Kext Inject - SearchAndReplace %d times.\n",__FUNCTION__, Num);
#ifdef KEXT_INJECT_DEBUG
  Print (L"%a: Patch Kernel - SearchAndReplace %d times.\n", __FUNCTION__, Num);
#endif
}
#endif
