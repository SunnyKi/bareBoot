#ifndef _PICOPNG_H
#define _PICOPNG_H

typedef struct {
	UINT32 *data;
	UINT32 size;
	UINT32 allocsize;
} vector32_t;

typedef struct {
	UINT8 *data;
	UINT32 size;
	UINT32 allocsize;
} vector8_t;

typedef struct {
	UINT32 width, height;
	UINT32 colorType, bitDepth;
	UINT32 compressionMethod, filterMethod, interlaceMethod;
	UINT32 key_r, key_g, key_b;
	BOOLEAN key_defined; // is a transparent color key given?
	vector8_t *palette;
	vector8_t *image;
} PNG_info_t;

PNG_info_t *PNG_decode(const UINT8 *in, UINT32 size);
void png_alloc_free_all();

extern UINT8 PNG_error;

#endif
