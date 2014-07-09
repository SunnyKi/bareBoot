
/** @file
**/

#include <macosx.h>

#include "Graphics.h"
#include "picopng.h"

EFI_STATUS
ConvertPngToGopBlt (
  IN VOID *PngImage,
  IN UINTN PngImageSize,
  IN OUT VOID **GopBlt,
  IN OUT UINTN *GopBltSize,
  OUT UINTN *PixelHeight,
  OUT UINTN *PixelWidth
)
{
  UINT32 InX;
  UINT32 InY;
  UINT64 BltBufferSize;
  PNG_info_t *PngInfo;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *RawImage;

  RawImage = NULL;

  PngInfo = PNG_decode ((UINT8 *) PngImage, (UINT32) PngImageSize);

  if (PNG_error == 0) {
    BltBufferSize = MultU64x32 (PngInfo->width, PngInfo->height);
    BltBufferSize =
      MultU64x32 (BltBufferSize, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    *GopBltSize = (UINTN) BltBufferSize;

    *GopBlt = AllocateZeroPool ((UINTN) BltBufferSize);
    Buffer = *GopBlt;

    *PixelHeight = PngInfo->height;
    *PixelWidth = PngInfo->width;

    RawImage = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) PngInfo->image->data;

    for (InY = 0; InY < PngInfo->height; InY++) {
      for (InX = 0; InX < PngInfo->width; InX++) {
        Buffer->Red = RawImage->Blue;
        Buffer->Blue = RawImage->Red;
        Buffer->Green = RawImage->Green;
        Buffer->Reserved = RawImage->Reserved;

        Buffer++;
        RawImage++;
      }
    }

    png_alloc_free_all ();

  }
  return (EFI_STATUS) PNG_error;
}

/**
 Convert a *.BMP graphics image to a GOP blt buffer. If a NULL Blt buffer
 is passed in a GopBlt buffer will be allocated by this routine. If a GopBlt
 buffer is passed in it will be used if it is big enough.
 
 @param  BmpImage      Pointer to BMP file
 @param  BmpImageSize  Number of bytes in BmpImage
 @param  GopBlt        Buffer containing GOP version of BmpImage.
 @param  GopBltSize    Size of GopBlt in bytes.
 @param  PixelHeight   Height of GopBlt/BmpImage in pixels
 @param  PixelWidth    Width of GopBlt/BmpImage in pixels
 
 @retval EFI_SUCCESS           GopBlt and GopBltSize are returned.
 @retval EFI_UNSUPPORTED       BmpImage is not a valid *.BMP image
 @retval EFI_BUFFER_TOO_SMALL  The passed in GopBlt buffer is not big enough.
 GopBltSize will contain the required size.
 @retval EFI_OUT_OF_RESOURCES  No enough buffer to allocate.
 
 **/
