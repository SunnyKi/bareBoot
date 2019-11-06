/*++

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  EfiLoader.c

Abstract:

Revision History:

--*/

#include "EfiLdr.h"
#include "Support.h"
#include "Debug.h"
#include "PeLoader.h"
#include "LzmaDecompress.h"

#define EFILDR_VERBOSE 0

EFILDR_LOADED_IMAGE    DxeCoreImage;
EFILDR_LOADED_IMAGE    DxeIplImage;

VOID
SystemHang (
  CHAR8        *Message
  )
{
  PrintString (
    "%a## FATAL ERROR ##: Fail to load bareBoot images! System hang!\n",
    Message
    );
  CpuDeadLoop();
}

BOOLEAN
IsGoodEFILAddr (
  UINT32    EFILAddr
  )
{
  EFILDR_HEADER* eh;

  if (EFILAddr < 0x20000 || EFILAddr > 16 * 1024 * 1024) {
    return FALSE;
  }

  eh = (EFILDR_HEADER *)((UINTN) EFILAddr);

  // Signature is "EFIL" or 'LIFE'
  if (eh->Signature == 0x4C494645 && eh->NumberOfImages == 4) {
    return TRUE;
  }

  return FALSE;
}

VOID
EfiLoader (
  UINT32    BiosMemoryMapBaseAddress,
  UINT32    EFILAddr
  )
{
  BIOS_MEMORY_MAP       *BiosMemoryMap;    
  UINT8*                EFILBase;
  EFILDR_IMAGE          *EFILDRImage;
  EFI_MEMORY_DESCRIPTOR EfiMemoryDescriptor[EFI_MAX_MEMORY_DESCRIPTORS];
  EFI_STATUS            Status;
  UINTN                 NumberOfMemoryMapEntries;
  UINT32                DestinationSize;
  UINT32                ScratchSize;
  UINTN                 BfvPageNumber;
  UINTN                 BfvBase;
  EFI_MAIN_ENTRYPOINT   EfiMainEntrypoint;
  EFILDRHANDOFF         Handoff;

  ClearScreen();
#if EFILDR_VERBOSE
  PrintHeader ('A');
  
  PrintString ("Enter bareBoot Loader...\n");
  PrintString ("BiosMemoryMapBaseAddress = 0x%08x\n", (UINTN) BiosMemoryMapBaseAddress);
  PrintString ("EFILAddr = 0x%08x\n", (UINTN) EFILAddr);
#endif
  //
  // Add all EfiConventionalMemory descriptors to the table.  If there are partial pages, then
  // round the start address up to the next page, and round the length down to a page boundry.
  //
  BiosMemoryMap = (BIOS_MEMORY_MAP *) (UINTN) BiosMemoryMapBaseAddress;
  NumberOfMemoryMapEntries = 0;
  GenMemoryMap (&NumberOfMemoryMapEntries, EfiMemoryDescriptor, BiosMemoryMap);

#if EFILDR_VERBOSE
  PrintString ("Get %d entries of memory map!\n", NumberOfMemoryMapEntries);
#endif

  //
  // Get information on where the image is in memory
  //

  EFILBase  =  (UINT8 *)((UINTN) EFILAddr);
  if (!IsGoodEFILAddr (EFILAddr)) {
    EFILBase  = (UINT8 *)EFILDR_HEADER_ADDRESS;
    if (!IsGoodEFILAddr (EFILDR_HEADER_ADDRESS)) {
      SystemHang ("No valid FV Datum found!\n");
    }
  }
  EFILDRImage = (EFILDR_IMAGE *) (EFILBase + sizeof (EFILDR_HEADER));


  //
  // Point to the 4th image (Bfv)
  //
  EFILDRImage += 3;

#if EFILDR_VERBOSE
  PrintString (
    "Decompress BFV image, Image Address = 0x%x Offset = 0x%x\n", 
    (UINTN) (EFILBase + EFILDRImage->Offset),
    (UINTN) EFILDRImage->Offset
    );
#endif
  //
  // Decompress the image
  //
  Status = LzmaUefiDecompressGetInfo (
             (VOID *)(UINTN)(EFILBase + EFILDRImage->Offset),
             EFILDRImage->Length,
             &DestinationSize, 
             &ScratchSize
             );

  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to get decompress information for BFV!\n");
  }
