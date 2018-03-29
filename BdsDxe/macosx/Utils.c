
/* $Id: Utils.c $ */

/** @file
 * Utils.c - VirtualBox Console control emulation
 */

#include <macosx.h>
#include <Library/plist.h>

#include "cpu.h"

CHAR8                           *OSVersion;
EFI_GUID                        gPlatformUuid;
EFI_GUID                        gSystemID;
SETTINGS_DATA                   gSettings;
VOID                            *gConfigPlist;

CHAR8 *DefaultMemEntry = "N/A";
CHAR8 *DefaultSerial = "CT288GT9VT6";
CHAR8 *BiosVendor = "Apple Inc.";
CHAR8 *AppleManufacturer = "Apple Computer, Inc.";
EFI_UNICODE_COLLATION_PROTOCOL *gUnicodeCollation = NULL;

CHAR8 *AppleFirmwareVersion[] = {
  "MB11.88Z.0061.B03.0610121324",   // MB11
  "MB21.88Z.00A5.B07.0706270922",   // MB21
  "MB31.88Z.008E.B02.0803051832",   // MB31
  "MB41.88Z.00C1.B00.0802091535",   // MB41
  "MB51.88Z.007D.B03.0904271443",   // MB51
  "MB52.88Z.0088.B05.0904162222",   // MB52
  "MB61.88Z.00C8.B02.1003151501",   // MB61
  "MB71.88Z.0039.B14.1702180039",   // MB71
  "MB81.88Z.0164.B25.1702161608",   // MB81
  "MB91.88Z.0154.B17.1702161815",   // MB91
  "MBP11.88Z.0055.B08.0610121325",  // MBP11
  "MBP12.88Z.0061.B03.0610121334",  // MBP12
  "MBP21.88Z.00A5.B08.0708131242",  // MBP21
  "MBP22.88Z.00A5.B07.0708131242",  // MBP22
  "MBP31.88Z.0070.B07.0803051658",  // MBP31
  "MBP41.88Z.00C1.B03.0802271651",  // MBP41
  "MBP51.88Z.007E.B06.1202061253",  // MBP51
  "MBP52.88Z.008E.B05.0905042202",  // MBP52
  "MBP53.88Z.00AC.B03.0906151647",  // MBP53
  "MBP53.88Z.00AC.B03.0906151647",  // MBP54
  "MBP55.88Z.00AC.B03.0906151708",  // MBP55
  "MBP61.88Z.0057.B16.1702172057",  // MBP61
  "MBP61.88Z.0057.B16.1702172057",  // MBP62
  "MBP71.88Z.0039.B13.1702172144",  // MBP71
  "MBP81.88Z.0047.B32.1702180130",  // MBP81
  "MBP81.88Z.0047.B32.1702180130",  // MBP82
  "MBP81.88Z.0047.B32.1702180130",  // MBP83
  "MBP91.88Z.00D3.B15.1702171521",  // MBP91
  "MBP91.88Z.00D3.B15.1702171521",  // MBP92
  "MBP101.88Z.00EE.B12.1702171357", // MBP101
  "MBP102.88Z.0106.B12.1702171357", // MBP102
  "MBP111.88Z.0138.B25.1702171721", // MBP111
  "MBP112.88Z.0138.B25.1702171721", // MBP112
  "MBP112.88Z.0138.B25.1702171721", // MBP113
  "MBP114.88Z.0172.B16.1702161608", // MBP114
  "MBP114.88Z.0172.B16.1702161608", // MBP115
  "MBP121.88Z.0167.B24.1702161608", // MBP121
  "MBP131.88Z.0205.B15.1702161827", // MBP131
  "MBP132.88Z.0226.B11.1702161827", // MBP132
  "MBP133.88Z.0226.B11.1702161827", // MBP133
  "MBA11.88Z.00BB.B03.0803171226",  // MBA11
  "MBA21.88Z.0075.B05.1003051506",  // MBA21
  "MBA31.88Z.0061.B0D.1702172358",  // MBA31
  "MBA31.88Z.0061.B0D.1702172358",  // MBA32
  "MBA41.88Z.0077.B1B.1702201326",  // MBA41
  "MBA41.88Z.0077.B1B.1702201326",  // MBA42
  "MBA51.88Z.00EF.B0C.1702171357",  // MBA51
  "MBA51.88Z.00EF.B0C.1702171357",  // MBA52
  "MBA61.88Z.0099.B33.1702181657",  // MBA61
  "MBA61.88Z.0099.B33.1702181657",  // MBA62
  "MBA71.88Z.0166.B19.1702161608",  // MBA71
  "MBA71.88Z.0166.B19.1702161608",  // MBA72
  "MM11.88Z.0055.B08.0610121326",   // MM11
  "MM21.88Z.009A.B00.0706281359",   // MM21
  "MM31.88Z.0081.B06.0904271717",   // MM31
  "MM41.88Z.0042.B08.1702172008",   // MM41
  "MM51.88Z.0077.B1B.1702201326",   // MM51
  "MM51.88Z.0077.B1B.1702201326",   // MM52
  "MM51.88Z.0077.B1B.1702201326",   // MM53
  "MM61.88Z.0106.B12.1702171720",   // MM61
  "MM61.88Z.0106.B12.1702171720",   // MM62
  "MM71.88Z.0220.B14.1702161608",   // MM71
  "IM41.88Z.0055.B08.0609061538",   // IM41
  "IM42.88Z.0071.B03.0610121320",   // IM42
  "IM51.88Z.0090.B09.0706270921",   // IM51
  "IM52.88Z.0090.B09.0706270913",   // IM52
  "IM61.88Z.0093.B07.0804281538",   // IM61
  "IM71.88Z.007A.B03.0803051705",   // IM71
  "IM81.88Z.00C1.B00.0802091538",   // IM81
  "IM91.88Z.008D.B08.0904271717",   // IM91
  "IM101.88Z.00CC.B00.0909031926",  // IM101
  "IM111.88Z.0034.B04.1509231906",  // IM111
  "IM112.88Z.0057.B08.1702180225",  // IM112
  "IM112.88Z.0057.B08.1702180225",  // IM113
  "IM121.88Z.0047.B29.1702171916",  // IM121
  "IM121.88Z.0047.B29.1702171916",  // IM122
  "IM131.88Z.010A.B11.1702171357",  // IM131
  "IM131.88Z.010A.B11.1702171357",  // IM132
  "IM131.88Z.010A.B11.1702171357",  // IM133
  "IM141.88Z.0118.B20.1702171523",  // IM141
  "IM142.88Z.0118.B20.1702171523",  // IM142
  "IM143.88Z.0118.B20.1702171523",  // IM143
  "IM144.88Z.0179.B21.1702171856",  // IM144
  "IM151.88Z.0207.B16.1702171108",  // IM151
  "IM161.88Z.0207.B11.1702161134",  // IM161
  "IM162.88Z.0207.B11.1702161134",  // IM162
  "IM171.88Z.0105.B20.1702221516",  // IM171
  "MP11.88Z.005C.B08.0707021221",   // MP11
  "MP21.88Z.007F.B06.0707021348",   // MP21
  "MP31.88Z.006C.B05.0802291410",   // MP31
  "MP41.88Z.0081.B07.0910130729",   // MP41
  "MP51.88Z.007F.B03.1010071432",   // MP51
  "MP61.88Z.0116.B25.1702171857",   // MP61
  "XS11.88Z.0080.B01.0706271533",   // XS11
  "XS21.88Z.006C.B06.0804011317",   // XS21
  "XS31.88Z.0081.B06.0908061300",   // XS31
};