EFI_STATUS
ConvertBmpToGopBlt (
  IN VOID *BmpImage,
  IN UINTN BmpImageSize,
  IN OUT VOID **GopBlt,
  IN OUT UINTN *GopBltSize,
  OUT UINTN *PixelHeight,
  OUT UINTN *PixelWidth
)
{
  UINT8 *Image;
  UINT8 *ImageHeader;
  BMP_IMAGE_HEADER *BmpHeader;
  BMP_COLOR_MAP *BmpColorMap;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  UINT64 BltBufferSize;
  UINTN Index;
  UINTN Height;
  UINTN Width;
  UINTN ImageIndex;
  UINT32 DataSizePerLine;
  BOOLEAN IsAllocated;
  UINT32 ColorMapNum;

  if (sizeof (BMP_IMAGE_HEADER) > BmpImageSize) {
    return EFI_INVALID_PARAMETER;
  }

  BmpHeader = (BMP_IMAGE_HEADER *) BmpImage;

  if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
    return EFI_UNSUPPORTED;
  }

  //
  // Doesn't support compress.
  //
  if (BmpHeader->CompressionType != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Only support BITMAPINFOHEADER format.
  // BITMAPFILEHEADER + BITMAPINFOHEADER = BMP_IMAGE_HEADER
  //
  if (BmpHeader->HeaderSize !=
      sizeof (BMP_IMAGE_HEADER) - OFFSET_OF (BMP_IMAGE_HEADER, HeaderSize)) {
    return EFI_UNSUPPORTED;
  }

  //
  // The data size in each line must be 4 byte alignment.
  //
  DataSizePerLine =
    ((BmpHeader->PixelWidth * BmpHeader->BitPerPixel + 31) >> 3) & (~0x3);
  BltBufferSize = MultU64x32 (DataSizePerLine, BmpHeader->PixelHeight);
  if (BltBufferSize > (UINT32) ~0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BmpHeader->Size != BmpImageSize) ||
      (BmpHeader->Size < BmpHeader->ImageOffset) ||
      (BmpHeader->Size - BmpHeader->ImageOffset !=
       BmpHeader->PixelHeight * DataSizePerLine)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Calculate Color Map offset in the image.
  //
  Image = BmpImage;
  BmpColorMap = (BMP_COLOR_MAP *) (Image + sizeof (BMP_IMAGE_HEADER));

  if (BmpHeader->ImageOffset < sizeof (BMP_IMAGE_HEADER)) {
    return EFI_INVALID_PARAMETER;
  }

  if (BmpHeader->ImageOffset > sizeof (BMP_IMAGE_HEADER)) {
    switch (BmpHeader->BitPerPixel) {
    case 1:
      ColorMapNum = 2;
      break;
    case 4:
      ColorMapNum = 16;
      break;
    case 8:
      ColorMapNum = 256;
      break;
    default:
      ColorMapNum = 0;
      break;
    }
    //
    // BMP file may has padding data between the bmp header section and the bmp data section.
    //
    if (BmpHeader->ImageOffset - sizeof (BMP_IMAGE_HEADER) <
        sizeof (BMP_COLOR_MAP) * ColorMapNum) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Calculate graphics image data address in the image
  //
  Image = ((UINT8 *) BmpImage) + BmpHeader->ImageOffset;
  ImageHeader = Image;

  //
  // Calculate the BltBuffer needed size.
  //
  BltBufferSize =
    MultU64x32 ((UINT64) BmpHeader->PixelWidth, BmpHeader->PixelHeight);
  //
  // Ensure the BltBufferSize * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) doesn't overflow
  //
  if (BltBufferSize >
      DivU64x32 ((UINTN) ~0, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))) {
    return EFI_UNSUPPORTED;
  }
  BltBufferSize =
    MultU64x32 (BltBufferSize, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

  IsAllocated = FALSE;
  if (*GopBlt == NULL) {
    //
    // GopBlt is not allocated by caller.
    //
    *GopBltSize = (UINTN) BltBufferSize;
    *GopBlt = AllocatePool (*GopBltSize);
    IsAllocated = TRUE;
    if (*GopBlt == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }
  else {
    //
    // GopBlt has been allocated by caller.
    //
    if (*GopBltSize < (UINTN) BltBufferSize) {
      *GopBltSize = (UINTN) BltBufferSize;
      return EFI_BUFFER_TOO_SMALL;
    }
  }

  *PixelWidth = BmpHeader->PixelWidth;
  *PixelHeight = BmpHeader->PixelHeight;

  //
  // Convert image from BMP to Blt buffer format
  //
  BltBuffer = *GopBlt;
  for (Height = 0; Height < BmpHeader->PixelHeight; Height++) {
    Blt =
      &BltBuffer[(BmpHeader->PixelHeight - Height - 1) * BmpHeader->PixelWidth];
    for (Width = 0; Width < BmpHeader->PixelWidth; Width++, Image++, Blt++) {
      switch (BmpHeader->BitPerPixel) {
      case 1:
        //
        // Convert 1-bit (2 colors) BMP to 24-bit color
        //
        for (Index = 0; Index < 8 && Width < BmpHeader->PixelWidth; Index++) {
          Blt->Red = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Red;
          Blt->Green = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Green;
          Blt->Blue = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Blue;
          Blt++;
          Width++;
        }

        Blt--;
        Width--;
        break;

      case 4:
        //
        // Convert 4-bit (16 colors) BMP Palette to 24-bit color
        //
        Index = (*Image) >> 4;
        Blt->Red = BmpColorMap[Index].Red;
        Blt->Green = BmpColorMap[Index].Green;
        Blt->Blue = BmpColorMap[Index].Blue;
        if (Width < (BmpHeader->PixelWidth - 1)) {
          Blt++;
          Width++;
          Index = (*Image) & 0x0f;
          Blt->Red = BmpColorMap[Index].Red;
          Blt->Green = BmpColorMap[Index].Green;
          Blt->Blue = BmpColorMap[Index].Blue;
        }
        break;

      case 8:
        //
        // Convert 8-bit (256 colors) BMP Palette to 24-bit color
        //
        Blt->Red = BmpColorMap[*Image].Red;
        Blt->Green = BmpColorMap[*Image].Green;
        Blt->Blue = BmpColorMap[*Image].Blue;
        break;

      case 24:
        //
        // It is 24-bit BMP.
        //
        Blt->Blue = *Image++;
        Blt->Green = *Image++;
        Blt->Red = *Image;
        break;

      default:
        //
        // Other bit format BMP is not supported.
        //
        if (IsAllocated) {
          FreePool (*GopBlt);
          *GopBlt = NULL;
        }
        return EFI_UNSUPPORTED;
        break;
      };

    }

    ImageIndex = (UINTN) (Image - ImageHeader);
    if ((ImageIndex % 4) != 0) {
      //
      // Bmp Image starts each row on a 32-bit boundary!
      //
      Image = Image + (4 - (ImageIndex % 4));
    }
  }

  return EFI_SUCCESS;
}

VOID
ClearScreen (
  UINT32 HexColor,
  EFI_GUID *NameGuid
)
{
  EFI_STATUS Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Pixel;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  UINT8                         *ImageData;
  UINTN                         ImageSize;
  UINTN                         BltSize;
  UINTN                         Height;
  UINTN                         Width;
  INTN                          DestX;
  INTN                          DestY;
  

  Status = EFI_SUCCESS;
  GraphicsOutput = NULL;
  Pixel = NULL;
  Blt = NULL;
  ImageData = NULL;
  ImageSize = 0;

  Status =
    gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid,
                         (VOID **) &GraphicsOutput);
  if (EFI_ERROR (Status)) {
    return;
  }

  CopyMem ((UINT8 *) Pixel, (UINT8 *) &HexColor,
           sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

#if 0
  Pixel = AllocateZeroPool (sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  Pixel->Blue = 0;
  Pixel->Green = 0;
  Pixel->Red = 0;
  Pixel->Reserved = 0;
#endif
  GraphicsOutput->Blt (GraphicsOutput, Pixel, EfiBltVideoFill, 0, 0, 0, 0,
                       GraphicsOutput->Mode->Info->HorizontalResolution,
                       GraphicsOutput->Mode->Info->VerticalResolution, 0);

  if (NameGuid != NULL) {
    Status = GetSectionFromAnyFv (
               NameGuid,
               EFI_SECTION_RAW,
               0,
               (VOID **) &ImageData,
               &ImageSize
               );
    
    if (EFI_ERROR (Status)) {
      DBG ("%a: GetSectionFromAnyFv  fail with status: %r", __FUNCTION__, Status);
      goto Down;
    }
    
    Status =
    ConvertPngToGopBlt (ImageData, ImageSize, (VOID **) &Blt, &BltSize, &Height,
                        &Width);
    
    if (EFI_ERROR (Status)) {
      DBG ("%a: ConvertPngToGopBlt  fail with status: %r", __FUNCTION__, Status);
      goto Down;
    }

    DestX = (GraphicsOutput->Mode->Info->HorizontalResolution - Width) / 2;
    DestY = (GraphicsOutput->Mode->Info->VerticalResolution - Height) / 2;

    if ((DestX >= 0) && (DestY >= 0)) {
      Status =
      BltWithAlpha (GraphicsOutput, Blt, EfiBltBufferToVideo, 0, 0,
                    (UINTN) DestX, (UINTN) DestY, Width, Height,
                    Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), TRUE);
      if (EFI_ERROR (Status)) {
        DBG ("%a: BltWithAlpha  fail with status: %r", __FUNCTION__, Status);
      }
    }

  Down:
    if (Pixel != NULL) {
      FreePool (Pixel);
    }
    if (ImageData != NULL) {
      FreePool (ImageData);
    }
    if (Blt != NULL) {
      FreePool (Blt);
    }
  }
}

VOID
SetModeScreen (
  UINT32 PrefMode
)
{
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;
  EFI_HANDLE *HandleBuffer;
  UINTN HandleCount;
  UINTN Index;

  GraphicsOutput = NULL;

  Status =
    gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid,
                         (VOID **) &GraphicsOutput);
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = GraphicsOutput->SetMode (GraphicsOutput, PrefMode);

  Status =
    gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleTextOutProtocolGuid, NULL,
                             &HandleCount, &HandleBuffer);

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
    }
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }
    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }
  }
}

