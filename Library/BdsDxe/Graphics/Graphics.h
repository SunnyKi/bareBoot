/** @file

**/

#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include <Protocol/GraphicsOutput.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include <IndustryStandard/Bmp.h>

//#include <FrameworkDxe.h>

//#include <IndustryStandard/Pci.h>
//#include <IndustryStandard/PeImage.h>

//#include <Protocol/BlockIo.h>
//#include <Protocol/LoadedImage.h>
//#include <Protocol/Cpu.h>
//#include <Protocol/SimpleFileSystem.h>
//#include <Protocol/LoadFile.h>
//#include <Protocol/DebugPort.h>
//#include <Protocol/DevicePath.h>
//#include <Protocol/SimpleTextIn.h>
//#include <Protocol/LegacyBios.h>
//#include <Protocol/SimpleTextOut.h>
//#include <Protocol/SimpleNetwork.h>
//#include <Protocol/DevicePathToText.h>
//#include <Protocol/FirmwareVolume2.h>
//#include <Protocol/PciIo.h>
//#include <Protocol/AcpiS3Save.h>
//#include <Protocol/OEMBadging.h>
//#include <Protocol/UgaDraw.h>
//#include <Protocol/HiiFont.h>
//#include <Protocol/HiiImage.h>
//#include <Protocol/UsbIo.h>

//#include <Guid/MemoryTypeInformation.h>
//#include <Guid/FileInfo.h>
//#include <Guid/GlobalVariable.h>
//#include <Guid/PcAnsi.h>
//#include <Guid/Performance.h>
//#include <Guid/BdsLibHii.h>
//#include <Guid/HdBootVariable.h>
//#include <Guid/LastEnumLang.h>

//#include <Library/PrintLib.h>
//#include <Library/DebugLib.h>
//#include <Library/UefiLib.h>
//#include <Library/DxeServicesTableLib.h>
//#include <Library/UefiRuntimeServicesTableLib.h>
//#include <Library/HobLib.h>
//#include <Library/DevicePathLib.h>
//#include <Library/PerformanceLib.h>
//#include <Library/PcdLib.h>
//#include <Library/PeCoffGetEntryPointLib.h>
//#include <GenericBdsLib.h>
//#include <Library/TimerLib.h>
//#include <Library/PcdLib.h>
//#include <Library/DxeServicesLib.h>
//#include <Library/ReportStatusCodeLib.h>

EFI_STATUS
ConvertPngToGopBlt (
  IN     VOID      *PngImage,
  IN     UINTN     PngImageSize,
  IN OUT VOID      **GopBlt,
  IN OUT UINTN     *GopBltSize,
     OUT UINTN     *PixelHeight,
     OUT UINTN     *PixelWidth
  );

EFI_STATUS
ConvertBmpToGopBlt (
  IN     VOID      *BmpImage,
  IN     UINTN     BmpImageSize,
  IN OUT VOID      **GopBlt,
  IN OUT UINTN     *GopBltSize,
  OUT UINTN     *PixelHeight,
  OUT UINTN     *PixelWidth
  );

VOID
ClearScreen (
  UINT32     HexColor
  );

VOID
SetModeScreen (
 UINT32     PrefMode
 );

VOID
UseAlpha (
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput,
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt,
  UINT32 DestX,
  UINT32 DestY,
  UINT32 Width,
  UINT32 Height
  );

EFI_STATUS
BltWithAlpha (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
  IN UINTN SourceX,
  IN UINTN SourceY,
  IN UINTN DestinationX,
  IN UINTN DestinationY,
  IN UINTN Width,
  IN UINTN Height,
  IN UINTN Delta,
  IN BOOLEAN Alpha
  );

EFI_STATUS
ShowPngFile (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
  IN CHAR16 *FileName,
  IN INTN   DestX,
  IN INTN   DestY,
  IN BOOLEAN Alpha
  );

EFI_STATUS
ShowAString (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput,
  IN CHAR8  *AString,
  IN INTN   DestX,
  IN INTN   DestY,
  IN BOOLEAN Alpha
  );

#endif // _GRAPHICS_H_
