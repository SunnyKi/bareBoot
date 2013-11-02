/*
 *  NVidia injector
 *
 *  Copyright (C) 2009  Jasmin Fazlic, iNDi
 *
 *  NVidia injector is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  NVidia driver and injector is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NVidia injector.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * DCB-Table parsing is based on software (nouveau driver) originally distributed under following license:
 *
 *
 * Copyright 2005-2006 Erik Waling
 * Copyright 2006 Stephane Marchesin
 * Copyright 2007-2009 Stuart Bennett
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _NVIDIA_H
#define _NVIDIA_H

#define DCB_MAX_NUM_ENTRIES     16
#define DCB_MAX_NUM_I2C_ENTRIES 16
#define DCB_LOC_ON_CHIP         0

#define NV_PMC_OFFSET           0x000000
#define NV_PMC_SIZE             0x2ffff
#define NV_PDISPLAY_OFFSET      0x610000
#define NV_PDISPLAY_SIZE        0x10000
#define NV_PROM_OFFSET          0x300000
#define NV_PROM_SIZE            0x0000ffff
#define NV_PRAMIN_OFFSET        0x00700000
#define NV_PRAMIN_SIZE          0x00100000

#define NV04_PFB_FIFO_DATA      0x0010020c

#define NV10_PFB_FIFO_DATA_RAM_AMOUNT_MB_MASK   0xfff00000
#define NV10_PFB_FIFO_DATA_RAM_AMOUNT_MB_SHIFT  20
#define NVC0_MEM_CTRLR_COUNT                    0x00121c74
#define NVC0_MEM_CTRLR_RAM_AMOUNT               0x0010f20c

#define NV_PBUS_PCI_NV_20 0x00001850
#define NV_PBUS_PCI_NV_20_ROM_SHADOW_DISABLED (0 << 0)
#define NV_PBUS_PCI_NV_20_ROM_SHADOW_ENABLED  (1 << 0)

#define NV_ARCH_03  0x03
#define NV_ARCH_04  0x04
#define NV_ARCH_10  0x10
#define NV_ARCH_20  0x20
#define NV_ARCH_30  0x30
#define NV_ARCH_40  0x40
#define NV_ARCH_50  0x50
#define NV_ARCH_C0  0xC0

#define CHIPSET_NV03     0x0010
#define CHIPSET_NV04     0x0020
#define CHIPSET_NV10     0x0100
#define CHIPSET_NV11     0x0110
#define CHIPSET_NV15     0x0150
#define CHIPSET_NV17     0x0170
#define CHIPSET_NV18     0x0180
#define CHIPSET_NFORCE   0x01A0
#define CHIPSET_NFORCE2  0x01F0
#define CHIPSET_NV20     0x0200
#define CHIPSET_NV25     0x0250
#define CHIPSET_NV28     0x0280
#define CHIPSET_NV30     0x0300
#define CHIPSET_NV31     0x0310
#define CHIPSET_NV34     0x0320
#define CHIPSET_NV35     0x0330
#define CHIPSET_NV36     0x0340
#define CHIPSET_NV40     0x0040
#define CHIPSET_NV41     0x00C0
#define CHIPSET_NV43     0x0140
#define CHIPSET_NV44     0x0160
#define CHIPSET_NV44A    0x0220
#define CHIPSET_NV45     0x0210
#define CHIPSET_NV50     0x0190
#define CHIPSET_NV84     0x0400
#define CHIPSET_MISC_BRIDGED  0x00F0
#define CHIPSET_G70      0x0090
#define CHIPSET_G71      0x0290
#define CHIPSET_G72      0x01D0
#define CHIPSET_G73      0x0390

// integrated GeForces (6100, 6150)
#define CHIPSET_C51      0x0240

// variant of C51, seems based on a G70 design
#define CHIPSET_C512     0x03D0
#define CHIPSET_G73_BRIDGED 0x02E0

#define NVIDIA_ROM_SIZE             0x20000
#define PATCH_ROM_SUCCESS           1
#define PATCH_ROM_SUCCESS_HAS_LVDS  2
#define PATCH_ROM_FAILED            0
#define MAX_NUM_DCB_ENTRIES         16
#define TYPE_GROUPED                0xff
#define PCI_BASE_ADDRESS_0          0x10    /* 32 bits */
#define REG32(base, reg)  ((volatile UINT32 *)(UINTN)base)[(reg) >> 2]

#define NVCAP_LEN ( sizeof(default_NVCAP) / sizeof(UINT8) )
#define NVPM_LEN ( sizeof(default_NVPM) / sizeof(UINT8) )
#define DCFG0_LEN ( sizeof(default_dcfg_0) / sizeof(UINT8) )
#define DCFG1_LEN ( sizeof(default_dcfg_1) / sizeof(UINT8) )

struct nv_chipsets_t {
  UINT32 device;
  CHAR8 *name;
};

struct bios {
  UINT16  signature;    /* 0x55AA */
  UINT8   size;     /* Size in multiples of 512 */
};

const CHAR8 *nvidia_compatible_0[]  = { "@0,compatible",  "NVDA,NVMac" };
const CHAR8 *nvidia_compatible_1[]  = { "@1,compatible",  "NVDA,NVMac" };
const CHAR8 *nvidia_device_type_0[] = { "@0,device_type", "display"    };
const CHAR8 *nvidia_device_type_1[] = { "@1,device_type", "display"    };
const CHAR8 *nvidia_device_type_parent[] = { "device_type",  "NVDA,Parent" };
const CHAR8 *nvidia_device_type_child[]	 = { "device_type",	"NVDA,Child"   };
const CHAR8 *nvidia_name_0[]      = { "@0,name",    "NVDA,Display-A" };
const CHAR8 *nvidia_name_1[]      = { "@1,name",    "NVDA,Display-B" };
const CHAR8 *nvidia_slot_name[]   = { "AAPL,slot-name", "Slot-1"     };

static UINT8 default_NVCAP[] = {
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
	0x00, 0x00, 0x00, 0x00
};

static UINT8 default_dcfg_0[] = {0x03, 0x01, 0x03, 0x00};
static UINT8 default_dcfg_1[] = {0xff, 0xff, 0x00, 0x01};