VOID
UseAlpha (
  EFI_GRAPHICS_OUTPUT_PROTOCOL * GraphicsOutput,
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL * Foreground,
  UINT32 DestX,
  UINT32 DestY,
  UINT32 Width,
  UINT32 Height
)
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32 InX;
  UINT32 InY;
  UINTN Temp;
  UINT8 *Buffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *FrgnPixel;

  if (GraphicsOutput == NULL) {
    return;
  }

  Buffer =
    AllocatePool (Width * Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

  Status =
    GraphicsOutput->Blt (GraphicsOutput,
                         (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) Buffer,
                         EfiBltVideoToBltBuffer, DestX, DestY, 0, 0, Width,
                         Height, Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
    );
  if (EFI_ERROR (Status)) {
    goto Down;
  }

  FrgnPixel = Foreground;

  for (InY = 0; InY < Height; InY++) {
    for (InX = 0; InX < Width; InX++) {
      Background =
        (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) (Buffer + (InY * Width * 4) +
                                           (InX * 4));

      Temp =
        Background->Blue * (255 - FrgnPixel->Reserved) +
        FrgnPixel->Blue * FrgnPixel->Reserved + 0x80;
      FrgnPixel->Blue = (UINT8) ((Temp + (Temp >> 8)) >> 8);
      Temp =
        Background->Green * (255 - FrgnPixel->Reserved) +
        FrgnPixel->Green * FrgnPixel->Reserved + 0x80;
      FrgnPixel->Green = (UINT8) ((Temp + (Temp >> 8)) >> 8);
      Temp =
        Background->Red * (255 - FrgnPixel->Reserved) +
        FrgnPixel->Red * FrgnPixel->Reserved + 0x80;
      FrgnPixel->Red = (UINT8) ((Temp + (Temp >> 8)) >> 8);

      FrgnPixel++;
    }
  }

Down:
  if (Buffer != NULL) {
    FreePool (Buffer);
  }
  return;
}

