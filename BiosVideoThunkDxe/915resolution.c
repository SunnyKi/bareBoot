/*
 *  resolution.h
 *
 *  Created by Evan Lojewski on 3/4/10.
 *  Copyright 2009. All rights reserved.
 *
 *  remake for Clover by dmazar and slice 2012
 */
#ifndef _RESOLUTION_H_
#define _RESOLUTION_H_

#include "915resolution.h"

static const UINT8 nvda_pattern[] = {
  0x44, 0x01, 0x04, 0x00
};

static const CHAR8 nvda_string[] = "NVID";


//more advanced tables from pene
#define RESOLUTIONS_NUMBER 11
static TABLE_0 nvda_res[RESOLUTIONS_NUMBER] = {
  {1280,  720},
  {1280,  800},
  {1360,  768},
  {1400, 1050},
  {1440,  900},
  {1600,  900},
  {1600, 1200},
  {1680, 1050},
  {1920, 1080},
  {1920, 1200},
  {2048, 1536}
};

static UINT8 Sample0[] = 
      {0x34, 0x2d, 0x27, 0x28, 0x90, 0x2b, 0xa0, 0xbf, 0x9c, 0x8f, 0x96, 0xb9, 0x8e, 0x1f, 0x00, 0x00, 0x00};

static TABLE_A nvda_key0[] = {
  {3, {0x16, 0xCB, 0x9F, 0x9F, 0x8F, 0xA7, 0x17, 0xEA, 0xD2, 0xCF, 0xCF, 0xEB, 0x47, 0xE0, 0xC0, 0x00, 0x01}},
  {0, {0x12, 0xCD, 0x9F, 0x9F, 0x91, 0xA9, 0x1A, 0x3A, 0x21, 0x1F, 0x1F, 0x3B, 0x44, 0xFE, 0xC0, 0x00, 0x01}},
  {3, {0x16, 0xB9, 0xA9, 0x9F, 0x8F, 0xB2, 0x16, 0x14, 0x01, 0xFF, 0xCF, 0xEB, 0x46, 0xEA, 0xC0, 0x00, 0x01}},
  {1, {0x12, 0xE6, 0xAE, 0xAE, 0x8A, 0xBB, 0x8E, 0x3D, 0x1B, 0x19, 0x19, 0x3E, 0x0E, 0x00, 0xC0, 0x24, 0x12}},
  {0, {0x12, 0xE9, 0xB3, 0xB3, 0x8D, 0xBF, 0x92, 0xA3, 0x85, 0x83, 0x83, 0xA4, 0x48, 0xFE, 0xC0, 0x00, 0x00}},
  {3, {0x1A, 0xD7, 0xC7, 0xC7, 0x9B, 0xCD, 0x11, 0x9C, 0x86, 0x83, 0x83, 0x9D, 0x4B, 0xFE, 0xC0, 0x00, 0x00}},
  {1, {0x12, 0x03, 0xC7, 0xC7, 0x87, 0xD1, 0x09, 0xE0, 0xB1, 0xAF, 0xAF, 0xE1, 0x04, 0x00, 0x01, 0x24, 0x13}},
  {0, {0x12, 0x15, 0xD1, 0xD1, 0x99, 0xE0, 0x17, 0x3D, 0x1B, 0x19, 0x19, 0x3E, 0x0E, 0x00, 0x01, 0x24, 0x13}},  
  {3, {0x16, 0x0E, 0xEF, 0x9F, 0x8F, 0xFD, 0x02, 0x63, 0x3B, 0x37, 0xCF, 0xEB, 0x40, 0x00, 0xC1, 0x24, 0x02}},
  {0, {0x12, 0x3F, 0xEF, 0xEF, 0x83, 0x01, 0x1B, 0xD8, 0xB1, 0xAF, 0xAF, 0xD9, 0x04, 0x00, 0x41, 0x25, 0x12}},
  {1, {0x12, 0x63, 0xFF, 0xFF, 0x9D, 0x12, 0x0E, 0x34, 0x01, 0x00, 0x00, 0x35, 0x44, 0xE0, 0x41, 0x25, 0x13}}
};

static UINT8 Sample1[] = 
      {0x28, 0x00, 0x19, 0x00, 0x28, 0x18, 0x08, 0x08, 0x05};

