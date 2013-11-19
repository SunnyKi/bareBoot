/** @file
**/

#include <IndustryStandard/Bmp.h>

#include "InternalBdsLib.h"

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
  IN     VOID      *BmpImage,
  IN     UINTN     BmpImageSize,
  IN OUT VOID      **GopBlt,
  IN OUT UINTN     *GopBltSize,
     OUT UINTN     *PixelHeight,
     OUT UINTN     *PixelWidth
  )
{
  UINT8                         *Image;
  UINT8                         *ImageHeader;
  BMP_IMAGE_HEADER              *BmpHeader;
  BMP_COLOR_MAP                 *BmpColorMap;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  UINT64                        BltBufferSize;
  UINTN                         Index;
  UINTN                         Height;
  UINTN                         Width;
  UINTN                         ImageIndex;
  UINT32                        DataSizePerLine;
  BOOLEAN                       IsAllocated;
  UINT32                        ColorMapNum;
  
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
  if (BmpHeader->HeaderSize != sizeof (BMP_IMAGE_HEADER) - OFFSET_OF(BMP_IMAGE_HEADER, HeaderSize)) {
    return EFI_UNSUPPORTED;
  }
  
  //
  // The data size in each line must be 4 byte alignment.
  //
  DataSizePerLine = ((BmpHeader->PixelWidth * BmpHeader->BitPerPixel + 31) >> 3) & (~0x3);
  BltBufferSize = MultU64x32 (DataSizePerLine, BmpHeader->PixelHeight);
  if (BltBufferSize > (UINT32) ~0) {
    return EFI_INVALID_PARAMETER;
  }
  
  if ((BmpHeader->Size != BmpImageSize) ||
      (BmpHeader->Size < BmpHeader->ImageOffset) ||
      (BmpHeader->Size - BmpHeader->ImageOffset !=  BmpHeader->PixelHeight * DataSizePerLine)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Calculate Color Map offset in the image.
  //
  Image       = BmpImage;
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
    if (BmpHeader->ImageOffset - sizeof (BMP_IMAGE_HEADER) < sizeof (BMP_COLOR_MAP) * ColorMapNum) {
      return EFI_INVALID_PARAMETER;
    }
  }
  
  //
  // Calculate graphics image data address in the image
  //
  Image         = ((UINT8 *) BmpImage) + BmpHeader->ImageOffset;
  ImageHeader   = Image;

  //
  // Calculate the BltBuffer needed size.
  //
  BltBufferSize = MultU64x32 ((UINT64) BmpHeader->PixelWidth, BmpHeader->PixelHeight);
  //
  // Ensure the BltBufferSize * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) doesn't overflow
  //
  if (BltBufferSize > DivU64x32 ((UINTN) ~0, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))) {
    return EFI_UNSUPPORTED;
  }
  BltBufferSize = MultU64x32 (BltBufferSize, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

  IsAllocated   = FALSE;
  if (*GopBlt == NULL) {
    //
    // GopBlt is not allocated by caller.
    //
    *GopBltSize = (UINTN) BltBufferSize;
    *GopBlt     = AllocatePool (*GopBltSize);
    IsAllocated = TRUE;
    if (*GopBlt == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // GopBlt has been allocated by caller.
    //
    if (*GopBltSize < (UINTN) BltBufferSize) {
      *GopBltSize = (UINTN) BltBufferSize;
      return EFI_BUFFER_TOO_SMALL;
    }
  }

  *PixelWidth   = BmpHeader->PixelWidth;
  *PixelHeight  = BmpHeader->PixelHeight;

  //
  // Convert image from BMP to Blt buffer format
  //
  BltBuffer = *GopBlt;
  for (Height = 0; Height < BmpHeader->PixelHeight; Height++) {
    Blt = &BltBuffer[(BmpHeader->PixelHeight - Height - 1) * BmpHeader->PixelWidth];
    for (Width = 0; Width < BmpHeader->PixelWidth; Width++, Image++, Blt++) {
      switch (BmpHeader->BitPerPixel) {
        case 1:
          //
          // Convert 1-bit (2 colors) BMP to 24-bit color
          //
          for (Index = 0; Index < 8 && Width < BmpHeader->PixelWidth; Index++) {
            Blt->Red    = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Red;
            Blt->Green  = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Green;
            Blt->Blue   = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Blue;
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
          Index       = (*Image) >> 4;
          Blt->Red    = BmpColorMap[Index].Red;
          Blt->Green  = BmpColorMap[Index].Green;
          Blt->Blue   = BmpColorMap[Index].Blue;
          if (Width < (BmpHeader->PixelWidth - 1)) {
            Blt++;
            Width++;
            Index       = (*Image) & 0x0f;
            Blt->Red    = BmpColorMap[Index].Red;
            Blt->Green  = BmpColorMap[Index].Green;
            Blt->Blue   = BmpColorMap[Index].Blue;
          }
          break;
          
        case 8:
          //
          // Convert 8-bit (256 colors) BMP Palette to 24-bit color
          //
          Blt->Red    = BmpColorMap[*Image].Red;
          Blt->Green  = BmpColorMap[*Image].Green;
          Blt->Blue   = BmpColorMap[*Image].Blue;
          break;
          
        case 24:
          //
          // It is 24-bit BMP.
          //
          Blt->Blue   = *Image++;
          Blt->Green  = *Image++;
          Blt->Red    = *Image;
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
 UINT32     HexColor
 )
{
  EFI_STATUS                      Status = EFI_SUCCESS;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *Pixel;

  GraphicsOutput = NULL;
  Pixel = NULL;

  Status = gBS->LocateProtocol (
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  (VOID **) &GraphicsOutput
                );
  if (EFI_ERROR (Status)) {
    return;
  }

  CopyMem ((UINT8 *) Pixel, (UINT8 *) &HexColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
/*
  Pixel = AllocateZeroPool (sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  Pixel->Blue = 0;
  Pixel->Green = 0;
  Pixel->Red = 0;
  Pixel->Reserved = 0;
*/
  GraphicsOutput->Blt (
                   GraphicsOutput,
                   Pixel,
                   EfiBltVideoFill,
                   0, 0, 0, 0,
                   GraphicsOutput->Mode->Info->HorizontalResolution,
                   GraphicsOutput->Mode->Info->VerticalResolution,
                   0
                  );
  FreePool (Pixel);
}

VOID
UseAlpha (
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput,
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Foreground,
  UINT32 DestX,
  UINT32 DestY,
  UINT32 Width,
  UINT32 Height
  )
{
  EFI_STATUS          Status = EFI_SUCCESS;
	UINT32							InX;
	UINT32							InY;
  UINTN               Temp;
	UINT8               *Buffer;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Background;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *FrgnPixel;

  if (GraphicsOutput == NULL) {
    return;
  }

  Buffer = AllocatePool (Width * Height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

	Status = GraphicsOutput->Blt (
                            GraphicsOutput,
                            (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*) Buffer,
                            EfiBltVideoToBltBuffer,
                            DestX, DestY, 0, 0, Width, Height,
                            Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                           );
  if (EFI_ERROR (Status)) {
    goto Down;
  }

	FrgnPixel = Foreground;

	for (InY = 0; InY < Height; InY++) {
		for (InX = 0; InX < Width; InX++) {
			Background = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*) (Buffer + (InY * Width * 4) + (InX * 4));

      Temp = Background->Blue * (255 - FrgnPixel->Reserved) + FrgnPixel->Blue * FrgnPixel->Reserved + 0x80;
      FrgnPixel->Blue =(UINT8) ((Temp + (Temp >> 8)) >> 8);
      Temp = Background->Green * (255 - FrgnPixel->Reserved) + FrgnPixel->Green * FrgnPixel->Reserved + 0x80;
      FrgnPixel->Green =(UINT8) ((Temp + (Temp >> 8)) >> 8);
      Temp = Background->Red * (255 - FrgnPixel->Reserved) + FrgnPixel->Red * FrgnPixel->Reserved + 0x80;
      FrgnPixel->Red =(UINT8) ((Temp + (Temp >> 8)) >> 8);

			FrgnPixel++;
    }
  }

Down:
	if (Buffer != NULL) {
		FreePool (Buffer);
  }
  return;
}
