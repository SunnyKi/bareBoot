/*
 *  ati.h
 *
 *  Created by Slice on 19.02.12.
 *
 *  the code ported from Chameleon project as well as from RadeonFB by Joblo and RadeonHD by dong
 *  bis thank to Islam M. Ahmed Zaid for the updating the collection
 */

#ifndef _ATI_H_
#define _ATI_H_

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include "ati_reg.h"

#define OFFSET_TO_GET_ATOMBIOS_STRINGS_START 0x6e

#define DATVAL(x)     {kPtr, sizeof(x), (UINT8 *)x}
#define STRVAL(x)     {kStr, sizeof(x)-1, (UINT8 *)x}
#define BYTVAL(x)     {kCst, 1, (UINT8 *)x}
#define WRDVAL(x)     {kCst, 2, (UINT8 *)x}
#define DWRVAL(x)     {kCst, 4, (UINT8 *)x}
#define QWRVAL(x)     {kCst, 8, (UINT8 *)x}
#define NULVAL        {kNul, 0, (UINT8 *)NULL}

/* Flags */
#define MKFLAG(n)     (1 << n)
#define FLAGTRUE      MKFLAG(0)
#define EVERGREEN     MKFLAG(1)
#define FLAGMOBILE    MKFLAG(2)
#define FLAGOLD       MKFLAG(3)

typedef enum {
  kNul,
  kStr,
  kPtr,
  kCst
} type_t;

typedef enum {
  CHIP_FAMILY_UNKNOW,
  /* Old */
  CHIP_FAMILY_R420,
  CHIP_FAMILY_RV410,
  CHIP_FAMILY_RV515,
  CHIP_FAMILY_R520,
  CHIP_FAMILY_RV530,
  CHIP_FAMILY_RV560,
  CHIP_FAMILY_RV570,
  CHIP_FAMILY_R580,

  /* IGP */
  CHIP_FAMILY_RS600,
  CHIP_FAMILY_RS690,
  CHIP_FAMILY_RS740,
  CHIP_FAMILY_RS780,
  CHIP_FAMILY_RS880,
  /* R600 */
  CHIP_FAMILY_R600,
  CHIP_FAMILY_RV610,
  CHIP_FAMILY_RV620,
  CHIP_FAMILY_RV630,
  CHIP_FAMILY_RV635,
  CHIP_FAMILY_RV670,
  /* R700 */
  CHIP_FAMILY_RV710,
  CHIP_FAMILY_RV730,
  CHIP_FAMILY_RV740,
  CHIP_FAMILY_RV770,
  /* Evergreen */
  CHIP_FAMILY_CEDAR,
  CHIP_FAMILY_CYPRESS,
  CHIP_FAMILY_HEMLOCK,
  CHIP_FAMILY_JUNIPER,
  CHIP_FAMILY_REDWOOD,
  /* Northern Islands */
  CHIP_FAMILY_BARTS,
  CHIP_FAMILY_CAICOS,
  CHIP_FAMILY_CAYMAN,
  CHIP_FAMILY_TURKS,
  CHIP_FAMILY_TAHITI,
  CHIP_FAMILY_VERDE,
  CHIP_FAMILY_LOMBOK,
  CHIP_FAMILY_SUMO,
  CHIP_FAMILY_MANHATTAN,
  CHIP_FAMILY_VANCOUVER,
  CHIP_FAMILY_WRESTLER,
  CHIP_FAMILY_TRINITY,
  CHIP_FAMILY_LAST
} ati_chip_family_t;

typedef struct {
  const CHAR8   *name;
  UINT8     ports;
} card_config_t;

typedef enum {
  kNull,
  kWormy,
  kAlopias,
  kAlouatta,
  kBaboon,
  kCardinal,
  kCaretta,
  kColobus,
  kDouc,
  kEulemur,
  kFlicker,
  kGalago,
  kGliff,
  kHoolock,
  kHypoprion,
  kIago,
  kKakapo,
  kKipunji,
  kLamna,
  kLangur,
  kMegalodon,
  kMotmot,
  kNomascus,
  kOrangutan,
  kPeregrine,
  kQuail,
  kRaven,
  kShrike,
  kSphyrna,
  kTriakis,
  kUakari,
  kVervet,
  kZonalis,
  kPithecia,
  kBulrushes,
  kCattail,
  kHydrilla,
  kDuckweed,
  kFanwort,
  kElodea,
  kKudzu,
  kGibba,
  kLotus,
  kIpomoea,
  kMangabey,
  kMuskgrass,
  kJuncus,
  kPondweed,
#if 0
  kOsmunda,
#endif
  kAji, //4
  kBuri, //4 M
  kChutoro, //5 M
  kDashimaki, //4
  kEbi, //5 M
  kFutomaki, //5
  kHamachi, //4
  kGari, //5 M
  kCfgEnd
} config_name_t;

typedef struct {
  UINT16            device_id;
  UINT32            subsys_id;
  ati_chip_family_t chip_family;
  const CHAR8       *model_name;
  config_name_t     cfg_name;
} radeon_card_info_t;

typedef struct {
  DevPropDevice         *device;
  radeon_card_info_t    *info;
  pci_dt_t              *pci_dev;
  UINT8                 *fb;
  UINT8                 *mmio;
  UINT8                 *io;
  UINT8                 *rom;
  UINT32                rom_size;
  UINT64                vram_size;
  const CHAR8           *cfg_name;
  UINT8                 ports;
  UINT32                flags;
  BOOLEAN               posted;
} card_t;

typedef struct {
  type_t        type;
  UINT32        size;
  UINT8         *data;
} value_t;

typedef struct {
  UINT32        flags;
  BOOLEAN       all_ports;
  CHAR8         *name;
  BOOLEAN       (*get_value) (value_t *val);
  value_t       default_val;
} AtiDevProp;

const CHAR8 *chip_family_name[] = {
  "UNKNOW",
  "R420",
  "RV410",
  "RV515",
  "R520",
  "RV530",
  "RV560",
  "RV570",
  "R580",
  /* IGP */
  "RS600",
  "RS690",
  "RS740",
  "RS780",
  "RS880",
  /* R600 */
  "R600",
  "RV610",
  "RV620",
  "RV630",
  "RV635",
  "RV670",
  /* R700 */
  "RV710",
  "RV730",
  "RV740",
  "RV770",
  /* Evergreen */
  "Cedar",
  "Cypress",
  "Hemlock",
  "Juniper",
  "Redwood",
  /* Northern Islands */
  "Barts",
  "Caicos",
  "Cayman",
  "Turks",
  "Tahiti",
  ""
};