static TABLE_B nvda_key1[] = {
  {3, {0x00, 0x05, 0xD0, 0x02, 0xA0, 0x2C, 0x10, 0x07, 0x05}},
  {0, {0x00, 0x05, 0x20, 0x03, 0xA0, 0x32, 0x10, 0x23, 0x05}},
  {3, {0x50, 0x05, 0x00, 0x03, 0xAA, 0x2F, 0x10, 0x07, 0x05}},
  {1, {0x78, 0x05, 0x1A, 0x04, 0xAF, 0x4A, 0x0E, 0x21, 0x05}},
  {0, {0xA0, 0x05, 0x84, 0x03, 0xB4, 0x38, 0x10, 0x24, 0x05}},
  {3, {0x40, 0x06, 0x84, 0x03, 0xC8, 0x38, 0x10, 0x27, 0x05}},
  {1, {0x40, 0x06, 0xB0, 0x04, 0xC8, 0x4A, 0x10, 0x19, 0x05}},
  {0, {0x90, 0x06, 0x1A, 0x04, 0xD2, 0x41, 0x10, 0x25, 0x05}},
  {3, {0x80, 0x07, 0x38, 0x04, 0xF0, 0x42, 0x10, 0x07, 0x05}},
  {0, {0x80, 0x07, 0xB0, 0x04, 0xF0, 0x4B, 0x10, 0x26, 0x05}},
  {1, {0x00, 0x08, 0x00, 0x06, 0x00, 0x60, 0x10, 0x22, 0x05}}  
};

static UINT8 Sample2[] = 
      {0x82, 0x0f, 0x03, 0x01, 0x00, 0x00, 0x08, 0x04, 0x14, 0x00, 0x00, 0x08, 0x17};

static TABLE_D nvda_key2[] = {
  {3, {0x7B, 0x01, 0x03, 0x7B, 0x01, 0x08, 0x01, 0x20, 0x80, 0x02, 0xFF, 0xFF, 0x20}},
  {0, {0x61, 0x01, 0x03, 0x61, 0x01, 0x08, 0x01, 0x20, 0x80, 0x02, 0xFF, 0xFF, 0x20}},
  {3, {0x4D, 0x01, 0x03, 0x4D, 0x01, 0x08, 0x01, 0x20, 0xA8, 0x02, 0xFF, 0xFF, 0x20}},
  {1, {0x49, 0x01, 0x03, 0x49, 0x01, 0x08, 0x01, 0x20, 0xBC, 0x02, 0xFF, 0xFF, 0x20}},
  {0, {0x65, 0x01, 0x03, 0x65, 0x01, 0x08, 0x01, 0x20, 0xD0, 0x02, 0xFF, 0xFF, 0x20}},
  {3, {0x67, 0x01, 0x03, 0x67, 0x01, 0x08, 0x01, 0x20, 0x20, 0x03, 0xFF, 0xFF, 0x20}},
  {1, {0x4A, 0x01, 0x03, 0x4A, 0x01, 0x08, 0x01, 0x20, 0x20, 0x03, 0xFF, 0xFF, 0x20}},
  {0, {0x69, 0x01, 0x03, 0x69, 0x01, 0x08, 0x01, 0x20, 0x48, 0x03, 0xFF, 0xFF, 0x20}},
  {3, {0x4D, 0x01, 0x03, 0x4D, 0x01, 0x08, 0x01, 0x20, 0xC0, 0x03, 0xFF, 0xFF, 0x20}},
  {0, {0x7D, 0x01, 0x03, 0x7D, 0x01, 0x08, 0x01, 0x20, 0xC0, 0x03, 0xFF, 0xFF, 0x20}},
  {1, {0x7A, 0x01, 0x03, 0x52, 0x01, 0x08, 0x01, 0x20, 0x00, 0x04, 0xFF, 0xFF, 0x20}}    
};

static UINT8 Sample3[] = {0x40, 0x06, 0xba, 0xb0, 0x04};

static TABLE_LIMIT nvda_key3[] = {
  {3, {0x00, 0x05, 0xBA, 0xD0, 0x02}},
  {0, {0x00, 0x05, 0xBA, 0x20, 0x03}},
  {3, {0x50, 0x05, 0xBA, 0x00, 0x03}},
  {1, {0x78, 0x05, 0xBA, 0x1A, 0x04}},
  {0, {0xA0, 0x05, 0xBA, 0x84, 0x03}},
  {3, {0x40, 0x06, 0xBA, 0x84, 0x03}},
  {1, {0x40, 0x06, 0xBA, 0xB0, 0x04}},
  {0, {0x90, 0x06, 0xBA, 0x1A, 0x04}},
  {3, {0x80, 0x07, 0xBA, 0x38, 0x04}},
  {0, {0x80, 0x07, 0xBA, 0xB0, 0x04}},
  {1, {0x00, 0x08, 0xBA, 0x00, 0x06}}  
  
};