#if EFILDR_VERBOSE
  PrintString ("BFV decompress: DestinationSize = 0x%x, ScratchSize = 0x%x\n", (UINTN) DestinationSize, (UINTN) ScratchSize);
#endif
  Status =  LzmaUefiDecompress (
    (VOID *)(UINTN)(EFILBase + EFILDRImage->Offset),
    EFILDRImage->Length,
    (VOID *)(UINTN)EFI_DECOMPRESSED_BUFFER_ADDRESS, 
    (VOID *)(UINTN)((EFI_DECOMPRESSED_BUFFER_ADDRESS + DestinationSize + 0x1000) & 0xfffff000)
    );
  

  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to decompress BFV!\n");
  }

  BfvPageNumber = EFI_SIZE_TO_PAGES (DestinationSize);
  BfvBase = (UINTN) FindSpace (BfvPageNumber, &NumberOfMemoryMapEntries, EfiMemoryDescriptor, EfiRuntimeServicesData, EFI_MEMORY_WB);
  if (BfvBase == 0) {
    SystemHang ("Failed to find free space to hold decompressed BFV\n");
  }
  ZeroMem ((VOID *)(UINTN)BfvBase, BfvPageNumber * EFI_PAGE_SIZE);
  CopyMem ((VOID *)(UINTN)BfvBase, (VOID *)(UINTN)EFI_DECOMPRESSED_BUFFER_ADDRESS, DestinationSize);

#if EFILDR_VERBOSE
  PrintHeader ('B');
#endif

  //
  // Point to the 2nd image (DxeIpl)
  //
    
  EFILDRImage -= 2;

#if EFILDR_VERBOSE
  PrintString (
    "Decompress DxeIpl image, Image Address = 0x%x Offset = 0x%x\n", 
    (UINTN) (EFILBase + EFILDRImage->Offset),
    (UINTN) EFILDRImage->Offset
    );
#endif
  //
  // Decompress the image
  //
  Status = LzmaUefiDecompressGetInfo (
             (VOID *)(UINTN)(EFILBase + EFILDRImage->Offset),
             EFILDRImage->Length,
             &DestinationSize, 
             &ScratchSize
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to get decompress information for DxeIpl!\n");
  }

  Status = LzmaUefiDecompress (
             (VOID *)(UINTN)(EFILBase + EFILDRImage->Offset),
             EFILDRImage->Length,
             (VOID *)(UINTN)EFI_DECOMPRESSED_BUFFER_ADDRESS,
             (VOID *)(UINTN)((EFI_DECOMPRESSED_BUFFER_ADDRESS + DestinationSize + 0x1000) & 0xfffff000)
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to decompress DxeIpl image\n");
  }

#if EFILDR_VERBOSE
  PrintString ("Start load DxeIpl PE image\n");  
#endif

  //
  // Load and relocate the EFI PE/COFF Firmware Image 
  //
  Status = EfiLdrPeCoffLoadPeImage (
             (VOID *)(UINTN)(EFI_DECOMPRESSED_BUFFER_ADDRESS), 
             &DxeIplImage, 
             &NumberOfMemoryMapEntries, 
             EfiMemoryDescriptor
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to load and relocate DxeIpl PE image!\n");
  }
#if EFILDR_VERBOSE
  PrintString (
    "DxeIpl PE image is successed loaded at 0x%lx, entry=0x%p\n",
    DxeIplImage.ImageBasePage,
    DxeIplImage.EntryPoint
    );

PrintHeader ('C');
#endif

  //
  // Point to the 3rd image (DxeMain)
  //

  EFILDRImage++;

#if EFILDR_VERBOSE
  PrintString (
    "Decompress DxeMain FV image, Image Address = 0x%x Offset = 0x%x\n",
    (UINTN)(EFILBase + EFILDRImage->Offset),
    (UINTN) EFILDRImage->Offset
    );