card_config_t card_configs[] = {
  {NULL, 0},
  {"Wormy", 2},
  {"Alopias", 2},
  {"Alouatta", 4},
  {"Baboon", 3},
  {"Cardinal", 2},
  {"Caretta", 1},
  {"Colobus", 2},
  {"Douc", 2},
  {"Eulemur", 3},
  {"Flicker", 3},
  {"Galago", 2},
  {"Gliff", 3},
  {"Hoolock", 1},
  {"Hypoprion", 2},
  {"Iago", 2},
  {"Kakapo", 3},
  {"Kipunji", 4},
  {"Lamna", 2},
  {"Langur", 3},
  {"Megalodon", 3},
  {"Motmot", 2},
  {"Nomascus", 5},
  {"Orangutan", 2},
  {"Peregrine", 2},
  {"Quail", 3},
  {"Raven", 3},
  {"Shrike", 3},
  {"Sphyrna", 1},
  {"Triakis", 2},
  {"Uakari", 4},
  {"Vervet", 4},
  {"Zonalis", 6},
  {"Pithecia", 3},
  {"Bulrushes", 6},
  {"Cattail", 4},
  {"Hydrilla", 5},
  {"Duckweed", 4},
  {"Fanwort", 4},
  {"Elodea", 5},
  {"Kudzu", 2},
  {"Gibba", 5},
  {"Lotus", 3},
  {"Ipomoea", 3},
  {"Mangabey", 2},
  {"Muskgrass", 4},
  {"Juncus", 4},
  {"Pondweed", 3},
  {"Aji", 4},
  {"Buri", 4},
  {"Chutoro", 5},
  {"Dashimaki", 4},
  {"Ebi", 5},
  {"Futomaki", 5},
  {"Hamachi", 4},
  {"Gari", 5},
};