//DTD string to replace in Intel BIOSes
static UINT8 DTD_1024[] = {0x64, 0x19, 0x00, 0x40, 0x41, 0x00, 0x26, 0x30, 0x18, 
                           0x88, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18};

/**
  Searches Source for Search pattern of size SearchSize
  and replaces it with Replace up to MaxReplaces times.
 
  @param  Source      Source bytes that will be searched.
  @param  SourceSize  Number of bytes in Source region.
  @param  Search      Bytes to search for.
  @param  SearchSize  Number of bytes in Search.
  @param  Replace     Bytes that will replace found bytes in Source (size is SearchSize).
  @param  MaxReplaces Maximum number of replaces. If MaxReplaces <= 0, then there is no restriction.

  @retval Number of replaces done.

 **/
UINTN
VideoBiosPatchSearchAndReplace (
  IN  UINT8       *Source,
  IN  UINTN       SourceSize,
  IN  UINT8       *Search,
  IN  UINTN       SearchSize,
  IN  UINT8       *Replace,
  IN  INTN        MaxReplaces
  )
{
  UINTN     NumReplaces = 0;
  BOOLEAN   NoReplacesRestriction = MaxReplaces <= 0;
  UINT8     *End = Source + SourceSize;
  
  while (Source < End && (NoReplacesRestriction || MaxReplaces > 0)) {
    if (CompareMem(Source, Search, SearchSize) == 0) {
      CopyMem(Source, Replace, SearchSize);
      NumReplaces++;
      MaxReplaces--;
      Source += SearchSize;
    } else {
      Source++;
    }
  }
  return NumReplaces;
}

/* Copied from 915 resolution created by steve tomljenovic
 *
 * This code is based on the techniques used in :
 *
 *   - 855patch.  Many thanks to Christian Zietz (czietz gmx net)
 *     for demonstrating how to shadow the VBIOS into system RAM
 *     and then modify it.
 *
 *   - 1280patch by Andrew Tipton (andrewtipton null li).
 *
 *   - 855resolution by Alain Poirier
 *
 * This source code is into the public domain.
 */

CHAR8
detect_ati_bios_type (
  vbios_map * map
)
{	
	return map->mode_table_size % sizeof(ATOM_MODE_TIMING) == 0;
}

