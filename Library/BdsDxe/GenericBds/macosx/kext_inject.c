
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
	UINT32 nfat, swapped, size = 0;
	FAT_HEADER *fhp = (FAT_HEADER *)*binary;
	FAT_ARCH   *fap = (FAT_ARCH *)(*binary + sizeof(FAT_HEADER));
	cpu_type_t fapcputype;
	UINT32 fapoffset;
	UINT32 fapsize;	

  swapped = 0;
	if (fhp->magic == FAT_MAGIC) {
		nfat = fhp->nfat_arch;
	} else if (fhp->magic == FAT_CIGAM) {
		nfat = SwapBytes32(fhp->nfat_arch);
		swapped = 1;
    //already thin    
	} else if (fhp->magic == THIN_X64){
    if (archCpuType == CPU_TYPE_X86_64) {
      return EFI_SUCCESS;
    }
    return EFI_NOT_FOUND;
	} else if (fhp->magic == THIN_IA32){
    if (archCpuType == CPU_TYPE_I386) {
      return EFI_SUCCESS;
    }
    return EFI_NOT_FOUND;
  } else {
    return EFI_NOT_FOUND;
  }

	for (; nfat > 0; nfat--, fap++) {
		if (swapped) {
			fapcputype = SwapBytes32(fap->cputype);
			fapoffset = SwapBytes32(fap->offset);
			fapsize = SwapBytes32(fap->size);
		} else {
			fapcputype = fap->cputype;
			fapoffset = fap->offset;
			fapsize = fap->size;
		}
		if (fapcputype == archCpuType) {
			*binary = (*binary + fapoffset);
			size = fapsize;
			break;
		}
	}
	if (length != 0) *length = size;

	return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LoadKext (
  IN CHAR16 *FileName,
  IN cpu_type_t archCpuType,
  IN OUT _DeviceTreeBuffer *kext
)
{
	EFI_STATUS      Status;
	UINT8*          executableFatBuffer = NULL;
	UINT8*          executableBuffer = NULL;
	UINTN           executableBufferLength = 0;
	CHAR8*          bundlePathBuffer = NULL;
	UINTN           bundlePathBufferLength = 0;
	CHAR16          TempName[256];
	CHAR16          Executable[256];
  VOID*           plist;
  BOOLEAN         NoContents = FALSE;
  _BooterKextFileInfo *infoAddr = NULL;

	UINT8*      infoDictBuffer = NULL;
	UINTN       infoDictBufferLength = 0;

	UnicodeSPrint(TempName, 512, L"%s\\%s", FileName, L"Contents\\Info.plist");
  plist = LoadPListFile (gRootFHandle, TempName);

  if (plist == NULL) {
    UnicodeSPrint(TempName, 512, L"%s\\%s", FileName, L"Info.plist");
    plist = LoadPListFile (gRootFHandle, TempName);

    if (plist == NULL) {
      DBG ("Kext Inject: Error loading kext %s plist!\r\n", FileName);
      return EFI_NOT_FOUND;
    }
    NoContents = TRUE;
  }

	Status = egLoadFile (gRootFHandle, TempName, &infoDictBuffer, &infoDictBufferLength);
  if (EFI_ERROR(Status)) {
    DBG ("Kext Inject: Error loading kext %s plist!\r\n", FileName);
    return EFI_NOT_FOUND;
  }

  if (GetUnicodeProperty (plist, "CFBundleExecutable", Executable)) {
    if (NoContents) {
      UnicodeSPrint(TempName, 512, L"%s\\%s", FileName, Executable);
    } else {
      UnicodeSPrint(TempName, 512, L"%s\\%s\\%s", FileName, L"Contents\\MacOS",Executable);
    }
    Status = egLoadFile (gRootFHandle, TempName, &executableFatBuffer, &executableBufferLength);
    if (EFI_ERROR(Status)) {
      DBG ("Kext Inject: Failed to load extra kext: %s\n", FileName);
      return EFI_NOT_FOUND;
    }
    executableBuffer = executableFatBuffer;
    if (ThinFatFile(&executableBuffer, &executableBufferLength, archCpuType)) {
      FreePool(infoDictBuffer);
      FreePool(executableBuffer);
      DBG ("Kext Inject: Thinning failed %s\n", FileName);
      return EFI_NOT_FOUND;
    }
  }
  plNodeDelete (plist);

  bundlePathBufferLength = StrLen(FileName) + 1;
  bundlePathBuffer = AllocateZeroPool(bundlePathBufferLength);
  UnicodeStrToAsciiStr(FileName, bundlePathBuffer);

  kext->length = (UINT32)(sizeof(_BooterKextFileInfo) + infoDictBufferLength + executableBufferLength + bundlePathBufferLength);
  infoAddr = (_BooterKextFileInfo *)AllocatePool(kext->length);
  infoAddr->infoDictPhysAddr = sizeof(_BooterKextFileInfo);
  infoAddr->infoDictLength = (UINT32)infoDictBufferLength;
  infoAddr->executablePhysAddr = (UINT32)(sizeof(_BooterKextFileInfo) + infoDictBufferLength);
  infoAddr->executableLength = (UINT32)executableBufferLength;
  infoAddr->bundlePathPhysAddr = (UINT32)(sizeof(_BooterKextFileInfo) + infoDictBufferLength + executableBufferLength);
  infoAddr->bundlePathLength = (UINT32)bundlePathBufferLength;
    kext->paddr = (UINT32)(UINTN)infoAddr;

  CopyMem((CHAR8 *)infoAddr + sizeof(_BooterKextFileInfo), infoDictBuffer, infoDictBufferLength);
  CopyMem((CHAR8 *)infoAddr + sizeof(_BooterKextFileInfo) + infoDictBufferLength, executableBuffer, executableBufferLength);
  CopyMem((CHAR8 *)infoAddr + sizeof(_BooterKextFileInfo) + infoDictBufferLength + executableBufferLength, bundlePathBuffer, bundlePathBufferLength);

  FreePool(infoDictBuffer);
  FreePool(executableFatBuffer);
  FreePool(bundlePathBuffer);

	return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
AddKext (
  IN CHAR16 *FileName,
  IN cpu_type_t archCpuType
)
{
	EFI_STATUS	Status;
	KEXT_ENTRY	*KextEntry;

	KextEntry = AllocatePool (sizeof(KEXT_ENTRY));
	KextEntry->Signature = KEXT_SIGNATURE;
	Status = LoadKext (FileName, archCpuType, &KextEntry->kext);
	if(EFI_ERROR(Status)) {
    DBG ("Kext Inject: load kext %s failed\n", FileName);
		FreePool(KextEntry);
	} else {
    DBG ("Kext Inject: load kext %s successful\n", FileName);
		InsertTailList (&gKextList, &KextEntry->Link);
	}

	return Status;
}

UINT16
GetListCount (
  LIST_ENTRY const* List
)
{
	LIST_ENTRY		*Link;
	UINT16			Count=0;

	if(!IsListEmpty(List)) {
		for (Link = List->ForwardLink; Link != List; Link = Link->ForwardLink) 
			Count++;
	} else {
#ifdef KEXT_INJECT_DEBUG
    Print (L"Kext Inject: Get List Count - list is empty.\n");
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
	LIST_ENTRY		*Link;
	KEXT_ENTRY		*KextEntry;
	UINT32			kextsSize=0;

	if(!IsListEmpty(&gKextList)) {
		for (Link = gKextList.ForwardLink; Link != &gKextList; Link = Link->ForwardLink) {
			KextEntry = CR(Link, KEXT_ENTRY, Link, KEXT_SIGNATURE);
			kextsSize += RoundPage(KextEntry->kext.length);
		}
	} else {
    
  }
	return kextsSize;
}


CHAR16*
GetExtraKextsDir (
  VOID
)
{
  CHAR16    *OSTypeStr = NULL;
  CHAR16    *KextsDir;

  OSTypeStr = NULL;
  KextsDir = NULL;
  
  // get os version as string
  if (AsciiStrnCmp (OSVersion, "10.4", 4) == 0) {
    OSTypeStr = L"10.4";
  } else if (AsciiStrnCmp (OSVersion, "10.5", 4) == 0) {
    OSTypeStr = L"10.5";
  } else if (AsciiStrnCmp (OSVersion, "10.6", 4) == 0) {
    OSTypeStr = L"10.6";
  } else if (AsciiStrnCmp (OSVersion, "10.7", 4) == 0) {
    OSTypeStr = L"10.7";
  } else if (AsciiStrnCmp (OSVersion, "10.8", 4) == 0) {
    OSTypeStr = L"10.8";
  } else if (AsciiStrnCmp (OSVersion, "10.9", 4) == 0) {
    OSTypeStr = L"10.9";
  } else {
    OSTypeStr = L"other";
  }
  // find source injection folder with kexts
  // note: we are just checking for existance of particular folder, not checking if it is empty or not
  // check OEM subfolders: version speciffic or default to Other
  if (gPNDirExists) {
    KextsDir = AllocateZeroPool (StrSize (gProductNameDir) + StrSize (L"kexts\\") + StrSize (OSTypeStr));
    StrCpy (KextsDir, gProductNameDir);
    StrCat (KextsDir, L"kexts\\");
    StrCat (KextsDir, OSTypeStr);
  } else {
    KextsDir = AllocateZeroPool (StrSize (L"\\EFI\\bareboot\\kexts\\") + StrSize (OSTypeStr));
    StrCpy (KextsDir, L"\\EFI\\bareboot\\kexts\\");
    StrCat (KextsDir,OSTypeStr);
  }

  DBG ("Kext Inject: expected extra kexts dir is %s\n", KextsDir);

  if (!FileExists(gRootFHandle, KextsDir)) {
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
	EFI_STATUS              Status;
	BOOLEAN                 CommonKextsDir = TRUE;
	CHAR16                  *KextsDir = NULL;
	DIR_ITER                KextIter;
	EFI_FILE_INFO           *KextFile;
	DIR_ITER                PlugInIter;
	EFI_FILE_INFO           *PlugInFile;
	CHAR16                  FileName[256];
	CHAR16                  PlugIns[256];
	UINTN					mm_extra_size;
	VOID					*mm_extra;
	UINTN					extra_size;
	VOID					*extra;
  UINT16        KextCount;

#if defined(MDE_CPU_X64)
	cpu_type_t archCpuType = CPU_TYPE_X86_64;
#else
	cpu_type_t archCpuType = CPU_TYPE_I386;
#endif

  if      (AsciiStrStr (gSettings.BootArgs,"arch=x86_64") != NULL)	archCpuType = CPU_TYPE_X86_64;
  else if (AsciiStrStr (gSettings.BootArgs,"arch=i386") != NULL)		archCpuType = CPU_TYPE_I386;
  else if (OSVersion != NULL) {
    if      (AsciiStrnCmp (OSVersion,"10.9",4) == 0)                 archCpuType = CPU_TYPE_X86_64;
    else if (AsciiStrnCmp (OSVersion,"10.8",4) == 0)                 archCpuType = CPU_TYPE_X86_64;
    else if (AsciiStrnCmp (OSVersion,"10.7",4) != 0)                 archCpuType = CPU_TYPE_I386;
  }

  InitializeUnicodeCollationProtocol ();

  if (gPNDirExists) {
    KextsDir = AllocateZeroPool (StrSize (gProductNameDir) + StrSize (L"kexts\\common"));
    StrCpy (KextsDir, gProductNameDir);
    StrCat (KextsDir, L"kexts\\common");
  } else {
    KextsDir = AllocateZeroPool (StrSize (L"\\EFI\\bareboot\\kexts\\common"));
    StrCpy (KextsDir, L"\\EFI\\bareboot\\kexts\\common");
  }

  if (!FileExists(gRootFHandle, KextsDir)) {
    FreePool (KextsDir);
    CommonKextsDir = FALSE;

    DBG ("Kext Inject: No common extra kexts.\n");

    KextsDir = GetExtraKextsDir ();
    if (KextsDir == NULL) {
      DBG ("Kext Inject: No extra kexts.\n");
      return FALSE;
    }
  }

  while (KextsDir != NULL) {
    // look through contents of the directory
    DBG ("Kext Inject: extra kexts dir is %s\n", KextsDir);

    DirIterOpen(gRootFHandle, KextsDir, &KextIter);
    while (DirIterNext(&KextIter, 1, L"*.kext", &KextFile)) {
      if (KextFile->FileName[0] == '.' || StrStr(KextFile->FileName, L".kext") == NULL)
        continue;   // skip this
      DBG ("Kext Inject: KextFile->FileName = %s\n", KextFile->FileName);

      UnicodeSPrint(FileName, 512, L"%s\\%s", KextsDir, KextFile->FileName);
      AddKext(FileName, archCpuType);

      UnicodeSPrint(PlugIns, 512, L"%s\\%s", FileName, L"Contents\\PlugIns");
      DirIterOpen(gRootFHandle, PlugIns, &PlugInIter);
      while (DirIterNext(&PlugInIter, 1, L"*.kext", &PlugInFile)) {
        if (PlugInFile->FileName[0] == '.' || StrStr(PlugInFile->FileName, L".kext") == NULL)
          continue;   // skip this
        DBG ("Kext Inject:  PlugInFile->FileName = %s\n", PlugInFile->FileName);

        UnicodeSPrint(FileName, 512, L"%s\\%s", PlugIns, PlugInFile->FileName);
        AddKext(FileName, archCpuType);
      }
      DirIterClose(&PlugInIter);
    }
    DirIterClose(&KextIter);

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
  DBG ("Kext Inject:  KextCount = %d\n", KextCount);
  if (KextCount > 0) {
    mm_extra_size = KextCount * (sizeof(DeviceTreeNodeProperty) + sizeof(_DeviceTreeBuffer));
    mm_extra = AllocateZeroPool(mm_extra_size - sizeof(DeviceTreeNodeProperty));
    Status =  LogDataHub (
                &gEfiMiscSubClassGuid,
                L"mm_extra",
                mm_extra,
                (UINT32) (mm_extra_size - sizeof(DeviceTreeNodeProperty))
                );

    extra_size = GetKextsSize();
    extra = AllocateZeroPool (extra_size - sizeof(DeviceTreeNodeProperty) + EFI_PAGE_SIZE);
    Status =  LogDataHub (
                &gEfiMiscSubClassGuid,
                L"extra",
                extra,
                (UINT32) (extra_size - sizeof(DeviceTreeNodeProperty) + EFI_PAGE_SIZE)
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
  IN UINT32* deviceTreeLength
)
{
	UINT8					*dtEntry = (UINT8*)(UINTN) deviceTreeP;
	UINTN					dtLength = (UINTN) *deviceTreeLength;
	DTEntry				platformEntry = NULL;
	DTEntry				memmapEntry = NULL;
	CHAR8 				*ptr;
	UINT8					*infoPtr = 0;
	UINT8					*extraPtr = 0;
	UINT8					*drvPtr = 0;
	UINTN					offset = 0;
	UINTN					Index;

	LIST_ENTRY				*Link;
	KEXT_ENTRY				*KextEntry;
	UINTN					KextBase = 0;

	struct OpaqueDTPropertyIterator OPropIter;
	DTPropertyIterator		iter = &OPropIter;
	DeviceTreeNodeProperty	*prop = NULL;
	_DeviceTreeBuffer		*mm;
	_BooterKextFileInfo		*drvinfo;
	

	if (GetKextCount () == 0) {
#ifdef KEXT_INJECT_DEBUG
    Print (L"Kext Inject: extra kexts not found.\n");
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

	DTInit(dtEntry);
	if(DTLookupEntry(NULL,"/chosen/memory-map",&memmapEntry)==kSuccess) {
		if(DTCreatePropertyIteratorNoAlloc(memmapEntry,iter)==kSuccess) {
			while(DTIterateProperties(iter,&ptr)==kSuccess) {
				prop = iter->currentProperty;
				drvPtr = (UINT8*) prop;
				if(AsciiStrnCmp(prop->name, "Driver-", 7)==0 || AsciiStrnCmp(prop->name, "DriversPackage-", 15)==0) {
					break;
				}
			}
		}
	}

	if(DTLookupEntry(NULL,"/efi/platform",&platformEntry)==kSuccess) {
		if(DTCreatePropertyIteratorNoAlloc(platformEntry,iter)==kSuccess) {
			while(DTIterateProperties(iter,&ptr)==kSuccess) {
				prop = iter->currentProperty;
				if(AsciiStrCmp(prop->name,"mm_extra")==0) {
					infoPtr = (UINT8*) prop;
				}
				if(AsciiStrCmp(prop->name,"extra")==0) {
					extraPtr = (UINT8*) prop;
				}
			}
		}
	}

	if (drvPtr == 0 || infoPtr == 0 || extraPtr == 0 || drvPtr > infoPtr || drvPtr > extraPtr || infoPtr > extraPtr) {
#ifdef KEXT_INJECT_DEBUG
		Print (L"Kext Inject: Invalid device tree for kext injection\n");
#endif
		return EFI_INVALID_PARAMETER;
	}

	// make space for memory map entries
	platformEntry->nProperties -= 2;
	offset = sizeof(DeviceTreeNodeProperty) + ((DeviceTreeNodeProperty*) infoPtr)->length;
	CopyMem(drvPtr+offset, drvPtr, infoPtr-drvPtr);

	// make space behind device tree
	// platformEntry->nProperties--;
	offset = sizeof(DeviceTreeNodeProperty)+((DeviceTreeNodeProperty*) extraPtr)->length;
	CopyMem(extraPtr, extraPtr+offset, dtLength-(UINTN)(extraPtr-dtEntry)-offset);
	*deviceTreeLength -= (UINT32)offset; 

	KextBase = RoundPage(dtEntry + *deviceTreeLength);
	if(!IsListEmpty(&gKextList)) {
		Index = 1;
		for (Link = gKextList.ForwardLink; Link != &gKextList; Link = Link->ForwardLink) {
			KextEntry = CR(Link, KEXT_ENTRY, Link, KEXT_SIGNATURE);

			CopyMem((VOID*) KextBase, (VOID*)(UINTN) KextEntry->kext.paddr, KextEntry->kext.length);
			drvinfo = (_BooterKextFileInfo*) KextBase;
			drvinfo->infoDictPhysAddr += (UINT32) KextBase;
			drvinfo->executablePhysAddr += (UINT32) KextBase;
			drvinfo->bundlePathPhysAddr += (UINT32) KextBase;

			memmapEntry->nProperties++;
			prop = ((DeviceTreeNodeProperty*) drvPtr);
			prop->length = sizeof(_DeviceTreeBuffer);
			mm = (_DeviceTreeBuffer*) (((UINT8*)prop) + sizeof(DeviceTreeNodeProperty));
			mm->paddr = (UINT32) KextBase;
			mm->length = KextEntry->kext.length;
			AsciiSPrint(prop->name, 31, "Driver-%x", KextBase);

			drvPtr += sizeof(DeviceTreeNodeProperty) + sizeof(_DeviceTreeBuffer);
			KextBase = RoundPage (KextBase + KextEntry->kext.length);
			Index++;
		}
	} else {
#ifdef KEXT_INJECT_DEBUG
    Print (L"Kext Inject: kext list is empty.\n");
#endif
		return EFI_INVALID_PARAMETER;
  }

	return EFI_SUCCESS;
}


////////////////////////////////////
//
// KernelBooterExtensionsPatch to load extra kexts besides kernelcache
//
// 

UINT8   KBELionSearch_X64[]  = { 0xE8, 0x0C, 0xFD, 0xFF, 0xFF, 0xEB, 0x08, 0x48, 0x89, 0xDF };
UINT8   KBELionReplace_X64[] = { 0xE8, 0x0C, 0xFD, 0xFF, 0xFF, 0x90, 0x90, 0x48, 0x89, 0xDF };

UINT8   KBELionSearch_i386[]  = { 0xE8, 0xAA, 0xFB, 0xFF, 0xFF, 0xEB, 0x08, 0x89, 0x34, 0x24 };
UINT8   KBELionReplace_i386[] = { 0xE8, 0xAA, 0xFB, 0xFF, 0xFF, 0x90, 0x90, 0x89, 0x34, 0x24 };

UINT8   KBEMLSearch[]  = { 0xC6, 0xE8, 0x30, 0x00, 0x00, 0x00, 0xEB, 0x08, 0x48, 0x89, 0xDF };
UINT8   KBEMLReplace[] = { 0xC6, 0xE8, 0x30, 0x00, 0x00, 0x00, 0x90, 0x90, 0x48, 0x89, 0xDF };

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

  UINTN   Num = 0;
  UINTN   NumLion_X64 = 0;
  UINTN   NumLion_i386 = 0;
  UINTN   NumML = 0;
  
  if (is64BitKernel) {
    NumLion_X64 = SearchAndCount (Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBELionSearch_X64, sizeof(KBELionSearch_X64));
    NumML = SearchAndCount (Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBEMLSearch, sizeof(KBEMLSearch));
  } else {
    NumLion_i386 = SearchAndCount (Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBELionSearch_i386, sizeof(KBELionSearch_i386));
  }
  
  if (NumLion_X64 + NumLion_i386 + NumML > 1) {
    // more then one pattern found - we do not know what to do with it
    // and we'll skipp it
#ifdef KEXT_INJECT_DEBUG
		Print (L"Kext Inject: more then one pattern found - we do not know what to do with it.\n");
#endif
    return;
  }
  
  if (NumLion_X64 == 1) {
    Num = SearchAndReplace(Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBELionSearch_X64, sizeof(KBELionSearch_X64), (CHAR8 *) KBELionReplace_X64, 1);
  }
  else if (NumLion_i386 == 1) {
    Num = SearchAndReplace(Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBELionSearch_i386, sizeof(KBELionSearch_i386), (CHAR8 *) KBELionReplace_i386, 1);
  }
  else if (NumML == 1) {
    Num = SearchAndReplace(Kernel, KERNEL_MAX_SIZE, (CHAR8 *) KBEMLSearch, sizeof(KBEMLSearch), (CHAR8 *) KBEMLReplace, 1);
  }
#ifdef KEXT_INJECT_DEBUG
  Print (L"Kext Inject: SearchAndReplace %d times.\n", Num);
#endif
}