radeon_card_info_t radeon_cards[] = {

  // Earlier cards are not supported
  //
  // Layout is device_id, fake_id, chip_family_name, display name, frame buffer
  // Cards are grouped by device id  to make it easier to add new cards
  //

  /*old series*/
  { 0x5D48, 0x00000000, CHIP_FAMILY_R420, "ATI Radeon HD Mobile \0", kNull     },
  { 0x5D49, 0x00000000, CHIP_FAMILY_R420, "ATI Radeon HD Mobile \0", kNull     },
  { 0x5D4A, 0x00000000, CHIP_FAMILY_R420, "ATI Radeon HD Mobile \0", kNull     },
  { 0x5D4C, 0x00000000, CHIP_FAMILY_R420, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5D4D, 0x00000000, CHIP_FAMILY_R420, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5D4E, 0x00000000, CHIP_FAMILY_R420, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5D4F, 0x00000000, CHIP_FAMILY_R420, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5D50, 0x00000000, CHIP_FAMILY_R420, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5D52, 0x00000000, CHIP_FAMILY_R420, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5D57, 0x00000000, CHIP_FAMILY_R420, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5E48, 0x00000000, CHIP_FAMILY_RV410, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5E4A, 0x00000000, CHIP_FAMILY_RV410, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5E4B, 0x00000000, CHIP_FAMILY_RV410, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5E4C, 0x00000000, CHIP_FAMILY_RV410, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5E4D, 0x00000000, CHIP_FAMILY_RV410, "ATI Radeon HD Desktop \0", kNull     },
  { 0x5E4F, 0x00000000, CHIP_FAMILY_RV410, "ATI Radeon HD Desktop \0", kNull     },
  //X1000 0x71871002 0x72101002 0x71DE1002 0x71461002 0x71421002 0x71091002 0x71C51002
  //      0x71C01002 0x72401002 0x72491002 0x72911002
  { 0x7100, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Desktop \0", kNull     },
  { 0x7101, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Mobile \0", kNull     },
  { 0x7102, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Mobile \0", kNull     },
  { 0x7103, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Mobile \0", kNull     },
  { 0x7104, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Desktop \0", kNull     },
  { 0x7105, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Desktop \0", kNull     },
  { 0x7106, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Mobile \0", kNull     },
  { 0x7108, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Desktop \0", kNull     },
  { 0x7109, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Desktop \0", kNull     },
  { 0x710A, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Desktop \0", kNull     },
  { 0x710B, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Desktop \0", kNull     },
  { 0x710C, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Desktop \0", kNull     },
  { 0x710E, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Desktop \0", kNull     },
  { 0x710F, 0x00000000, CHIP_FAMILY_R520, "ATI Radeon HD Desktop \0", kNull     },
  { 0x7140, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7141, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7142, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7143, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7144, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  { 0x7145, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  //7146, 7187 - Caretta
  { 0x7146, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7147, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7149, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  { 0x714A, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  { 0x714B, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  { 0x714C, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  { 0x714D, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x714E, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x714F, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7151, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7152, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7153, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x715E, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x715F, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7180, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7181, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7183, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7186, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  { 0x7187, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7188, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD2300 Mobile \0", kCaretta     },
  { 0x718A, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  { 0x718B, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  { 0x718C, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  { 0x718D, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  { 0x718F, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7193, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x7196, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Mobile \0", kCaretta     },
  { 0x719B, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x719F, 0x00000000, CHIP_FAMILY_RV515, "ATI Radeon HD Desktop \0", kCaretta     },
  { 0x71C0, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Desktop \0", kWormy     },
  { 0x71C1, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Desktop \0", kWormy     },
  { 0x71C2, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Desktop \0", kWormy     },
  { 0x71C3, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Desktop \0", kWormy     },
  { 0x71C4, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Mobile \0", kWormy     },
  //71c5 -Wormy
  { 0x71C5, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD1600 Mobile \0", kWormy     },
  { 0x71C6, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Desktop \0", kWormy     },
  { 0x71C7, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Desktop \0", kWormy     },
  { 0x71CD, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Desktop \0", kWormy     },
  { 0x71CE, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Desktop \0", kWormy     },
  { 0x71D2, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Desktop \0", kWormy     },
  { 0x71D4, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Mobile \0", kWormy     },
  { 0x71D5, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Mobile \0", kWormy     },
  { 0x71D6, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Mobile \0", kWormy     },
  { 0x71DA, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Desktop \0", kWormy     },
  { 0x71DE, 0x00000000, CHIP_FAMILY_RV530, "ASUS M66 ATI Radeon Mobile \0", kWormy     },
  { 0x7200, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Desktop \0", kWormy     },
  { 0x7210, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Mobile \0", kWormy     },
  { 0x7211, 0x00000000, CHIP_FAMILY_RV530, "ATI Radeon HD Mobile \0", kWormy     },
  { 0x7240, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7243, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7244, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7245, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7246, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7247, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7248, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  //7249 -Alopias
  { 0x7249, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x724A, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x724B, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x724C, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x724D, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x724E, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x724F, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7280, 0x00000000, CHIP_FAMILY_RV570, "ATI Radeon X1950 Pro \0", kAlopias     },
  { 0x7281, 0x00000000, CHIP_FAMILY_RV560, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7283, 0x00000000, CHIP_FAMILY_RV560, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7284, 0x00000000, CHIP_FAMILY_R580, "ATI Radeon HD Mobile \0", kAlopias     },
  { 0x7287, 0x00000000, CHIP_FAMILY_RV560, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7288, 0x00000000, CHIP_FAMILY_RV570, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7289, 0x00000000, CHIP_FAMILY_RV570, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x728B, 0x00000000, CHIP_FAMILY_RV570, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x728C, 0x00000000, CHIP_FAMILY_RV570, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7290, 0x00000000, CHIP_FAMILY_RV560, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7291, 0x00000000, CHIP_FAMILY_RV560, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7293, 0x00000000, CHIP_FAMILY_RV560, "ATI Radeon HD Desktop \0", kAlopias     },
  { 0x7297, 0x00000000, CHIP_FAMILY_RV560, "ATI Radeon HD Desktop \0", kAlopias     },
  //IGP
  { 0x791E, 0x00000000, CHIP_FAMILY_RS690, "ATI Radeon IGP \0", kNull     },
  { 0x791F, 0x00000000, CHIP_FAMILY_RS690, "ATI Radeon IGP \0", kNull     },
  { 0x796C, 0x00000000, CHIP_FAMILY_RS740, "ATI Radeon IGP \0", kNull     },
  { 0x796D, 0x00000000, CHIP_FAMILY_RS740, "ATI Radeon IGP \0", kNull     },
  { 0x796E, 0x00000000, CHIP_FAMILY_RS740, "ATI Radeon IGP \0", kNull     },
  { 0x796F, 0x00000000, CHIP_FAMILY_RS740, "ATI Radeon IGP \0", kNull     },


  //X2000 0x94001002 0x94011002 0x94021002 0x94031002 0x95811002 0x95831002 0x95881002 0x94c81002 0x94c91002
  //      0x95001002 0x95011002 0x95051002 0x95071002 0x95041002 0x95061002 0x95981002 0x94881002 0x95991002
  //      0x95911002 0x95931002 0x94401002 0x94421002 0x944A1002 0x945A1002 0x94901002 0x949E1002 0x94801002
  //      0x95401002 0x95411002 0x954E1002 0x954F1002 0x95521002 0x95531002 0x94a01002
  /* standard/default models */
  { 0x9400, 0x00000000, CHIP_FAMILY_R600, "ATI Radeon HD 2900 XT\0", kNull    },
  { 0x9401, 0x00000000, CHIP_FAMILY_R600, "ATI Radeon HD 2900 GT\0", kNull    },
  { 0x9402, 0x00000000, CHIP_FAMILY_R600, "ATI Radeon HD 2900 GT\0", kNull    },
  { 0x9403, 0x00000000, CHIP_FAMILY_R600, "ATI Radeon HD 2900 GT\0", kNull    },
  { 0x9405, 0x00000000, CHIP_FAMILY_R600, "ATI Radeon HD 2900 GT\0", kNull    },
  { 0x940A, 0x00000000, CHIP_FAMILY_R600, "ATI FireGL V8650\0", kNull    },
  { 0x940B, 0x00000000, CHIP_FAMILY_R600, "ATI FireGL V8600\0", kNull    },
  { 0x940F, 0x00000000, CHIP_FAMILY_R600, "ATI FireGL V7600\0", kNull    },
  //9440, 944A - Cardinal
  { 0x9440, 0x00000000, CHIP_FAMILY_RV770, "ATI Radeon HD 4870 \0", kMotmot    },
  { 0x9441, 0x00000000, CHIP_FAMILY_RV770, "ATI Radeon HD 4870 X2\0", kMotmot    },
  { 0x9442, 0x00000000, CHIP_FAMILY_RV770, "ATI Radeon HD 4850 Series\0", kMotmot    },
  { 0x9443, 0x00000000, CHIP_FAMILY_RV770, "ATI Radeon HD 4850 X2\0", kMotmot    },
  { 0x9444, 0x00000000, CHIP_FAMILY_RV770, "ATI FirePro V8750 (FireGL)\0", kMotmot    },
  { 0x9446, 0x00000000, CHIP_FAMILY_RV770, "ATI FirePro V7770 (FireGL)\0", kMotmot    },
  { 0x9447, 0x00000000, CHIP_FAMILY_RV770, "ATI FirePro V8700 Duo (FireGL)\0", kMotmot    },
  { 0x944A, 0x00000000, CHIP_FAMILY_RV770, "ATI Mobility Radeon HD 4850\0", kMotmot    },//iMac - Quail
  { 0x944B, 0x00000000, CHIP_FAMILY_RV770, "ATI Mobility Radeon HD 4850 X2\0", kMotmot    },//iMac - Quail
  { 0x944C, 0x00000000, CHIP_FAMILY_RV770, "ATI Radeon HD 4830 Series\0", kMotmot    },
  { 0x944E, 0x00000000, CHIP_FAMILY_RV770, "ATI Radeon HD 4810 Series\0", kMotmot    },

  { 0x9450, 0x00000000, CHIP_FAMILY_RV770, "AMD FireStream 9270\0", kMotmot    },
  { 0x9452, 0x00000000, CHIP_FAMILY_RV770, "AMD FireStream 9250\0", kMotmot    },
  { 0x9456, 0x00000000, CHIP_FAMILY_RV770, "ATI FirePro V8700 (FireGL)\0", kMotmot    },
  { 0x945A, 0x00000000, CHIP_FAMILY_RV770, "ATI Mobility Radeon HD 4870\0", kMotmot    },

  { 0x9460, 0x00000000, CHIP_FAMILY_RV770, "ATI Radeon HD 4800 Series\0", kMotmot    },
  { 0x9462, 0x00000000, CHIP_FAMILY_RV770, "ATI Radeon HD 4800 Series\0", kMotmot    },
  //9488, 9490 - Gliff
  { 0x9480, 0x00000000, CHIP_FAMILY_RV730, "ATI Mobility Radeon HD 550v\0", kGliff  },
  { 0x9488, 0x00000000, CHIP_FAMILY_RV730, "ATI Radeon HD 4650 Series\0", kGliff  },
  { 0x9490, 0x00000000, CHIP_FAMILY_RV730, "ATI Radeon HD 4670 Series\0", kGliff  },
  { 0x9491, 0x00000000, CHIP_FAMILY_RV730, "ATI Radeon HD 4600 Series\0", kGliff  },
  { 0x9495, 0x00000000, CHIP_FAMILY_RV730, "ATI Radeon HD 4650 Series\0", kGliff  },
  { 0x9498, 0x00000000, CHIP_FAMILY_RV730, "ATI Radeon HD 4710 Series\0", kGliff  },

  { 0x94B3, 0x00000000, CHIP_FAMILY_RV740, "ATI Radeon HD 4770\0", kFlicker  },
  { 0x94B4, 0x00000000, CHIP_FAMILY_RV740, "ATI Radeon HD 4700 Series\0", kFlicker  },
  { 0x94B5, 0x00000000, CHIP_FAMILY_RV740, "ATI Radeon HD 4770\0", kFlicker  },
  //94C8 -Iago
  { 0x94C1, 0x00000000, CHIP_FAMILY_RV610, "ATI Radeon HD 2400 Series\0", kIago    },
  { 0x94C3, 0x00000000, CHIP_FAMILY_RV610, "ATI Radeon HD 2350 Series\0", kIago    },
  { 0x94C4, 0x00000000, CHIP_FAMILY_RV610, "ATI Radeon HD 2400 Series\0", kIago    },
  { 0x94C5, 0x00000000, CHIP_FAMILY_RV610, "ATI Radeon HD 2400 Series\0", kIago    },
  { 0x94C6, 0x00000000, CHIP_FAMILY_RV610, "ATI Radeon HD 2400 Series\0", kIago    },
  { 0x94C7, 0x00000000, CHIP_FAMILY_RV610, "ATI Radeon HD 2350\0", kIago    },
  { 0x94C8, 0x00000000, CHIP_FAMILY_RV610, "ATI Radeon HD 2400 Series\0", kIago    },
  { 0x94C9, 0x00000000, CHIP_FAMILY_RV610, "ATI Radeon HD 2400 Series\0", kIago    },
  { 0x94CB, 0x00000000, CHIP_FAMILY_RV610, "ATI Radeon HD 2400 Series\0", kIago    },
  { 0x94CC, 0x00000000, CHIP_FAMILY_RV610, "ATI Radeon HD 2400 Series\0", kIago    },
  //9501 - Megalodon, Triakis HD3800
  { 0x9500, 0x00000000, CHIP_FAMILY_RV670, "ATI Radeon HD 3800 Series\0", kMegalodon  },
  { 0x9501, 0x00000000, CHIP_FAMILY_RV670, "ATI Radeon HD 3690 Series\0", kMegalodon  },
  { 0x9505, 0x00000000, CHIP_FAMILY_RV670, "ATI Radeon HD 3850\0", kMegalodon  },
  { 0x9507, 0x00000000, CHIP_FAMILY_RV670, "ATI Radeon HD 3830\0", kMegalodon  },
  { 0x950F, 0x00000000, CHIP_FAMILY_RV670, "ATI Radeon HD 3870 X2\0", kMegalodon  },
  { 0x9511, 0x00000000, CHIP_FAMILY_RV670, "ATI Radeon HD 3850 X2\0", kMegalodon  },

  { 0x9513, 0x00000000, CHIP_FAMILY_RV670, "ATI Radeon HD 3850 X2\0", kMegalodon  },
  { 0x9519, 0x00000000, CHIP_FAMILY_RV670, "AMD FireStream 9170\0", kMegalodon  },

  { 0x9540, 0x00000000, CHIP_FAMILY_RV710, "ATI Radeon HD 4550\0", kFlicker    },
  { 0x954F, 0x00000000, CHIP_FAMILY_RV710, "ATI Radeon HD 4350\0", kFlicker    },
  { 0x9552, 0x00000000, CHIP_FAMILY_RV710, "ATI Mobility Radeon HD 4330\0", kShrike     },
  { 0x9553, 0x00000000, CHIP_FAMILY_RV710, "ATI Mobility Radeon HD 4570\0", kShrike     },
  { 0x9555, 0x00000000, CHIP_FAMILY_RV710, "ATI Mobility Radeon HD 4550\0", kShrike     },
  //9583, 9588 - Lamna, Hypoprion HD2600
  { 0x9581, 0x00000000, CHIP_FAMILY_RV630, "ATI Radeon HD 2600 XT\0", kHypoprion  },
  { 0x9583, 0x00000000, CHIP_FAMILY_RV630, "ATI Radeon HD 2600 XT\0", kHypoprion  },
  { 0x9588, 0x00000000, CHIP_FAMILY_RV630, "ATI Radeon HD 2600 XT\0", kHypoprion  },
  { 0x9589, 0x00000000, CHIP_FAMILY_RV630, "ATI Radeon HD 2600 PRO\0", kHypoprion  },
  { 0x958A, 0x00000000, CHIP_FAMILY_RV630, "ATI Radeon HD 2600 X2 Series\0", kLamna      },
  { 0x958B, 0x00000000, CHIP_FAMILY_RV630, "ATI Radeon HD 2600 X2 Series\0", kLamna      },
  { 0x958C, 0x00000000, CHIP_FAMILY_RV630, "ATI Radeon HD 2600 X2 Series\0", kLamna      },
  { 0x958D, 0x00000000, CHIP_FAMILY_RV630, "ATI Radeon HD 2600 X2 Series\0", kLamna      },
  { 0x958E, 0x00000000, CHIP_FAMILY_RV630, "ATI Radeon HD 2600 X2 Series\0", kLamna      },

  { 0x9591, 0x00000000, CHIP_FAMILY_RV635, "ATI Radeon HD 3600 Series\0", kMegalodon  },
  { 0x9598, 0x00000000, CHIP_FAMILY_RV635, "ATI Radeon HD 3600 Series\0", kMegalodon  },

  { 0x95C0, 0x00000000, CHIP_FAMILY_RV620, "ATI Radeon HD 3550 Series\0", kIago       },
  { 0x95C4, 0x00000000, CHIP_FAMILY_RV620, "ATI Radeon HD 3470 Series\0", kIago       },
  { 0x95C5, 0x00000000, CHIP_FAMILY_RV620, "ATI Radeon HD 3450 Series\0", kIago       },
  { 0x95C6, 0x00000000, CHIP_FAMILY_RV620, "ATI Radeon HD 3450 AGP\0", kIago       },

  /* IGP */
  { 0x9610, 0x00000000, CHIP_FAMILY_RS780, "ATI Radeon HD 3200 Graphics\0", kNull       },
  { 0x9611, 0x00000000, CHIP_FAMILY_RS780, "ATI Radeon HD 3100 Graphics\0", kNull       },
  { 0x9614, 0x00000000, CHIP_FAMILY_RS780, "ATI Radeon HD 3300 Graphics\0", kNull       },
  { 0x9616, 0x00000000, CHIP_FAMILY_RS780, "AMD 760G\0", kNull       },

  //SUMO
  //mobile = G desktop = D
  { 0x9640, 0x00000000, CHIP_FAMILY_SUMO, "AMD Radeon HD 6550D\0", kNull       },
  { 0x9641, 0x00000000, CHIP_FAMILY_SUMO, "AMD Radeon HD 6620G\0", kNull       },
  { 0x9642, 0x00000000, CHIP_FAMILY_SUMO, "AMD Radeon HD 6370D\0", kNull       },
  { 0x9643, 0x00000000, CHIP_FAMILY_SUMO, "AMD Radeon HD 6380G\0", kNull       },
  { 0x9644, 0x00000000, CHIP_FAMILY_SUMO, "AMD Radeon HD 6410D\0", kNull       },
  { 0x9645, 0x00000000, CHIP_FAMILY_SUMO, "AMD Radeon HD 6410D\0", kNull       },
  { 0x9647, 0x00000000, CHIP_FAMILY_SUMO, "AMD Radeon HD 6520G\0", kNull       },
  { 0x9648, 0x00000000, CHIP_FAMILY_SUMO, "AMD Radeon HD 6480G\0", kNull       },
  { 0x9649, 0x00000000, CHIP_FAMILY_SUMO, "AMD Radeon(TM) HD 6480G\0", kNull       },
  { 0x964A, 0x00000000, CHIP_FAMILY_SUMO, "AMD Radeon HD 6530D\0", kNull       },

  { 0x9710, 0x00000000, CHIP_FAMILY_RS880, "ATI Radeon HD 4200\0", kNull       },
  { 0x9715, 0x00000000, CHIP_FAMILY_RS880, "ATI Radeon HD 4250\0", kNull       },
  { 0x9714, 0x00000000, CHIP_FAMILY_RS880, "ATI Radeon HD 4290\0", kNull       },

  // 0x9804 - AMD HD6250 Wrestler
  { 0x9802, 0x00000000, CHIP_FAMILY_WRESTLER, "AMD Radeon HD 6310 Graphics\0", kNull       },
  { 0x9803, 0x00000000, CHIP_FAMILY_WRESTLER, "AMD Radeon HD 6250 Graphics\0", kNull       },
  { 0x9804, 0x00000000, CHIP_FAMILY_WRESTLER, "AMD Radeon HD 6250 Graphics\0", kNull       },
  { 0x9805, 0x00000000, CHIP_FAMILY_WRESTLER, "AMD Radeon HD 6250 Graphics\0", kNull       },
  { 0x9806, 0x00000000, CHIP_FAMILY_WRESTLER, "AMD Radeon HD 6320 Graphics\0", kNull       },
  { 0x9807, 0x00000000, CHIP_FAMILY_WRESTLER, "AMD Radeon HD 6290 Graphics\0", kNull       },
  { 0x9808, 0x00000000, CHIP_FAMILY_WRESTLER, "AMD Radeon HD 7340 Graphics\0", kNull       },
  { 0x9809, 0x00000000, CHIP_FAMILY_WRESTLER, "AMD Radeon HD 7310 Graphics\0", kNull       },

  //TrinityGL
  //mobile = G desktop = D
  { 0x9900, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7660G\0", kNull       },
  { 0x9901, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7660D\0", kNull       },
  { 0x9903, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7640G\0", kNull       },
  { 0x9904, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7560D\0", kNull       },
  { 0x9906, 0x00000000, CHIP_FAMILY_TRINITY, "AMD FirePro A300 Series\0", kNull       },
  { 0x9907, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7620G\0", kNull       },
  { 0x9908, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7600G\0", kNull       },
  { 0x9910, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7660G\0", kNull       },
  { 0x9913, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7640G\0", kNull       },

  { 0x9990, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7520G\0", kNull       },
  { 0x9991, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7540D\0", kNull       },
  { 0x9992, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7420G\0", kNull       },
  { 0x9994, 0x00000000, CHIP_FAMILY_TRINITY, "AMD Radeon HD 7400G\0", kNull       },

  //X2000.kext
  //0x94001002 0x94011002 0x94021002 0x94031002 0x95811002 0x95831002 0x95881002 0x94c81002 0x94c91002
  //0x95001002 0x95011002 0x95051002 0x95071002 0x95041002 0x95061002 0x95981002 0x94881002 0x95991002
  //0x95911002 0x95931002 0x94401002 0x94421002 0x944A1002 0x945A1002 0x94901002 0x949E1002 0x94801002
  //0x95401002 0x95411002 0x954E1002 0x954F1002 0x95521002 0x95531002 0x94a01002

  //what about ATI6320 devID=0x9806? same as 4330 but IGP

  //X3000 -
  //0x68881002 0x68891002 0x68981002 0x68991002 0x689C1002 0x689D1002 0x68801002 0x68901002 0x68A81002
  //0x68A91002 0x68B81002 0x68B91002 0x68BE1002 0x68A01002 0x68A11002 0x68B01002 0x68B11002 0x68C81002
  //0x68C91002 0x68D81002 0x68D91002 0x68DE1002 0x68C01002 0x68C11002 0x68D01002 0x68D11002 0x68E81002
  //0x68E91002 0x68F81002 0x68F91002 0x68FE1002 0x68E01002 0x68E11002 0x68F01002 0x68F11002 0x67011002
  //0x67021002 0x67031002 0x67041002 0x67051002 0x67061002 0x67071002 0x67081002 0x67091002 0x67181002
  //0x67191002 0x671C1002 0x671D1002 0x67221002 0x67231002 0x67261002 0x67271002 0x67281002 0x67291002
  //0x67381002 0x67391002 0x67201002 0x67211002 0x67241002 0x67251002 0x67421002 0x67431002 0x67461002
  //0x67471002 0x67481002 0x67491002 0x67501002 0x67581002 0x67591002 0x67401002 0x67411002 0x67441002
  //0x67451002 0x67621002 0x67631002 0x67661002 0x67671002 0x67681002 0x67701002 0x67791002 0x67601002
  //0x67611002 0x67641002 0x67651002
  /* Evergreen */
  //0x68981002 0x68991002 0x68E01002 0x68E11002 0x68D81002 0x68C01002 0x68C11002
  //0x68D91002 0x68B81002 0x68B01002 0x68B11002 0x68A01002 0x68A11002
  //Hoolock, Langur, Orangutan, Zonalis
  //10.9 AMD5000
  //  0x68981002 0x68991002 0x68E01002 0x68E11002 0x68D81002 0x68C01002 0x68C11002 0x68D91002 0x68B81002
  //  0x68B01002 0x68B11002 0x68A01002 0x68A11002

  { 0x688D, 0x00000000, CHIP_FAMILY_CYPRESS, "AMD FireStream 9350\0", kZonalis  },

  { 0x6898, 0x00000000, CHIP_FAMILY_CYPRESS, "ATI Radeon HD 5870 Series\0", kUakari   },
  { 0x6899, 0x00000000, CHIP_FAMILY_CYPRESS, "ATI Radeon HD 5850 Series\0", kUakari   },
  { 0x689C, 0x00000000, CHIP_FAMILY_HEMLOCK, "ATI Radeon HD 5970 Series\0", kUakari    },
  { 0x689E, 0x00000000, CHIP_FAMILY_HEMLOCK, "ATI Radeon HD 5800 Series\0", kUakari   },

  { 0x68A0, 0x00000000, CHIP_FAMILY_MANHATTAN, "ATI Radeon HD 5770 Series\0", kHoolock  },
  { 0x68A1, 0x00000000, CHIP_FAMILY_MANHATTAN, "ATI Radeon HD 5850 Series\0", kHoolock  },
  { 0x68A8, 0x00000000, CHIP_FAMILY_MANHATTAN, "ATI Radeon HD 6850M \0", kHoolock  },
  { 0x68A9, 0x00000000, CHIP_FAMILY_JUNIPER, "ATI FirePro V5800 (FireGL)\0", kHoolock  },
  //was Vervet but Hoolock is better.
  //doesn't matter if you made connectors patch
  { 0x68B0, 0x00000000, CHIP_FAMILY_JUNIPER, "ATI Radeon HD 5770 Series\0", kHoolock    },
  { 0x68B1, 0x00000000, CHIP_FAMILY_JUNIPER, "ATI Radeon HD 5770 Series\0", kHoolock    },
  { 0x68B8, 0x00000000, CHIP_FAMILY_JUNIPER, "ATI Radeon HD 5770 Series\0", kHoolock    },
  { 0x68B9, 0x00000000, CHIP_FAMILY_JUNIPER, "ATI Radeon HD 5700 Series\0", kHoolock    },
  { 0x68BA, 0x00000000, CHIP_FAMILY_JUNIPER, "ATI Radeon HD 6700 Series\0", kHoolock    },
  { 0x68BC, 0x00000000, CHIP_FAMILY_JUNIPER, "AMD FireStream 9370\0", kHoolock    },
  { 0x68BD, 0x00000000, CHIP_FAMILY_JUNIPER, "AMD FireStream 9350\0", kHoolock    },
  { 0x68BE, 0x00000000, CHIP_FAMILY_JUNIPER, "ATI Radeon HD 5750 Series\0", kHoolock    },
  { 0x68BF, 0x00000000, CHIP_FAMILY_JUNIPER, "ATI Radeon HD 6750 Series\0", kHoolock    },

  { 0x68C0, 0x00000000, CHIP_FAMILY_REDWOOD, "ATI Radeon HD 5730 Series\0", kBaboon    },
  { 0x68C1, 0x00000000, CHIP_FAMILY_REDWOOD, "ATI Radeon HD 5650 Series\0", kBaboon    },
  { 0x68C7, 0x00000000, CHIP_FAMILY_REDWOOD, "ATI Mobility Radeon HD 5570\0", kEulemur  },
  { 0x68C8, 0x00000000, CHIP_FAMILY_REDWOOD, "ATI Radeon HD 5650 Series\0", kBaboon    },
  { 0x68C9, 0x00000000, CHIP_FAMILY_REDWOOD, "FirePro 3D V3800\0", kBaboon    },
  { 0x68D8, 0x00000000, CHIP_FAMILY_REDWOOD, "ATI Radeon HD 5670 Series\0", kBaboon    },
  { 0x68D9, 0x00000000, CHIP_FAMILY_REDWOOD, "ATI Radeon HD 5570 Series\0", kBaboon    },
  { 0x68DA, 0x00000000, CHIP_FAMILY_REDWOOD, "ATI Radeon HD 5500 Series\0", kBaboon    },

  { 0x68E0, 0x00000000, CHIP_FAMILY_CEDAR, "ATI Radeon HD 5470 Series\0", kEulemur  },
  { 0x68E1, 0x00000000, CHIP_FAMILY_CEDAR, "AMD Radeon HD 6230\0", kEulemur  },
  { 0x68E4, 0x00000000, CHIP_FAMILY_CEDAR, "ATI Radeon HD 6370M Series\0", kEulemur  },
  { 0x68E5, 0x00000000, CHIP_FAMILY_CEDAR, "ATI Radeon HD 6300M Series\0", kEulemur  },
  //EvergreenGL
  { 0x68F1, 0x00000000, CHIP_FAMILY_CEDAR, "AMD FirePro 2460\0", kEulemur  },
  { 0x68F2, 0x00000000, CHIP_FAMILY_CEDAR, "AMD FirePro 2270\0", kEulemur  },
  { 0x68F9, 0x00000000, CHIP_FAMILY_CEDAR, "ATI Radeon HD 5450 Series\0", kEulemur  },
  { 0x68FA, 0x00000000, CHIP_FAMILY_CEDAR, "ATI Radeon HD 7300 Series\0", kEulemur  },

  /* Northen Islands */
  //0x67681002 0x67701002 0x67791002 0x67601002 0x67611002 0x67501002 0x67581002 0x67591002
  //0x67401002 0x67411002 0x67451002 0x67381002 0x67391002 0x67201002 0x67221002 0x67181002
  //Gibba, Lotus, Muskgrass
  //id from AMD6000 10.9
  //0x67681002 0x67701002 0x67791002 0x67601002 0x67611002 0x67501002 0x67581002 0x67591002
  //0x67401002 0x67411002 0x67451002 0x67381002 0x67391002 0x67201002 0x67221002 0x67181002
  //0x67191002 0x68401002 0x68411002 0x67041002
  { 0x6704, 0x00000000, CHIP_FAMILY_CAYMAN, "AMD FirePro V7900\0", kLotus    },
  { 0x6707, 0x00000000, CHIP_FAMILY_CAYMAN, "AMD FirePro V5900\0", kLotus    },
  { 0x6718, 0x00000000, CHIP_FAMILY_CAYMAN, "AMD Radeon HD 6970 Series\0", kLotus    },
  { 0x6719, 0x00000000, CHIP_FAMILY_CAYMAN, "AMD Radeon HD 6950 Series\0", kLotus    },
  { 0x671C, 0x00000000, CHIP_FAMILY_CAYMAN, "AMD Radeon HD 6970 Series\0", kLotus    },
  { 0x671D, 0x00000000, CHIP_FAMILY_CAYMAN, "AMD Radeon HD 6950 Series\0", kLotus    },
  { 0x671F, 0x00000000, CHIP_FAMILY_CAYMAN, "AMD Radeon HD 6930 Series\0", kLotus    },


  { 0x6720, 0x00000000, CHIP_FAMILY_VANCOUVER, "AMD Radeon HD 6900M Series\0", kFanwort   },
  { 0x6722, 0x00000000, CHIP_FAMILY_BARTS, "AMD Radeon HD 6900M Series\0", kFanwort   },
  { 0x6729, 0x00000000, CHIP_FAMILY_BARTS, "AMD Radeon HD 6900M Series\0", kFanwort   },
  { 0x6738, 0x00000000, CHIP_FAMILY_BARTS, "AMD Radeon HD 6870\0", kDuckweed  },
  { 0x6739, 0x00000000, CHIP_FAMILY_BARTS, "AMD Radeon HD 6850 X2\0", kDuckweed  },
  { 0x673E, 0x00000000, CHIP_FAMILY_BARTS, "AMD Radeon HD 6790 Series\0", kDuckweed   },

  { 0x6740, 0x00000000, CHIP_FAMILY_VANCOUVER, "AMD Radeon HD 6770M Series\0", kCattail    },
  { 0x6741, 0x00000000, CHIP_FAMILY_VANCOUVER, "AMD Radeon HD 6750M\0", kCattail    },
  { 0x6742, 0x00000000, CHIP_FAMILY_VANCOUVER, "AMD Radeon HD 7500/7600 Series\0", kCattail    },
  { 0x6745, 0x00000000, CHIP_FAMILY_VANCOUVER, "AMD Radeon HD 6600M Series\0", kCattail    },
  { 0x6749, 0x00000000, CHIP_FAMILY_CAYMAN, "ATI Radeon FirePro V4900\0", kPithecia    },
  { 0x674A, 0x00000000, CHIP_FAMILY_CAYMAN, "AMD FirePro V3900\0", kPithecia    },
  { 0x6750, 0x00000000, CHIP_FAMILY_TURKS, "AMD Radeon HD 6670 Series\0", kPithecia   },
  { 0x6758, 0x00000000, CHIP_FAMILY_TURKS, "AMD Radeon HD 6670 Series\0", kPithecia   },
  { 0x6759, 0x00000000, CHIP_FAMILY_TURKS, "AMD Radeon HD 6570 Series\0", kPithecia   },
  { 0x675B, 0x00000000, CHIP_FAMILY_TURKS, "AMD Radeon HD 7600 Series\0", kPithecia   },
  { 0x675D, 0x00000000, CHIP_FAMILY_TURKS, "AMD Radeon HD 7570M Series\0", kCattail    },
  { 0x675F, 0x00000000, CHIP_FAMILY_TURKS, "AMD Radeon HD 6510 Series\0", kPithecia   },

  { 0x6760, 0x00000000, CHIP_FAMILY_CAICOS, "AMD Radeon HD 6470M Series\0", kHydrilla  },
  { 0x6761, 0x00000000, CHIP_FAMILY_CAICOS, "AMD Radeon HD 6430M Series\0", kHydrilla  },
  { 0x6763, 0x00000000, CHIP_FAMILY_CAICOS, "AMD Radeon E6460\0", kHydrilla  },
  { 0x6768, 0x00000000, CHIP_FAMILY_CAICOS, "AMD Radeon HD 6400M Series\0", kHydrilla  },
  { 0x6770, 0x00000000, CHIP_FAMILY_CAICOS, "AMD Radeon HD 6400 Series\0", kBulrushes  },
  { 0x6772, 0x00000000, CHIP_FAMILY_CAICOS, "AMD Radeon HD 7400A Series\0", kBulrushes  },
  { 0x6778, 0x00000000, CHIP_FAMILY_CAICOS, "AMD Radeon HD 7000 Series\0", kBulrushes  },
  { 0x6779, 0x00000000, CHIP_FAMILY_CAICOS, "AMD Radeon HD 7450 Series\0", kBulrushes  },
  { 0x677B, 0x00000000, CHIP_FAMILY_CAICOS, "AMD Radeon HD 7400 Series\0", kBulrushes  },
  //Tahiti
  //Framebuffers: Aji - 4 Desktop, Buri - 4 Mobile, Chutoro - 5 Mobile, Dashimaki - 4, IkuraS - HMDI
  // Ebi - 5 Mobile, Gari - 5 M, Futomaki - 4 D, Hamachi - 4 D, OPM - 6 Server, Ikura - 6,
  { 0x6780, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7900 Series\0", kFutomaki  },
  { 0x678A, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7900 Series\0", kFutomaki  },
  { 0x6790, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7900 Series\0", kFutomaki  },
  { 0x6798, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7970 X-Edition\0", kFutomaki  },
  { 0x679A, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7950 Series\0", kFutomaki  },
  { 0x679E, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7870\0", kFutomaki  },
  { 0x679F, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7950 Series\0", kFutomaki  },
  //Pitcairn
  { 0x6800, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7970m \0", kBuri  },
  { 0x6806, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7600 Series\0", kFutomaki  },
  { 0x6808, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7600 Series\0", kFutomaki  },
  { 0x6818, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7870 Series\0", kFutomaki  },
  { 0x6819, 0x00000000, CHIP_FAMILY_TAHITI, "AMD Radeon HD 7850 Series\0", kFutomaki  },

  { 0x6820, 0x00000000, CHIP_FAMILY_VERDE, "AMD Radeon HD 7700 Series\0", kBuri  },
  { 0x6821, 0x00000000, CHIP_FAMILY_VERDE, "AMD Radeon HD 7700 Series\0", kBuri  },
  { 0x6825, 0x00000000, CHIP_FAMILY_VERDE, "AMD Radeon HD 7800m Series\0", kPondweed  },
  { 0x6827, 0x00000000, CHIP_FAMILY_VERDE, "AMD Radeon HD 7800m Series\0", kPondweed  },
  { 0x682D, 0x00000000, CHIP_FAMILY_VERDE, "AMD Radeon HD 7700 Series\0", kBuri  },
  { 0x682F, 0x00000000, CHIP_FAMILY_VERDE, "AMD Radeon HD 7700 Series\0", kBuri  },
  { 0x6839, 0x00000000, CHIP_FAMILY_VERDE, "AMD Radeon HD 7700 Series\0", kBuri  },
  { 0x683B, 0x00000000, CHIP_FAMILY_VERDE, "AMD Radeon HD 7700 Series\0", kBuri  },
  { 0x683D, 0x00000000, CHIP_FAMILY_VERDE, "AMD Radeon HD 7770 Series\0", kBuri  },
  { 0x683F, 0x00000000, CHIP_FAMILY_VERDE, "AMD Radeon HD 7750 Series\0", kBuri  },
  //actually they are controlled by 6000Controller
  { 0x6840, 0x00000000, CHIP_FAMILY_LOMBOK, "AMD Radeon HD 7670m \0", kPondweed  },
  { 0x6841, 0x00000000, CHIP_FAMILY_LOMBOK, "AMD Radeon HD 7650m \0", kPondweed  },
  { 0x6842, 0x00000000, CHIP_FAMILY_LOMBOK, "AMD Radeon HD 7600m Series\0", kPondweed  },
  { 0x6843, 0x00000000, CHIP_FAMILY_VANCOUVER, "AMD Radeon HD 7670M\0", kPondweed  },
  { 0x6849, 0x00000000, CHIP_FAMILY_LOMBOK, "AMD Radeon HD 7600m Series\0", kPondweed  },
  { 0x6850, 0x00000000, CHIP_FAMILY_LOMBOK, "AMD Radeon HD 7600m Series\0", kPondweed  },
  { 0x6859, 0x00000000, CHIP_FAMILY_LOMBOK, "AMD Radeon HD 7600m Series\0", kPondweed  },
  { 0x0000, 0x00000000, CHIP_FAMILY_UNKNOW, "AMD Unknown\0", kNull    }
};

BOOLEAN get_bootdisplay_val(value_t *val);
BOOLEAN get_vrammemory_val(value_t *val);
BOOLEAN get_edid_val(value_t *val);
BOOLEAN get_display_type(value_t *val);
BOOLEAN get_name_val(value_t *val);
BOOLEAN get_nameparent_val(value_t *val);
BOOLEAN get_model_val(value_t *val);
BOOLEAN get_conntype_val(value_t *val);
BOOLEAN get_vrammemsize_val(value_t *val);
BOOLEAN get_binimage_val(value_t *val);
BOOLEAN get_binimage_owr(value_t *val);
BOOLEAN get_romrevision_val(value_t *val);
BOOLEAN get_deviceid_val(value_t *val);
BOOLEAN get_mclk_val(value_t *val);
BOOLEAN get_sclk_val(value_t *val);
BOOLEAN get_refclk_val(value_t *val);
BOOLEAN get_platforminfo_val(value_t *val);
BOOLEAN get_vramtotalsize_val(value_t *val);
BOOLEAN get_dual_link_val(value_t *val);
BOOLEAN get_name_pci_val(value_t *val);

AtiDevProp ati_devprop_list[] = {
#if 0
  { FLAGTRUE, FALSE, "@0,ATY,EFIDisplay", NULL, STRVAL ("TMDSA") },
  { FLAGTRUE, FALSE, "@0,display-connect-flags", NULL, DWRVAL ((UINT32) 0) },
  { FLAGTRUE, FALSE, "AAPL00,Dither", NULL, DWRVAL(0) },
  { FLAGTRUE, FALSE, "ATY,MCLK", get_mclk_val, NULVAL },
  { FLAGTRUE, FALSE, "ATY,RefCLK", get_refclk_val, DWRVAL ((UINT32) 0x0a8c) },
  { FLAGTRUE, FALSE, "ATY,SCLK", get_sclk_val, NULVAL },
  { FLAGTRUE, FALSE, "VRAM,totalsize", get_vramtotalsize_val, NULVAL },

  { FLAGTRUE, TRUE, "@0,AAPL,vram-memory", get_vrammemory_val, NULVAL },
  { FLAGTRUE, TRUE, "@0,compatible", get_name_val, NULVAL },
  { FLAGTRUE, TRUE, "@0,connector-type", get_conntype_val, NULVAL },
  { FLAGTRUE, TRUE, "@0,display-type", NULL, STRVAL ("NONE") },
  { FLAGTRUE, TRUE, "@0,VRAM,memsize", get_vrammemsize_val, NULVAL },
  { FLAGTRUE, TRUE, "AAPL00,override-no-connect", get_edid_val, NULVAL },

  { FLAGOLD, FALSE, "compatible", get_name_pci_val, NULVAL },
#endif
  { FLAGMOBILE, FALSE, "@0,backlight-control", NULL, DWRVAL(1) },
  { FLAGMOBILE, FALSE, "@0,display-dither-support", NULL, DWRVAL(0) },
  { FLAGMOBILE, FALSE, "@0,display-link-component-bits", NULL, DWRVAL(6) },
  { FLAGMOBILE, FALSE, "@0,display-pixel-component-bits", NULL, DWRVAL(6) },
  { FLAGMOBILE, FALSE, "AAPL,HasLid", NULL, DWRVAL(1) },
  { FLAGMOBILE, FALSE, "AAPL,HasPanel", NULL, DWRVAL(1) },

  { FLAGTRUE, FALSE, "@0,AAPL,boot-display", get_bootdisplay_val, NULVAL },
  { FLAGTRUE, FALSE, "AAPL,aux-power-connected", NULL, DWRVAL ((UINT32) 1) },
  { FLAGTRUE, FALSE, "AAPL,overwrite_binimage", get_binimage_owr, NULVAL  },
  { FLAGTRUE, FALSE, "AAPL,DualLink", get_dual_link_val, NULVAL  },
  { FLAGTRUE, FALSE, "ATY,bin_image", get_binimage_val, NULVAL },
  { FLAGTRUE, FALSE, "ATY,Card#", get_romrevision_val, NULVAL },
  { FLAGTRUE, FALSE, "ATY,Copyright", NULL, STRVAL ("Copyright AMD Inc. All Rights Reserved. 2005-2011") },
  { FLAGTRUE, FALSE, "ATY,DeviceID", get_deviceid_val, NULVAL },
  { FLAGTRUE, FALSE, "ATY,EFIVersion", NULL, STRVAL("01.00.3180")                  },
  { FLAGTRUE, FALSE, "ATY,PlatformInfo", get_platforminfo_val, NULVAL },
  { FLAGTRUE, FALSE, "ATY,VendorID", NULL, WRDVAL ((UINT32)0x1002) },
  { FLAGTRUE, FALSE, "device_type", get_nameparent_val, NULVAL },
  { FLAGTRUE, FALSE, "model", get_model_val, STRVAL ("ATI Radeon") },
  { FLAGTRUE, FALSE, "name", get_nameparent_val, NULVAL },

  { FLAGTRUE, TRUE, "@0,device_type", NULL, STRVAL ("display") },
  { FLAGTRUE, TRUE, "@0,name", get_name_val, NULVAL },

  { FLAGTRUE, FALSE, NULL, NULL, NULVAL }
};

/**
  native ID for 10.8.3

  ATI7000
  0x26001002 0x22001002 0x67901002 0x67981002 0x679A1002 0x679E1002 0x67801002 0x68201002 0x68211002
  0x68251002 0x68271002 0x682D1002 0x682F1002 0x68391002 0x683B1002 0x683D1002 0x683F1002 0x68001002
  0x68061002 0x68081002 0x68181002

  Barts
  0x67381002 0x67391002 0x67201002 0x67221002

  Bonaire
  0x66401002 0x66501002

  Caicos
  0x67681002 0x67701002 0x67791002 0x67601002 0x67611002

  Cayman
  0x67181002 0x67191002 0x67041002

  Cedar
  0x68E01002

  Cypress
  0x68981002 0x68991002

  Juniper
  0x68B81002 0x68B01002 0x68B11002 0x68A01002 0x68A11002

  Lombok = R476
  0x68401002 0x68411002

  Pitcairn = R575B
  0x68001002 0x68061002 0x68081002 0x68181002

  Redwood
  0x68D81002 0x68C01002 0x68C11002 0x68D91002

  Tahiti
  0x67901002 0x67981002 0x679A1002 0x679E1002 0x67801002

  Turks = NI
  0x67501002 0x67581002 0x67591002 0x67401002 0x67411002 0x67451002

  Verde = R575A, R576
  0x68201002 0x68211002 0x68251002 0x68271002 0x682D1002 0x682F1002 0x68391002 0x683B1002 0x683D1002 0x683F1002
**/

#endif /* _ATI_H_ */