vbios_map*
open_vbios (
  chipset_type forced_chipset
)
{
	UINTN i;
	UINTN j;
	
	vbios_map * map = AllocateZeroPool(sizeof(vbios_map));

	/*
	 *  Map the video bios to memory
	 */
	map->bios_ptr=(CHAR8*)(UINTN)VBIOS_START;
	
	/*
	 * check if we have ATI Radeon
	 */
	map->ati_tables.base = map->bios_ptr;
	map->ati_tables.AtomRomHeader = (ATOM_ROM_HEADER *) (map->bios_ptr + *(UINT16 *) (map->bios_ptr + OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER)); 
	if (AsciiStrCmp ((CHAR8 *) map->ati_tables.AtomRomHeader->uaFirmWareSignature, "ATOM") == 0) {
    UINT16 std_vesa_offset;
    ATOM_STANDARD_VESA_TIMING * std_vesa;

		// ATI Radeon Card
		DBG("PatchVBIOS: ATI");
		map->bios = BT_ATI_1;
		
		map->ati_tables.MasterDataTables = (UINT16 *) &((ATOM_MASTER_DATA_TABLE *) (map->bios_ptr + map->ati_tables.AtomRomHeader->usMasterDataTableOffset))->ListOfDataTables;
		DBG(", MasterDataTables: 0x%p", map->ati_tables.MasterDataTables);
		std_vesa_offset = (UINT16) ((ATOM_MASTER_LIST_OF_DATA_TABLES *)map->ati_tables.MasterDataTables)->StandardVESA_Timing;
		std_vesa = (ATOM_STANDARD_VESA_TIMING *) (map->bios_ptr + std_vesa_offset);
		DBG(", std_vesa: 0x%p", std_vesa);
		
		map->ati_mode_table = (CHAR8 *) &std_vesa->aModeTimings;
		DBG(", ati_mode_table: 0x%p", map->ati_mode_table);
		if (map->ati_mode_table == 0)	{
			DBG("\nPatchVBIOS: Unable to locate the mode table.\n");
      FreePool(map);
			return 0;
		}
		map->mode_table_size = std_vesa->sHeader.usStructureSize - sizeof(ATOM_COMMON_TABLE_HEADER);
		DBG(", mode_table_size: 0x%x", map->mode_table_size);
		
		if (!detect_ati_bios_type(map)) map->bios = BT_ATI_2;
		DBG(" %a\n", map->bios == BT_ATI_1 ? "BT_ATI_1" : "BT_ATI_2");
	}
	else {
		
		/*
		 * check if we have NVIDIA
		 */

		for (i = 0; i < 512; i++) {
      // we don't need to look through the whole bios, just the first 512 bytes
			if (CompareMem(map->bios_ptr+i, nvda_string, 4) == 0) {
				UINT16 nv_data_table_offset = 0;
				UINT16 * nv_data_table;
				NV_VESA_TABLE * std_vesa;

        DBG("PatchVBIOS: nVidia");
        map->bios = BT_NVDA;

				for (j = 0; j < 0x300; j++) {
          //We don't need to look for the table in the whole bios, the 768 first bytes only
					if (CompareMem(map->bios_ptr+j, nvda_pattern, 4)==0) {
						nv_data_table_offset = *((UINT16*)(map->bios_ptr+j+4));
						DBG(", nv_data_table_offset: 0x%x", (UINTN)nv_data_table_offset);
						break;
					}
				}
				
				nv_data_table = (UINT16 *) (map->bios_ptr + (nv_data_table_offset + OFFSET_TO_VESA_TABLE_INDEX));
				DBG(", nv_data_table: 0x%p", nv_data_table);
				std_vesa = (NV_VESA_TABLE *) (map->bios_ptr + *nv_data_table);
				DBG(", std_vesa: 0x%p", std_vesa);
				
				map->nv_mode_table = (CHAR8*)std_vesa+sizeof(NV_COMMON_TABLE_HEADER);
				DBG(", nv_mode_table: 0x%p", map->nv_mode_table);
				
				if (map->nv_mode_table == 0) {
					DBG("\nPatchVBIOS: Unable to locate the mode table.\n");
          FreePool(map);
					return 0;
				}
				map->mode_table_size = std_vesa->sHeader.usTable_Size;
				DBG(", mode_table_size: 0x%x\n", map->mode_table_size);
				
				break;
			}
		}
	}
	
	/*
	 * check if we have Intel
	 */
  
	if (*(UINT16*)(map->bios_ptr + 0x44) == 0x8086) {
    map->bios = BT_INTEL;
  }
	
	return map;
}