EFI_STATUS
BltWithAlpha (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL * GraphicsOutput,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL * BltBuffer,
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
  IN UINTN SourceX,
  IN UINTN SourceY,
  IN UINTN DestinationX,
  IN UINTN DestinationY,
  IN UINTN Width,
  IN UINTN Height,
  IN UINTN Delta,
  IN BOOLEAN Alpha
)
{
  EFI_STATUS Status;

  if (GraphicsOutput == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Alpha) {
#if 1
    UseAlpha (GraphicsOutput, BltBuffer, (UINT32) DestinationX,
              (UINT32) DestinationY, (UINT32) Width, (UINT32) Height);
#else
    UseAlpha (GraphicsOutput, BltBuffer, DestinationX, DestinationY, Width,
              Height);
#endif
  }

  Status =
    GraphicsOutput->Blt (GraphicsOutput, BltBuffer, BltOperation, SourceX,
                         SourceY, DestinationX, DestinationY, Width, Height,
                         Delta);

  return Status;
}

EFI_STATUS
ShowPngFile (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL * GraphicsOutput,
  IN CHAR16 *FileName,
  IN INTN DestX,
  IN INTN DestY,
  IN BOOLEAN Alpha
)
{
  EFI_STATUS Status;
  UINT8 *ImageData;
  UINTN ImageSize;
  UINTN BltSize;
  UINTN Height;
  UINTN Width;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;

  Blt = NULL;
  ImageData = NULL;
  ImageSize = 0;

  Status = egLoadFile (gRootFHandle, FileName, &ImageData, &ImageSize);
  if (EFI_ERROR (Status)) {
    goto Down;
  }

  Status =
    ConvertPngToGopBlt (ImageData, ImageSize, (VOID **) &Blt, &BltSize, &Height,
                        &Width);

  if (EFI_ERROR (Status)) {
    goto Down;
  }

  if ((DestX >= 0) && (DestY >= 0)) {
    Status =
      BltWithAlpha (GraphicsOutput, Blt, EfiBltBufferToVideo, 0, 0,
                    (UINTN) DestX, (UINTN) DestY, Width, Height,
                    Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), Alpha);
  }