CHAR8 *AppleBoardID[] = {
  "Mac-F4208CC8",          // MB11    Intel Core Duo T2500 @ 2.00 GHz 
  "Mac-F4208CA9",          // MB21    Intel Core 2 Duo T7200 @ 2.00 GHz 
  "Mac-F22788C8",          // MB31    Intel Core 2 Duo T7500 @ 2.20 GHz 
  "Mac-F22788A9",          // MB41    Intel Core 2 Duo T8300 @ 2.39 GHz 
  "Mac-F42D89C8",          // MB51    Intel Core 2 Duo P7350 @ 2.00 GHz 
  "Mac-F22788AA",          // MB52    Intel Core 2 Duo P7450 @ 2.13 GHz 
  "Mac-F22C8AC8",          // MB61    Intel Core 2 Duo P7550 @ 2.26 GHz 
  "Mac-F22C89C8",          // MB71    Intel Core 2 Duo P8600 @ 2.40 GHz 
  "Mac-BE0E8AC46FE800CC",  // MB81    Intel Core M-5Y51 @ 1.20 GHz 
  "Mac-9AE82516C7C6B903",  // MB91    Intel Core m5-6Y54 @ 1.20 GHz 
  "Mac-F425BEC8",          // MBP11   Intel Core Duo T2500 @ 2.00 GHz
  "Mac-F42DBEC8",          // MBP12   Intel Core Duo T2600 @ 2.17 GHz 
  "Mac-F42189C8",          // MBP21   Intel Core 2 Duo T7600 @ 2.33 GHz 
  "Mac-F42187C8",          // MBP22   Intel Core 2 Duo T7400 @ 2.16 GHz 
  "Mac-F4238BC8",          // MBP31   Intel Core 2 Duo T7700 @ 2.40 GHz 
  "Mac-F42C89C8",          // MBP41   Intel Core 2 Duo T8300 @ 2.40 GHz 
  "Mac-F42D86C8",          // MBP51   Intel Core 2 Duo P8600 @ 2.40 GHz 
  "Mac-F2268EC8",          // MBP52   Intel Core 2 Duo T9600 @ 2.80 GHz 
  "Mac-F22587C8",          // MBP53   Intel Core 2 Duo P8800 @ 2.66 GHz 
  "Mac-F22587A1",          // MBP54   Intel Core 2 Duo P8700 @ 2.53 GHz 
  "Mac-F2268AC8",          // MBP55   Intel Core 2 Duo P7550 @ 2.26 GHz 
  "Mac-F22589C8",          // MBP61   Intel Core i5-540M @ 2.53 GHz 
  "Mac-F22586C8",          // MBP62   Intel Core i7-620M @ 2.66 GHz 
  "Mac-F222BEC8",          // MBP71   Intel Core 2 Duo P8600 @ 2.40 GHz 
  "Mac-94245B3640C91C81",  // MBP81   Intel Core i5-2415M @ 2.30 GHz 
  "Mac-94245A3940C91C80",  // MBP82   Intel Core i7-2675QM @ 2.20 GHz 
  "Mac-942459F5819B171B",  // MBP83   Intel Core i7-2860QM @ 2.49 GHz 
  "Mac-4B7AC7E43945597E",  // MBP91   Intel Core i7-3720QM @ 2.60 GHz 
  "Mac-6F01561E16C75D06",  // MBP92   Intel Core i5-3210M @ 2.50 GHz 
  "Mac-C3EC7CD22292981F",  // MBP101  Intel Core i7-3740QM @ 2.70 GHz 
  "Mac-AFD8A9D944EA4843",  // MBP102  Intel Core i5-3230M @ 2.60 GHz 
  "Mac-189A3D4F975D5FFC",  // MBP111  Intel Core i7-4558U @ 2.80 GHz 
  "Mac-3CBD00234E554E41",  // MBP112  Intel Core i7-4750HQ @ 2.00 GHz 
  "Mac-2BD1B31983FE1663",  // MBP113  Intel Core i7-4870HQ @ 2.50 GHz 
  "Mac-06F11FD93F0323C5",  // MBP114  Intel Core i7-4770HQ @ 2.20 GHz 
  "Mac-06F11F11946D27C5",  // MBP115  Intel Core i7-4870HQ @ 2.50 GHz 
  "Mac-E43C1C25D4880AD6",  // MBP121  Intel Core i5-5257U @ 2.70 GHz 
  "Mac-473D31EABEB93F9B",  // MBP131  Intel Core i5-6360U @ 2.00 GHz 
  "Mac-66E35819EE2D0D05",  // MBP132  Intel Core i5-6287U @ 3.10 GHz 
  "Mac-A5C67F76ED83108C",  // MBP133  Intel Core i7-6920HQ @ 2.90 GHz 
  "Mac-F42C8CC8",          // MBA11   Intel Core 2 Duo P7500 @ 1.60 GHz 
  "Mac-F42D88C8",          // MBA21   Intel Core 2 Duo L9600 @ 2.13 GHz 
  "Mac-942452F5819B1C1B",  // MBA31   Intel Core 2 Duo U9400 @ 1.40 GHz
  "Mac-942C5DF58193131B",  // MBA32   Intel Core 2 Duo L9600 @ 2.13 GHz 
  "Mac-C08A6BB70A942AC2",  // MBA41   Intel Core i7-2677M @ 1.80 GHz 
  "Mac-742912EFDBEE19B3",  // MBA42   Intel Core i5-2557M @ 1.70 GHz 
  "Mac-66F35F19FE2A0D05",  // MBA51   Intel Core i7-3667U @ 2.00 GHz 
  "Mac-2E6FAB96566FE58C",  // MBA52   Intel Core i5-3427U @ 1.80 GHz 
  "Mac-35C1E88140C3E6CF",  // MBA61   Intel Core i7-4650U @ 1.70 GHz 
  "Mac-7DF21CB3ED6977E5",  // MBA62   Intel Core i5-4250U @ 1.30 GHz 
  "Mac-9F18E312C5C2BF0B",  // MBA71   Intel Core i5-5250U @ 1.60 GHz 
  "Mac-937CB26E2E02BB01",  // MBA72   Intel Core i7-5650U @ 2.20 GHz 
  "Mac-F4208EC8",          // MM11    Intel Core 2 Duo T7200 @ 2.00 GHz 
  "Mac-F4208EAA",          // MM21    Intel Core 2 Duo T5600 @ 1.83 GHz
  "Mac-F22C86C8",          // MM31    Intel Core 2 Duo P7550 @ 2.26 GHz 
  "Mac-F2208EC8",          // MM41    Intel Core 2 Duo P8600 @ 2.40 GHz 
  "Mac-8ED6AF5B48C039E1",  // MM51    Intel Core i5-2415M @ 2.30 GHz 
  "Mac-4BC72D62AD45599E",  // MM52    Intel Core i7-2620M @ 2.70 GHz 
  "Mac-7BA5B2794B2CDB12",  // MM53    Intel Core i7-2635QM @ 2.00 GHz 
  "Mac-031AEE4D24BFF0B1",  // MM61    Intel Core i5-3210M @ 2.50 GHz 
  "Mac-F65AE981FFA204ED",  // MM62    Intel Core i7-3615QM @ 2.30 GHz 
  "Mac-35C5E08120C7EEAF",  // MM71    Intel Core i5-4278U @ 2.60 GHz 
  "Mac-F42787C8",          // IM41    Intel Core 2 Duo T7200 @ 2.00 GHz 
  "Mac-F4218EC8",          // IM42    Intel Core 2 Duo T5600 @ 1.83 GHz 
  "Mac-F4228EC8",          // IM51    Intel Core 2 Duo T7400 @ 2.16 GHz 
  "Mac-F4218EC8",          // IM52    Intel Core 2 Duo T5600 @ 1.83 GHz 
  "Mac-F4218FC8",          // IM61    Intel Core 2 Duo T7400 @ 2.16 GHz 
  "Mac-F42386C8",          // IM71    Intel Core 2 Extreme X7900 @ 2.80 GHz 
  "Mac-F227BEC8",          // IM81    Intel Core 2 Duo E8435 @ 3.06 GHz 
  "Mac-F2218FA9",          // IM91    Intel Core 2 Duo E8135 @ 2.66 GHz 
  "Mac-F2268CC8",          // IM101   Intel Core 2 Duo E7600 @ 3.06 GHz 
  "Mac-F2268DAE",          // IM111   Intel Core i7-860 @ 2.80 GHz 
  "Mac-F2238AC8",          // IM112   Intel Core i3-540 @ 3.06 GHz 
  "Mac-F2238BAE",          // IM113   Intel Core i5-760 @ 2.80 GHz 
  "Mac-942B5BF58194151B",  // IM121   Intel Core i7-2600S @ 2.80 GHz 
  "Mac-942B59F58194171B",  // IM122   Intel Core i5-2500S @ 2.70 GHz 
  "Mac-00BE6ED71E35EB86",  // IM131   Intel Core i7-3770S @ 3.10 GHz 
  "Mac-FC02E91DDD3FA6A4",  // IM132   Intel Core i5-3470 @ 3.20 GHz 
  "Mac-7DF2A3B5E5D671ED",  // IM133   Intel Core i3-3225 @ 3.30 GHz 
  "Mac-031B6874CF7F642A",  // IM141   Intel Core i5-4570R @ 2.70 GHz 
  "Mac-27ADBB7B4CEE8E61",  // IM142   Intel Core i5-4570 @ 3.20 GHz 
  "Mac-77EB7D7DAF985301",  // IM143   Intel Core i5-4570S @ 2.90 GHz 
  "Mac-81E3E92DD6088272",  // IM144   Intel Core i5-4260U @ 1.40 GHz 
  "Mac-42FD25EABCABB274",  // IM151   Intel Core i5-4690 @ 3.50 GHz  
  "Mac-A369DDC4E67F1C45",  // IM161   Intel Core i5-5250U @ 1.60 GHz 
  "Mac-FFE5EF870D7BA81A",  // IM162   Intel Core i5-5575R @ 2.80 GHz 
  "Mac-DB15BD556843C820",  // IM171   Intel Core i7-6700K @ 4.00 GHz 
  "Mac-F4208DC8",          // MP11    Intel Xeon X5355 @ 2.66 GHz x2
  "Mac-F4208DA9",          // MP21    Intel Xeon X5365 @ 2.99 GHz x2
  "Mac-F42C88C8",          // MP31    Intel Xeon E5462 @ 2.80 GHz x2
  "Mac-F221BEC8",          // MP41    Intel Xeon X5670 @ 2.93 GHz x2
  "Mac-F221BEC8",          // MP51    Intel Xeon X5675 @ 3.06 GHz x2
  "Mac-F60DEB81FF30ACF6",  // MP61    Intel Xeon E5-1650 v2 @ 3.50 GHz 
  "Mac-F4208AC8",          // XS11    Intel Xeon E5345 @ 2.33 GHz x2
  "Mac-F42289C8",          // XS21    Intel Xeon E5472 @ 3.00 GHz x2
  "Mac-F223BEC8",          // XS31    Intel Xeon E5520 @ 2.26 GHz
};

CHAR8 *AppleReleaseDate[] = {
  "10/12/06",    // MB11
  "06/27/07",    // MB21
  "03/05/08",    // MB31
  "02/09/08",    // MB41
  "04/27/09",    // MB51
  "04/16/09",    // MB52
  "03/15/10",    // MB61
  "02/18/17",    // MB71
  "02/16/2017",  // MB81
  "02/16/2017",  // MB91
  "10/12/06",    // MBP11
  "10/12/06",    // MBP12
  "08/13/07",    // MBP21
  "08/13/07",    // MBP22
  "03/05/08",    // MBP31
  "02/27/08",    // MBP41
  "02/06/12",    // MBP51
  "05/04/09",    // MBP52
  "06/15/09",    // MBP53
  "06/15/09",    // MBP54
  "06/15/09",    // MBP55
  "02/17/17",    // MBP61
  "02/17/17",    // MBP62
  "02/17/17",    // MBP71
  "02/18/17",    // MBP81
  "02/18/17",    // MBP82
  "02/18/17",    // MBP83
  "02/17/2017",  // MBP91
  "02/17/2017",  // MBP92
  "02/17/2017",  // MBP101
  "02/17/2017",  // MBP102
  "02/17/2017",  // MBP111
  "02/17/2017",  // MBP112
  "02/17/2017",  // MBP113
  "02/16/2017",  // MBP114
  "02/16/2017",  // MBP115
  "02/16/2017",  // MBP121
  "02/16/2017",  // MBP131
  "02/16/2017",  // MBP132
  "02/16/2017",  // MBP133
  "03/17/08",    // MBA11
  "03/05/10",    // MBA21
  "02/17/17",    // MBA31
  "02/17/17",    // MBA32
  "02/20/2017",  // MBA41
  "02/20/2017",  // MBA42
  "02/17/2017",  // MBA51
  "02/17/2017",  // MBA52
  "02/18/2017",  // MBA61
  "02/18/2017",  // MBA62
  "02/16/2017",  // MBA71
  "02/16/2017",  // MBA72
  "10/12/06",    // MM11
  "06/28/07",    // MM21
  "04/27/09",    // MM31
  "02/17/17",    // MM41
  "02/20/2017",  // MM51
  "02/20/2017",  // MM52
  "02/20/2017",  // MM53
  "02/17/2017",  // MM61
  "02/17/2017",  // MM62
  "02/16/2017",  // MM71
  "09/06/06",    // IM41
  "10/12/06",    // IM42
  "06/27/07",    // IM51
  "06/27/07",    // IM52
  "04/28/08",    // IM61
  "03/05/08",    // IM71
  "02/09/08",    // IM81
  "04/27/09",    // IM91
  "09/03/09",    // IM101
  "09/23/15",    // IM111
  "02/18/17",    // IM112
  "02/18/17",    // IM113
  "02/17/17",    // IM121
  "02/17/17",    // IM122
  "02/17/2017",  // IM131
  "02/17/2017",  // IM132
  "02/17/2017",  // IM133
  "02/17/2017",  // IM141
  "02/17/2017",  // IM142
  "02/17/2017",  // IM143
  "02/17/2017",  // IM144
  "02/17/2017",  // IM151
  "02/16/2017",  // IM161
  "02/16/2017",  // IM162
  "02/22/2017",  // IM171
  "07/02/07",    // MP11
  "07/02/07",    // MP21
  "02/29/08",    // MP31
  "10/13/09",    // MP41
  "10/07/10",    // MP51
  "02/17/2017",  // MP61
  "06/27/07",    // XS11
  "04/01/08",    // XS21
  "08/06/09",    // XS31
};

