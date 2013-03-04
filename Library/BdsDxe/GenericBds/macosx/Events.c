/*
Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

*/

#include "macosx.h"

EFI_EVENT   mVirtualAddressChangeEvent = NULL;
EFI_EVENT   OnReadyToBootEvent = NULL;
EFI_EVENT   ExitBootServiceEvent = NULL;

#if 0
#include "device_tree.h"

#define NextMemoryDescriptor(_Ptr, _Size)   ((EFI_MEMORY_DESCRIPTOR *) (((UINT8 *) (_Ptr)) + (_Size)))
#define ArchConvertPointerToAddress(A)    ((UINT64)(A))
#define LdrStaticVirtualToPhysical(V)   ((V) & (1 * 1024 * 1024 * 1024 - 1))

UINT64
AllocateKernelMemory (
  UINTN* bufferLength,
  UINT64* virtualAddress
)
{
  UINTN  pagesCount = EFI_SIZE_TO_PAGES (*bufferLength);
  UINT64 kernelVirtualAddress = 0;
  UINT64 physicalAddress = LdrStaticVirtualToPhysical (kernelVirtualAddress);
  UINT64 MmpKernelVirtualBegin = (UINT64) (-1);
  UINT64 MmpKernelVirtualEnd = 0;
  UINT64 MmpKernelPhysicalBegin = (UINT64) (-1);
  UINT64 MmpKernelPhysicalEnd = 0;

  if (!bufferLength || !gBS) {
    return 0;
  }

  if (!virtualAddress || *virtualAddress == 0) {
    if (MmpKernelVirtualBegin == -1) {
      return 0;
    }

    kernelVirtualAddress = MmpKernelVirtualEnd;
  } else {
    kernelVirtualAddress = *virtualAddress;
  }

  if (!AllocatePages (AllocateAddress)
      && AllocatePages (EfiLoaderData)
      && AllocatePages (pagesCount)
      && AllocatePages ((UINTN) &physicalAddress)) {
    return 0;
  }

  *bufferLength = pagesCount << EFI_PAGE_SHIFT;

  if (kernelVirtualAddress < MmpKernelVirtualBegin) {
    MmpKernelVirtualBegin = kernelVirtualAddress;
  }

  if (kernelVirtualAddress + *bufferLength > MmpKernelVirtualEnd) {
    MmpKernelVirtualEnd = kernelVirtualAddress + *bufferLength;
  }

  if (physicalAddress < MmpKernelPhysicalBegin) {
    MmpKernelPhysicalBegin = physicalAddress;
  }

  if (physicalAddress + *bufferLength > MmpKernelPhysicalEnd) {
    MmpKernelPhysicalEnd = physicalAddress + *bufferLength;
  }

  if (virtualAddress) {
    *virtualAddress = kernelVirtualAddress;
  }

  return physicalAddress;
}

VOID
GetKernelPhysicalRange (
  UINT64* lowerAddress,
  UINT64* upperAddress
)
{
  UINT64 KernelPhysicalBegin = (UINT64) (-1);
  UINT64 KernelPhysicalEnd = 0;
  *lowerAddress = KernelPhysicalBegin;
  *upperAddress = KernelPhysicalEnd;
}

UINT64
GetKernelVirtualStart (
  VOID
)
{
  UINT64 KernelVirtualBegin = (UINT64) (-1);
  return KernelVirtualBegin;
}

EFI_STATUS
GetMemoryMap (
  UINTN* memoryMapSize,
  EFI_MEMORY_DESCRIPTOR** memoryMap,
  UINTN* memoryMapKey,
  UINTN* descriptorSize,
  UINT32* descriptorVersion
)
{
  EFI_STATUS             Status = EFI_SUCCESS;
  EFI_MEMORY_DESCRIPTOR* allocatedMap = 0;
  UINTN                  allocatedSize = 0;
  UINTN                  i;

  for (i = 0; i < 5; i ++) {
    UINTN currentSize = 0;
    Status = gBS->GetMemoryMap (&currentSize, 0, memoryMapKey, descriptorSize, descriptorVersion);

    if (Status != EFI_BUFFER_TOO_SMALL) {
      return Status;
    }

    if (currentSize > allocatedSize) {
      if (allocatedMap) {
        FreePool (allocatedMap);
      }

      allocatedSize = currentSize + 512;
      currentSize = allocatedSize;
      allocatedMap = (EFI_MEMORY_DESCRIPTOR*) (AllocatePool (allocatedSize));

      if (!allocatedMap) {
        return EFI_OUT_OF_RESOURCES;
      }
    }

    Status = gBS->GetMemoryMap (&currentSize, allocatedMap, memoryMapKey, descriptorSize, descriptorVersion);

    if (!EFI_ERROR (Status)) {
      *memoryMap = allocatedMap;
      *memoryMapSize = currentSize;
      break;
    }
  }

  return Status;
}
#endif