VOID set_mode (
  vbios_map   *map,
  UINT8       *edidInfo,
  edid_mode   mode
)
{

	// patch first available mode
	DBG("PatchVBIOS: ");
	switch (map->bios) {
		case BT_INTEL:
    {
      UINTN     NumReplaces;

      DBG("BT_INTEL: ");

      NumReplaces = 0;
      NumReplaces = VideoBiosPatchSearchAndReplace (
                      (UINT8*)(UINTN)VBIOS_START,
                      VBIOS_SIZE,
                      (UINT8*)&DTD_1024[0], 18,
                      edidInfo,
                      -1
                    );
      DBG (" patched %d time(s)\n", NumReplaces);
      
			return;
    }

		case BT_ATI_1:
		{
			ATOM_MODE_TIMING *mode_timing = (ATOM_MODE_TIMING *) map->ati_mode_table;

			DBG("BT_ATI_1\n");
      DBG(" mode 0 (%dx%d) patched to %dx%d\n",
        mode_timing->usCRTC_H_Disp, mode_timing->usCRTC_V_Disp,
        mode.h_active, mode.v_active
        );
      mode_timing->usCRTC_H_Total = mode.h_active + mode.h_blanking;
      mode_timing->usCRTC_H_Disp = mode.h_active;
      mode_timing->usCRTC_H_SyncStart = mode.h_active + mode.h_sync_offset;
      mode_timing->usCRTC_H_SyncWidth = mode.h_sync_width;
        
      mode_timing->usCRTC_V_Total = mode.v_active + mode.v_blanking;
      mode_timing->usCRTC_V_Disp = mode.v_active;
      mode_timing->usCRTC_V_SyncStart = mode.v_active + mode.v_sync_offset;
      mode_timing->usCRTC_V_SyncWidth = mode.v_sync_width;

      mode_timing->usPixelClock = mode.pixel_clock;
			break;
		}

		case BT_ATI_2:
		{
			ATOM_DTD_FORMAT *mode_timing = (ATOM_DTD_FORMAT *) map->ati_mode_table;
			
			DBG("BT_ATI_2\n");
      DBG(" mode 0 (%dx%d) patched to %dx%d\n",
        mode_timing->usHActive, mode_timing->usVActive,
        mode.h_active, mode.v_active
        );
      mode_timing->usHBlanking_Time = mode.h_blanking;
      mode_timing->usHActive = mode.h_active;
      mode_timing->usHSyncOffset = mode.h_sync_offset;
      mode_timing->usHSyncWidth = mode.h_sync_width;
      mode_timing->usVBlanking_Time = mode.v_blanking;
      mode_timing->usVActive = mode.v_active;
      mode_timing->usVSyncOffset = mode.v_sync_offset;
      mode_timing->usVSyncWidth = mode.v_sync_width;
      mode_timing->usPixClk = mode.pixel_clock;

			break;
		}

		case BT_NVDA:
		{
      UINTN             NumReplaces;
      UINTN             NumReplacesTotal;
      UINTN             Index = 0;
      
			DBG("BT_NVDA\n");
			// totally revised on work by pene
      // http://www.projectosx.com/forum/index.php?showtopic=2562&view=findpost&p=22683
			
      // Search for desired mode in our matrix
      for (Index = 0; Index < RESOLUTIONS_NUMBER; Index++) {
        if ((nvda_res[Index].HRes == mode.h_active) && (nvda_res[Index].VRes == mode.v_active)) {
          break;
        }
      }
      if (Index == RESOLUTIONS_NUMBER) {
        DBG("the patch is not ready for the desired resolution\n");
        break; // not found
      }
      
      NumReplaces = 0;
      NumReplacesTotal = 0;
      NumReplaces = VideoBiosPatchSearchAndReplace (
                      (UINT8*)(UINTN)VBIOS_START,
                      VBIOS_SIZE,
                      (UINT8*)&Sample0[0], 17,
                      (UINT8*)&nvda_key0[Index].Matrix[0],
                      -1
                    );
      NumReplacesTotal += NumReplaces;
      DBG (" patch 0: patched %d time(s)\n", NumReplaces);
      NumReplaces = VideoBiosPatchSearchAndReplace (
                      (UINT8*)(UINTN)VBIOS_START,
                      VBIOS_SIZE,
                      (UINT8*)&Sample1[0], 9,
                      (UINT8*)&nvda_key1[Index].Matrix[0],
                      -1
                    );
      NumReplacesTotal += NumReplaces;
      DBG (" patch 1: patched %d time(s)\n", NumReplaces);
      NumReplaces = VideoBiosPatchSearchAndReplace (
                      (UINT8*)(UINTN)VBIOS_START,
                      VBIOS_SIZE,
                      (UINT8*)&Sample2[0], 13,
                      (UINT8*)&nvda_key2[Index].Matrix[0],
                      -1
                    );
      NumReplacesTotal += NumReplaces;
      DBG (" patch 2: patched %d time(s)\n", NumReplaces);
      NumReplaces = VideoBiosPatchSearchAndReplace (
                      (UINT8*)(UINTN)VBIOS_START,
                      VBIOS_SIZE,
                      (UINT8*)&Sample3[0], 5,
                      (UINT8*)&nvda_key3[Index].Matrix[0],
                      -1
                    );
      NumReplacesTotal += NumReplaces;
      DBG (" patch 3: patched %d time(s)\n", NumReplaces);
      
      if ((*((UINT8*)(UINTN)(VBIOS_START + 0x34)) & 0x8F) == 0x80 ) {
        *((UINT8*)(UINTN)(VBIOS_START + 0x34)) |= 0x01; 
      }
      
			break;
		}

		case BT_1:
		case BT_2:
		case BT_3:
		case BT_UNKNOWN:
		{
			DBG("unknown - not patching\n");
			break;
		}
	}
}

#endif // _RESOLUTION_H_