CHAR8 *AppleProductName[] = {
  "MacBook1,1",
  "MacBook2,1",
  "MacBook3,1",
  "MacBook4,1",
  "MacBook5,1",
  "MacBook5,2",
  "MacBook6,1",
  "MacBook7,1",
  "MacBook8,1",
  "MacBook9,1",
  "MacBookPro1,1",
  "MacBookPro1,2",
  "MacBookPro2,1",
  "MacBookPro2,2",
  "MacBookPro3,1",
  "MacBookPro4,1",
  "MacBookPro5,1",
  "MacBookPro5,2",
  "MacBookPro5,3",
  "MacBookPro5,4",
  "MacBookPro5,5",
  "MacBookPro6,1",
  "MacBookPro6,2",
  "MacBookPro7,1",
  "MacBookPro8,1",
  "MacBookPro8,2",
  "MacBookPro8,3",
  "MacBookPro9,1",
  "MacBookPro9,2",
  "MacBookPro10,1",
  "MacBookPro10,2",
  "MacBookPro11,1",
  "MacBookPro11,2",
  "MacBookPro11,3",
  "MacBookPro11,4",
  "MacBookPro11,5",
  "MacBookPro12,1",
  "MacBookPro13,1",
  "MacBookPro13,2",
  "MacBookPro13,3",
  "MacBookAir1,1",
  "MacBookAir2,1",
  "MacBookAir3,1",
  "MacBookAir3,2",
  "MacBookAir4,1",
  "MacBookAir4,2",
  "MacBookAir5,1",
  "MacBookAir5,2",
  "MacBookAir6,1",
  "MacBookAir6,2",
  "MacBookAir7,1",
  "MacBookAir7,2",
  "Macmini1,1",
  "Macmini2,1",
  "Macmini3,1",
  "Macmini4,1",
  "Macmini5,1",
  "Macmini5,2",
  "Macmini5,3",
  "Macmini6,1",
  "Macmini6,2",
  "Macmini7,1",
  "iMac4,1",
  "iMac4,2",
  "iMac5,1",
  "iMac5,2",
  "iMac6,1",
  "iMac7,1",
  "iMac8,1",
  "iMac9,1",
  "iMac10,1",
  "iMac11,1",
  "iMac11,2",
  "iMac11,3",
  "iMac12,1",
  "iMac12,2",
  "iMac13,1",
  "iMac13,2",
  "iMac13,3",
  "iMac14,1",
  "iMac14,2",
  "iMac14,3",
  "iMac14,4",
  "iMac15,1",
  "iMac16,1",
  "iMac16,2",
  "iMac17,1",
  "MacPro1,1",
  "MacPro2,1",
  "MacPro3,1",
  "MacPro4,1",
  "MacPro5,1",
  "MacPro6,1",
  "Xserve1,1",
  "Xserve2,1",
  "Xserve3,1",
};

CHAR8 *AppleFamilies[] = {
  "MacBook",       // MB11
  "MacBook",       // MB21
  "MacBook",       // MB31
  "MacBook",       // MB41
  "MacBook",       // MB51
  "MacBook",       // MB52
  "MacBook",       // MB61
  "MacBook",       // MB71
  "MacBook",       // MB81
  "MacBook",       // MB91
  "MacBook Pro",   // MBP11
  "MacBook Pro",   // MBP21
  "MacBook Pro",   // MBP21
  "MacBook Pro",   // MBP22
  "MacBook Pro",   // MBP31
  "MacBook Pro",   // MBP41
  "MacBook Pro",   // MBP51
  "MacBook Pro",   // MBP52
  "MacBook Pro",   // MBP53
  "MacBook Pro",   // MBP54
  "MacBook Pro",   // MBP55
  "MacBook Pro",   // MBP61
  "MacBook Pro",   // MBP62
  "MacBook Pro",   // MBP71
  "MacBook Pro",   // MBP81
  "MacBook Pro",   // MBP82
  "MacBook Pro",   // MBP83
  "MacBook Pro",   // MBP91
  "MacBook Pro",   // MBP92
  "MacBook Pro",   // MBP101
  "MacBook Pro",   // MBP102
  "MacBook Pro",   // MBP111
  "MacBook Pro",   // MBP112
  "MacBook Pro",   // MBP113
  "MacBook Pro",   // MBP114
  "MacBook Pro",   // MBP115
  "MacBook Pro",   // MBP121
  "MacBook Pro",   // MBP131
  "MacBook Pro",   // MBP132
  "MacBook Pro",   // MBP133
  "MacBook Air",   // MBA11
  "MacBook Air",   // MBA21
  "MacBook Air",   // MBA31
  "MacBook Air",   // MBA32
  "MacBook Air",   // MBA41
  "MacBook Air",   // MBA42
  "MacBook Air",   // MBA51
  "MacBook Air",   // MBA52
  "MacBook Air",   // MBA61
  "MacBook Air",   // MBA62
  "MacBook Air",   // MBA71
  "MacBook Air",   // MBA72
  "Mac mini",      // MM11
  "Mac mini",      // MM21
  "Mac mini",      // MM31
  "Mac mini",      // MM41
  "Mac mini",      // MM51
  "Mac mini",      // MM52
  "Mac mini",      // MM53
  "Mac mini",      // MM61
  "Mac mini",      // MM62
  "Mac mini",      // MM71
  "iMac",          // IM41
  "iMac",          // IM42
  "iMac",          // IM51
  "iMac",          // IM52
  "iMac",          // IM61
  "iMac",          // IM71
  "iMac",          // IM81
  "iMac",          // IM91
  "iMac",          // IM101
  "iMac",          // IM111
  "iMac",          // IM112
  "iMac",          // IM113
  "iMac",          // IM121
  "iMac",          // IM122
  "iMac",          // IM131
  "iMac",          // IM132
  "iMac",          // IM133
  "iMac",          // IM141
  "iMac",          // IM142
  "iMac",          // IM143
  "iMac",          // IM144
  "iMac",          // IM151
  "iMac",          // IM161
  "iMac",          // IM162
  "iMac17,1",      // IM171
  "MacPro",        // MP11
  "MacPro",        // MP21
  "MacPro",        // MP31
  "MacPro",        // MP41
  "MacPro",        // MP51
  "MacPro",        // MP61
  "Xserve",        // XS11
  "Xserve",        // XS21
  "Xserve",        // XS31
};

CHAR8 *AppleSystemVersion[] = {
  "1.1",  // MB11
  "1.2",  // MB21
  "1.3",  // MB31
  "1.3",  // MB41
  "1.3",  // MB51
  "1.3",  // MB52
  "1.0",  // MB61
  "1.0",  // MB71
  "1.0",  // MB81
  "1.0",  // MB91
  "1.0",  // MBP11
  "1.0",  // MBP12
  "1.0",  // MBP21
  "1.0",  // MBP22
  "1.0",  // MBP31
  "1.0",  // MBP41
  "1.0",  // MBP51
  "1.0",  // MBP52
  "1.0",  // MBP53
  "1.0",  // MBP54
  "1.0",  // MBP55
  "1.0",  // MBP61
  "1.0",  // MBP62
  "1.0",  // MBP71
  "1.0",  // MBP81
  "1.0",  // MBP82
  "1.0",  // MBP83
  "1.0",  // MBP91
  "1.0",  // MBP92
  "1.0",  // MBP101
  "1.0",  // MBP102
  "1.0",  // MBP111
  "1.0",  // MBP112
  "1.0",  // MBP113
  "1.0",  // MBP114
  "1.0",  // MBP115
  "1.0",  // MBP121
  "1.0",  // MBP131
  "1.0",  // MBP132
  "1.0",  // MBP133
  "1.0",  // MBA11
  "1.0",  // MBA21
  "1.0",  // MBA31
  "1.0",  // MBA32
  "1.0",  // MBA41
  "1.0",  // MBA42
  "1.0",  // MBA51
  "1.0",  // MBA52 
  "1.0",  // MBA61
  "1.0",  // MBA62
  "1.0",  // MBA71
  "1.0",  // MBA72
  "1.0",  // MM11
  "1.1",  // MM21
  "1.0",  // MM31
  "1.0",  // MM41
  "1.0",  // MM51
  "1.0",  // MM52
  "1.0",  // MM53
  "1.0",  // MM61
  "1.0",  // MM62
  "1.0",  // MM71
  "1.0",  // IM41
  "1.0",  // IM42
  "1.0",  // IM51
  "1.0",  // IM52
  "1.0",  // IM61
  "1.0",  // IM71
  "1.3",  // IM81
  "1.0",  // IM91
  "1.0",  // IM101
  "1.0",  // IM111
  "1.2",  // IM112
  "1.0",  // IM113
  "1.9",  // IM121
  "1.9",  // IM122
  "1.0",  // IM131
  "1.0",  // IM132
  "1.0",  // IM133
  "1.0",  // IM141
  "1.0",  // IM142
  "1.0",  // IM143
  "1.0",  // IM144
  "1.0",  // IM151
  "1.0",  // IM161
  "1.0",  // IM162
  "1.0",  // IM171
  "1.0",  // MP11
  "1.0",  // MP21
  "1.3",  // MP31
  "1.4",  // MP41
  "1.2",  // MP51
  "1.0",  // MP61
  "1.0",  // XS11
  "1.0",  // XS21
  "1.0",  // XS31
};

CHAR8 *AppleSerialNumber[] = {
  "W80A041AU9B",  // MB11
  "W88A041AWGP",  // MB21
  "W8803HACY51",  // MB31
  "W88A041A0P0",  // MB41
  "W8944T1S1AQ",  // MB51
  "W88AAAAA9GU",  // MB52
  "451131JCGAY",  // MB61
  "451211MEF5X",  // MB71
  "C02RCE58GCN3", // MB81
  "C02RM408HDNK", // MB91
  "W884857JVJ1",  // MBP11
  "W8629HACTHY",  // MBP12
  "W88130WUW0H",	// MBP21
  "W8827B4CW0L",  // MBP22
  "W8841OHZX91",  // MBP31
  "W88484F2YP4",  // MBP41
  "W88439FE1G0",  // MBP51
  "W8908HAC2QP",  // MBP52
  "W8035TG97XK",  // MBP53
  "W8948HAC7XJ",  // MBP54
  "W8035TG966D",  // MBP55
  "C02G5834DC79", // MBP61
  "CK132A91AGW",  // MBP62
  "CK145C7NATM",  // MBP71
  "W89F9196DH2G", // MBP81
  "C02HL0FGDF8X", // MBP82
  "W88F9CDEDF93", // MBP83
  "C02LW984F1G4", // MBP91
  "C02HA041DTY3", // MBP92
  "C02K2HACDKQ1", // MBP101
  "C02K2HACG4N7", // MBP102
  "C02LSHACFH00", // MBP111
  "C02LSHACG86R", // MBP112
  "C02LSHACFR1M", // MBP113
  "C02SNHACG8WN", // MBP114
  "C02LSHACG85Y", // MBP115
  "C02Q51OSH1DP", // MBP121
  "C02SLHACGVC1", // MBP131
  "C02SLHACGYFH", // MBP132
  "C02SLHACGTFN", // MBP133
  "W864947A18X",  // MBA11
  "W86494769A7",  // MBA21
  "C02FLHACD0QX", // MBA31
  "C02DRHACDDR3", // MBA32
  "C02KGHACDRV9", // MBA41
  "C02GLHACDJWT", // MBA42
  "C02J6HACDRV6", // MBA51
  "C02HA041DRVC", // MBA52
  "C02KTHACF5NT", // MBA61
  "C02HACKUF5V7", // MBA62
  "C02PCLGFH569", // MBA71
  "C02Q1HACG940", // MBA72
  "W8702N1JU35",  // MM11
  "W8705W9LYL2",  // MM21
  "W8905BBE19X",  // MM31
  "C02FHBBEDD6H", // MM41
  "C07GA041DJD0", // MM51
  "C07HVHACDJD1", // MM52
  "C07GWHACDKDJ", // MM53
  "C07JNHACDY3H", // MM61
  "C07JD041DWYN", // MM62
  "C02NN7NHG1J0", // MM71
  "W8608HACU2P",  // IM41
  "W8627HACV2H",  // IM42
  "CK637HACX1A",  // IM51
  "W8716HACWH5",  // IM52
  "W8652HACVGN",  // IM61
  "W8803HACY51",  // IM71
  "W8755HAC2E2",  // IM81
  "W89A00A36MJ",  // IM91
  "W80AA98A5PE",  // IM101
  "G8942B1V5PJ",  // IM111
  "W8034342DB7",  // IM112
  "QP0312PBDNR",  // IM113
  "W80CF65ADHJF", // IM121
  "W88GG136DHJQ", // IM122
  "C02JA041DNCT", // IM131
  "C02JB041DNCW", // IM132
  "C02KVHACFFYW", // IM133
  "D25LHACKF8J2", // IM141
  "D25LHACKF8JC", // IM142
  "D25LHACKF8J3", // IM143
  "D25LHACKFY0T", // IM144
  "C02Q6HACFY10", // IM151
  "C02QQHACGF1J",	// IM161
  "C02PNHACGG7G",	// IM162
  "C02QFHACGG7L", // IM171
  "W88A77AXUPZ",  // MP11
  "W8930518UPZ",  // MP21
  "W88A77AA5J4",  // MP31
  "CT93051DK9Y",  // MP41
  "C07J77F7F4MC", // MP51  - C07J50F7F4MC  CK04000AHFC  "CG154TB9WU3"
  "F5KLA770F9VM", // MP61
  "CK703E1EV2Q",  // XS11
  "CK830DLQX8S",  // XS21
  "CK933YJ16HS",  // XS31
};