VOID
EFIAPI
OnExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *Context
)
{
#if 0
  UINT8             *DTptr = (UINT8*) (UINTN) 0x100000;
  BootArgs1         *bootArgs;
  UINTN             archMode = sizeof (UINTN) * 8;
  CHAR8             *dtRoot;
  UINT64            kernelBegin = 0;
  UINT64            kernelEnd = 0;

  BootArgs2*        bootArgs2;
  DTEntry           efiPlatform;
  UINTN             Version = 0;
  EFI_STATUS                      Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput = NULL;
  EFI_MEMORY_DESCRIPTOR           *memoryMap = 0;
  UINTN                           memoryMapSize = 0;
  UINTN                           memoryMapKey = 0;
  UINTN                           descriptorSize = 0;
  UINT32                          descriptorVersion = 0;
  UINT64                          runtimeMemoryPages = 0;
  UINT64                          runtimeServicesVirtualAddress = 0;
  UINTN                           bufferLength = 0;
  UINT64                          efiSystemTablePhysicalAddress = ArchConvertPointerToAddress (gST);
  UINT64                          runtimeServicesPhysicalAddress = AllocateKernelMemory (&bufferLength, &runtimeServicesVirtualAddress);
  UINT32                      gBootMode = GRAPHICS_MODE;

  Status = GetMemoryMap (&memoryMapSize, &memoryMap, &memoryMapKey, &descriptorSize, &descriptorVersion);

  if (EFI_ERROR (Status)) {
    Print (L"Cannot Get MemoryMap!\n");
    gBS->Stall (2000000);
    return;
  }

  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **) &GraphicsOutput);

  if (EFI_ERROR (Status)) {
    Print (L"Cannot Find Graphics Output Protocol\n");
    gBS->Stall (2000000);
    return;
  }

  memoryMap = NextMemoryDescriptor (memoryMap, descriptorSize);
  runtimeMemoryPages += memoryMap->NumberOfPages;

  while (TRUE) {
    bootArgs = (BootArgs1*) DTptr;

    if ((bootArgs->Revision == 6 || bootArgs->Revision == 5 || bootArgs->Revision == 4) &&
    (bootArgs->Version == 1) && ((UINTN) bootArgs->efiMode == 32 || (UINTN) bootArgs->efiMode == 64)) {
      dtRoot = (CHAR8*) (UINTN) bootArgs->deviceTreeP;
      bootArgs->efiMode = archMode;
      break;
    }

    DTptr += 0x1000;

    if ((UINT32) (UINTN) DTptr > 0x3000000) {
      Print (L"bootArgs not found!\n");
      gBS->Stall (2000000);
      return;
    }
  }

  CorrectMemoryMap (bootArgs1->MemoryMap,
                    bootArgs1->MemoryMapDescriptorSize,
                    &bootArgs1->MemoryMapSize);
  GetKernelPhysicalRange (&kernelBegin, &kernelEnd);

  bootArgs->MemoryMap = (UINT32) (UINTN) memoryMap;
  bootArgs->MemoryMap = (UINT32) memoryMap->VirtualStart;
  bootArgs->MemoryMapSize = (UINT32) memoryMapSize;
  bootArgs->MemoryMapDescriptorSize = (UINT32) descriptorSize;
  bootArgs->MemoryMapDescriptorVersion = (UINT32) descriptorVersion;
  bootArgs->Video.v_baseAddr = (UINT32) GraphicsOutput->Mode->FrameBufferBase;
  bootArgs->Video.v_display = (UINT32) gBootMode;
  bootArgs->Video.v_width = (UINT32) GraphicsOutput->Mode->Info->HorizontalResolution;
  bootArgs->Video.v_height = (UINT32) GraphicsOutput->Mode->Info->VerticalResolution;
  bootArgs->Video.v_depth = 32;
  bootArgs->Video.v_rowBytes = (UINT32) (GraphicsOutput->Mode->Info->PixelsPerScanLine * 32) / 8;
  bootArgs->kaddr = (UINT32) (kernelBegin);
  bootArgs->ksize = (UINT32) (kernelEnd - kernelBegin);
  bootArgs->efiRuntimeServicesPageStart = (UINT32) runtimeServicesPhysicalAddress >> EFI_PAGE_SHIFT;
  bootArgs->efiRuntimeServicesPageCount = (UINT32) runtimeMemoryPages;
  bootArgs->efiRuntimeServicesVirtualPageStart = (UINT64) runtimeServicesVirtualAddress >> EFI_PAGE_SHIFT;
  bootArgs->efiSystemTable = (UINT32) efiSystemTablePhysicalAddress;
  bootArgs->efiMode = GetKernelVirtualStart() > (UINTN) (-1) ? sizeof (UINT64) * 8 : sizeof (UINTN) * 8;

  bootArgs->efiSystemTable = (UINT32) (UINTN) gST;
  USBOwnerFix();
  DisableUsbLegacySupport();
#endif
}

VOID
EFIAPI
OnReadyToBoot (
               IN      EFI_EVENT   Event,
               IN      VOID        *Context
               )
{
//
}

VOID
EFIAPI
VirtualAddressChangeEvent (
                           IN EFI_EVENT  Event,
                           IN VOID       *Context
                           )
{
#if 0
  EfiConvertPointer (0x0, (VOID **) &mProperty);
  EfiConvertPointer (0x0, (VOID **) &mSmmCommunication);
#endif
}

EFI_STATUS
EFIAPI
EventsInitialize (
  VOID
)
{
  //
  // Register the event to reclaim variable for OS usage.
  //
#if 0
  EfiCreateEventReadyToBoot (&OnReadyToBootEvent);
  EfiCreateEventReadyToBootEx (
    TPL_NOTIFY, 
    OnReadyToBoot, 
    NULL, 
    &OnReadyToBootEvent
    );
#endif
  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         OnExitBootServices,
         NULL,
         &gEfiEventExitBootServicesGuid,
         &ExitBootServiceEvent
         ); 

  //
  // Register the event to convert the pointer for runtime.
  //
  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         VirtualAddressChangeEvent,
         NULL,
         &gEfiEventVirtualAddressChangeGuid,
         &mVirtualAddressChangeEvent
         );
  
  return EFI_SUCCESS;
}