#endif
  //
  // Decompress the image
  //
  Status = LzmaUefiDecompressGetInfo (
             (VOID *)(UINTN)(EFILBase + EFILDRImage->Offset),
             EFILDRImage->Length,
             &DestinationSize, 
             &ScratchSize
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to get decompress information for DxeMain FV image!\n");
  }

  Status = LzmaUefiDecompress (
             (VOID *)(UINTN)(EFILBase + EFILDRImage->Offset),
              EFILDRImage->Length,
             (VOID *)(UINTN)EFI_DECOMPRESSED_BUFFER_ADDRESS,
             (VOID *)(UINTN)((EFI_DECOMPRESSED_BUFFER_ADDRESS + DestinationSize + 0x1000) & 0xfffff000)
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to decompress DxeMain FV image!\n");
  }

  //
  // Load and relocate the EFI PE/COFF Firmware Image 
  //
  Status = EfiLdrPeCoffLoadPeImage (
             (VOID *)(UINTN)(EFI_DECOMPRESSED_BUFFER_ADDRESS), 
             &DxeCoreImage, 
             &NumberOfMemoryMapEntries, 
             EfiMemoryDescriptor
             );
  if (EFI_ERROR (Status)) {
    SystemHang ("Failed to load/relocate DxeMain!\n");
  }
#if EFILDR_VERBOSE
  PrintString (
    "DxeCore PE image is successed loaded at 0x%lx, entry=0x%p\n",
    DxeCoreImage.ImageBasePage,
    DxeCoreImage.EntryPoint
    );

  PrintHeader ('E');

  //
  // Display the table of memory descriptors.
  //
  {
    INTN Index;
    PrintString ("\nEFI Memory Descriptors\n");   
    for (Index = 0; Index < (INTN) NumberOfMemoryMapEntries; Index++) {
      PrintString (
        "Type = 0x%x Start = 0x%08lx NumberOfPages = 0x%08lx\n",
        EfiMemoryDescriptor[Index].Type, EfiMemoryDescriptor[Index].PhysicalStart, EfiMemoryDescriptor[Index].NumberOfPages
        );
    }
  }
#endif
  //
  // Jump to EFI Firmware
  //

  if (DxeIplImage.EntryPoint != NULL) {

    Handoff.MemDescCount      = NumberOfMemoryMapEntries;
    Handoff.MemDesc           = EfiMemoryDescriptor;
    Handoff.BfvBase           = (VOID *)(UINTN)BfvBase;
    Handoff.BfvSize           = BfvPageNumber * EFI_PAGE_SIZE;
    Handoff.DxeIplImageBase   = (VOID *)(UINTN)DxeIplImage.ImageBasePage;
    Handoff.DxeIplImageSize   = DxeIplImage.NoPages * EFI_PAGE_SIZE;
    Handoff.DxeCoreImageBase  = (VOID *)(UINTN)DxeCoreImage.ImageBasePage;
    Handoff.DxeCoreImageSize  = DxeCoreImage.NoPages * EFI_PAGE_SIZE;
    Handoff.DxeCoreEntryPoint = (VOID *)(UINTN)DxeCoreImage.EntryPoint;

#if EFILDR_VERBOSE
    PrintString ("Transfer to DxeIpl ...EntryPoint = 0x%p\n", DxeIplImage.EntryPoint);
#endif
    
    EfiMainEntrypoint = (EFI_MAIN_ENTRYPOINT) DxeIplImage.EntryPoint;
    EfiMainEntrypoint (&Handoff);
  }

#if EFILDR_VERBOSE
  PrintHeader ('F');
#endif

  //
  // There was a problem loading the image, so HALT the system.
  //

  SystemHang ("Failed to jump to DxeIpl!\n");
}

EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  UINT32    BiosMemoryMapBaseAddress,
  UINT32    EFILAddr
  )
{
#if defined(MEMLOG2SERIAL) || !defined(MDEPKG_NDEBUG)
  SerialPortInitialize ();
#endif
  EfiLoader (BiosMemoryMapBaseAddress, EFILAddr);
  return EFI_SUCCESS;
}