CHAR8 *AppleChassisAsset[] = {
  "MacBook-White",      // MB11
  "MacBook-White",      // MB21
  "MacBook-White",      // MB31
  "MacBook-Black",      // MB41
  "MacBook-Black",      // MB51
  "MacBook-Black",      // MB52
  "MacBook-White",      // MB61
  "MacBook-White",      // MB71
  "MacBook-Aluminum",   // MB81
  "MacBook-Aluminum",   // MB91
  "MacBook-Aluminum",   // MBP11
  "MacBook-Aluminum",   // MBP12
  "MacBook-Aluminum",   // MBP21
  "MacBook-Aluminum",   // MBP22
  "MacBook-Aluminum",   // MBP31
  "MacBook-Aluminum",   // MBP41
  "MacBook-Aluminum",   // MBP51
  "MacBook-Aluminum",   // MBP52
  "MacBook-Aluminum",   // MBP53
  "MacBook-Aluminum",   // MBP54
  "MacBook-Aluminum",   // MBP55
  "MacBook-Aluminum",   // MBP61
  "MacBook-Aluminum",   // MBP62
  "MacBook-Aluminum",   // MBP71
  "MacBook-Aluminum",   // MBP81
  "MacBook-Aluminum",   // MBP82
  "MacBook-Aluminum",   // MBP83
  "MacBook-Aluminum",   // MBP91
  "MacBook-Aluminum",   // MBP92
  "MacBook-Aluminum",   // MBP101
  "MacBook-Aluminum",   // MBP102
  "MacBook-Aluminum",   // MBP111
  "MacBook-Aluminum",   // MBP112
  "MacBook-Aluminum",   // MBP113
  "MacBook-Aluminum",   // MBP114
  "MacBook-Aluminum",   // MBP115
  "MacBook-Aluminum",   // MBP121
  "MacBook-Aluminum",   // MBP131
  "MacBook-Aluminum",   // MBP132
  "MacBook-Aluminum",   // MBP133
  "Air-Enclosure",      // MBA11
  "Air-Enclosure",      // MBA21
  "Air-Enclosure",      // MBA31
  "Air-Enclosure",      // MBA32
  "Air-Enclosure",      // MBA41
  "Air-Enclosure",      // MBA42
  "Air-Enclosure",      // MBA51
  "Air-Enclosure",      // MBA52 
  "Air-Enclosure",      // MBA61
  "Air-Enclosure",      // MBA62
  "Air-Enclosure",      // MBA71
  "Air-Enclosure",      // MBA72
  "Mini-Aluminum",      // MM11
  "Mini-Aluminum",      // MM21
  "Mini-Aluminum",      // MM31
  "Mini-Aluminum",      // MM41
  "Mini-Aluminum",      // MM51
  "Mini-Aluminum",      // MM52
  "Mini-Aluminum",      // MM53
  "Mini-Aluminum",      // MM61
  "Mini-Aluminum",      // MM62
  "Mini-Aluminum",      // MM71
  "iMac",				// IM41
  "iMac",				// IM42
  "iMac",				// IM51
  "iMac",				// IM52
  "iMac",				// IM61
  "iMac-Aluminum",      // IM71
  "iMac-Aluminum",      // IM81
  "iMac-Aluminum",      // IM91
  "iMac-Aluminum",      // IM101
  "iMac-Aluminum",      // IM111
  "iMac-Aluminum",      // IM112
  "iMac-Aluminum",      // IM113
  "iMac-Aluminum",      // IM121
  "iMac-Aluminum",      // IM122
  "iMac-Aluminum",      // IM131
  "iMac-Aluminum",      // IM132
  "iMac-Aluminum",      // IM133
  "iMac-Aluminum",      // IM141
  "iMac-Aluminum",      // IM142
  "iMac-Aluminum",      // IM143
  "iMac-Aluminum",      // IM144
  "iMac-Aluminum",      // IM151
  "iMac-Aluminum",      // IM161
  "iMac-Aluminum",      // IM162
  "iMac-Aluminum",      // IM171
  "Pro-Enclosure",      // MP11
  "Pro-Enclosure",      // MP21
  "Pro-Enclosure",      // MP31
  "Pro-Enclosure",      // MP41
  "Pro-Enclosure",      // MP51
  "Pro-Enclosure",      // MP61
  "Xserve",             // XS11
  "Xserve",             // XS21
  "Xserve",             // XS31
};

CHAR8 *AppleBoardSN = "C02032101R5DC771H";
CHAR8 *AppleBoardLocation = "Part Component";

UINT16
GetAdvancedCpuType (
  VOID
)
{
  if (gCPUStructure.Vendor == CPU_VENDOR_INTEL) {
    switch (gCPUStructure.Family) {

    case 0x06:{
        switch (gCPUStructure.Model) {
        case CPU_MODEL_DOTHAN: // Dothan
        case CPU_MODEL_YONAH:  // Yonah
          return 0x201;

        case CPU_MODEL_NEHALEM_EX: //Xeon 5300
          return 0x402;

        case CPU_MODEL_NEHALEM:  // Intel Core i7 LGA1366 (45nm)
          return 0x701; // Core i7

        case CPU_MODEL_FIELDS: // Lynnfield, Clarksfield, Jasper
          if (AsciiStrStr (gCPUStructure.BrandString, "i5") != NULL) {
            return 0x601; // Core i5
          }

          return 0x701; // Core i7

        case CPU_MODEL_DALES:  // Intel Core i5, i7 LGA1156 (45nm) (Havendale, Auburndale)
          if (AsciiStrStr (gCPUStructure.BrandString, "Core(TM) i3") != NULL) {
            return 0x901; // Core i3 //why not 902? Ask Apple
          }

          if (AsciiStrStr (gCPUStructure.BrandString, "i5") != NULL ||
              (gCPUStructure.Cores <= 2)) {
            return 0x602; // Core i5
          }

          return 0x702; // Core i7

          //case CPU_MODEL_ARRANDALE:
        case CPU_MODEL_CLARKDALE:  // Intel Core i3, i5, i7 LGA1156 (32nm) (Clarkdale, Arrandale)
          if (AsciiStrStr (gCPUStructure.BrandString, "i3") != NULL) {
            return 0x901; // Core i3
          }

          if (AsciiStrStr (gCPUStructure.BrandString, "i5") != NULL ||
              (gCPUStructure.Cores <= 2)) {
            return 0x601; // Core i5 - (M540 -> 0x0602)
          }

          return 0x701; // Core i7

        case CPU_MODEL_WESTMERE: // Intel Core i7 LGA1366 (32nm) 6 Core (Gulftown, Westmere-EP, Westmere-WS)
        case CPU_MODEL_WESTMERE_EX:  // Intel Core i7 LGA1366 (45nm) 6 Core ???
          return 0x701; // Core i7

        case CPU_MODEL_SANDY_BRIDGE:
          if (AsciiStrStr (gCPUStructure.BrandString, "i3") != NULL) {
            return 0x903; // Core i3
          }

          if (AsciiStrStr (gCPUStructure.BrandString, "i5") != NULL ||
              (gCPUStructure.Cores <= 2)) {
            return 0x603; // Core i5
          }

          return 0x703;

        case CPU_MODEL_IVY_BRIDGE:
        case CPU_MODEL_IVY_BRIDGE_E5:
          if (AsciiStrStr (gCPUStructure.BrandString, "i3") != NULL) {
            return 0x903; // Core i3 - Apple doesn't use it
          }

          if (AsciiStrStr (gCPUStructure.BrandString, "i5") != NULL ||
              (gCPUStructure.Cores <= 2)) {
            return 0x604; // Core i5
          }

          return 0x704;

        case CPU_MODEL_MEROM:  // Merom
          if (gCPUStructure.Cores >= 2) {
            if (AsciiStrStr (gCPUStructure.BrandString, "Xeon") != NULL) {
              return 0x402; // Quad-Core Xeon
            }
            else {
              return 0x301; // Core 2 Duo
            }
          }
          else {
            return 0x201; // Core Solo
          };

        case CPU_MODEL_PENRYN: // Penryn
        case CPU_MODEL_ATOM: // Atom (45nm)
        default:
          if (gCPUStructure.Cores >= 4) {
            return 0x402; // Quad-Core Xeon
          }
          else if (gCPUStructure.Cores == 1) {
            return 0x201; // Core Solo
          };
          return 0x301; // Core 2 Duo
        }
      }
    }
  }
  return 0x0;
}

MACHINE_TYPES
GetDefaultModel (
  VOID
)
{
  MACHINE_TYPES DefaultType = MacPro31;

  // TODO: Add more CPU models and configure the correct machines per CPU/GFX model
  if (gSettings.Mobile) {
    switch (gCPUStructure.Model) {
    case CPU_MODEL_ATOM:
      DefaultType = MacBookAir31; //MacBookAir1,1 doesn't support _PSS for speedstep!
      break;

    case CPU_MODEL_DOTHAN:
      DefaultType = MacBook11;
      break;

    case CPU_MODEL_YONAH:
      DefaultType = MacBook11;
      break;

    case CPU_MODEL_MEROM:
      DefaultType = MacBook21;
      break;

    case CPU_MODEL_PENRYN:
      if (gGraphics.Vendor == Nvidia) {
        DefaultType = MacBookPro51;
      }
      else {
        DefaultType = MacBook41;
      }

      break;

    case CPU_MODEL_JAKETOWN:
    case CPU_MODEL_SANDY_BRIDGE:
      if (AsciiStrStr (gCPUStructure.BrandString, "i3") != NULL ||
          AsciiStrStr (gCPUStructure.BrandString, "i5") != NULL) {
        DefaultType = MacBookPro81;
        break;
      }

      DefaultType = MacBookPro83;
      break;

    case CPU_MODEL_IVY_BRIDGE:
    case CPU_MODEL_IVY_BRIDGE_E5:
      DefaultType = MacBookAir52;
      break;

    default:
      if (gGraphics.Vendor == Nvidia) {
        DefaultType = MacBookPro51;
      }
      else {
        DefaultType = MacBook52;
      }

      break;
    }
  }
  else {
    switch (gCPUStructure.Model) {
    case CPU_MODEL_CELERON:
      DefaultType = MacMini21;
      break;

    case CPU_MODEL_LINCROFT:
      DefaultType = MacMini21;
      break;

    case CPU_MODEL_ATOM:
      DefaultType = MacMini21;
      break;

    case CPU_MODEL_MEROM:
      DefaultType = iMac81;
      break;

    case CPU_MODEL_PENRYN:
      DefaultType = MacPro31; //speedstep without patching; Hapertown is also a Penryn, according to Wikipedia
      break;

    case CPU_MODEL_NEHALEM:
      DefaultType = MacPro41;
      break;

    case CPU_MODEL_NEHALEM_EX:
      DefaultType = MacPro41;
      break;

    case CPU_MODEL_FIELDS:
      DefaultType = iMac112;
      break;

    case CPU_MODEL_DALES:
      DefaultType = iMac112;
      break;

    case CPU_MODEL_CLARKDALE:
      DefaultType = iMac112;
      break;

    case CPU_MODEL_WESTMERE:
      DefaultType = MacPro51;
      break;

    case CPU_MODEL_WESTMERE_EX:
      DefaultType = MacPro51;
      break;

    case CPU_MODEL_SANDY_BRIDGE:
      if (gGraphics.Vendor == Intel) {
        DefaultType = MacMini51;
      }

      if (AsciiStrStr (gCPUStructure.BrandString, "i3") != NULL ||
          AsciiStrStr (gCPUStructure.BrandString, "i5") != NULL) {
        DefaultType = iMac112;
        break;
      }

      if (AsciiStrStr (gCPUStructure.BrandString, "i7") != NULL) {
        DefaultType = iMac121;
        break;
      }

      DefaultType = MacPro51;
      break;

    case CPU_MODEL_IVY_BRIDGE:
    case CPU_MODEL_IVY_BRIDGE_E5:
      DefaultType = iMac122;  //do not make 13,1 by default because of OS 10.8.2 doesn't know it
      break;

    case CPU_MODEL_JAKETOWN:
      DefaultType = MacPro41;
      break;

    default:
      DefaultType = MacPro31;
      break;
    }
  }

  return DefaultType;
}