Down:
  if (ImageData != NULL) {
    FreeAlignedPages (ImageData, EFI_SIZE_TO_PAGES (ImageSize));
  }
  if (Blt != NULL) {
    FreePool (Blt);
  }
  return Status;
}

EFI_STATUS
ShowAString (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL * GraphicsOutput,
  IN CHAR8 *AString,
  IN INTN DestX,
  IN INTN DestY,
  IN BOOLEAN Alpha
)
{
  EFI_STATUS Status;
  UINT8 *ImageData;
  UINTN ImageSize;
  UINTN BltSize;
  UINTN Height;
  UINTN Width;
  UINTN ImageStringWidth;
  UINT8 FontWidth;
  UINT32 FontSize;
  UINT32 InX;
  UINT32 InStr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *ImageString;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *InBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *OutBuffer;

  ImageData = NULL;
  ImageString = NULL;
  Blt = NULL;

  if (AString == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Down;
  }

  Status =
    GetSectionFromAnyFv (PcdGetPtr (PcdFontsFile), EFI_SECTION_RAW, 0,
                         (VOID **) &ImageData, &ImageSize);

  if (EFI_ERROR (Status)) {
    goto Down;
  }

  Status =
    ConvertPngToGopBlt (ImageData, ImageSize, (VOID **) &Blt, &BltSize, &Height,
                        &Width);

  if (EFI_ERROR (Status)) {
    goto Down;
  }

  FontWidth = (UINT8) (Width / 95);
  ImageStringWidth = FontWidth * AsciiStrLen (AString);
  ImageString =
    AllocateZeroPool (Height * ImageStringWidth *
                      sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  FontSize = FontWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

  for (InStr = 0; InStr < AsciiStrLen (AString); InStr++) {
    InBuffer = ImageString + FontWidth * InStr;
    OutBuffer = Blt + FontWidth * (AString[InStr] - 0x20);
    for (InX = 0; InX < Height; InX++) {
      CopyMem ((UINT8 *) (InBuffer), (UINT8 *) (OutBuffer), FontSize);
      InBuffer += ImageStringWidth;
      OutBuffer += Width;
    }
  }

  if ((DestX >= 0) && (DestY >= 0)) {
    Status =
      BltWithAlpha (GraphicsOutput, ImageString, EfiBltBufferToVideo, 0, 0,
                    (UINTN) DestX, (UINTN) DestY, ImageStringWidth, Height,
                    ImageStringWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL),
                    Alpha);
  }

Down:
  if (ImageData != NULL) {
    FreePool (ImageData);
  }
  if (ImageString != NULL) {
    FreePool (ImageString);
  }
  if (Blt != NULL) {
    FreePool (Blt);
  }
  return Status;
}