static UINT8 default_NVPM[] = {
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

static struct nv_chipsets_t NVKnownChipsets[] = {
  /*Unknown*/
  { 0x10DE0000,	"Unknown" },
	// 0000 - 0040
	// 0040 - 004F	
	{ 0x10DE0040,	"GeForce 6800 Ultra" },
	{ 0x10DE0041,	"GeForce 6800" },
	{ 0x10DE0042,	"GeForce 6800 LE" },
	{ 0x10DE0043,	"GeForce 6800 XE" },
	{ 0x10DE0044,	"GeForce 6800 XT" },
	{ 0x10DE0045,	"GeForce 6800 GT" },
	{ 0x10DE0046,	"GeForce 6800 GT" },
	{ 0x10DE0047,	"GeForce 6800 GS" },
	{ 0x10DE0048,	"GeForce 6800 XT" },
	{ 0x10DE004D,	"Quadro FX 3400" },
	{ 0x10DE004E,	"Quadro FX 4000" },
	// 0050 - 005F
	// 0060 - 006F
	// 0070 - 007F
	// 0080 - 008F
	// 0090 - 009F
	{ 0x10DE0090,	"GeForce 7800 GTX" },
	{ 0x10DE0091,	"GeForce 7800 GTX" },
	{ 0x10DE0092,	"GeForce 7800 GT" },
	{ 0x10DE0093,	"GeForce 7800 GS" },
	{ 0x10DE0095,	"GeForce 7800 SLI" },
	{ 0x10DE0098,	"GeForce Go 7800" },
	{ 0x10DE0099,	"GeForce Go 7800 GTX" },
	{ 0x10DE009D,	"Quadro FX 4500" },
	// 00A0 - 00AF	
	// 00B0 - 00BF	
	// 00C0 - 00CF	
	{ 0x10DE00C0,	"GeForce 6800 GS" },
	{ 0x10DE00C1,	"GeForce 6800" },
	{ 0x10DE00C2,	"GeForce 6800 LE" },
	{ 0x10DE00C3,	"GeForce 6800 XT" },
	{ 0x10DE00C8,	"GeForce Go 6800" },
	{ 0x10DE00C9,	"GeForce Go 6800 Ultra" },
	{ 0x10DE00CC,	"Quadro FX Go1400" },
	{ 0x10DE00CD,	"Quadro FX 3450/4000 SDI" },
	{ 0x10DE00CE,	"Quadro FX 1400" },
	// 00D0 - 00DF	
	// 00E0 - 00EF	
	// 00F0 - 00FF	
	{ 0x10DE00F1,	"GeForce 6600 GT" },
	{ 0x10DE00F2,	"GeForce 6600" },
	{ 0x10DE00F3,	"GeForce 6200" },
	{ 0x10DE00F4,	"GeForce 6600 LE" },
	{ 0x10DE00F5,	"GeForce 7800 GS" },
	{ 0x10DE00F6,	"GeForce 6800 GS/XT" },
	{ 0x10DE00F8,	"Quadro FX 3400/4400" },
	{ 0x10DE00F9,	"GeForce 6800 Series GPU" },
	// 0100 - 010F	
	// 0110 - 011F	
	// 0120 - 012F	
	// 0130 - 013F	
	// 0140 - 014F	
	{ 0x10DE0140,	"GeForce 6600 GT" },
	{ 0x10DE0141,	"GeForce 6600" },
	{ 0x10DE0142,	"GeForce 6600 LE" },
	{ 0x10DE0143,	"GeForce 6600 VE" },
	{ 0x10DE0144,	"GeForce Go 6600" },
	{ 0x10DE0145,	"GeForce 6610 XL" },
	{ 0x10DE0146,	"GeForce Go 6600 TE/6200 TE" },
	{ 0x10DE0147,	"GeForce 6700 XL" },
	{ 0x10DE0148,	"GeForce Go 6600" },
	{ 0x10DE0149,	"GeForce Go 6600 GT" },
	{ 0x10DE014A,	"Quadro NVS 440" },
	{ 0x10DE014C,	"Quadro FX 550" },
	{ 0x10DE014D,	"Quadro FX 550" },
	{ 0x10DE014E,	"Quadro FX 540" },
	{ 0x10DE014F,	"GeForce 6200" },
	// 0150 - 015F	
	// 0160 - 016F	
	{ 0x10DE0160,	"GeForce 6500" },
	{ 0x10DE0161,	"GeForce 6200 TurboCache(TM)" },
	{ 0x10DE0162,	"GeForce 6200SE TurboCache(TM)" },
	{ 0x10DE0163,	"GeForce 6200 LE" },
	{ 0x10DE0164,	"GeForce Go 6200" },
	{ 0x10DE0165,	"Quadro NVS 285" },
	{ 0x10DE0166,	"GeForce Go 6400" },
	{ 0x10DE0167,	"GeForce Go 6200" },
	{ 0x10DE0168,	"GeForce Go 6400" },
	{ 0x10DE0169,	"GeForce 6250" },
	{ 0x10DE016A,	"GeForce 7100 GS" },
	{ 0x10DE016C,	"NVIDIA NV44GLM" },
	{ 0x10DE016D,	"NVIDIA NV44GLM" },
	// 0170 - 017F	
	// 0180 - 018F	
	// 0190 - 019F		
	{ 0x10DE0191,	"GeForce 8800 GTX" },
	{ 0x10DE0193,	"GeForce 8800 GTS" },
	{ 0x10DE0194,	"GeForce 8800 Ultra" },
	{ 0x10DE0197,	"Tesla C870" },
	{ 0x10DE019D,	"Quadro FX 5600" },
	{ 0x10DE019E,	"Quadro FX 4600" },
	// 01A0 - 01AF
	// 01B0 - 01BF
	// 01C0 - 01CF
	// 01D0 - 01DF
	{ 0x10DE01D0,	"GeForce 7350 LE" },
	{ 0x10DE01D1,	"GeForce 7300 LE" },
	{ 0x10DE01D2,	"GeForce 7550 LE" },
	{ 0x10DE01D3,	"GeForce 7300 SE/7200 GS" },
	{ 0x10DE01D6,	"GeForce Go 7200" },
	{ 0x10DE01D7,	"Quadro NVS 110M / GeForce Go 7300" },
	{ 0x10DE01D8,	"GeForce Go 7400" },
	{ 0x10DE01D9,	"GeForce Go 7450" },
	{ 0x10DE01DA,	"Quadro NVS 110M" },
	{ 0x10DE01DB,	"Quadro NVS 120M" },
	{ 0x10DE01DC,	"Quadro FX 350M" },
	{ 0x10DE01DD,	"GeForce 7500 LE" },
	{ 0x10DE01DE,	"Quadro FX 350" },
	{ 0x10DE01DF,	"GeForce 7300 GS" },
	// 01E0 - 01EF	
	// 01F0 - 01FF
	{ 0x10DE01F0,	"GeForce4 MX" },
	// 0200 - 020F	
	// 0210 - 021F	
	{ 0x10DE0211,	"GeForce 6800" },
	{ 0x10DE0212,	"GeForce 6800 LE" },
	{ 0x10DE0215,	"GeForce 6800 GT" },
	{ 0x10DE0218,	"GeForce 6800 XT" },
	// 0220 - 022F
	{ 0x10DE0221,	"GeForce 6200" },
	{ 0x10DE0222,	"GeForce 6200 A-LE" },
	{ 0x10DE0228,	"NVIDIA NV44M" },
	// 0230 - 023F
	// 0240 - 024F
	{ 0x10DE0240,	"GeForce 6150" },
	{ 0x10DE0241,	"GeForce 6150 LE" },
	{ 0x10DE0242,	"GeForce 6100" },
	{ 0x10DE0243,	"NVIDIA C51" },
	{ 0x10DE0244,	"GeForce Go 6150" },
	{ 0x10DE0245,	"Quadro NVS 210S / GeForce 6150LE" },
	{ 0x10DE0247,	"GeForce Go 6100" },
	// 0250 - 025F
	{ 0x10DE025B,	"Quadro4 700 XGL" },
	// 0260 - 026F
	// 0270 - 027F
	// 0280 - 028F
	// 0290 - 029F
	{ 0x10DE0290,	"GeForce 7900 GTX" },
	{ 0x10DE0291,	"GeForce 7900 GT/GTO" },
	{ 0x10DE0292,	"GeForce 7900 GS" },
	{ 0x10DE0293,	"GeForce 7950 GX2" },
	{ 0x10DE0294,	"GeForce 7950 GX2" },
	{ 0x10DE0295,	"GeForce 7950 GT" },
	{ 0x10DE0298,	"GeForce Go 7900 GS" },
	{ 0x10DE0299,	"GeForce Go 7900 GTX" },
	{ 0x10DE029A,	"Quadro FX 2500M" },
	{ 0x10DE029B,	"Quadro FX 1500M" },
	{ 0x10DE029C,	"Quadro FX 5500" },
	{ 0x10DE029D,	"Quadro FX 3500" },
	{ 0x10DE029E,	"Quadro FX 1500" },
	{ 0x10DE029F,	"Quadro FX 4500 X2" },
	// 02A0 - 02AF
	// 02B0 - 02BF
	// 02C0 - 02CF
	// 02D0 - 02DF
	// 02E0 - 02EF
	{ 0x10DE02E0,	"GeForce 7600 GT" },
	{ 0x10DE02E1,	"GeForce 7600 GS" },
	{ 0x10DE02E2,	"GeForce 7300 GT" },
	{ 0x10DE02E3,	"GeForce 7900 GS" },
	{ 0x10DE02E4,	"GeForce 7950 GT" },
	// 02F0 - 02FF
	// 0300 - 030F
	{ 0x10DE0301,	"GeForce FX 5800 Ultra" },
	{ 0x10DE0302,	"GeForce FX 5800" },
	{ 0x10DE0308,	"Quadro FX 2000" },
	{ 0x10DE0309,	"Quadro FX 1000" },
	// 0310 - 031F
	{ 0x10DE0311,	"GeForce FX 5600 Ultra" },
	{ 0x10DE0312,	"GeForce FX 5600" },
	{ 0x10DE0314,	"GeForce FX 5600XT" },
	{ 0x10DE031A,	"GeForce FX Go5600" },
	{ 0x10DE031B,	"GeForce FX Go5650" },
	{ 0x10DE031C,	"Quadro FX Go700" },
	// 0320 - 032F
	{ 0x10DE0320,	"GeForce FX 5200" },
	{ 0x10DE0321,	"GeForce FX 5200 Ultra" },
	{ 0x10DE0322,	"GeForce FX 5200" },
	{ 0x10DE0323,	"GeForce FX 5200 LE" },
	{ 0x10DE0324,	"GeForce FX Go5200" },
	{ 0x10DE0325,	"GeForce FX Go5250" },
	{ 0x10DE0326,	"GeForce FX 5500" },
	{ 0x10DE0328,	"GeForce FX Go5200 32M/64M" },
	{ 0x10DE0329,	"GeForce FX Go5200" },
	{ 0x10DE032A,	"Quadro NVS 55/280 PCI" },
	{ 0x10DE032B,	"Quadro FX 500/600 PCI" },
	{ 0x10DE032C,	"GeForce FX Go53xx Series" },
	{ 0x10DE032D,	"GeForce FX Go5100" },
	{ 0x10DE032F,	"NVIDIA NV34GL" },
	// 0330 - 033F
	{ 0x10DE0330,	"GeForce FX 5900 Ultra" },
	{ 0x10DE0331,	"GeForce FX 5900" },
	{ 0x10DE0332,	"GeForce FX 5900XT" },
	{ 0x10DE0333,	"GeForce FX 5950 Ultra" },
	{ 0x10DE0334,	"GeForce FX 5900ZT" },
	{ 0x10DE0338,	"Quadro FX 3000" },
	{ 0x10DE033F,	"Quadro FX 700" },
	// 0340 - 034F
	{ 0x10DE0341,	"GeForce FX 5700 Ultra" },
	{ 0x10DE0342,	"GeForce FX 5700" },
	{ 0x10DE0343,	"GeForce FX 5700LE" },
	{ 0x10DE0344,	"GeForce FX 5700VE" },
	{ 0x10DE0345,	"NV36.5" },
	{ 0x10DE0347,	"GeForce FX Go5700" },
	{ 0x10DE0348,	"GeForce FX Go5700" },
	{ 0x10DE0349,	"NV36M Pro" },
	{ 0x10DE034B,	"NV36MAP" },
	{ 0x10DE034C,	"Quadro FX Go1000" },
	{ 0x10DE034E,	"Quadro FX 1100" },
	{ 0x10DE034F,	"NV36GL" },
	// 0350 - 035F
	// 0360 - 036F
	// 0370 - 037F
	// 0380 - 038F
	{ 0x10DE038B,	"GeForce 7650 GS" },
	// 0390 - 039F
	{ 0x10DE0390,	"GeForce 7650 GS" },
	{ 0x10DE0391,	"GeForce 7600 GT" },
	{ 0x10DE0392,	"GeForce 7600 GS" },
	{ 0x10DE0393,	"GeForce 7300 GT" },
	{ 0x10DE0394,	"GeForce 7600 LE" },
	{ 0x10DE0395,	"GeForce 7300 GT" },
	{ 0x10DE0397,	"GeForce Go 7700" },
	{ 0x10DE0398,	"GeForce Go 7600" },
	{ 0x10DE0399,	"GeForce Go 7600 GT"},
	{ 0x10DE039A,	"Quadro NVS 300M" },
	{ 0x10DE039B,	"GeForce Go 7900 SE" },
	{ 0x10DE039C,	"Quadro FX 560M" },
	{ 0x10DE039E,	"Quadro FX 560" },
	// 03A0 - 03AF
	// 03B0 - 03BF
	// 03C0 - 03CF
	// 03D0 - 03DF
	{ 0x10DE03D0,	"GeForce 6150SE nForce 430" },
	{ 0x10DE03D1,	"GeForce 6100 nForce 405" },
	{ 0x10DE03D2,	"GeForce 6100 nForce 400" },
	{ 0x10DE03D5,	"GeForce 6100 nForce 420" },
	{ 0x10DE03D6,	"GeForce 7025 / nForce 630a" },
	// 03E0 - 03EF
	// 03F0 - 03FF
	// 0400 - 040F
	{ 0x10DE0400,	"GeForce 8600 GTS" },
	{ 0x10DE0401,	"GeForce 8600 GT" },
	{ 0x10DE0402,	"GeForce 8600 GT" },
	{ 0x10DE0403,	"GeForce 8600 GS" },
	{ 0x10DE0404,	"GeForce 8400 GS" },
	{ 0x10DE0405,	"GeForce 9500M GS" },
	{ 0x10DE0406,	"GeForce 8300 GS" },
	{ 0x10DE0407,	"GeForce 8600M GT" },
	{ 0x10DE0408,	"GeForce 9650M GS" },
	{ 0x10DE0409,	"GeForce 8700M GT" },
	{ 0x10DE040A,	"Quadro FX 370" },
	{ 0x10DE040B,	"Quadro NVS 320M" },
	{ 0x10DE040C,	"Quadro FX 570M" },
	{ 0x10DE040D,	"Quadro FX 1600M" },
	{ 0x10DE040E,	"Quadro FX 570" },
	{ 0x10DE040F,	"Quadro FX 1700" },
	// 0410 - 041F
	{ 0x10DE0410,	"GeForce GT 330" },
	// 0420 - 042F
	{ 0x10DE0420,	"GeForce 8400 SE" },
	{ 0x10DE0421,	"GeForce 8500 GT" },
	{ 0x10DE0422,	"GeForce 8400 GS" },
	{ 0x10DE0423,	"GeForce 8300 GS" },
	{ 0x10DE0424,	"GeForce 8400 GS" },
	{ 0x10DE0425,	"GeForce 8600M GS" },
	{ 0x10DE0426,	"GeForce 8400M GT" },
	{ 0x10DE0427,	"GeForce 8400M GS" },
	{ 0x10DE0428,	"GeForce 8400M G" },
	{ 0x10DE0429,	"Quadro NVS 140M" },
	{ 0x10DE042A,	"Quadro NVS 130M" },
	{ 0x10DE042B,	"Quadro NVS 135M" },
	{ 0x10DE042C,	"GeForce 9400 GT" },
	{ 0x10DE042D,	"Quadro FX 360M" },
	{ 0x10DE042E,	"GeForce 9300M G" },
	{ 0x10DE042F,	"Quadro NVS 290" },
	// 0430 - 043F
	// 0440 - 044F
	// 0450 - 045F
	// 0460 - 046F
	// 0470 - 047F
	// 0480 - 048F
	// 0490 - 049F
	// 04A0 - 04AF
	// 04B0 - 04BF
	// 04C0 - 04CF
	{ 0x10DE04C0,	"NVIDIA G78" },
	{ 0x10DE04C1,	"NVIDIA G78" },
	{ 0x10DE04C2,	"NVIDIA G78" },
	{ 0x10DE04C3,	"NVIDIA G78" },
	{ 0x10DE04C4,	"NVIDIA G78" },
	{ 0x10DE04C5,	"NVIDIA G78" },
	{ 0x10DE04C6,	"NVIDIA G78" },
	{ 0x10DE04C7,	"NVIDIA G78" },
	{ 0x10DE04C8,	"NVIDIA G78" },
	{ 0x10DE04C9,	"NVIDIA G78" },
	{ 0x10DE04CA,	"NVIDIA G78" },
	{ 0x10DE04CB,	"NVIDIA G78" },
	{ 0x10DE04CC,	"NVIDIA G78" },
	{ 0x10DE04CD,	"NVIDIA G78" },
	{ 0x10DE04CE,	"NVIDIA G78" },
	{ 0x10DE04CF,	"NVIDIA G78" },
	// 04D0 - 04DF
	// 04E0 - 04EF
	// 04F0 - 04FF
	// 0500 - 050F
	// 0510 - 051F
	// 0520 - 052F
	// 0530 - 053F
	{ 0x10DE0530,	"GeForce 7190M / nForce 650M" },
	{ 0x10DE0531,	"GeForce 7150M / nForce 630M" },
	{ 0x10DE0533,	"GeForce 7000M / nForce 610M" },
	{ 0x10DE053A,	"GeForce 7050 PV / nForce 630a" },
	{ 0x10DE053B,	"GeForce 7050 PV / nForce 630a" },
	{ 0x10DE053E,	"GeForce 7025 / nForce 630a" },
	// 0540 - 054F
	// 0550 - 055F
	// 0560 - 056F
	// 0570 - 057F
	// 0580 - 058F
	// 0590 - 059F
	// 05A0 - 05AF
	// 05B0 - 05BF
	// 05C0 - 05CF
	// 05D0 - 05DF
	// 05E0 - 05EF
	{ 0x10DE05E0,	"GeForce GTX 295" },
	{ 0x10DE05E1,	"GeForce GTX 280" },
	{ 0x10DE05E2,	"GeForce GTX 260" },
	{ 0x10DE05E3,	"GeForce GTX 285" },
	{ 0x10DE05E4,	"NVIDIA GT200" },
	{ 0x10DE05E5,	"NVIDIA GT200" },
	{ 0x10DE05E6,	"GeForce GTX 275" },
	{ 0x10DE05E7,	"nVidia Tesla C1060" },
	{ 0x10DE05E8,	"NVIDIA GT200" },
	{ 0x10DE05E9,	"NVIDIA GT200" },
	{ 0x10DE05EA,	"GeForce GTX 260" },
	{ 0x10DE05EB,	"GeForce GTX 295" },
	{ 0x10DE05EC,	"NVIDIA GT200" },
	{ 0x10DE05ED,	"Quadroplex 2200 D2" },
	{ 0x10DE05EE,	"NVIDIA GT200" },
	{ 0x10DE05EF,	"NVIDIA GT200" },
	// 05F0 - 05FF
	{ 0x10DE05F0,	"NVIDIA GT200" },
	{ 0x10DE05F1,	"NVIDIA GT200" },
	{ 0x10DE05F2,	"NVIDIA GT200" },
	{ 0x10DE05F3,	"NVIDIA GT200" },
	{ 0x10DE05F4,	"NVIDIA GT200" },
	{ 0x10DE05F5,	"NVIDIA GT200" },
	{ 0x10DE05F6,	"NVIDIA GT200" },
	{ 0x10DE05F7,	"NVIDIA GT200" },
	{ 0x10DE05F8,	"Quadroplex 2200 S4" },
	{ 0x10DE05F9,	"NVIDIA Quadro CX" },
	{ 0x10DE05FA,	"NVIDIA GT200" },
	{ 0x10DE05FB,	"NVIDIA GT200" },
	{ 0x10DE05FC,	"NVIDIA GT200" },
	{ 0x10DE05FD,	"Quadro FX 5800" },
	{ 0x10DE05FE,	"Quadro FX 4800" },
	{ 0x10DE05FF,	"Quadro FX 3800" },
	// 0600 - 060F
	{ 0x10DE0600,	"GeForce 8800 GTS 512" },
	{ 0x10DE0601,	"GeForce 9800 GT" },
	{ 0x10DE0602,	"GeForce 8800 GT" },
	{ 0x10DE0603,	"GeForce GT 230" },
	{ 0x10DE0604,	"GeForce 9800 GX2" },
	{ 0x10DE0605,	"GeForce 9800 GT" },
	{ 0x10DE0606,	"GeForce 8800 GS" },
	{ 0x10DE0607,	"GeForce GTS 240" },
	{ 0x10DE0608,	"GeForce 9800M GTX" },
	{ 0x10DE0609,	"GeForce 8800M GTS" },
	{ 0x10DE060A,	"GeForce GTX 280M" },
	{ 0x10DE060B,	"GeForce 9800M GT" },
	{ 0x10DE060C,	"GeForce 8800M GTX" },
	{ 0x10DE060D,	"GeForce 8800 GS" },
	{ 0x10DE060F,	"GeForce GTX 285M" },
	// 0610 - 061F
	{ 0x10DE0610,	"GeForce 9600 GSO" },
	{ 0x10DE0611,	"GeForce 8800 GT" },
	{ 0x10DE0612,	"GeForce 9800 GTX" },
	{ 0x10DE0613,	"GeForce 9800 GTX+" },
	{ 0x10DE0614,	"GeForce 9800 GT" },
	{ 0x10DE0615,	"GeForce GTS 250" },
	{ 0x10DE0617,	"GeForce 9800M GTX" },
	{ 0x10DE0618,	"GeForce GTX 170M" },
	{ 0x10DE0619,	"Quadro FX 4700 X2" },
	{ 0x10DE061A,	"Quadro FX 3700" },
	{ 0x10DE061B,	"Quadro VX 200" },
	{ 0x10DE061C,	"Quadro FX 3600M" },
	{ 0x10DE061D,	"Quadro FX 2800M" },
	{ 0x10DE061E,	"Quadro FX 3700M" },
	{ 0x10DE061F,	"Quadro FX 3800M" },
	// 0620 - 062F
	{ 0x10DE0620,	"NVIDIA G94" },
	{ 0x10DE0621,	"GeForce GT 230" },
	{ 0x10DE0622,	"GeForce 9600 GT" },
	{ 0x10DE0623,	"GeForce 9600 GS" },
	{ 0x10DE0624,	"NVIDIA G94" },
	{ 0x10DE0625,	"GeForce 9600 GSO 512"},
	{ 0x10DE0626,	"GeForce GT 130" },
	{ 0x10DE0627,	"GeForce GT 140" },
	{ 0x10DE0628,	"GeForce 9800M GTS" },
	{ 0x10DE0629,	"NVIDIA G94" },
	{ 0x10DE062A,	"GeForce 9700M GTS" },
	{ 0x10DE062B,	"GeForce 9800M GS" },
	{ 0x10DE062C,	"GeForce 9800M GTS" },
	{ 0x10DE062D,	"GeForce 9600 GT" },
	{ 0x10DE062E,	"GeForce 9600 GT" },
	{ 0x10DE062F,	"GeForce 9800 S" },
	// 0630 - 063F
	{ 0x10DE0630,	"GeForce 9700 S" },
	{ 0x10DE0631,	"GeForce GTS 160M" },
	{ 0x10DE0632,	"GeForce GTS 150M" },
	{ 0x10DE0633,	"NVIDIA G94" },
	{ 0x10DE0634,	"NVIDIA G94" },
	{ 0x10DE0635,	"GeForce 9600 GSO" },
	{ 0x10DE0636,	"NVIDIA G94" },
	{ 0x10DE0637,	"GeForce 9600 GT" },
	{ 0x10DE0638,	"Quadro FX 1800" },
	{ 0x10DE0639,	"NVIDIA G94" },
	{ 0x10DE063A,	"Quadro FX 2700M" },
	{ 0x10DE063B,	"NVIDIA G94" },
	{ 0x10DE063C,	"NVIDIA G94" },
	{ 0x10DE063D,	"NVIDIA G94" },
	{ 0x10DE063E,	"NVIDIA G94" },
	{ 0x10DE063F,	"NVIDIA G94" },
	// 0640 - 064F
	{ 0x10DE0640,	"GeForce 9500 GT" },
	{ 0x10DE0641,	"GeForce 9400 GT" },
	{ 0x10DE0642,	"GeForce 8400 GS" },
	{ 0x10DE0643,	"GeForce 9500 GT" },
	{ 0x10DE0644,	"GeForce 9500 GS" },
	{ 0x10DE0645,	"GeForce 9500 GS" },
	{ 0x10DE0646,	"GeForce GT 120" },
	{ 0x10DE0647,	"GeForce 9600M GT" },
	{ 0x10DE0648,	"GeForce 9600M GS" },
	{ 0x10DE0649,	"GeForce 9600M GT" },
	{ 0x10DE064A,	"GeForce 9700M GT" },
	{ 0x10DE064B,	"GeForce 9500M G" },
	{ 0x10DE064C,	"GeForce 9650M GT" },
	// 0650 - 065F
	{ 0x10DE0650,	"NVIDIA G96-825" },
	{ 0x10DE0651,	"GeForce G 110M" },
	{ 0x10DE0652,	"GeForce GT 130M" },
	{ 0x10DE0653,	"GeForce GT 120M" },
	{ 0x10DE0654,	"GeForce GT 220M" },
	{ 0x10DE0655,	"GeForce GT 120" },
	{ 0x10DE0656,	"GeForce 9650 S" },
	{ 0x10DE0657,	"NVIDIA G96" },
	{ 0x10DE0658,	"Quadro FX 380" },
	{ 0x10DE0659,	"Quadro FX 580" },
	{ 0x10DE065A,	"Quadro FX 1700M" },
	{ 0x10DE065B,	"GeForce 9400 GT" },
	{ 0x10DE065C,	"Quadro FX 770M" },
	{ 0x10DE065D,	"NVIDIA G96" },
	{ 0x10DE065E,	"NVIDIA G96" },
	{ 0x10DE065F,	"GeForce G210" },
	// 0660 - 066F
	// 0670 - 067F
	// 0680 - 068F
	// 0690 - 069F
	// 06A0 - 06AF
	{ 0x10DE06A0,	"NVIDIA GT214" },
	// 06B0 - 06BF
	{ 0x10DE06B0,	"NVIDIA GT214" },
	// 06C0 - 06CF
	{ 0x10DE06C0,	"GeForce GTX 480" },
	{ 0x10DE06C3,	"GeForce GTX D12U" },
	{ 0x10DE06C4,	"GeForce GTX 465" },
	{ 0x10DE06CA,	"GeForce GTX 480M" },
	{ 0x10DE06CD,	"GeForce GTX 470" },
	// 06D0 - 06DF
	{ 0x10DE06D1,	"Tesla C2050 / C2070" },
	{ 0x10DE06D2,	"Tesla M2070 / X2070" },
	{ 0x10DE06D8,	"Quadro 6000" },
	{ 0x10DE06D9,	"Quadro 5000" },
	{ 0x10DE06DA,	"Quadro 5000M" },
	{ 0x10DE06DC,	"Quadro 6000" },
	{ 0x10DE06DD,	"nVidia Quadro 4000" },
	{ 0x10DE06DE,	"nVidia Tesla S2050" },
	{ 0x10DE06DF,	"Tesla M2070Q" },
	// 06E0 - 06EF
	{ 0x10DE06E0,	"GeForce 9300 GE" },
	{ 0x10DE06E1,	"GeForce 9300 GS" },
	{ 0x10DE06E2,	"GeForce 8400" },
	{ 0x10DE06E3,	"GeForce 8400 SE" },
	{ 0x10DE06E4,	"GeForce 8400 GS" },
	{ 0x10DE06E5,	"GeForce 9300M GS" },
	{ 0x10DE06E6,	"GeForce G100" },
	{ 0x10DE06E7,	"GeForce 9300 SE" },
	{ 0x10DE06E8,	"GeForce 9200M GS" },
	{ 0x10DE06E9,	"GeForce 9300M GS" },
	{ 0x10DE06EA,	"Quadro NVS 150M" },
	{ 0x10DE06EB,	"Quadro NVS 160M" },
	{ 0x10DE06EC,	"GeForce G 105M" },
	{ 0x10DE06ED,	"NVIDIA G98" },
	{ 0x10DE06EF,	"GeForce G 103M" },
	// 06F0 - 06FF
	{ 0x10DE06F0,	"NVIDIA G98" },
	{ 0x10DE06F1,	"GeForce G105M" },
	{ 0x10DE06F2,	"NVIDIA G98" },
	{ 0x10DE06F3,	"NVIDIA G98" },
	{ 0x10DE06F4,	"NVIDIA G98" },
	{ 0x10DE06F5,	"NVIDIA G98" },
	{ 0x10DE06F6,	"NVIDIA G98" },
	{ 0x10DE06F7,	"NVIDIA G98" },
	{ 0x10DE06F8,	"Quadro NVS 420" },
	{ 0x10DE06F9,	"Quadro FX 370 LP" },
	{ 0x10DE06FA,	"Quadro NVS 450" },
	{ 0x10DE06FB,	"Quadro FX 370M" },
	{ 0x10DE06FC,	"NVIDIA G98" },
	{ 0x10DE06FD,	"Quadro NVS 295" },
	{ 0x10DE06FE,	"NVIDIA G98" },
	{ 0x10DE06FF,	"HICx16 + Graphics" },
	// 0700 - 070F
	// 0710 - 071F
	// 0720 - 072F
	// 0730 - 073F
	// 0740 - 074F
	// 0750 - 075F
	// 0760 - 076F
	// 0770 - 077F
	// 0780 - 078F
	// 0790 - 079F
	// 07A0 - 07AF
	// 07B0 - 07BF
	// 07C0 - 07CF
	// 07D0 - 07DF
	// 07E0 - 07EF
	{ 0x10DE07E0,	"GeForce 7150 / nForce 630i" },
	{ 0x10DE07E1,	"GeForce 7100 / nForce 630i" },
	{ 0x10DE07E2,	"GeForce 7050 / nForce 630i" },
	{ 0x10DE07E3,	"GeForce 7050 / nForce 610i" },
	{ 0x10DE07E5,	"GeForce 7050 / nForce 620i" },
	// 07F0 - 07FF
	// 0800 - 080F
	// 0810 - 081F
	// 0820 - 082F
	// 0830 - 083F
	// 0840 - 084F
	{ 0x10DE0840,	"GeForce 8200M" },
	{ 0x10DE0844,	"GeForce 9100M G" },
	{ 0x10DE0845,	"GeForce 8200M G" },
	{ 0x10DE0846,	"GeForce 9200" },
	{ 0x10DE0847,	"GeForce 9100" },
	{ 0x10DE0848,	"GeForce 8300" },
	{ 0x10DE0849,	"GeForce 8200" },
	{ 0x10DE084A,	"nForce 730a" },
	{ 0x10DE084B,	"GeForce 9200" }, // nVidia GeForce 8200 ??
	{ 0x10DE084C,	"nForce 980a/780a SLI" },
	{ 0x10DE084D,	"nForce 750a SLI" },
	{ 0x10DE084F,	"GeForce 8100 / nForce 720a" },
	// 0850 - 085F
	// 0860 - 086F
	{ 0x10DE0860,	"GeForce 9300" },
	{ 0x10DE0861,	"GeForce 9400" },
	{ 0x10DE0862,	"GeForce 9400M G" },
	{ 0x10DE0863,	"GeForce 9400M" },
	{ 0x10DE0864,	"GeForce 9300" },
	{ 0x10DE0865,	"GeForce 9300" }, // ION ??
	{ 0x10DE0866,	"GeForce 9400M G" },
	{ 0x10DE0867,	"GeForce 9400" },
	{ 0x10DE0868,	"nForce 760i SLI" },
	{ 0x10DE0869,	"GeForce 9400" },
	{ 0x10DE086A,	"GeForce 9400" },
	{ 0x10DE086C,	"GeForce 9300 / nForce 730i" },
	{ 0x10DE086D,	"GeForce 9200" },
	{ 0x10DE086E,	"GeForce 9100M G" },
	{ 0x10DE086F,	"GeForce 8200M G" },
	// 0870 - 087F
	{ 0x10DE0870,	"GeForce 9400M" },
	{ 0x10DE0871,	"GeForce 9200" },
	{ 0x10DE0872,	"GeForce G102M" },
	{ 0x10DE0873,	"GeForce G205M" },
	{ 0x10DE0874,	"ION 9300M" },
	{ 0x10DE0876,	"ION 9400M" },
	{ 0x10DE087A,	"GeForce 9400" },
	{ 0x10DE087D,	"ION 9400M" },
	{ 0x10DE087E,	"ION LE" },
	{ 0x10DE087F,	"ION LE" },
	// 0880 - 088F
	// 0890 - 089F
	// 08A0 - 08AF
	{ 0x10DE08A0,	"GeForce 320M" },
	{ 0x10DE08A1,	"MCP89-MZT" },
	{ 0x10DE08A2,	"GeForce 320M" },
	{ 0x10DE08A3,	"GeForce 320M" },
	{ 0x10DE08A4,	"GeForce 320M" },
	{ 0x10DE08A5,	"GeForce 320M" },
	// 08B0 - 08BF
	{ 0x10DE08B0,	"MCP83 MMD" },
	{ 0x10DE08B1,	"GeForce 300M" },
	{ 0x10DE08B2,	"GeForce 300M" }, // MCP83-MJ
	{ 0x10DE08B3,	"MCP89 MM9" },
	// 08C0 - 08CF
	// 08D0 - 08DF
	// 08E0 - 08EF
	// 08F0 - 08FF
	// 0900 - 090F
	// 0910 - 091F
	// 0920 - 092F
	// 0930 - 093F
	// 0940 - 094F
	// 0950 - 095F
	// 0960 - 096F
	// 0970 - 097F
	// 0980 - 098F
	// 0990 - 099F
	// 09A0 - 09AF
	// 09B0 - 09BF
	// 09C0 - 09CF
	// 09D0 - 09DF
	// 09E0 - 09EF
	// 09F0 - 09FF
	// 0A00 - 0A0F
	// { 0x10DE0A00,	"NVIDIA GT212" },
	// 0A10 - 0A1F
	// { 0x10DE0A10,	"NVIDIA GT212" },
	// 0A20 - 0A2F
	{ 0x10DE0A20,	"GeForce GT 220" },
	{ 0x10DE0A21,	"D10M2-20" },
	{ 0x10DE0A22,	"GeForce 315" },
	{ 0x10DE0A23,	"GeForce 210" },
	{ 0x10DE0A26,	"GeForce 405" },
	{ 0x10DE0A27,	"GeForce 405" },
	{ 0x10DE0A28,	"GeForce GT 230M" },
	{ 0x10DE0A29,	"GeForce GT 330M" },
	{ 0x10DE0A2A,	"GeForce GT 230M" },
	{ 0x10DE0A2B,	"GeForce GT 330M" },
	{ 0x10DE0A2C,	"NVS 5100M" },
	{ 0x10DE0A2D,	"GeForce GT 320M" },	
	// 0A30 - 0A3F
	{ 0x10DE0A30,	"GeForce GT 330M" },
	{ 0x10DE0A32,	"GeForce GT 415" },
	{ 0x10DE0A34,	"GeForce GT 240M" },
	{ 0x10DE0A35,	"GeForce GT 325M" },
	{ 0x10DE0A38,	"Quadro 400" },
	{ 0x10DE0A3C,	"Quadro FX 880M" },
	{ 0x10DE0A3D,	"N10P-ES" },
	{ 0x10DE0A3F,	"GT216-INT" },
	// 0A40 - 0A4F
	// 0A50 - 0A5F
	// 0A60 - 0A6F
	{ 0x10DE0A60,	"GeForce G210" },
	{ 0x10DE0A61,	"NVS 2100" },
	{ 0x10DE0A62,	"GeForce 205" },
	{ 0x10DE0A63,	"GeForce 310" },
	{ 0x10DE0A64,	"ION" },
	{ 0x10DE0A65,	"GeForce 210" },
	{ 0x10DE0A66,	"GeForce 310" },
	{ 0x10DE0A67,	"GeForce 315" },
	{ 0x10DE0A68,	"GeForce G105M" },
	{ 0x10DE0A69,	"GeForce G105M" },
	{ 0x10DE0A6A,	"NVS 2100M" },
	{ 0x10DE0A6C,	"NVS 3100M" },
	{ 0x10DE0A6E,	"GeForce 305M" },
	{ 0x10DE0A6F,	"ION" },	
	// 0A70 - 0A7F
	{ 0x10DE0A70,	"GeForce 310M" },
	{ 0x10DE0A71,	"GeForce 305M" },
	{ 0x10DE0A72,	"GeForce 310M" },
	{ 0x10DE0A73,	"GeForce 305M" },
	{ 0x10DE0A74,	"GeForce G210M" },
	{ 0x10DE0A75,	"GeForce G310M" },
	{ 0x10DE0A76,	"ION" },
	{ 0x10DE0A78,	"Quadro FX 380 LP" },
	// { 0x10DE0A79,	"N12M-NS-S" },
	{ 0x10DE0A7A,	"GeForce 315M" },
	{ 0x10DE0A7B,	"GeForce 505" },
	{ 0x10DE0A7C,	"Quadro FX 380M" },
	{ 0x10DE0A7D,	"N11M-ES" }, //SUBIDS
	{ 0x10DE0A7E,	"GT218-INT-S" },
	{ 0x10DE0A7F,	"GT218-INT-B" },
	// 0A80 - 0A8F
	// 0A90 - 0A9F
	// 0AA0 - 0AAF
	// 0AB0 - 0ABF
	// 0AC0 - 0ACF
	// 0AD0 - 0ADF
	// 0AE0 - 0AEF
	// 0AF0 - 0AFF
	// 0B00 - 0B0F
	// 0B10 - 0B1F
	// 0B20 - 0B2F
	// 0B30 - 0B3F
	// 0B40 - 0B4F
	// 0B50 - 0B5F
	// 0B60 - 0B6F
	// 0B70 - 0B7F
	// 0B80 - 0B8F
	// 0B90 - 0B9F
	// 0BA0 - 0BAF
	// 0BB0 - 0BBF
	// 0BC0 - 0BCF
	// 0BD0 - 0BDF
	// 0BE0 - 0BEF
	// 0BF0 - 0BFF
	// 0C00 - 0C0F
	// 0C10 - 0C1F
	// 0C20 - 0C2F
	// 0C30 - 0C3F
	// 0C40 - 0C4F
	// 0C50 - 0C5F
	// 0C60 - 0C6F
	// 0C70 - 0C7F
	// 0C80 - 0C8F
	// 0C90 - 0C9F
	// 0CA0 - 0CAF
	{ 0x10DE0CA0,	"GeForce GT 330 " },
	{ 0x10DE0CA2,	"GeForce GT 320" },
	{ 0x10DE0CA3,	"GeForce GT 240" },
	{ 0x10DE0CA4,	"GeForce GT 340" },
	{ 0x10DE0CA5,	"GeForce GT 220" },
	{ 0x10DE0CA7,	"GeForce GT 330" },
	{ 0x10DE0CA8,	"GeForce GTS 260M" },
	{ 0x10DE0CA9,	"GeForce GTS 250M" },
	{ 0x10DE0CAC,	"GeForce GT 220" },
	{ 0x10DE0CAD,	"N10E-ES" },
	{ 0x10DE0CAE,	"GT215-INT" },
	{ 0x10DE0CAF,	"GeForce GT 335M" },
	// 0CB0 - 0CBF	
	{ 0x10DE0CB0,	"GeForce GTS 350M" },
	{ 0x10DE0CB1,	"GeForce GTS 360M" },
	{ 0x10DE0CBC,	"Quadro FX 1800M" },
	// 0CC0 - 0CCF
	// 0CD0 - 0CDF
	// 0CE0 - 0CEF
	// 0CF0 - 0CFF
	// 0D00 - 0D0F
	// 0D10 - 0D1F
	// 0D20 - 0D2F
	// 0D30 - 0D3F
	// 0D40 - 0D4F
	// 0D50 - 0D5F
	// 0D60 - 0D6F
	// 0D70 - 0D7F
	// 0D80 - 0D8F
	// 0D90 - 0D9F
	// 0DA0 - 0DAF
	// 0DB0 - 0DBF
	// 0DC0 - 0DCF
	{ 0x10DE0DC0,	"GeForce GT 440" },
	//  { 0x10DE0DC1,	"D12-P1-35" },
	//  { 0x10DE0DC2,	"D12-P1-35" },
	{ 0x10DE0DC4,	"GeForce GTS 450" },
	{ 0x10DE0DC5,	"GeForce GTS 450" },
	{ 0x10DE0DC6,	"GeForce GTS 450" },
	//  { 0x10DE0DCA,	"GF10x" },
	//  { 0x10DE0DCC,	"N12E-GS" },
	{ 0x10DE0DCD,	"GeForce GT 555M" },
	{ 0x10DE0DCE,	"GeForce GT 555M" },
	//  { 0x10DE0DCF,	"N12P-GT-B" },
	// 0DD0 - 0DDF	
	//  { 0x10DE0DD0,	"N11E-GT" },
	{ 0x10DE0DD1,	"GeForce GTX 460M" },
	{ 0x10DE0DD2,	"GeForce GT 445M" },
	{ 0x10DE0DD3,	"GeForce GT 435M" },
	{ 0x10DE0DD6,	"GeForce GT 550M" },
	{ 0x10DE0DD8,	"Quadro 2000" },
	{ 0x10DE0DDA,	"Quadro 2000M" },
	{ 0x10DE0DDE,	"GF106-ES" },
	//  { 0x10DE0DDF,	"GF106-INT" },
	// 0DE0 - 0DEF
	{ 0x10DE0DE0,	"GeForce GT 440" },
	{ 0x10DE0DE1,	"GeForce GT 430" },
	{ 0x10DE0DE2,	"GeForce GT 420" },
	{ 0x10DE0DE3,	"GeForce GT 635M" },
	{ 0x10DE0DE4,	"GeForce GT 520" },
	{ 0x10DE0DE5,	"GeForce GT 530" },
	{ 0x10DE0DE8,	"GeForce GT 620M" },
	{ 0x10DE0DE9,	"GeForce GT 630M" },
	{ 0x10DE0DEA,	"GeForce GT 610M" },
	{ 0x10DE0DEB,	"GeForce GT 555M" },
	{ 0x10DE0DEC,	"GeForce GT 525M" },
	{ 0x10DE0DED,	"GeForce GT 520M" },
	{ 0x10DE0DEE,	"GeForce GT 415M" },
	{ 0x10DE0DEF,	"N13P-NS1-A1" },
	// 0DF0 - 0DFF
	{ 0x10DE0DF0,	"GeForce GT 425M" },
	{ 0x10DE0DF1,	"GeForce GT 420M" },
	{ 0x10DE0DF2,	"GeForce GT 435M" },
	{ 0x10DE0DF3,	"GeForce GT 420M" },
	{ 0x10DE0DF4,	"GeForce GT 540M" },
	{ 0x10DE0DF5,	"GeForce GT 525M" },
	{ 0x10DE0DF6,	"GeForce GT 550M" },
	{ 0x10DE0DF7,	"GeForce GT 520M" },
	{ 0x10DE0DF8,	"Quadro 600" },
	{ 0x10DE0DF9,	"Quadro 500M" },
	{ 0x10DE0DFA,	"Quadro 1000M" },
	{ 0x10DE0DFC,	"NVS 5200M" },
	{ 0x10DE0DFE,	"GF108 ES" },
	//  { 0x10DE0DFF,	"GF108 INT" },
	// 0E00 - 0E0F
	// 0E10 - 0E1F
	// 0E20 - 0E2F
	{ 0x10DE0E21,	"D12U-25" },
	{ 0x10DE0E22,	"GeForce GTX 460" },
	{ 0x10DE0E23,	"GeForce GTX 460 SE" },
	{ 0x10DE0E24,	"GeForce GTX 460" },
	//  { 0x10DE0E25,	"D12U-50" },
	{ 0x10DE0E28,	"GeForce GTX 460" },
	// 0E30 - 0E3F
	{ 0x10DE0E30,	"GeForce GTX 470M" },
	{ 0x10DE0E31,	"GeForce GTX 485M" },
	//  { 0x10DE0E32,	"N12E-GT" },
	{ 0x10DE0E38,	"GF104GL" },
	{ 0x10DE0E3A,	"Quadro 3000M" },
	{ 0x10DE0E3B,	"Quadro 4000M" },
	//  { 0x10DE0E3E,	"GF104-ES" },
	//  { 0x10DE0E3F,	"GF104-INT" },
	// 0E40 - 0E4F
	// 0E50 - 0E5F
	// 0E60 - 0E6F
	// 0E70 - 0E7F
	// 0E80 - 0E8F
	// 0E90 - 0E9F
	// 0EA0 - 0EAF
	// 0EB0 - 0EBF
	// 0EC0 - 0ECF
	// 0ED0 - 0EDF
	// 0EE0 - 0EEF
	// 0EF0 - 0EFF
	// 0F00 - 0F0F
	{ 0x10DE0F00,	"GeForce GT 630" },
	{ 0x10DE0F01,	"GeForce GT 620" },
	// 0F10 - 0F1F
	// 0F20 - 0F2F
	// 0F30 - 0F3F
	// 0F40 - 0F4F
	// 0F50 - 0F5F
	// 0F60 - 0F6F
	// 0F70 - 0F7F
	// 0F80 - 0F8F
	// 0F90 - 0F9F
	// 0FA0 - 0FAF
	// 0FB0 - 0FBF
	// 0FC0 - 0FCF
	{ 0x10DE0FC0,	"GeForce GT 640" },
	{ 0x10DE0FC1,	"GeForce GT 640" },
	{ 0x10DE0FC2,	"GeForce GT 630" },
	{ 0x10DE0FC6,	"GeForce GTX 650" },
	// 0FD0 - 0FDF
	{ 0x10DE0FD1,	"GeForce GT 650M" },
	{ 0x10DE0FD2,	"GeForce GT 640M" },
	{ 0x10DE0FD3,	"GeForce GT 640M LE" },
	{ 0x10DE0FD4,	"GeForce GTX 660M" },
	{ 0x10DE0FD5,	"GeForce GT 650M" },  //GK100
	{ 0x10DE0FDB,	"GK107-ESP-A1" },
	// 0FE0 - 0FEF
	{ 0x10DE0FE0,	"GeForce GTX 660M" },
	// 0FF0 - 0FFF
	{ 0x10DE0FFB,	"Quadro K2000M" },
	{ 0x10DE0FFC,	"Quadro K1000M" },
	{ 0x10DE0FFD,	"NVS 510" },
	{ 0x10DE0FFF,	"Quadro 410" },
	// 1000 - 100F
	// 1010 - 101F
	// 1020 - 102F
	// 1030 - 103F
	// 1040 - 104F
	{ 0x10DE1040,	"GeForce GT 520" },
	//  { 0x10DE1041,	"D13M1-45" },
	{ 0x10DE1042,	"GeForce 510" },
	{ 0x10DE1048,	"GeForce 605" },
	{ 0x10DE1049,	"GeForce GT 620" },
	{ 0x10DE104A,	"GeForce GT 610" },
	// 1050 - 105F
	{ 0x10DE1050,	"GeForce GT 520M" },
	{ 0x10DE1051,	"GeForce GT 520MX" },
	{ 0x10DE1052,	"GeForce GT 520M" },
	{ 0x10DE1054,	"GeForce GT 410M" },
	{ 0x10DE1055,	"GeForce 410M" },
	{ 0x10DE1056,	"Quadro NVS 4200M" },
	{ 0x10DE1057,	"Quadro NVS 4200M" },
	{ 0x10DE1058,	"GeForce 610M" },
	{ 0x10DE1059,	"GeForce 610M" },
	{ 0x10DE105A,	"GeForce 610M" },
	// 1060 - 106F
	// 1070 - 107F
	{ 0x10DE107D,	"NVS 310" },
	//  { 0x10DE107E,	"GF119-INT" },
	//  { 0x10DE107F,	"GF119-ES" },
	// 1080 - 108F
	{ 0x10DE1080,	"GeForce GTX 580" },
	{ 0x10DE1081,	"GeForce GTX 570" },
	{ 0x10DE1082,	"GeForce GTX 560 Ti" },
	{ 0x10DE1083,	"D13U" },
	{ 0x10DE1084,	"GeForce GTX 560" },
	{ 0x10DE1086,	"GeForce GTX 570" },
	{ 0x10DE1087,	"GeForce GTX 560 Ti-448" },
	{ 0x10DE1088,	"GeForce GTX 590" },
	{ 0x10DE1089,	"GeForce GTX 580" },
	{ 0x10DE108B,	"GeForce GTX 590" },
	//  { 0x10DE108C,	"D13U" },
	{ 0x10DE108E,	"Tesla C2090" },
	// 1090 - 109F
	{ 0x10DE1091,	"nVidia Tesla M2090" },
	{ 0x10DE1094,	"Tesla M2075 Dual-Slot Computing Processor Module" },
	{ 0x10DE1096,	"Tesla C2075" },
	{ 0x10DE1098,	"D13U" },
	{ 0x10DE109A,	"Quadro 5010M" },
	{ 0x10DE109B,	"Quadro 7000" },
	// 10A0 - 10AF
	// 10B0 - 10BF
	// 10C0 - 10CF
	{ 0x10DE10C0,	"GeForce 9300 GS" },
	{ 0x10DE10C3,	"GeForce 8400 GS" },
	{ 0x10DE10C4,	"NVIDIA ION" },
	{ 0x10DE10C5,	"GeForce 405" },
	// 10D0 - 10DF
	{ 0x10DE10D8,	"NVS 300" },
	// 10E0 - 10EF
	// 10F0 - 10FF
	// 1100 - 110F
	// 1110 - 111F
	// 1120 - 112F
	// 1130 - 113F
	// 1140 - 114F
	{ 0x10DE1140,	"GeForce GT 610M" },
	{ 0x10DE1141,	"GeForce 610M" },
	{ 0x10DE1142,	"GeForce 620M" },
	//  { 0x10DE1143,	"N13P-GV" },
	//  { 0x10DE1144,	"GF117" },
	//  { 0x10DE1145,	"GF117" },
	//  { 0x10DE1146,	"GF117" },
	//  { 0x10DE1147,	"GF117" },
	//  { 0x10DE1149,	"GF117-ES" },
	//  { 0x10DE114A,	"GF117-INT" },
	//  { 0x10DE114B,	"PCI-GEN3-B" },
	// 1150 - 115F
	// 1160 - 116F
	// 1170 - 117F
	// 1180 - 118F
	{ 0x10DE1180,	"GeForce GTX 680" },
	{ 0x10DE1183,	"GeForce GTX 660 Ti" },
  { 0x10DE1184,	"GeForce GTX 770" },
	{ 0x10DE1185,	"GeForce GTX 660" },
	{ 0x10DE1188,	"GeForce GTX 690" },
	{ 0x10DE1189,	"GeForce GTX 670" },
  // 118A - 118E
  { 0x10DE118F,	"Tesla K10" },
	// 1190 - 119F
	// 11A0 - 11AF
	{ 0x10DE11A0,	"GeForce GTX 680M" }, //optimus
	{ 0x10DE11A1,	"GeForce GTX 670MX" },
	{ 0x10DE11A7,	"GeForce GTX 675MX" },
	// 11B0 - 11BF
	{ 0x10DE11BA,	"Quadro K5000" },
	{ 0x10DE11BC,	"Quadro K5000M" },
	{ 0x10DE11BD,	"Quadro K4000M" },
	{ 0x10DE11BE,	"Quadro K3000M" },
	// 11C0 - 11CF
  { 0x10DE11C0,	"GeForce GTX 660" },
  { 0x10DE11C2,	"GeForce GTX 650 TI BOOST" },
	{ 0x10DE11C6,	"GeForce GTX 650 TI" },
  { 0x10DE11D0, "GK106-INT353" },
	// 11D0 - 11DF
	// 11E0 - 11EF
	// 11F0 - 11FF
	{ 0x10DE11FA,	"Quadro K4000" },
	// 1200 - 120F
	{ 0x10DE1200,	"GeForce GTX 560 Ti" },
	{ 0x10DE1201,	"GeForce GTX 560" },
	{ 0x10DE1202,	"GeForce GTX 560 Ti" },
	{ 0x10DE1203,	"GeForce GTX 460 SE v2" },
	{ 0x10DE1205,	"GeForce GTX 460 v2" },
	{ 0x10DE1206,	"GeForce GTX 555" },
	{ 0x10DE1207,	"GeForce GT 645" },
	{ 0x10DE1208,	"GeForce GTX 560 SE" },
	{ 0x10DE1210,	"GeForce GTX 570M" },
	{ 0x10DE1211,	"GeForce GTX 580M" },
	{ 0x10DE1212,	"GeForce GTX 675M" },
	{ 0x10DE1213,	"GeForce GTX 670M" },
	{ 0x10DE1240,	"GeForce GT 620M" },
	{ 0x10DE1241,	"GeForce GT 545" },
	{ 0x10DE1243,	"GeForce GT 545" },
	{ 0x10DE1244,	"GeForce GTX 550 Ti" },
	{ 0x10DE1245,	"GeForce GTS 450" },
	{ 0x10DE1246,	"GeForce GTX 550M" },
	{ 0x10DE1247,	"GeForce GT 555M" },
	{ 0x10DE1248,	"GeForce GTX 555M" },
	{ 0x10DE1249,	"GeForce GTS 450" }, // 450M?
	{ 0x10DE124B,	"GeForce GT 640" },
	{ 0x10DE124D,	"GeForce GTX 555M" },
	{ 0x10DE1251,	"GeForce GTX 560M" },
  { 0x10DE1280,	"GeForce GT 635" },
  { 0x10DE1282,	"GeForce GT 640" },
  { 0x10DE1284,	"GeForce GT 630" },
	// 1290 - 129F
	// 12A0 - 12AF
	// 12B0 - 12BF
	// 12C0 - 12CF
	// 12D0 - 12DF
	// 12E0 - 12EF
	// 12F0 - 12FF
};

BOOLEAN
setup_nvidia_devprop (
  pci_dt_t *nvda_dev
  );

#endif /* !_NVIDIA_H */