//---------------------------------------------------------------------------------

VOID
Pause (
  IN CHAR16 *Message
)
{
  if (Message != NULL) {
    Print (L"%s", Message);
  }

  gBS->Stall (4000000);
}

BOOLEAN
FileExists (
  IN EFI_FILE * RootFileHandle,
  IN CHAR16 *RelativePath
)
{
  EFI_STATUS Status;
  EFI_FILE *TestFile;

  Status =
    RootFileHandle->Open (RootFileHandle, &TestFile, RelativePath,
                          EFI_FILE_MODE_READ, 0);

  if (Status == EFI_SUCCESS) {
    TestFile->Close (TestFile);
    return TRUE;
  }

  return FALSE;
}

EFI_STATUS
egLoadFile (
  IN EFI_FILE_HANDLE BaseDir,
  IN CHAR16 *FileName,
  OUT UINT8 **FileData,
  OUT UINTN *FileDataLength
)
{
  EFI_STATUS Status;
  EFI_FILE_HANDLE FileHandle;
  EFI_FILE_INFO *FileInfo;
  UINT64 ReadSize;
  UINTN BufferSize;
  UINT8 *Buffer;

  if (BaseDir == NULL) {
    return EFI_NOT_FOUND;
  }

  Status =
    BaseDir->Open (BaseDir, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  FileInfo = EfiLibFileInfo (FileHandle);

  if (FileInfo == NULL) {
    FileHandle->Close (FileHandle);
    return EFI_NOT_FOUND;
  }

  ReadSize = FileInfo->FileSize;

  FreePool (FileInfo);

  if (ReadSize == 0) {
    FileHandle->Close (FileHandle);
    *FileData = NULL;
    *FileDataLength = 0;
    return EFI_SUCCESS;
  }

  if (ReadSize > MAX_FILE_SIZE) {
    ReadSize = MAX_FILE_SIZE;
  }

  BufferSize = (UINTN) ReadSize;  // was limited to 1 GB above, so this is safe
  Buffer = (UINT8 *) AllocateAlignedPages (EFI_SIZE_TO_PAGES (BufferSize), 16);

  if (Buffer == NULL) {
    FileHandle->Close (FileHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = FileHandle->Read (FileHandle, &BufferSize, Buffer);
  FileHandle->Close (FileHandle);

  if (EFI_ERROR (Status)) {
    FreeAlignedPages (Buffer, EFI_SIZE_TO_PAGES (BufferSize));
    return Status;
  }

  *FileData = Buffer;
  *FileDataLength = BufferSize;
  return EFI_SUCCESS;
}

EFI_STATUS
egSaveFile (
  IN EFI_FILE_HANDLE BaseDir,
  IN CHAR16 *FileName,
  IN UINT8 *FileData,
  IN UINTN FileDataLength
)
{
  EFI_STATUS Status;
  EFI_FILE_HANDLE FileHandle;
  UINTN BufferSize;

  if (BaseDir == NULL) {
    return EFI_NOT_FOUND;
  }

  Status =
    BaseDir->Open (BaseDir, &FileHandle, FileName,
                   EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);

  if (!EFI_ERROR (Status)) {
    FileHandle->Delete (FileHandle);
  }

  Status =
    BaseDir->Open (BaseDir, &FileHandle, FileName,
                   EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE |
                   EFI_FILE_MODE_CREATE, 0);
  if (EFI_ERROR (Status))
    return Status;

  BufferSize = FileDataLength;
  Status = FileHandle->Write (FileHandle, &BufferSize, FileData);
  FileHandle->Close (FileHandle);

  return Status;
}

EFI_STATUS
SaveBooterLog (
  IN EFI_FILE_HANDLE BaseDir,
  IN CHAR16 *FileName
)
{
  CHAR8 *MemLogBuffer;
  UINTN MemLogLen;

  MemLogBuffer = GetMemLogBuffer ();
  MemLogLen = GetMemLogLen ();

  if (MemLogBuffer == NULL || MemLogLen == 0) {
    return EFI_NOT_FOUND;
  }

  return egSaveFile (BaseDir, FileName, (UINT8 *) MemLogBuffer, MemLogLen);
}

EFI_STATUS
InitializeUnicodeCollationProtocol (
  VOID
)
{
  EFI_STATUS Status;

  if (gUnicodeCollation != NULL) {
    return EFI_SUCCESS;
  }

  //
  // BUGBUG: Proper impelmentation is to locate all Unicode Collation Protocol
  // instances first and then select one which support English language.
  // Current implementation just pick the first instance.
  //
  Status =
    gBS->LocateProtocol (&gEfiUnicodeCollation2ProtocolGuid, NULL,
                         (VOID **) &gUnicodeCollation);
  if (EFI_ERROR (Status)) {
    Status =
      gBS->LocateProtocol (&gEfiUnicodeCollationProtocolGuid, NULL,
                           (VOID **) &gUnicodeCollation);

  }
  return Status;
}

BOOLEAN
MetaiMatch (
  IN CHAR16 *String,
  IN CHAR16 *Pattern
)
{
  if (!gUnicodeCollation) {
    // quick fix for driver loading on UEFIs without UnicodeCollation
    //return FALSE;
    return TRUE;
  }
  return gUnicodeCollation->MetaiMatch (gUnicodeCollation, String, Pattern);
}

EFI_STATUS
DirNextEntry (
  IN EFI_FILE * Directory,
  IN OUT EFI_FILE_INFO ** DirEntry,
  IN UINTN FilterMode
)
{
  EFI_STATUS Status;
  VOID *Buffer;
  UINTN LastBufferSize, BufferSize;
  INTN IterCount;

  for (;;) {

    // free pointer from last call
    if (*DirEntry != NULL) {
      FreePool (*DirEntry);
      *DirEntry = NULL;
    }

    // read next directory entry
    LastBufferSize = BufferSize = 256;
    Buffer = AllocateZeroPool (BufferSize);
    for (IterCount = 0;; IterCount++) {
      Status = Directory->Read (Directory, &BufferSize, Buffer);
      if (Status != EFI_BUFFER_TOO_SMALL || IterCount >= 4)
        break;
      if (BufferSize <= LastBufferSize) {
        BufferSize = LastBufferSize * 2;
      }
      Buffer = ReallocatePool (LastBufferSize, BufferSize, Buffer);
      LastBufferSize = BufferSize;
    }
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      break;
    }

    // check for end of listing
    if (BufferSize == 0) {  // end of directory listing
      FreePool (Buffer);
      break;
    }

    // entry is ready to be returned
    *DirEntry = (EFI_FILE_INFO *) Buffer;
    if (*DirEntry) {
      // filter results
      if (FilterMode == 1) {  // only return directories
        if (((*DirEntry)->Attribute & EFI_FILE_DIRECTORY))
          break;
      }
      else if (FilterMode == 2) { // only return files
        if (((*DirEntry)->Attribute & EFI_FILE_DIRECTORY) == 0)
          break;
      }
      else  // no filter or unknown filter -> return everything
        break;
    }
  }
  return Status;
}

VOID
DirIterOpen (
  IN EFI_FILE * BaseDir,
  IN CHAR16 *RelativePath OPTIONAL,
  OUT DIR_ITER * DirIter
)
{
  if (RelativePath == NULL) {
    DirIter->LastStatus = EFI_SUCCESS;
    DirIter->DirHandle = BaseDir;
    DirIter->CloseDirHandle = FALSE;
  }
  else {
    DirIter->LastStatus =
      BaseDir->Open (BaseDir, &(DirIter->DirHandle), RelativePath,
                     EFI_FILE_MODE_READ, 0);
    DirIter->CloseDirHandle = EFI_ERROR (DirIter->LastStatus) ? FALSE : TRUE;
  }
  DirIter->LastFileInfo = NULL;
}

BOOLEAN
DirIterNext (
  IN OUT DIR_ITER * DirIter,
  IN UINTN FilterMode,
  IN CHAR16 *FilePattern OPTIONAL,
  OUT EFI_FILE_INFO ** DirEntry
)
{
  if (DirIter->LastFileInfo != NULL) {
    FreePool (DirIter->LastFileInfo);
    DirIter->LastFileInfo = NULL;
  }

  if (EFI_ERROR (DirIter->LastStatus)) {
    return FALSE; // stop iteration
  }

  for (;;) {
    DirIter->LastStatus =
      DirNextEntry (DirIter->DirHandle, &(DirIter->LastFileInfo), FilterMode);
    if (EFI_ERROR (DirIter->LastStatus)) {
      return FALSE;
    }
    if (DirIter->LastFileInfo == NULL) {  // end of listing
      return FALSE;
    }
    if (FilePattern != NULL) {
      if ((DirIter->LastFileInfo->Attribute & EFI_FILE_DIRECTORY)) {
        break;
      }
      if (MetaiMatch (DirIter->LastFileInfo->FileName, FilePattern)) {
        break;
      }
      // else continue loop
    }
    else
      break;
  }

  *DirEntry = DirIter->LastFileInfo;
  return TRUE;
}

EFI_STATUS
DirIterClose (
  IN OUT DIR_ITER * DirIter
)
{
  if (DirIter->LastFileInfo != NULL) {
    FreePool (DirIter->LastFileInfo);
    DirIter->LastFileInfo = NULL;
  }
  if (DirIter->CloseDirHandle)
    DirIter->DirHandle->Close (DirIter->DirHandle);
  return DirIter->LastStatus;
}

//---------------------------------------------------------------------------------

VOID *
GetDictionary (
  IN VOID *topDict,
  IN CHAR8 *dictName
)
{
  return plDictFind (topDict, dictName, (unsigned int) AsciiStrLen (dictName), plKindDict);
}

BOOLEAN
IsHexDigit (
  CHAR8 c
)
{
  return (IS_DIGIT (c) || (c >= 'A' && c <= 'F') ||
          (c >= 'a' && c <= 'f')) ? TRUE : FALSE;
}

UINT32
hex2bin (
  IN CHAR8 *hex,
  OUT UINT8 *bin,
  INT32 len
)
{
  CHAR8 *p;
  UINT32 i;
  UINT32 outlen;
  CHAR8 buf[3];

  outlen = 0;

  if (hex == NULL || bin == NULL || len <= 0 ||
      (INT32) AsciiStrLen (hex) != len * 2) {
    return 0;
  }

  buf[2] = '\0';
  p = (CHAR8 *) hex;

  for (i = 0; i < (UINT32) len; i++) {
    while ((*p == ' ') || (*p == ',')) {
      p++;
    }
    if (*p == '\0') {
      break;
    }
    if (!IsHexDigit (p[0]) || !IsHexDigit (p[1])) {
      return 0;
    }

    buf[0] = *p++;
    buf[1] = *p++;
    bin[i] = (UINT8) AsciiStrHexToUintn (buf);
    outlen++;
  }
  return outlen;
}

VOID *
GetDataSetting (
  IN VOID *dict,
  IN CHAR8 *propName,
  OUT UINTN *dataLen
)
{
  VOID *prop;
  UINT8 *data;
  UINT32 len;

  data = NULL;
  if (dataLen != NULL) {
    *dataLen = 0;
  }

  prop =
    plDictFind (dict, propName, (unsigned int) AsciiStrLen (propName),
                plKindAny);

  if (prop == NULL) {
    return NULL;
  }
  switch (plNodeGetKind (prop)) {
  case plKindData:
    len = plNodeGetSize (prop);
    data = AllocateCopyPool (len, plNodeGetBytes (prop));
    break;
  case plKindString:
    // assume data in hex encoded string property
    len = plNodeGetSize (prop) >> 1;  // 2 chars per byte
    data = AllocatePool (len);
    len = hex2bin (plNodeGetBytes (prop), data, len);
    break;
  default:
    len = 0;
    break;
  }

  if (dataLen != NULL) {
    *dataLen = len;
  }

  return data;
}

UINTN
GetNumProperty (
  VOID *dict,
  CHAR8 *key,
  UINTN def
)
{
  VOID *dentry;

  dentry =
    plDictFind (dict, key, (unsigned int) AsciiStrLen (key), plKindInteger);
  if (dentry != NULL) {
    def = (UINTN) plIntegerGet (dentry);
  }
  return def;
}

BOOLEAN
GetBoolProperty (
  VOID *dict,
  CHAR8 *key,
  BOOLEAN def
)
{
  VOID *dentry;

  dentry = plDictFind (dict, key, (unsigned int) AsciiStrLen (key), plKindBool);
  if (dentry != NULL) {
    return plBoolGet (dentry) ? TRUE : FALSE;
  }
  return def;
}

VOID
GetAsciiProperty (
  VOID *dict,
  CHAR8 *key,
  CHAR8 *aptr
)
{
  VOID *dentry;
  UINTN slen;

  dentry =
    plDictFind (dict, key, (unsigned int) AsciiStrLen (key), plKindString);
  if (dentry != NULL) {
    slen = plNodeGetSize (dentry);
    CopyMem (aptr, plNodeGetBytes (dentry), slen);
    aptr[slen] = '\0';
  }
}

BOOLEAN
GetUnicodeProperty (
  VOID *dict,
  CHAR8 *key,
  CHAR16 *uptr,
  UINTN usz
)
{
  VOID *dentry;
  CHAR8 *tbuf;
  UINTN tsiz;

  dentry =
    plDictFind (dict, key, (unsigned int) AsciiStrLen (key), plKindString);
  if (dentry != NULL) {
    tsiz = plNodeGetSize (dentry);
    tbuf = AllocateCopyPool (tsiz + 1, plNodeGetBytes (dentry));
    if (tbuf != NULL) {
      tbuf[tsiz] = '\0';
      AsciiStrToUnicodeStrS (tbuf, uptr, usz);
      FreePool (tbuf);
      return TRUE;
    }
  }
  return FALSE;
}

CHAR8 *
GetStringProperty (
  VOID *dict,
  CHAR8 *key
)
{
  VOID *dentry;
  CHAR8 *tbuf;
  UINTN tsiz;

  tbuf = NULL;
  dentry =
    plDictFind (dict, key, (unsigned int) AsciiStrLen (key), plKindString);
  if (dentry != NULL) {
    tsiz = plNodeGetSize (dentry);
    tbuf = AllocateCopyPool (tsiz + 1, plNodeGetBytes (dentry));
    if (tbuf != NULL) {
      tbuf[tsiz] = '\0';
    }
  }
  return tbuf;
}

VOID
plist2dbg (
VOID* plist
)
{
#define XMLBUFSIZE 4096
  plbuf_t xbuf;
  char* lst;
  char* lbyte;

  xbuf.dat = AllocatePool(XMLBUFSIZE + 1);
  if (xbuf.dat == NULL) {
    return;
  }
  xbuf.dat[XMLBUFSIZE] = '\0';
  xbuf.len = XMLBUFSIZE;
  xbuf.pos = 0;

  plNodeToXml (plist, &xbuf);

  lbyte = &xbuf.dat[xbuf.pos];
  for (lst = xbuf.dat; lst < lbyte;) {
    char* eol;

    eol = ScanMem8(lst, lbyte - lst, '\n');
    if (eol == NULL) {
      break;
    } else {
      *eol = '\0';
      DBG ("%a\n", lst);
      lst = eol  + 1;
    }
  }
  FreePool (xbuf.dat);
#undef XMLBUFSIZE
}

VOID *
LoadPListBuffer (
  IN UINT8 **rbuf,
  IN UINTN *rlen
)
{
  EFI_STATUS Status;
  plbuf_t pbuf;

  pbuf.pos = 0;
  pbuf.dat = rbuf;
  pbuf.len = rlen;

  return plXmlToNode (&pbuf);
}

VOID *
LoadPListFile (
  IN EFI_FILE * RootFileHandle,
  IN CHAR16 *XmlPlistPath
)
{
  EFI_STATUS Status;
  plbuf_t pbuf;
  VOID *plist;

  Status = EFI_NOT_FOUND;
  pbuf.pos = 0;

  Status =
    egLoadFile (RootFileHandle, XmlPlistPath, (UINT8 **) &pbuf.dat,
                (UINTN *) &pbuf.len);

  if (EFI_ERROR (Status) || pbuf.dat == NULL) {
    return NULL;
  }

  plist = plXmlToNode (&pbuf);
  FreeAlignedPages (pbuf.dat, EFI_SIZE_TO_PAGES (pbuf.len));
  if (plist == NULL) {
    Print (L"Error loading plist from %s\n", XmlPlistPath);
    return NULL;
  }

  return plist;
}

// ----============================----
EFI_STATUS
GetBootDefault (
  IN EFI_FILE * RootFileHandle
)
{
  VOID *spdict;

  ZeroMem (gSettings.DefaultBoot, sizeof (gSettings.DefaultBoot));

  gConfigPlist = LoadPListFile (RootFileHandle,
                                gPNDirExists ? gPNConfigPlist : BB_HOME_DIR L"config.plist");

  if (gConfigPlist == NULL) {
    Print (L"Error loading bootdefault plist!\r\n");
    return EFI_NOT_FOUND;
  }

  DBG ("%a: config.plist begin\n", __FUNCTION__);
  plist2dbg (gConfigPlist);
  DBG ("%a: config.plist end\n", __FUNCTION__);

  spdict = GetDictionary (gConfigPlist, "SystemParameters");

  gSettings.SaveVideoRom = GetBoolProperty (spdict, "SaveVideoRom", FALSE);
  gSettings.ScreenMode = (UINT32) GetNumProperty (spdict, "ScreenMode", 0xffff);
  gSettings.BootTimeout = (UINT16) GetNumProperty (spdict, "Timeout", 0);
  gSettings.YoBlack = GetBoolProperty (spdict, "YoBlack", FALSE);
  gSettings.DivertLogs = GetNumProperty (spdict, "DivertLogs", 0);	// 0 - no divertion

  if (!GetUnicodeProperty (spdict, "DefaultBootVolume", gSettings.DefaultBoot, ARRAY_SIZE (gSettings.DefaultBoot))) {
    gSettings.BootTimeout = 0xFFFF;
  }
  DBG
    ("GetBootDefault: DefaultBootVolume = %s, Timeout = %d, ScreenMode = %d\n",
     gSettings.DefaultBoot, gSettings.BootTimeout, gSettings.ScreenMode);

  return EFI_SUCCESS;
}

EFI_STATUS
GetUserSettings (
  VOID
)
{
  EFI_STATUS Status;
  VOID *dictPointer;
  VOID *array;
  VOID *prop;
  VOID *tmpval;
  MACHINE_TYPES Model;
  UINTN len;
  UINT32 i;
  CHAR8 cUUID[64];

  Status = EFI_NOT_FOUND;
  array = NULL;
  i = 0;

  if (gConfigPlist == NULL) {
    Print (L"Error loading usersettings plist!\r\n");
    return EFI_NOT_FOUND;
  }

  ZeroMem (gSettings.Language, sizeof (gSettings.Language));
  ZeroMem (gSettings.BootArgs, sizeof (gSettings.BootArgs));
  ZeroMem (gSettings.SerialNr, sizeof (gSettings.SerialNr));

  gSettings.CustomEDID = NULL;
  gSettings.ProcessorInterconnectSpeed = 0;

  dictPointer = GetDictionary (gConfigPlist, "SystemParameters");

  if (dictPointer != NULL) {
    GetAsciiProperty (dictPointer, "prev-lang", gSettings.Language);
    gSettings.LoadExtraKexts = GetBoolProperty (dictPointer, "LoadExtraKexts", TRUE);
    gSettings.CheckFakeSMC = GetBoolProperty (dictPointer, "CheckFakeSMC", TRUE);
    gSettings.NvRam = GetBoolProperty (dictPointer, "NvRam", FALSE);
    gSettings.Hibernate = GetBoolProperty (dictPointer, "Hibernate", FALSE);
    gSettings.FwFeatures = (UINT32) GetNumProperty (dictPointer, "FirmwareFeatures", 0xe907f537);
    gSettings.FwFeaturesMask = (UINT32) GetNumProperty (dictPointer, "FirmwareFeaturesMask", 0xff1fff3f);
    gSettings.CsrActiveConfig = (UINT32) GetNumProperty (dictPointer, "CsrActiveConfig", 0);
    DBG ("GetUserSettings: CsrActiveConfig = %d\n", gSettings.CsrActiveConfig);

    GetAsciiProperty (dictPointer, "boot-args", gSettings.BootArgs);
    if (AsciiStrLen (AddBootArgs) != 0) {
      AsciiStrCatS (gSettings.BootArgs, sizeof (gSettings.BootArgs), AddBootArgs);
    }

    /*
     * XXX: previous implementation was not RFC4112 conforming.
     * Uuid string was treated as linear byte dump, so do the same
     */
    cUUID[0] = '\0';
    GetAsciiProperty (dictPointer, "PlatformUUID", cUUID);
    (void) AsciiStrXuidToBinary (cUUID, &gPlatformUuid);

    cUUID[0] = '\0';
    GetAsciiProperty (dictPointer, "SystemID", cUUID);
    (void) AsciiStrXuidToBinary (cUUID, &gSystemID);

    /* MLB/ROM */
    gSettings.MLB[0] = '\0';
    GetAsciiProperty (dictPointer, "MLB", gSettings.MLB);
    gSettings.ROM = GetDataSetting (dictPointer, "ROM", &gSettings.ROMLen);
  }

  dictPointer = GetDictionary (gConfigPlist, "Graphics");

  if (dictPointer != NULL) {
    gSettings.GraphicsInjector =
      GetBoolProperty (dictPointer, "GraphicsInjector", FALSE);
    gSettings.VRAM = LShiftU64 (GetNumProperty (dictPointer, "VRAM", 0), 20);
    gSettings.LoadVBios = GetBoolProperty (dictPointer, "LoadVBios", FALSE);
    gSettings.VideoPorts = (UINT16) GetNumProperty (dictPointer, "VideoPorts", 0);
    gSettings.DualLink = (UINT16) GetNumProperty (dictPointer, "DualLink", 0);
    GetUnicodeProperty (dictPointer, "FBName", gSettings.FBName, ARRAY_SIZE (gSettings.FBName));

    tmpval = GetDataSetting (dictPointer, "NVCAP", &len);
    if (tmpval != NULL) {
      if (len == sizeof (gSettings.NVCAP)) {
        CopyMem(gSettings.NVCAP, tmpval, sizeof (gSettings.NVCAP));
      }
      FreePool (tmpval);
    }

    tmpval = GetDataSetting (dictPointer, "DisplayCfg", &len);
    if (tmpval != NULL) {
      if (len == sizeof (gSettings.Dcfg)) {
        CopyMem(gSettings.Dcfg, tmpval, sizeof (gSettings.Dcfg));
      }
      FreePool (tmpval);
    }

    /* Real custom edid size ignored, assume 128 bytes always */
    gSettings.CustomEDID = GetDataSetting (dictPointer, "CustomEDID", &len);
  }

  dictPointer = GetDictionary (gConfigPlist, "PCI");

  if (dictPointer != NULL) {
    gSettings.PCIRootUID   = (UINT16) GetNumProperty (dictPointer, "PCIRootUID", 0xFFFF);
    gSettings.ETHInjection = GetBoolProperty (dictPointer, "ETHInjection", FALSE);
    gSettings.USBInjection = GetBoolProperty (dictPointer, "USBInjection", FALSE);
    gSettings.ResetHDA     = GetBoolProperty (dictPointer, "ResetHDA", FALSE);
    gSettings.HDALayoutId  = (UINT16) GetNumProperty (dictPointer, "HDAInjection", 0);

    prop = plDictFind (dictPointer, "DeviceProperties", 16, plKindString);

    if (prop != NULL) {
      len = plNodeGetSize (prop);
      cDevProp = AllocatePool (len + 1);
      CopyMem (cDevProp, plNodeGetBytes (prop), len);
      cDevProp[len] = '\0';
      DBG ("GetUserSettings: cDevProp = <%a>\n", cDevProp);
    }
  }

  dictPointer = GetDictionary (gConfigPlist, "ACPI");

  gSettings.DropSSDT = GetBoolProperty (dictPointer, "DropOemSSDT", FALSE);
  gSettings.DropDMAR = GetBoolProperty (dictPointer, "DropDMAR", FALSE);
  gSettings.FixRegions = GetBoolProperty (dictPointer, "FixRegions", FALSE);
  // known pair for ResetAddr/ResetVal is 0x0[C/2]F9/0x06, 0x64/0xFE
  gSettings.ResetAddr =
    (UINT64) GetNumProperty (dictPointer, "ResetAddress", 0);
  gSettings.ResetVal = (UINT8) GetNumProperty (dictPointer, "ResetValue", 0);
  gSettings.PMProfile = (UINT8) GetNumProperty (dictPointer, "PMProfile", 0);
  gSettings.SavePatchedDsdt =
    GetBoolProperty (dictPointer, "SavePatchedDsdt", FALSE);

  gSettings.PatchDsdtNum = 0;
  array = plDictFind (dictPointer, "Patches", 7, plKindArray);
  if (array != NULL) {
    gSettings.PatchDsdtNum = (UINT32) plNodeGetSize (array);
    gSettings.PatchDsdtFind =
      AllocateZeroPool (gSettings.PatchDsdtNum * sizeof (UINT8 *));
    gSettings.PatchDsdtReplace =
      AllocateZeroPool (gSettings.PatchDsdtNum * sizeof (UINT8 *));
    gSettings.LenToFind =
      AllocateZeroPool (gSettings.PatchDsdtNum * sizeof (UINT32));
    gSettings.LenToReplace =
      AllocateZeroPool (gSettings.PatchDsdtNum * sizeof (UINT32));
    DBG ("gSettings.PatchDsdtNum = %d\n", gSettings.PatchDsdtNum);
    for (i = 0; i < gSettings.PatchDsdtNum; i++) {
      prop = plNodeGetItem (array, i);
      gSettings.PatchDsdtFind[i] = GetDataSetting (prop, "Find", &len);
      gSettings.LenToFind[i] = (UINT32) len;
      gSettings.PatchDsdtReplace[i] = GetDataSetting (prop, "Replace", &len);
      gSettings.LenToReplace[i] = (UINT32) len;

      DBG ("  %d. FindLen = %d; ReplaceLen = %d\n", (i + 1),
           gSettings.LenToFind[i], gSettings.LenToReplace[i]
        );
    }
  }

  gSettings.ACPIDropTables = NULL;
  array = plDictFind (dictPointer, "DropTables", 10, plKindArray);
  if (array != NULL) {
    UINT16 NrTableIds;
    ACPI_DROP_TABLE *DropTable;

    NrTableIds = (UINT16) plNodeGetSize (array);
    DBG ("Dropping %d tables\n", NrTableIds);
    if (NrTableIds > 0) {
      for (i = 0; i < NrTableIds; ++i) {
        UINT32 Signature = 0;
        UINT64 TableId = 0;
        CHAR8 *SigStr;
        CHAR8 *TablStr;
        CHAR8 s1 = 0, s2 = 0, s3 = 0, s4 = 0;
        UINTN idi = 0;
        CHAR8 id[8];

        DBG (" Drop table %d", (i + 1));
        prop = plNodeGetItem (array, i);
        // Get the table signatures to drop
        SigStr = GetStringProperty (prop, "Signature");
        if (AsciiStrLen (SigStr) != 4) {
          DBG (", bad signature\n");
          continue;
        }
        DBG (" signature = '");
        if (*SigStr) {
          s1 = *SigStr++;
          DBG ("%c", s1);
        }
        if (*SigStr) {
          s2 = *SigStr++;
          DBG ("%c", s2);
        }
        if (*SigStr) {
          s3 = *SigStr++;
          DBG ("%c", s3);
        }
        if (*SigStr) {
          s4 = *SigStr++;
          DBG ("%c", s4);
        }
        Signature = SIGNATURE_32 (s1, s2, s3, s4);
        DBG ("' (%8.8X)", Signature);
        // Get the table ids to drop
        TablStr = GetStringProperty (prop, "TableId");
        ZeroMem (id, sizeof (id));
        if (TablStr) {
          DBG (" table-id = '");
          while (*TablStr && (idi < 8)) {
            DBG ("%c", *TablStr);
            id[idi++] = *TablStr++;
          }
        }
        CopyMem (&TableId, id, 8);
        DBG ("' (%16.16lX)\n", TableId);

        DropTable = AllocateZeroPool (sizeof (ACPI_DROP_TABLE));

        DropTable->Signature = Signature;
        DropTable->TableId = TableId;

        DropTable->Next = gSettings.ACPIDropTables;
        gSettings.ACPIDropTables = DropTable;
      }
      gSettings.DropSSDT = FALSE;
    }
  }

  AsciiStrCpyS (gSettings.BoardManufactureName, sizeof (gSettings.BoardManufactureName), BiosVendor);
  AsciiStrCpyS (gSettings.ChassisManufacturer, sizeof (gSettings.ChassisManufacturer), BiosVendor);
  AsciiStrCpyS (gSettings.ManufactureName, sizeof (gSettings.ManufactureName), BiosVendor);
  AsciiStrCpyS (gSettings.VendorName, sizeof (gSettings.VendorName), BiosVendor);

  AsciiStrCpyS (gSettings.BoardSerialNumber, sizeof (gSettings.BoardSerialNumber), AppleBoardSN);
  AsciiStrCpyS (gSettings.LocationInChassis, sizeof (gSettings.LocationInChassis), AppleBoardLocation);

  dictPointer = GetDictionary (gConfigPlist, "SMBIOS");

  gSettings.Mobile = GetBoolProperty (dictPointer, "Mobile", FALSE);

  GetAsciiProperty (dictPointer, "ProductName", gSettings.ProductName);

  Model = Unknown;
  if (AsciiStrLen (gSettings.ProductName) > 0) {
    for (i = 0; i < ARRAY_SIZE (AppleProductName); i++) {
      if (AsciiStrStr (AppleProductName[i], gSettings.ProductName) != NULL) {
        Model = i;
        break;
      }
    }
  } else {
    Model = GetDefaultModel ();
  }

  if (Model != Unknown) {
    AsciiStrCpyS (gSettings.BoardNumber, sizeof (gSettings.BoardNumber), AppleBoardID[Model]);
    AsciiStrCpyS (gSettings.BoardVersion, sizeof (gSettings.BoardVersion), AppleSystemVersion[Model]);
    AsciiStrCpyS (gSettings.ChassisAssetTag, sizeof (gSettings.ChassisAssetTag), AppleChassisAsset[Model]);
    AsciiStrCpyS (gSettings.FamilyName, sizeof (gSettings.FamilyName), AppleFamilies[Model]);
    AsciiStrCpyS (gSettings.ProductName, sizeof (gSettings.ProductName), AppleProductName[Model]);
    AsciiStrCpyS (gSettings.ReleaseDate, sizeof (gSettings.ReleaseDate), AppleReleaseDate[Model]);
    AsciiStrCpyS (gSettings.RomVersion, sizeof (gSettings.RomVersion), AppleFirmwareVersion[Model]);
    AsciiStrCpyS (gSettings.SerialNr, sizeof (gSettings.SerialNr), AppleSerialNumber[Model]);
    AsciiStrCpyS (gSettings.VersionNr, sizeof (gSettings.VersionNr), AppleSystemVersion[Model]);
  }

  GetAsciiProperty (dictPointer, "BiosReleaseDate", gSettings.ReleaseDate);
  GetAsciiProperty (dictPointer, "BiosVendor", gSettings.VendorName);
  GetAsciiProperty (dictPointer, "BiosVersion", gSettings.RomVersion);
  GetAsciiProperty (dictPointer, "Board-ID", gSettings.BoardNumber);
  GetAsciiProperty (dictPointer, "BoardManufacturer", gSettings.BoardManufactureName);
  GetAsciiProperty (dictPointer, "BoardSerialNumber", gSettings.BoardSerialNumber);
  GetAsciiProperty (dictPointer, "BoardVersion", gSettings.BoardVersion);
  GetAsciiProperty (dictPointer, "ChassisAssetTag", gSettings.ChassisAssetTag);
  GetAsciiProperty (dictPointer, "ChassisManufacturer", gSettings.ChassisManufacturer);
  GetAsciiProperty (dictPointer, "Family", gSettings.FamilyName);
  GetAsciiProperty (dictPointer, "LocationInChassis", gSettings.LocationInChassis);
  GetAsciiProperty (dictPointer, "Manufacturer", gSettings.ManufactureName);
  GetAsciiProperty (dictPointer, "SerialNumber", gSettings.SerialNr);
  GetAsciiProperty (dictPointer, "Version", gSettings.VersionNr);

  DBG ("BEGIN Product smbios datum\n");
  DBG ("ProductName = %a\n", gSettings.ProductName);
  DBG ("Mobile = %a\n", gSettings.Mobile ? "true" : "false");

  DBG ("%a = %a\n", "BiosReleaseDate", gSettings.ReleaseDate);
  DBG ("%a = %a\n", "BiosVendor", gSettings.VendorName);
  DBG ("%a = %a\n", "BiosVersion", gSettings.RomVersion);
  DBG ("%a = %a\n", "Board-ID", gSettings.BoardNumber);
  DBG ("%a = %a\n", "BoardManufacturer", gSettings.BoardManufactureName);
  DBG ("%a = %a\n", "BoardSerialNumber", gSettings.BoardSerialNumber);
  DBG ("%a = %a\n", "BoardVersion", gSettings.BoardVersion);
  DBG ("%a = %a\n", "ChassisAssetTag", gSettings.ChassisAssetTag);
  DBG ("%a = %a\n", "ChassisManufacturer", gSettings.ChassisManufacturer);
  DBG ("%a = %a\n", "Family", gSettings.FamilyName);
  DBG ("%a = %a\n", "LocationInChassis", gSettings.LocationInChassis);
  DBG ("%a = %a\n", "Manufacturer", gSettings.ManufactureName);
  DBG ("%a = %a\n", "SerialNumber", gSettings.SerialNr);
  DBG ("%a = %a\n", "Version", gSettings.VersionNr);

  gSettings.SPDScan = GetBoolProperty (dictPointer, "SPDScan", FALSE);

  array = plDictFind (dictPointer, "MemoryDevices", 13, plKindArray);
  if (array != NULL) {
    UINT8 Slot;
    UINT8 NrSlot;

    NrSlot = (UINT8) plNodeGetSize (array);
    DBG ("Custom Memory Devices slots = %d\n", NrSlot);
    if (NrSlot <= MAX_RAM_SLOTS) {
      for (i = 0; i < NrSlot; i++) {
        prop = plNodeGetItem (array, i);
        Slot = (UINT8) GetNumProperty (prop, "Slot", 0xff);
        gSettings.cMemDevice[Slot].InUse = TRUE;
        gSettings.cMemDevice[Slot].MemoryType =
          (UINT8) GetNumProperty (prop, "MemoryType", 0x02);
        gSettings.cMemDevice[Slot].Speed = (UINT16) GetNumProperty (prop, "Speed", 0x00); //MHz
        gSettings.cMemDevice[Slot].Size = (UINT16) GetNumProperty (prop, "Size", 0xffff); //MB

        gSettings.cMemDevice[Slot].DeviceLocator =
          GetStringProperty (prop, "DeviceLocator");
        gSettings.cMemDevice[Slot].BankLocator =
          GetStringProperty (prop, "BankLocator");
        gSettings.cMemDevice[Slot].Manufacturer =
          GetStringProperty (prop, "Manufacturer");
        gSettings.cMemDevice[Slot].SerialNumber =
          GetStringProperty (prop, "SerialNumber");
        gSettings.cMemDevice[Slot].PartNumber =
          GetStringProperty (prop, "PartNumber");

        DBG (" gSettings.cMemDevice[%d].MemoryType = 0x%x\n", Slot,
             gSettings.cMemDevice[Slot].MemoryType);
        DBG (" gSettings.cMemDevice[%d].Speed = %d MHz\n", Slot,
             gSettings.cMemDevice[Slot].Speed);
        DBG (" gSettings.cMemDevice[%d].Size = %d MB\n", Slot,
             gSettings.cMemDevice[Slot].Size);
        DBG (" gSettings.cMemDevice[%d].DeviceLocator = %a\n", Slot,
             gSettings.cMemDevice[Slot].DeviceLocator);
        DBG (" gSettings.cMemDevice[%d].BankLocator = %a\n", Slot,
             gSettings.cMemDevice[Slot].BankLocator);
        DBG (" gSettings.cMemDevice[%d].Manufacturer = %a\n", Slot,
             gSettings.cMemDevice[Slot].Manufacturer);
        DBG (" gSettings.cMemDevice[%d].SerialNumber = %a\n", Slot,
             gSettings.cMemDevice[Slot].SerialNumber);
        DBG (" gSettings.cMemDevice[%d].PartNumber = %a\n", Slot,
             gSettings.cMemDevice[Slot].PartNumber);
      }
    }
  }
  DBG ("END Product smbios datum\n");

  DBG ("PlatformUUID is %g (rfc4112)\n", &gPlatformUuid);
  DBG ("SystemID is %g (rfc4112)\n", &gSystemID);

  dictPointer = GetDictionary (gConfigPlist, "CPU");

  gSettings.PatchLAPIC = GetBoolProperty (dictPointer, "PatchLAPIC", FALSE);
  gSettings.PatchPM = GetBoolProperty (dictPointer, "PatchPM", FALSE);
  gSettings.PatchCPU = GetBoolProperty (dictPointer, "PatchCPU", FALSE);
  gSettings.CpuIdSing = (UINT32) GetNumProperty (dictPointer, "CpuIdSing", 0);

  if (GetBoolProperty (dictPointer, "Turbo", FALSE)) {
    if (gCPUStructure.TurboMsr != 0) {
      AsmWriteMsr64 (MSR_IA32_PERF_CONTROL, gCPUStructure.TurboMsr);
      gBS->Stall (100);
      i = 100000;
      while (AsmReadMsr64 (MSR_IA32_PERF_STATUS) & (1 << 21)) {
        if (!i--) {
          break;
        }
      }
    }
    AsmReadMsr64 (MSR_IA32_PERF_STATUS);
  }

  gSettings.CPUFrequency =
    (UINT64) GetNumProperty (dictPointer, "CPUFrequency", 0);
  gSettings.FSBFrequency =
    (UINT64) GetNumProperty (dictPointer, "FSBFrequency", 0);
  gSettings.ProcessorInterconnectSpeed =
    (UINT32) GetNumProperty (dictPointer, "QPI", 0);
  gSettings.CpuType =
    (UINT16) GetNumProperty (dictPointer, "ProcessorType",
                             GetAdvancedCpuType ());

  if (gSettings.FSBFrequency != 0) {
    gCPUStructure.FSBFrequency = gSettings.FSBFrequency;
    if (gSettings.CPUFrequency == 0) {
      gCPUStructure.CPUFrequency =
        MultU64x32 (gCPUStructure.FSBFrequency, gCPUStructure.MaxRatio);
    }
    DBG ("GetUserSettings: gCPUStructure.FSBFrequency = %d\n",
         gCPUStructure.FSBFrequency);
  }

  if (gSettings.CPUFrequency != 0) {
    gCPUStructure.CPUFrequency = gSettings.CPUFrequency;
    if (gSettings.FSBFrequency == 0) {
      gCPUStructure.FSBFrequency =
        DivU64x32 (gCPUStructure.CPUFrequency, gCPUStructure.MaxRatio);
    }
    DBG ("GetUserSettings: gCPUStructure.CPUFrequency = %d\n",
         gCPUStructure.CPUFrequency);
  }

  // KernelAndKextPatches
  gSettings.KPKernelPatchesNeeded = FALSE;
  gSettings.KPKextPatchesNeeded = FALSE;

  array = plDictFind (gConfigPlist, "KernelPatches", 13, plKindArray);
  if (array != NULL) {
    gSettings.NrKernel = (UINT32) plNodeGetSize (array);
    DBG ("gSettings.NrKernel = %d\n", gSettings.NrKernel);
    if ((gSettings.NrKernel <= 100)) {
      for (i = 0; i < gSettings.NrKernel; i++) {
        gSettings.AnyKernelData[i] = 0;
        len = 0;

        dictPointer = plNodeGetItem (array, i);
        gSettings.AnyKernelData[i] =
          GetDataSetting (dictPointer, "Find", &gSettings.AnyKernelDataLen[i]);
        gSettings.AnyKernelPatch[i] =
          GetDataSetting (dictPointer, "Replace", &len);

        if (gSettings.AnyKernelDataLen[i] != len || len == 0) {
          gSettings.AnyKernelDataLen[i] = 0;
          continue;
        }
        gSettings.KPKernelPatchesNeeded = TRUE;
        DBG ("  %d. kernel patch, length = %d, %a\n", (i + 1),
             gSettings.AnyKernelDataLen[i]
          );
      }
    }
  }

  array = plDictFind (gConfigPlist, "KextPatches", 11, plKindArray);
  if (array != NULL) {
    gSettings.NrKexts = (UINT32) plNodeGetSize (array);
    DBG ("gSettings.NrKexts = %d\n", gSettings.NrKexts);
    if ((gSettings.NrKexts <= 100)) {
      for (i = 0; i < gSettings.NrKexts; i++) {
        gSettings.AnyKextDataLen[i] = 0;
        len = 0;
        dictPointer = plNodeGetItem (array, i);
        gSettings.AnyKext[i] = GetStringProperty (dictPointer, "Name");
        // check if this is Info.plist patch or kext binary patch
        gSettings.AnyKextInfoPlistPatch[i] =
          GetBoolProperty (dictPointer, "InfoPlistPatch", FALSE);
        if (gSettings.AnyKextInfoPlistPatch[i]) {
          // Info.plist
          // Find and Replace should be in <string>...</string>
          gSettings.AnyKextData[i] = GetStringProperty (dictPointer, "Find");
          if (gSettings.AnyKextData[i] != NULL) {
            gSettings.AnyKextDataLen[i] =
              AsciiStrLen (gSettings.AnyKextData[i]);
          }
          gSettings.AnyKextPatch[i] =
            GetStringProperty (dictPointer, "Replace");
          if (gSettings.AnyKextPatch[i] != NULL) {
            len = AsciiStrLen (gSettings.AnyKextPatch[i]);
          }
        }
        else {
          // kext binary patch
          // Find and Replace should be in <data>...</data> or <string>...</string>
          gSettings.AnyKextData[i] =
            GetDataSetting (dictPointer, "Find", &gSettings.AnyKextDataLen[i]);
          gSettings.AnyKextPatch[i] =
            GetDataSetting (dictPointer, "Replace", &len);
        }
        if (gSettings.AnyKextDataLen[i] != len || len == 0) {
          gSettings.AnyKextDataLen[i] = 0;
          continue;
        }
        gSettings.KPKextPatchesNeeded = TRUE;
        DBG ("  %d. name = %a, length = %d, %a\n", (i + 1),
             gSettings.AnyKext[i], gSettings.AnyKextDataLen[i],
             gSettings.AnyKextInfoPlistPatch[i] ? "KextInfoPlistPatch " : "");
      }
    }
  }

  plNodeDelete (gConfigPlist);

  return Status;
}

STATIC CHAR16 *OSVersionFiles[] = {
  L"System\\Library\\CoreServices\\SystemVersion.plist",
  L"System\\Library\\CoreServices\\ServerVersion.plist",
  L"\\com.apple.recovery.boot\\SystemVersion.plist"
};

EFI_STATUS
GetOSVersion (
  IN EFI_FILE * FileHandle
)
{
  UINTN i;
  VOID *plist;

  /* Mac OS X */

  for (i = 0; i < 3; i++) {
    plist = LoadPListFile (FileHandle, OSVersionFiles[i]);
    if (plist != NULL) {
      OSVersion = GetStringProperty (plist, "ProductVersion");
      plNodeDelete (plist);
      DBG ("GetOSVersion: OSVersion = %a\n", OSVersion);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
