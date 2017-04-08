/*
 *  Copyright 2009 Jasmin Fazlic All rights reserved.
 */

/*
 *  Cleaned and merged by iNDi
 */
// UEFI adaptation by usr-sse2

#include <macosx.h>

#include "device_inject.h"
#include "nvidia.h"
#include "ati.h"
#include "gma.h"

GFX_PROPERTIES                  gGraphics;

UINT32 devices_number = 1;
UINT32 builtin_set = 0;
DevPropString *string = NULL;
UINT8 *stringdata = NULL;
UINT32 stringlength = 0;

#if 0
EFI_EDID_DISCOVERED_PROTOCOL *EdidDiscovered;
UINT8 *gEDID;
#endif
#if 0
pci_dt_t *nvdevice;
#endif

static value_t aty_name;
static value_t aty_nameparent;
static CONST CHAR8 *dtyp[] = { "LCD", "CRT", "DVI", "NONE" };
static UINT32 dti = 0;

#if 0
static CHAR8 pciName[15];
#endif

// Connector types

//0x0002: LVDS  
//0x0004: DVI-D
//0x0010: VGA
//0x0080: SVIDEO
//0x0200: DVI-I
//0x0400: DP
//0x0800: HDMI

static CONST UINT32 ctm[] = { 0x0002, 0x0010, 0x0800, 0x0400 }; //mobile
static CONST UINT32 ctd[] = { 0x0004, 0x0010, 0x0800, 0x0400 }; //desktop
static UINT32 cti = 0;

card_t *card;

#if 0
static value_t aty_model;
#endif

UINT16
dp_read16 (
  UINT8 *ptr,
  UINT16 offset
)
{
  UINT8 ret[2];

  ret[0] = ptr[offset + 1];
  ret[1] = ptr[offset];
  return *((UINT16 *) &ret);
}

DevPropString *
devprop_create_string (
  VOID
)
{
  string = (DevPropString *) AllocateZeroPool (sizeof (DevPropString));

  if (string == NULL) {
    return NULL;
  }

  string->length = 12;
  string->WHAT2 = 0x01000000;
  return string;
}

UINT32
pci_config_read32 (
  pci_dt_t * PciDt,
  UINT8 reg
)
{
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00 Pci;
  UINT32 res;

  Status =
    gBS->OpenProtocol (PciDt->DeviceHandle, &gEfiPciIoProtocolGuid,
                       (VOID **) &PciIo, gImageHandle, NULL,
                       EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (EFI_ERROR (Status)) {
    return 0;
  }

  Status =
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0,
                     sizeof (Pci) / sizeof (UINT32), &Pci);

  if (EFI_ERROR (Status)) {
    return 0;
  }

  Status =
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, (UINT64) (reg & ~3), 1, &res);

  if (EFI_ERROR (Status)) {
    return 0;
  }

  return res;
}

DevPropDevice *
devprop_add_device_pci (
  DevPropString * StringBuf,
  pci_dt_t * PciDt
)
{
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  DevPropDevice *device;
  UINT32 NumPaths;

  if (StringBuf == NULL || PciDt == NULL) {
    return NULL;
  }

  DevicePath = DevicePathFromHandle (PciDt->DeviceHandle);

  if (DevicePath == NULL) {
    return NULL;
  }

  device = AllocateZeroPool (sizeof (DevPropDevice));
  device->numentries = 0x00;

  //
  // check and copy ACPI_DEVICE_PATH
  //
  if (DevicePath->Type == ACPI_DEVICE_PATH && DevicePath->SubType == ACPI_DP) {
    device->acpi_dev_path.length = 0x0c;
    device->acpi_dev_path.type = 0x02;
    device->acpi_dev_path.subtype = 0x01;
    device->acpi_dev_path._HID = 0xd041030a;
    if (gSettings.PCIRootUID == 0xFFFF) {
      device->acpi_dev_path._UID = ((ACPI_HID_DEVICE_PATH*)DevicePath)->UID;
    } else {
      device->acpi_dev_path._UID = gSettings.PCIRootUID;
    }
  }
  else {
    FreePool (DevicePath);
    FreePool (device);
    return NULL;
  }

  //
  // copy PCI paths
  //
  for (NumPaths = 0; NumPaths < MAX_PCI_DEV_PATHS; NumPaths++) {
    DevicePath = NextDevicePathNode (DevicePath);

    if (DevicePath->Type == HARDWARE_DEVICE_PATH &&
        DevicePath->SubType == HW_PCI_DP) {
      CopyMem (&device->pci_dev_path[NumPaths], DevicePath,
               sizeof (struct PCIDevPath));
    }
    else {
      break;
    }
  }

  if (NumPaths == 0) {
    FreePool (device);
    return NULL;
  }

  device->num_pci_devpaths = (UINT8) NumPaths;
  device->length = (UINT32) (24U + (6U * NumPaths));
  device->path_end.length = 0x04;
  device->path_end.type = 0x7f;
  device->path_end.subtype = 0xff;
  device->string = StringBuf;
  device->data = NULL;

  StringBuf->length += device->length;

  if (StringBuf->entries == NULL) {
    StringBuf->entries =
      (DevPropDevice **) AllocateZeroPool (MAX_NUM_DEVICES * sizeof (device));

    if (StringBuf->entries == NULL) {
      FreePool (device);
      return NULL;
    }
  }

  StringBuf->entries[StringBuf->numentries++] = device;
  return device;
}

BOOLEAN
devprop_add_value (
  DevPropDevice * device,
  CHAR8 *nm,
  UINT8 *vl,
  UINT32 len
)
{
  UINT32 offset;
  UINT32 off;
  UINT32 length;
  UINT8 *data;
  UINTN i, l;
  UINT32 *datalength;
  UINT8 *newdata;

  if (device == NULL || nm == NULL || vl == NULL || len == 0) {
    return FALSE;
  }

  l = AsciiStrLen (nm);
  length = (UINT32) (((l * 2) + len + (2 * sizeof (UINT32)) + 2));
  data = (UINT8 *) AllocateZeroPool (length);

  if (data == NULL) {
    return FALSE;
  }

  off = 0;
  data[off + 1] = (UINT8) (((l * 2) + 6) >> 8);
  data[off] = ((l * 2) + 6) & 0x00FF;
  off += 4;

  for (i = 0; i < l; i++, off += 2) {
    data[off] = *nm++;
  }

  off += 2;
  l = len;
  datalength = (UINT32 *) &data[off];
  *datalength = (UINT32) (l + 4);
  off += 4;

  for (i = 0; i < l; i++, off++) {
    data[off] = *vl++;
  }

  offset = device->length - (24 + (6 * device->num_pci_devpaths));
  newdata = (UINT8 *) AllocateZeroPool ((length + offset));

  if (newdata == NULL) {
    return FALSE;
  }

  if ((device->data) && (offset > 1)) {
    CopyMem ((VOID *) newdata, (VOID *) device->data, offset);
  }

  CopyMem ((VOID *) (newdata + offset), (VOID *) data, length);
  device->length += length;
  device->string->length += length;
  device->numentries++;
  device->data = newdata;
  FreePool (data);
  return TRUE;
}

CHAR8 *
devprop_generate_string (
  DevPropString * pstring
)
{
  UINTN len;
  INT32 i, x;
  CHAR8 *buffer;
  CHAR8 *ptr;

  len = pstring->length * 2;
  i = 0;
  x = 0;
  buffer = (CHAR8 *) AllocatePool (len + 1);
  ptr = buffer;

  if (buffer == NULL) {
    return NULL;
  }

  AsciiSPrint (buffer, len, "%08x%08x%04x%04x", SwapBytes32 (pstring->length),
               pstring->WHAT2, SwapBytes16 (pstring->numentries),
               pstring->WHAT3);
  buffer += 24;

  while (i < pstring->numentries) {
    UINT8 *dataptr;

    AsciiSPrint (buffer, len, "%08x%04x%04x", SwapBytes32 (pstring->entries[i]->length), SwapBytes16 (pstring->entries[i]->numentries), pstring->entries[i]->WHAT2);  //FIXME: wrong buffer sizes!
    buffer += 16;
    AsciiSPrint (buffer, len, "%02x%02x%04x%08x%08x",
                 pstring->entries[i]->acpi_dev_path.type,
                 pstring->entries[i]->acpi_dev_path.subtype,
                 SwapBytes16 (pstring->entries[i]->acpi_dev_path.length),
                 pstring->entries[i]->acpi_dev_path._HID,
                 SwapBytes32 (pstring->entries[i]->acpi_dev_path._UID));
    buffer += 24;

    for (x = 0; x < pstring->entries[i]->num_pci_devpaths; x++) {
      AsciiSPrint (buffer, len, "%02x%02x%04x%02x%02x",
                   pstring->entries[i]->pci_dev_path[x].type,
                   pstring->entries[i]->pci_dev_path[x].subtype,
                   SwapBytes16 (pstring->entries[i]->pci_dev_path[x].length),
                   pstring->entries[i]->pci_dev_path[x].function,
                   pstring->entries[i]->pci_dev_path[x].device);
      buffer += 12;
    }

    AsciiSPrint (buffer, len, "%02x%02x%04x",
                 pstring->entries[i]->path_end.type,
                 pstring->entries[i]->path_end.subtype,
                 SwapBytes16 (pstring->entries[i]->path_end.length));
    buffer += 8;
    dataptr = pstring->entries[i]->data;

    for (x = 0;
         x <
         (INT32) ((pstring->entries[i]->length) -
                  (24 + (6 * pstring->entries[i]->num_pci_devpaths))); x++) {
      AsciiSPrint (buffer, len, "%02x", *dataptr++);
      buffer += 2;
    }

    i++;
  }

  return ptr;
}

VOID
devprop_free_string (
  DevPropString * pstring
)
{
  INT32 i;

  if (pstring == NULL) {
    return;
  }

  for (i = 0; i < pstring->numentries; i++) {
    if (pstring->entries[i] != NULL) {
      if (pstring->entries[i]->data != NULL) {
        FreePool (pstring->entries[i]->data);
        pstring->entries[i]->data = NULL;
      }

      FreePool (pstring->entries[i]);
      pstring->entries[i] = NULL;
    }
  }

  FreePool (pstring->entries);
  FreePool (pstring);
}

CHAR8 *
get_pci_dev_path (
  pci_dt_t * PciDt
)
{
  CHAR8 *tmp;
  CHAR16 *devpathstr;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;

  devpathstr = NULL;
  DevicePath = NULL;

  DevicePath = DevicePathFromHandle (PciDt->DeviceHandle);
  if (DevicePath == NULL) {
    return NULL;
  }
  devpathstr = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
  tmp = AllocateZeroPool ((StrLen (devpathstr) + 1) * sizeof (CHAR8));
  UnicodeStrToAsciiStr (devpathstr, tmp);
  return tmp;
}

// ---------------============== Ethernet built-in device injection

BOOLEAN
set_eth_props (
  pci_dt_t * eth_dev
)
{
  DevPropDevice *device;
  UINT8 builtin;

  builtin = 0;

  if (string == NULL) {
    string = devprop_create_string ();
  }

  device = devprop_add_device_pci (string, eth_dev);

  if (device == NULL) {
    return FALSE;
  }

  if (eth_dev->vendor_id != 0x168c && builtin_set == 0) {
    builtin_set = 1;
    builtin = 0x01;
  }

#if 0
  devprop_add_value (device, "device_type", (UINT8 *) "ethernet", 8);
#endif
  return devprop_add_value (device, "built-in", (UINT8 *) &builtin, 1);
}

#if 0
static UINT8 clock_id = 0;
#endif

// ---------------============== USB injection

BOOLEAN
set_usb_props (
  pci_dt_t * usb_dev
)
{
  DevPropDevice *device;
  UINT32 fake_devid;
  UINT8 builtin;
  UINT16 current_available;     //mA
  UINT16 current_extra;
  UINT16 current_in_sleep;

  builtin = 0;
  current_available = 1200; //mA
  current_extra = 700;
  current_in_sleep = 1000;

  if (string == NULL) {
    string = devprop_create_string ();
  }

  device = devprop_add_device_pci (string, usb_dev);

  if (device == NULL) {
    return FALSE;
  }

#if 0
  devprop_add_value (device, "AAPL,clock-id", (UINT8 *) &clock_id, 1);
  clock_id++;
#endif
  fake_devid = usb_dev->device_id & 0xFFFF;

  if ((fake_devid & 0xFF00) == 0x2900) {
    fake_devid &= 0xFEFF;
    devprop_add_value (device, "device-id", (UINT8 *) &fake_devid, 4);
  }

  switch (usb_dev->subclass) {
  case PCI_IF_UHCI:
    devprop_add_value (device, "device_type", (UINT8 *) "UHCI", 4);
    break;

  case PCI_IF_OHCI:
    devprop_add_value (device, "device_type", (UINT8 *) "OHCI", 4);
    break;

  case PCI_IF_EHCI:
    devprop_add_value (device, "device_type", (UINT8 *) "EHCI", 4);
    devprop_add_value (device, "AAPL,current-available",
                       (UINT8 *) &current_available, 2);
    devprop_add_value (device, "AAPL,current-extra", (UINT8 *) &current_extra,
                       2);
    devprop_add_value (device, "AAPL,current-in-sleep",
                       (UINT8 *) &current_in_sleep, 2);
    break;

  case PCI_IF_XHCI:
    devprop_add_value (device, "device_type", (UINT8 *) "XHCI", 4);
    break;

  default:
    break;
  }

  return devprop_add_value (device, "built-in", (UINT8 *) &builtin, 1);
}

// ---------------============== HDA injection (needs device HDEF in DSDT)

BOOLEAN
set_hda_props (
  EFI_PCI_IO_PROTOCOL * PciIo,
  pci_dt_t * hda_dev
)
{
  DevPropDevice *device;
  UINT32 layoutId;

  layoutId = 0;

  if (string == NULL) {
    string = devprop_create_string ();
  }

  device = devprop_add_device_pci (string, hda_dev);

  if (device == NULL) {
    return FALSE;
  }

  layoutId = (UINT32) gSettings.HDALayoutId;
  devprop_add_value (device, "layout-id", (UINT8 *) &layoutId, 4);
  layoutId = 0;
  devprop_add_value (device, "PinConfigurations", (UINT8 *) &layoutId, 1);
  return TRUE;
}

// ---------------============== NVIDIA injection

EFI_STATUS
read_nVidia_PRAMIN (
  pci_dt_t * nvda_dev,
  VOID *rom,
  UINT8 arch
)
{
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00 Pci;
  UINT32 vbios_vram;
  UINT32 old_bar0_pramin;

  vbios_vram = 0;
  old_bar0_pramin = 0;

  Status =
    gBS->OpenProtocol (nvda_dev->DeviceHandle, &gEfiPciIoProtocolGuid,
                       (VOID **) &PciIo, gImageHandle, NULL,
                       EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Status =
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0,
                     sizeof (Pci) / sizeof (UINT32), &Pci);

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  if (arch >= 0x50) {
    Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint32, 0, NV_PDISPLAY_OFFSET + 0x9f04, ///4,
                              1, &vbios_vram);
    vbios_vram = (vbios_vram & ~0xff) << 8;
    Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint32, 0, NV_PMC_OFFSET + 0x1700,  ///4,
                              1, &old_bar0_pramin);

    if (vbios_vram == 0) {
      vbios_vram = (old_bar0_pramin << 16) + 0xf0000;
    }

    vbios_vram >>= 16;
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, 0, NV_PMC_OFFSET + 0x1700, ///4,
                               1, &vbios_vram);
  }

  Status =
    PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, 0, NV_PRAMIN_OFFSET,
                     NVIDIA_ROM_SIZE, rom);

  if (arch >= 0x50) {
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, 0, NV_PMC_OFFSET + 0x1700, ///4,
                               1, &old_bar0_pramin);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
read_nVidia_PROM (
  pci_dt_t * nvda_dev,
  VOID *rom
)
{
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00 Pci;
  UINT32 value;

  Status =
    gBS->OpenProtocol (nvda_dev->DeviceHandle, &gEfiPciIoProtocolGuid,
                       (VOID **) &PciIo, gImageHandle, NULL,
                       EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Status =
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0,
                     sizeof (Pci) / sizeof (UINT32), &Pci);

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  value = NV_PBUS_PCI_NV_20_ROM_SHADOW_DISABLED;
  Status =
    PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, 0, NV_PBUS_PCI_NV_20 * 4, 1,
                      &value);
  Status =
    PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, 0, NV_PROM_OFFSET,
                     NVIDIA_ROM_SIZE, rom);
  value = NV_PBUS_PCI_NV_20_ROM_SHADOW_ENABLED;
  Status =
    PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, 0, NV_PBUS_PCI_NV_20 * 4, 1,
                      &value);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

INT32
patch_nvidia_rom (
  UINT8 *rom
)
{
  BOOLEAN has_lvds;
  UINT16 dcbptr;
  UINT8 *dcbtable;
  UINT8 *togroup;
  UINT8 channel1, channel2;
  UINT8 dcbtable_version;
  UINT8 headerlength;
  UINT8 num_outputs, i;
  UINT8 numentries;
  UINT8 recordlength;

  struct dcbentry {
    UINT8 type;
    UINT8 index;
    UINT8 *heads;
  } entries[MAX_NUM_DCB_ENTRIES];

  num_outputs = 0;
  i = 0;
  has_lvds = FALSE;
  channel1 = 0;
  channel2 = 0;
  headerlength = 0;
  numentries = 0;
  recordlength = 0;

  if (rom == NULL || (rom[0] != 0x55 && rom[1] != 0xaa)) {
    return PATCH_ROM_FAILED;
  }

  dcbptr = SwapBytes16 (dp_read16 (rom, 0x36));

  if (dcbptr == 0) {
    return PATCH_ROM_FAILED;
  }

  dcbtable = &rom[dcbptr];
  dcbtable_version = dcbtable[0];

  if (dcbtable_version >= 0x20) {
    UINT32 sig;

    if (dcbtable_version >= 0x30) {
      headerlength = dcbtable[1];
      numentries = dcbtable[2];
      recordlength = dcbtable[3];
      sig = *(UINT32 *) &dcbtable[6];
    }
    else {
      sig = *(UINT32 *) &dcbtable[4];
      headerlength = 8;
    }

    if (sig != 0x4edcbdcb) {
      return PATCH_ROM_FAILED;
    }
  }
  else if (dcbtable_version >= 0x14) {  /* some NV15/16, and NV11+ */
    CHAR8 sig[8];

    AsciiStrnCpy (sig, (CHAR8 *) &dcbtable[-7], 7);
    recordlength = 10;

    if (AsciiStrCmp (sig, "DEV_REC")) {
      return PATCH_ROM_FAILED;
    }
  }
  else {
    return PATCH_ROM_FAILED;
  }

  if (numentries >= MAX_NUM_DCB_ENTRIES) {
    numentries = MAX_NUM_DCB_ENTRIES;
  }

  for (i = 0; i < numentries; i++) {
    UINT32 connection;

    connection = *(UINT32 *) &dcbtable[headerlength + recordlength * i];

    /* Should we allow discontinuous DCBs? Certainly DCB I2C tables can be discontinuous */
    if ((connection & 0x0000000f) == 0x0000000f) {  /* end of records */
      continue;
    }

    if (connection == 0x00000000) { /* seen on an NV11 with DCB v1.5 */
      continue;
    }

    if ((connection & 0xf) == 0x6) {  /* we skip type 6 as it doesnt appear on macbook nvcaps */
      continue;
    }

    entries[num_outputs].type = connection & 0xf;
    entries[num_outputs].index = num_outputs;
    entries[num_outputs++].heads =
      (UINT8 *) &(dcbtable[(headerlength + recordlength * i) + 1]);
  }

  for (i = 0; i < num_outputs; i++) {
    if (entries[i].type == 3) {
      has_lvds = TRUE;
      channel1 |= (0x1 << entries[i].index);
      entries[i].type = TYPE_GROUPED;
    }
  }

  // if we have a LVDS output, we group the rest to the second channel
  if (has_lvds) {
    for (i = 0; i < num_outputs; i++) {
      if (entries[i].type == TYPE_GROUPED) {
        continue;
      }

      channel2 |= (0x1 << entries[i].index);
      entries[i].type = TYPE_GROUPED;
    }
  }
  else {
    INT32 x;

    // we loop twice as we need to generate two channels
    for (x = 0; x <= 1; x++) {
      for (i = 0; i < num_outputs; i++) {
        if (entries[i].type == TYPE_GROUPED) {
          continue;
        }

        // if type is TMDS, the prior output is ANALOG
        // we always group ANALOG and TMDS
        // if there is a TV output after TMDS, we group it to that channel as well
        if (i && entries[i].type == 0x2) {
          switch (x) {
          case 0:
            channel1 |= (0x1 << entries[i].index);
            entries[i].type = TYPE_GROUPED;

            if (entries[i - 1].type == 0x00) {
              channel1 |= (0x1 << entries[i - 1].index);
              entries[i - 1].type = TYPE_GROUPED;
            }

            // group TV as well if there is one
            if (((i + 1) < num_outputs) && (entries[i + 1].type == 0x1)) {
              channel1 |= (0x1 << entries[i + 1].index);
              entries[i + 1].type = TYPE_GROUPED;
            }
            break;

          case 1:
            channel2 |= (0x1 << entries[i].index);
            entries[i].type = TYPE_GROUPED;

            if (entries[i - 1].type == 0x00) {
              channel2 |= (0x1 << entries[i - 1].index);
              entries[i - 1].type = TYPE_GROUPED;
            }

            // group TV as well if there is one
            if (((i + 1) < num_outputs) && (entries[i + 1].type == 0x1)) {
              channel2 |= (0x1 << entries[i + 1].index);
              entries[i + 1].type = TYPE_GROUPED;
            }
            break;

          default:
            break;
          }

          break;
        }
      }
    }
  }

  // if we have left ungrouped outputs merge them to the empty channel
  togroup = &channel2;  // = (channel1 ? (channel2 ? NULL : &channel2) : &channel1);

  for (i = 0; i < num_outputs; i++) {
    if (entries[i].type != TYPE_GROUPED) {
      if (togroup) {
        *togroup |= (1 << entries[i].index);
      }

      entries[i].type = TYPE_GROUPED;
    }
  }

  if (channel1 > channel2) {
    UINT8 buff = channel1;

    channel1 = channel2;
    channel2 = buff;
  }

  default_NVCAP[6] = channel1;
  default_NVCAP[8] = channel2;

  // patching HEADS
  for (i = 0; i < num_outputs; i++) {
    if (channel1 & (1 << i)) {
      *entries[i].heads = 1;
    }
    else if (channel2 & (1 << i)) {
      *entries[i].heads = 2;
    }
  }

  return (has_lvds ? PATCH_ROM_SUCCESS_HAS_LVDS : PATCH_ROM_SUCCESS);
}

CHAR8 *
get_nvidia_model (
  UINT32 id
)
{
  INT32 i;
  CHAR8 *name;

  for (i = 1; i < ARRAY_SIZE (NVKnownChipsets); i++) {
    if (NVKnownChipsets[i].device == id) {
      UINTN size = AsciiStrLen (NVKnownChipsets[i].name);

      name = AllocatePool (size + 1);
      AsciiStrnCpy (name, NVKnownChipsets[i].name, size);
      name[size] = 0;
      return name;
    }
  }

  name = AllocatePool (80);
  AsciiSPrint (name, 80, "Unknown");
  return name;
}

INT32
devprop_add_nvidia_template (
  DevPropDevice * device
)
{
  CHAR8 tmp[16];

  if (device == NULL) {
    return 0;
  }

  if (!DP_ADD_TEMP_VAL (device, nvidia_compatible_0)) {
    return 0;
  }

  if (!DP_ADD_TEMP_VAL (device, nvidia_device_type_0)) {
    return 0;
  }

  if (!DP_ADD_TEMP_VAL (device, nvidia_name_0)) {
    return 0;
  }

  if (!DP_ADD_TEMP_VAL (device, nvidia_compatible_1)) {
    return 0;
  }

  if (!DP_ADD_TEMP_VAL (device, nvidia_device_type_1)) {
    return 0;
  }

  if (!DP_ADD_TEMP_VAL (device, nvidia_name_1)) {
    return 0;
  }

  if (devices_number == 1) {
    if (!DP_ADD_TEMP_VAL (device, nvidia_device_type_parent)) {
      return 0;
    }
  }
  else {
    if (!DP_ADD_TEMP_VAL (device, nvidia_device_type_child)) {
      return 0;
    }
  }

  AsciiSPrint (tmp, 16, "Slot-%x\0", devices_number);
  devprop_add_value (device, "AAPL,slot-name", (UINT8 *) tmp,
                     (UINT32) (AsciiStrLen (tmp) + 1));
  devices_number++;
  return 1;
}

UINT32
mem_detect (
  UINT8 nvCardType,
  pci_dt_t * nvda_dev
)
{
  UINT64 vram_size;

  vram_size = 0;

  if (nvCardType < NV_ARCH_50) {
    vram_size = REG32 (nvda_dev->regs, NV04_PFB_FIFO_DATA);
    vram_size &= NV10_PFB_FIFO_DATA_RAM_AMOUNT_MB_MASK;
  }
  else if (nvCardType < NV_ARCH_C0) {
    vram_size = REG32 (nvda_dev->regs, NV04_PFB_FIFO_DATA);
    vram_size |= LShiftU64 ((vram_size & 0xff), 32);
    vram_size &= 0xffffffff00ll;
  }
  else {  // >= NV_ARCH_C0
    vram_size =
      LShiftU64 (REG32 (nvda_dev->regs, NVC0_MEM_CTRLR_RAM_AMOUNT), 20);
    vram_size =
      MultU64x32 (vram_size, REG32 (nvda_dev->regs, NVC0_MEM_CTRLR_COUNT));
  }

  // Workaround
  switch (nvda_dev->device_id) {
  case 0x0647: // 9600M GT 0647
  case 0x0649: // 9600M GT 0649
    vram_size = 512 * 1024 * 1024;
    break;
  case 0x0A65: // GT 210
  case 0x0DE0: // GT 440
  case 0x0DE1: // GT 430
  case 0x0DE2: // GT 420
  case 0x0DEC: // GT 525M 0DEC
  case 0x0DF4: // GT 540M
  case 0x0DF5: // GT 525M 0DF5
    vram_size = 1024 * 1024 * 1024;
    break;
  case 0x1251: // GTX 560M
    vram_size = 1536 * 1024 * 1024;
    break;

  default:
    break;
  }

  return (UINT32) vram_size;
}

BOOLEAN
setup_nvidia_devprop (
  pci_dt_t * nvda_dev
)
{
  const INT32 MAX_BIOS_VERSION_LENGTH = 32;
  EFI_STATUS Status;
  CHAR8 *model;
  CHAR8 *version_str;
  CHAR8 *s;
  CHAR8 *s1;
  INT32 nvPatch;
  UINT16 i, j, version_start, crlf_count;
  UINT32 bar[7];
  UINT32 boot_display;
  UINT32 videoRam;
  UINT8 nvCardType;
  UINT8 *rom;
  BOOLEAN isVersion;
  DevPropDevice *device;
  option_rom_pci_header_t *rom_pci_header;
  UINTN bufferLen;
  CHAR16 FileName[40];
  UINT8 *buffer;

  device = NULL;
  nvCardType = 0;
  videoRam = 0;
  boot_display = 0;
  nvPatch = 0;
  crlf_count = 0;
  model = NULL;
  rom = NULL;
  buffer = NULL;
  bufferLen = 0;
  rom_pci_header = NULL;
  version_str = NULL;

  bar[0] = pci_config_read32 (nvda_dev, PCI_BASE_ADDRESS_0);
  nvda_dev->regs = (UINT8 *) (UINTN) (bar[0] & ~0x0f);
  // get card type
  nvCardType = (UINT8) ((REG32 (nvda_dev->regs, 0) >> 20) & 0x1ff);
  // Amount of VRAM in kilobytes
  videoRam = mem_detect (nvCardType, nvda_dev);
  model = get_nvidia_model ((nvda_dev->vendor_id << 16) | nvda_dev->device_id);

  if (gSettings.LoadVBios) {
    if (gPNDirExists) {
      UnicodeSPrint (FileName, sizeof (FileName), L"%srom\\%04x_%04x.rom",
                     gProductNameDir, nvda_dev->vendor_id, nvda_dev->device_id);
    }
    else {
      UnicodeSPrint (FileName, sizeof (FileName),
                     L"\\EFI\\bareboot\\rom\\%04x_%04x.rom",
                     nvda_dev->vendor_id, nvda_dev->device_id);
    }
    Status = egLoadFile (gRootFHandle, FileName, &buffer, &bufferLen);

    if (EFI_ERROR (Status)) {
      goto nofile;
    }

    if (bufferLen == 0) {
      FreeAlignedPages (buffer, EFI_SIZE_TO_PAGES (bufferLen));
      goto nofile;
    }

    rom = AllocateCopyPool (bufferLen, buffer);

    if (rom == NULL) {
      FreeAlignedPages (buffer, EFI_SIZE_TO_PAGES (bufferLen));
    }
  }

nofile:

  if (rom == NULL) {
    rom = AllocateZeroPool (NVIDIA_ROM_SIZE + 1);
    // PRAMIN first
    read_nVidia_PRAMIN (nvda_dev, rom, nvCardType);

    if ((rom[0] != 0x55) || (rom[1] != 0xaa)) {
      read_nVidia_PROM (nvda_dev, rom);
    }
  }

  if (rom[0] == 0x55 && rom[1] == 0xaa) {
    DBG ("NVidia devprop: ROM read successful");
    nvPatch = patch_nvidia_rom (rom);
  }

  rom_pci_header = (option_rom_pci_header_t *) (rom + *(UINT16 *) &rom[24]);

  // check for 'PCIR' sig
  if (rom_pci_header->signature == 0x52494350 /*50434952 */ ) { //for some reason, the reverse byte order
    if (rom_pci_header->device_id != nvda_dev->device_id) {
      // Get Model from the OpROM
      model =
        get_nvidia_model ((rom_pci_header->vendor_id << 16) | rom_pci_header->
                          device_id);
    }
  }

  // only search the first 384 bytes
  for (i = 0; i < 0x180; i++) {
    if (rom[i] == 0x0D && rom[i + 1] == 0x0A) {
      DBG (" CRLF pos = 0x%x", i);
      crlf_count++;
      if (rom[i - 1] == 0x20) {
        i--;  // strip last " "
      }
      for (version_start = i; version_start > (i - MAX_BIOS_VERSION_LENGTH);
           version_start--) {
        if (rom[version_start] == 0x00) {
          isVersion = FALSE;
          if (AsciiStrnCmp
              ((const CHAR8 *) rom + version_start + 1, "Version ", 8) == 0) {
            version_start += 8;
            isVersion = TRUE;
          }
          DBG (", ver pos = 0x%x", version_start);

          if (isVersion || (crlf_count == 2)) {
            version_start++;
            s = (CHAR8 *) (rom + version_start);
            version_str = (CHAR8 *) AllocateZeroPool (i - version_start + 1);
            s1 = version_str;
            for (j = version_start; j < i; j++) {
              *s1++ = *s++;
            }
            *s1 = 0;
            DBG (", version - %a", version_str);
            break;
          }
        }
      }
      if (version_str != NULL) {
        break;
      }
      if (rom[i] == 0x20) {
        i++;
      }
    }
  }
  DBG ("\n");

  if (string == NULL) {
    string = devprop_create_string ();
  }

  device = devprop_add_device_pci (string, nvda_dev);
  devprop_add_nvidia_template (device);
  /* FIXME: for primary graphics card only */
  boot_display = 1;

  devprop_add_value (device, "@0,AAPL,boot-display", (UINT8 *) &boot_display,
                     4);
  if (nvPatch == PATCH_ROM_SUCCESS_HAS_LVDS) {
    UINT8 built_in;

    built_in = 0x01;
    devprop_add_value (device, "@0,built-in", &built_in, 1);
  }
  if ((gSettings.NVCAP[0] != 0)) {
    devprop_add_value (device, "NVCAP", &gSettings.NVCAP[0], NVCAP_LEN);
  }
  else {
    devprop_add_value (device, "NVCAP", default_NVCAP, NVCAP_LEN);
  }
  devprop_add_value (device, "NVPM", default_NVPM, NVPM_LEN);
  if ((gSettings.VRAM != 0)) {
    devprop_add_value (device, "VRAM,totalsize", (UINT8 *) &gSettings.VRAM, 4);
  }
  else {
    devprop_add_value (device, "VRAM,totalsize", (UINT8 *) &videoRam, 4);
  }
  devprop_add_value (device, "model", (UINT8 *) model,
                     ((UINT32) AsciiStrLen (model) + 1));
  if (version_str != NULL) {
    devprop_add_value (device, "rom-revision", (UINT8 *) version_str,
                       ((UINT32) AsciiStrLen (version_str) + 1));
  }
  else {
    devprop_add_value (device, "rom-revision", (UINT8 *) "BB.FAKE.01\0", 11);
  }
  if ((gSettings.Dcfg[0] != 0) && (gSettings.Dcfg[1] != 0)) {
    devprop_add_value (device, "@0,display-cfg", &gSettings.Dcfg[0], DCFG0_LEN);
    devprop_add_value (device, "@1,display-cfg", &gSettings.Dcfg[4], DCFG1_LEN);
  }
  else {
    devprop_add_value (device, "@0,display-cfg", default_dcfg_0, DCFG0_LEN);
    devprop_add_value (device, "@1,display-cfg", default_dcfg_1, DCFG1_LEN);
  }
  devprop_add_value (device, "hda-gfx", (UINT8 *) "onboard-1\0", 10);

  if (rom != NULL) {
    FreePool (rom);
  }
  if (version_str != NULL) {
    FreePool (version_str);
  }
  return TRUE;
}

// ---------------============== ATI injection

BOOLEAN
get_bootdisplay_val (
  value_t * val
)
{
  static UINT32 v = 0;

  if (v) {
    return FALSE;
  }

  if (!card->posted) {
    return FALSE;
  }

  v = 1;
  val->type = kCst;
  val->size = 4;
  val->data = (UINT8 *) &v;
  return TRUE;
}

BOOLEAN
get_dual_link_val (
  value_t * val
)
{
  static UINT32 v = 0;

  if (v) {
    return FALSE;
  }

  v = gSettings.DualLink;
  val->type = kCst;
  val->size = 4;
  val->data = (UINT8 *) &v;

  return TRUE;
}

BOOLEAN
get_edid_val (
  value_t * val
)
{
  static UINT32 v = 0;

  if (v) {
    return FALSE;
  }

  if (gSettings.CustomEDID == NULL) {
    return FALSE;
  }
  v = 1;
  val->type = kPtr;
  val->size = 128;
  val->data = AllocateCopyPool (val->size, gSettings.CustomEDID);
  return TRUE;
}

BOOLEAN
get_vrammemory_val (
  value_t * val
)
{
  return FALSE;
}

BOOLEAN
get_display_type (
  value_t * val
)
{
  dti++;
  if (dti > 3) {
    dti = 0;
  }
  val->type = kStr;
  val->size = 4;
  val->data = (UINT8 *) dtyp[dti];

  return TRUE;
}

BOOLEAN
get_name_val (
  value_t * val
)
{
  val->type = aty_name.type;
  val->size = aty_name.size;
  val->data = aty_name.data;
  return TRUE;
}

BOOLEAN
get_nameparent_val (
  value_t * val
)
{
  val->type = aty_nameparent.type;
  val->size = aty_nameparent.size;
  val->data = aty_nameparent.data;
  return TRUE;
}

BOOLEAN
get_name_pci_val (
  value_t * val
)
{
#if 0
  if (card->info->model_name == NULL || gSettings.FakeATI == 0) {
    return FALSE;
  }
  AsciiSPrint (pciName, 15, "pci1002,%04x", gSettings.FakeATI >> 16);
  LowCase (pciName);
  val->type = kStr;
  val->size = 12;
  val->data = (UINT8 *) &pciName[0];

  return TRUE;
#endif

  return FALSE;
}

BOOLEAN
get_model_val (
  value_t * val
)
{
  if (card->info->model_name == NULL) {
    return FALSE;
  }

  val->type = kStr;
  val->size = (UINT32) AsciiStrLen (card->info->model_name);
  val->data = (UINT8 *) card->info->model_name;
  return TRUE;
}

BOOLEAN
get_conntype_val (
  value_t * val
)
{
  UINT32 *ct;

// Connector types

//0x0002: LVDS  
//0x0004: DVI-D
//0x0010: VGA
//0x0080: SVIDEO
//0x0200: DVI-I
//0x0400: DP
//0x0800: HDMI
//
  if (gSettings.Mobile) {
    ct = (UINT32 *) &ctm[0];
  }
  else {
    ct = (UINT32 *) &ctd[0];
  }

  val->type = kCst;
  val->size = 4;
  val->data = (UINT8 *) &ct[cti];

  cti++;
  if (cti > 3)
    cti = 0;

  return TRUE;
}

BOOLEAN
get_vrammemsize_val (
  value_t * val
)
{
  static INTN idx = -1;
  static UINT64 memsize;

  idx++;
  memsize = LShiftU64 (card->vram_size, 32);

  if (idx == 0) {
    memsize = memsize | (UINT64) card->vram_size;
  }

  val->type = kCst;
  val->size = 8;
  val->data = (UINT8 *) &memsize;
  return TRUE;
}

BOOLEAN
get_binimage_val (
  value_t * val
)
{
  if (card->rom == NULL) {
    return FALSE;
  }

  val->type = kPtr;
  val->size = card->rom_size;
  val->data = card->rom;
  return TRUE;
}

BOOLEAN
get_binimage_owr (
  value_t * val
)
{
  static UINT32 v = 0;

  if (!gSettings.LoadVBios)
    return FALSE;

  v = 1;
  val->type = kCst;
  val->size = 4;
  val->data = (UINT8 *) &v;

  return TRUE;
}

BOOLEAN
get_romrevision_val (
  value_t * val
)
{
  UINT8 *rev;
  CHAR8 *cRev;

  cRev = "109-B77101-00";

  if (card->rom == NULL) {
    val->type = kPtr;
    val->size = 13;
    val->data = AllocatePool (val->size);

    if (val->data == NULL) {
      return FALSE;
    }

    CopyMem (val->data, cRev, val->size);
    return TRUE;
  }

  rev = card->rom + *((UINT8 *) (card->rom + OFFSET_TO_GET_ATOMBIOS_STRINGS_START));
  val->type = kPtr;
  val->size = (UINT32) AsciiStrLen ((CHAR8 *) rev);

  if ((val->size < 3) || (val->size > 30)) {  //fool proof. Real value 13
    rev = (UINT8 *) cRev;
    val->size = 13;
  }

  val->data = AllocatePool (val->size);

  if (val->data == NULL) {
    return FALSE;
  }

  CopyMem (val->data, rev, val->size);
  return TRUE;
}

BOOLEAN
get_deviceid_val (
  value_t * val
)
{
  val->type = kCst;
  val->size = 2;
  val->data = (UINT8 *) &card->pci_dev->device_id;
  return TRUE;
}

BOOLEAN
get_mclk_val (
  value_t * val
)
{
  return FALSE;
}

BOOLEAN
get_sclk_val (
  value_t * val
)
{
  return FALSE;
}

BOOLEAN
get_refclk_val (
  value_t * val
)
{
  return FALSE;
}

BOOLEAN
get_platforminfo_val (
  value_t * val
)
{
  val->data = AllocateZeroPool (0x80);

  if (val->data == NULL) {
    return FALSE;
  }

  val->type = kPtr;
  val->size = 0x80;
  val->data[0] = '\x01';
  return TRUE;
}

BOOLEAN
get_vramtotalsize_val (
  value_t * val
)
{
  val->type = kCst;
  val->size = 4;
  val->data = (UINT8 *) &card->vram_size;
  return TRUE;
}

VOID
clear_val (
  value_t * val
)
{
  if (val->type == kPtr) {
    FreePool (val->data);
  }

  ZeroMem (val, sizeof (value_t));
}

VOID
devprop_add_list (
  AtiDevProp *devprop_list
)
{
  value_t *val;
  AtiDevProp *dpe;
  int pnum;

  val = AllocateZeroPool (sizeof (value_t));

  for (dpe = devprop_list; dpe != NULL && dpe->name != NULL; dpe++) {
    if ((dpe->flags == FLAGTRUE) || (dpe->flags & card->flags)) {
      if (dpe->get_value != NULL && dpe->get_value (val)) {
        devprop_add_value (card->device, dpe->name, val->data, val->size);
        clear_val (val);

        if (dpe->all_ports) {
          for (pnum = 1; pnum < card->ports; pnum++) {
            if (dpe->get_value (val)) {
              dpe->name[1] = (CHAR8) ('0' + pnum);  // convert to ascii
              devprop_add_value (card->device, dpe->name, val->data, val->size);
              clear_val (val);
            }
          }

          dpe->name[1] = '0'; // write back our "@0," for a next possible card
        }
      } else {
        if (dpe->default_val.type != kNul) {
          devprop_add_value (card->device, dpe->name,
                              dpe->default_val.type == kCst ?
                               (UINT8 *) &(dpe->default_val.data) :
                               dpe->default_val.data,
                              dpe->default_val.size);
        }

        if (dpe->all_ports) {
          for (pnum = 1; pnum < card->ports; pnum++) {
            if (dpe->default_val.type != kNul) {
              dpe->name[1] = (CHAR8) ('0' + pnum);  // convert to ascii
              devprop_add_value (card->device, dpe->name,
                                  dpe->default_val.type == kCst ?
                                   (UINT8 *) &(dpe->default_val.data) :
                                   dpe->default_val.data,
                                  dpe->default_val.size);
            }
          }

          dpe->name[1] = '0'; // write back our "@0," for a next possible card
        }
      }
    }
  }

  FreePool (val);
}

BOOLEAN
validate_rom (
  option_rom_header_t * rom_header,
  pci_dt_t * pci_dev
)
{
  option_rom_pci_header_t *rom_pci_header;

  if (rom_header->signature != 0xaa55) {
    return FALSE;
  }

  rom_pci_header =
    (option_rom_pci_header_t *) ((UINT8 *) rom_header +
                                 rom_header->pci_header_offset);

  if (rom_pci_header->signature != 0x52494350) {
    return FALSE;
  }

  if (rom_pci_header->vendor_id != pci_dev->vendor_id ||
      rom_pci_header->device_id != pci_dev->device_id) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
load_vbios_file (
  UINT16 vendor_id,
  UINT16 device_id
)
{
  EFI_STATUS Status;
  UINTN bufferLen;
  CHAR16 FileName[40];
  UINT8 *buffer;

  buffer = NULL;
  card->rom_size = 0;
  card->rom = NULL;

  if (gPNDirExists) {
    UnicodeSPrint (FileName, sizeof (FileName), L"%srom\\%04x_%04x.rom",
                   gProductNameDir, vendor_id, device_id);
  }
  else {
    UnicodeSPrint (FileName, sizeof (FileName),
                   L"\\EFI\\bareboot\\rom\\%04x_%04x.rom", vendor_id,
                   device_id);
  }

  Status = egLoadFile (gRootFHandle, FileName, &buffer, &bufferLen);

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (bufferLen == 0) {
    FreeAlignedPages (buffer, EFI_SIZE_TO_PAGES (bufferLen));
    return FALSE;
  }

  card->rom = AllocateCopyPool (bufferLen, buffer);

  if (card->rom == NULL) {
    FreeAlignedPages (buffer, EFI_SIZE_TO_PAGES (bufferLen));
    return FALSE;
  }
  card->rom_size = (UINT32) bufferLen;

  FreeAlignedPages (buffer, EFI_SIZE_TO_PAGES (bufferLen));

  if (!validate_rom ((option_rom_header_t *) card->rom, card->pci_dev)) {
    FreePool (card->rom);
    card->rom_size = 0;
    card->rom = NULL;
    return FALSE;
  }

  card->rom_size = ((option_rom_header_t *) card->rom)->rom_size * 512;

  return TRUE;
}

VOID
get_vram_size (
  VOID
)
{
  ati_chip_family_t chip_family;

  chip_family = card->info->chip_family;
  card->vram_size = 128 << 20;  //default 128Mb, this is minimum for OS

  if (gSettings.VRAM != 0) {
    card->vram_size = gSettings.VRAM;
  }
  else {
    if (chip_family >= CHIP_FAMILY_CEDAR) {
      // size in MB on evergreen
      // XXX watch for overflow!!!
      card->vram_size = LShiftU64 (REG32 (card->mmio, R600_CONFIG_MEMSIZE), 20);
    }
    else if (chip_family >= CHIP_FAMILY_R600) {
      card->vram_size = REG32 (card->mmio, R600_CONFIG_MEMSIZE);
    }
    else {
      card->vram_size = REG32 (card->mmio, RADEON_CONFIG_MEMSIZE);

      if (card->vram_size == 0) {
        card->vram_size = REG32 (card->mmio, RADEON_CONFIG_APER_SIZE);
        //Slice - previously I successfully made Radeon9000 working
        //by writing this register
        WRITEREG32 (card->mmio, RADEON_CONFIG_MEMSIZE, 0x30000);
      }
    }
  }
  gSettings.VRAM = card->vram_size;
}

BOOLEAN
read_vbios (
  BOOLEAN from_pci
)
{
  option_rom_header_t *rom_addr;

  if (from_pci) {
    rom_addr =
      (option_rom_header_t
       *) (UINTN) (pci_config_read32 (card->pci_dev,
                                      PCI_EXPANSION_ROM_BASE) & ~0x7ff);
  }
  else {
    rom_addr = (option_rom_header_t *) ((UINTN) 0xc0000);
  }

  if (!validate_rom (rom_addr, card->pci_dev)) {
    return FALSE;
  }

  card->rom_size = rom_addr->rom_size * 512;

  if (card->rom_size == 0) {
    return FALSE;
  }

  card->rom = AllocateZeroPool (card->rom_size);

  if (card->rom == NULL) {
    return FALSE;
  }

  CopyMem (card->rom, (void *) rom_addr, card->rom_size);
  return TRUE;
}

BOOLEAN
read_disabled_vbios (
  VOID
)
{
  BOOLEAN ret;
  ati_chip_family_t chip_family;

  ret = FALSE;
  chip_family = card->info->chip_family;

  if (chip_family >= CHIP_FAMILY_RV770) {
    UINT32 viph_control = REG32 (card->mmio, RADEON_VIPH_CONTROL);
    UINT32 bus_cntl = REG32 (card->mmio, RADEON_BUS_CNTL);
    UINT32 d1vga_control = REG32 (card->mmio, AVIVO_D1VGA_CONTROL);
    UINT32 d2vga_control = REG32 (card->mmio, AVIVO_D2VGA_CONTROL);
    UINT32 vga_render_control = REG32 (card->mmio, AVIVO_VGA_RENDER_CONTROL);
    UINT32 rom_cntl = REG32 (card->mmio, R600_ROM_CNTL);
    UINT32 cg_spll_func_cntl = 0;
    UINT32 cg_spll_status;

    // disable VIP
    WRITEREG32 (card->mmio, RADEON_VIPH_CONTROL,
                (viph_control & ~RADEON_VIPH_EN));
    // enable the rom
    WRITEREG32 (card->mmio, RADEON_BUS_CNTL,
                (bus_cntl & ~RADEON_BUS_BIOS_DIS_ROM));
    // Disable VGA mode
    WRITEREG32 (card->mmio, AVIVO_D1VGA_CONTROL,
                (d1vga_control &
                 ~(AVIVO_DVGA_CONTROL_MODE_ENABLE |
                   AVIVO_DVGA_CONTROL_TIMING_SELECT)));
    WRITEREG32 (card->mmio, AVIVO_D2VGA_CONTROL,
                (d2vga_control &
                 ~(AVIVO_DVGA_CONTROL_MODE_ENABLE |
                   AVIVO_DVGA_CONTROL_TIMING_SELECT)));
    WRITEREG32 (card->mmio, AVIVO_VGA_RENDER_CONTROL,
                (vga_render_control & ~AVIVO_VGA_VSTATUS_CNTL_MASK));

    if (chip_family == CHIP_FAMILY_RV730) {
      cg_spll_func_cntl = REG32 (card->mmio, R600_CG_SPLL_FUNC_CNTL);
      // enable bypass mode
      WRITEREG32 (card->mmio, R600_CG_SPLL_FUNC_CNTL,
                  (cg_spll_func_cntl | R600_SPLL_BYPASS_EN));
      // wait for SPLL_CHG_STATUS to change to 1
      cg_spll_status = 0;

      while ((cg_spll_status & R600_SPLL_CHG_STATUS) == 0) {
        cg_spll_status = REG32 (card->mmio, R600_CG_SPLL_STATUS);
      }

      WRITEREG32 (card->mmio, R600_ROM_CNTL, (rom_cntl & ~R600_SCK_OVERWRITE));
    }
    else {
      WRITEREG32 (card->mmio, R600_ROM_CNTL, (rom_cntl | R600_SCK_OVERWRITE));
    }

    ret = read_vbios (TRUE);

    // restore regs
    if (chip_family == CHIP_FAMILY_RV730) {
      WRITEREG32 (card->mmio, R600_CG_SPLL_FUNC_CNTL, cg_spll_func_cntl);
      // wait for SPLL_CHG_STATUS to change to 1
      cg_spll_status = 0;

      while ((cg_spll_status & R600_SPLL_CHG_STATUS) == 0) {
        cg_spll_status = REG32 (card->mmio, R600_CG_SPLL_STATUS);
      }
    }

    WRITEREG32 (card->mmio, RADEON_VIPH_CONTROL, viph_control);
    WRITEREG32 (card->mmio, RADEON_BUS_CNTL, bus_cntl);
    WRITEREG32 (card->mmio, AVIVO_D1VGA_CONTROL, d1vga_control);
    WRITEREG32 (card->mmio, AVIVO_D2VGA_CONTROL, d2vga_control);
    WRITEREG32 (card->mmio, AVIVO_VGA_RENDER_CONTROL, vga_render_control);
    WRITEREG32 (card->mmio, R600_ROM_CNTL, rom_cntl);
  }
  else if (chip_family >= CHIP_FAMILY_R600) {
    UINT32 viph_control = REG32 (card->mmio, RADEON_VIPH_CONTROL);
    UINT32 bus_cntl = REG32 (card->mmio, RADEON_BUS_CNTL);
    UINT32 d1vga_control = REG32 (card->mmio, AVIVO_D1VGA_CONTROL);
    UINT32 d2vga_control = REG32 (card->mmio, AVIVO_D2VGA_CONTROL);
    UINT32 vga_render_control = REG32 (card->mmio, AVIVO_VGA_RENDER_CONTROL);
    UINT32 rom_cntl = REG32 (card->mmio, R600_ROM_CNTL);
    UINT32 general_pwrmgt = REG32 (card->mmio, R600_GENERAL_PWRMGT);
    UINT32 low_vid_lower_gpio_cntl =
      REG32 (card->mmio, R600_LOW_VID_LOWER_GPIO_CNTL);
    UINT32 medium_vid_lower_gpio_cntl =
      REG32 (card->mmio, R600_MEDIUM_VID_LOWER_GPIO_CNTL);
    UINT32 high_vid_lower_gpio_cntl =
      REG32 (card->mmio, R600_HIGH_VID_LOWER_GPIO_CNTL);
    UINT32 ctxsw_vid_lower_gpio_cntl =
      REG32 (card->mmio, R600_CTXSW_VID_LOWER_GPIO_CNTL);
    UINT32 lower_gpio_enable = REG32 (card->mmio, R600_LOWER_GPIO_ENABLE);

    // disable VIP
    WRITEREG32 (card->mmio, RADEON_VIPH_CONTROL,
                (viph_control & ~RADEON_VIPH_EN));
    // enable the rom
    WRITEREG32 (card->mmio, RADEON_BUS_CNTL,
                (bus_cntl & ~RADEON_BUS_BIOS_DIS_ROM));
    // Disable VGA mode
    WRITEREG32 (card->mmio, AVIVO_D1VGA_CONTROL,
                (d1vga_control &
                 ~(AVIVO_DVGA_CONTROL_MODE_ENABLE |
                   AVIVO_DVGA_CONTROL_TIMING_SELECT)));
    WRITEREG32 (card->mmio, AVIVO_D2VGA_CONTROL,
                (d2vga_control &
                 ~(AVIVO_DVGA_CONTROL_MODE_ENABLE |
                   AVIVO_DVGA_CONTROL_TIMING_SELECT)));
    WRITEREG32 (card->mmio, AVIVO_VGA_RENDER_CONTROL,
                (vga_render_control & ~AVIVO_VGA_VSTATUS_CNTL_MASK));
    WRITEREG32 (card->mmio, R600_ROM_CNTL,
                ((rom_cntl & ~R600_SCK_PRESCALE_CRYSTAL_CLK_MASK) |
                 (1 << R600_SCK_PRESCALE_CRYSTAL_CLK_SHIFT) |
                 R600_SCK_OVERWRITE));
    WRITEREG32 (card->mmio, R600_GENERAL_PWRMGT,
                (general_pwrmgt & ~R600_OPEN_DRAIN_PADS));
    WRITEREG32 (card->mmio, R600_LOW_VID_LOWER_GPIO_CNTL,
                (low_vid_lower_gpio_cntl & ~0x400));
    WRITEREG32 (card->mmio, R600_MEDIUM_VID_LOWER_GPIO_CNTL,
                (medium_vid_lower_gpio_cntl & ~0x400));
    WRITEREG32 (card->mmio, R600_HIGH_VID_LOWER_GPIO_CNTL,
                (high_vid_lower_gpio_cntl & ~0x400));
    WRITEREG32 (card->mmio, R600_CTXSW_VID_LOWER_GPIO_CNTL,
                (ctxsw_vid_lower_gpio_cntl & ~0x400));
    WRITEREG32 (card->mmio, R600_LOWER_GPIO_ENABLE,
                (lower_gpio_enable | 0x400));
    ret = read_vbios (TRUE);
    // restore regs
    WRITEREG32 (card->mmio, RADEON_VIPH_CONTROL, viph_control);
    WRITEREG32 (card->mmio, RADEON_BUS_CNTL, bus_cntl);
    WRITEREG32 (card->mmio, AVIVO_D1VGA_CONTROL, d1vga_control);
    WRITEREG32 (card->mmio, AVIVO_D2VGA_CONTROL, d2vga_control);
    WRITEREG32 (card->mmio, AVIVO_VGA_RENDER_CONTROL, vga_render_control);
    WRITEREG32 (card->mmio, R600_ROM_CNTL, rom_cntl);
    WRITEREG32 (card->mmio, R600_GENERAL_PWRMGT, general_pwrmgt);
    WRITEREG32 (card->mmio, R600_LOW_VID_LOWER_GPIO_CNTL,
                low_vid_lower_gpio_cntl);
    WRITEREG32 (card->mmio, R600_MEDIUM_VID_LOWER_GPIO_CNTL,
                medium_vid_lower_gpio_cntl);
    WRITEREG32 (card->mmio, R600_HIGH_VID_LOWER_GPIO_CNTL,
                high_vid_lower_gpio_cntl);
    WRITEREG32 (card->mmio, R600_CTXSW_VID_LOWER_GPIO_CNTL,
                ctxsw_vid_lower_gpio_cntl);
    WRITEREG32 (card->mmio, R600_LOWER_GPIO_ENABLE, lower_gpio_enable);
  }

  return ret;
}

BOOLEAN
radeon_card_posted (
  VOID
)
{
  UINT32 reg;

  // first check CRTCs
  reg =
    REG32 (card->mmio, RADEON_CRTC_GEN_CNTL) | REG32 (card->mmio,
                                                      RADEON_CRTC2_GEN_CNTL);

  if (reg & RADEON_CRTC_EN) {
    return TRUE;
  }

  // then check MEM_SIZE, in case something turned the crtcs off
  reg = REG32 (card->mmio, R600_CONFIG_MEMSIZE);

  if (reg) {
    return TRUE;
  }

  return FALSE;
}

BOOLEAN
init_card (
  pci_dt_t * pci_dev
)
{
  CHAR8 *name;
  CHAR8 *name_parent;
  CHAR8 *CfgName;
  INTN NameLen, n_ports;
  UINTN i;
  BOOLEAN add_vbios = TRUE;

  NameLen = 0;
  n_ports = 0;
  add_vbios = gSettings.LoadVBios;

  card = AllocateZeroPool (sizeof (card_t));

  if (card == NULL) {
    return FALSE;
  }

  card->pci_dev = pci_dev;

  for (i = 0; radeon_cards[i].device_id; i++) {
    if (radeon_cards[i].device_id == pci_dev->device_id) {
      card->info = &radeon_cards[i];
      break;
    }
  }

  if (card->info->device_id == 0 || card->info->cfg_name == kNull) {
    for (i = 0; radeon_cards[i].device_id; i++) {
      if ((radeon_cards[i].device_id & ~0xf) == (pci_dev->device_id & ~0xf)) {
        card->info = &radeon_cards[i];
        break;
      }
    }

    if (card->info->cfg_name == kNull) {
      return FALSE;
    }
  }

  card->fb =
    (UINT8 *) (UINTN) (pci_config_read32 (pci_dev, PCI_BASE_ADDRESS_0) & ~0x0f);
  card->mmio =
    (UINT8 *) (UINTN) (pci_config_read32 (pci_dev, PCI_BASE_ADDRESS_2) & ~0x0f);
  card->io =
    (UINT8 *) (UINTN) (pci_config_read32 (pci_dev, PCI_BASE_ADDRESS_4) & ~0x03);
  pci_dev->regs = card->mmio;
  card->posted = radeon_card_posted ();
  get_vram_size ();

  if (add_vbios) {
    load_vbios_file (pci_dev->vendor_id, pci_dev->device_id);

    if (card->rom == NULL) {
      if (card->posted) {
        read_vbios (FALSE);
      }
      else {
        read_disabled_vbios ();
      }
    }
  }

  if (card->info->chip_family >= CHIP_FAMILY_CEDAR) {
    card->flags |= EVERGREEN;
  }
  if (card->info->chip_family <= CHIP_FAMILY_RV670) {
    card->flags |= FLAGOLD;
  }
  if (gSettings.Mobile) {
    card->flags |= FLAGMOBILE;
  }

  NameLen = StrLen (gSettings.FBName);

  if (NameLen > 2) {
    CfgName = AllocateZeroPool (NameLen);
    UnicodeStrToAsciiStr ((CHAR16 *) &gSettings.FBName[0], CfgName);
    card->cfg_name = CfgName;
  }
  else {
    // use cfg_name on radeon_cards, to retrive the default name from card_configs,
    card->cfg_name = card_configs[card->info->cfg_name].name;
    n_ports = card_configs[card->info->cfg_name].ports;
    // which means one of the fb's or kNull
  }

  if (gSettings.VideoPorts > 0) {
    n_ports = gSettings.VideoPorts;
  }

  if (n_ports > 0) {
    card->ports = (UINT8) n_ports;
  }
  else {
    // else, match cfg_name with card_configs list and retrive default nr of ports.
    for (i = 0; i < kCfgEnd; i++) {
      if (AsciiStrCmp (card->cfg_name, card_configs[i].name) == 0) {
        card->ports = card_configs[i].ports;  // default
      }
    }
  }

  if (card->ports == 0) {
    card->ports = 2;  //real minimum
  }

  name = AllocateZeroPool (24);
  AsciiSPrint (name, 24, "ATY,%a", card->cfg_name);
  aty_name.type = kStr;
  aty_name.size = (UINT32) AsciiStrLen (name);
  aty_name.data = (UINT8 *) name;
  name_parent = AllocateZeroPool (24);
  AsciiSPrint (name_parent, 24, "ATY,%aParent", card->cfg_name);
  aty_nameparent.type = kStr;
  aty_nameparent.size = (UINT32) AsciiStrLen (name_parent);
  aty_nameparent.data = (UINT8 *) name_parent;
  //how can we free pool when we leave the procedure? Make all pointers global?
  return TRUE;
}

BOOLEAN
setup_ati_devprop (
  pci_dt_t * ati_dev
)
{

  if (!init_card (ati_dev)) {
    return FALSE;
  }

  if (string == NULL) {
    string = devprop_create_string ();
  }

  card->device = devprop_add_device_pci (string, ati_dev);

  if (!card->device) {
    return FALSE;
  }

  devprop_add_list (ati_devprop_list);
  devprop_add_value (card->device, "hda-gfx", (UINT8 *) "onboard-1", 9);

  FreePool (card);
  return TRUE;
}

// ---------------============== GMA injection
CHAR8 *
get_gma_model (
  UINT32 id
)
{
  INT32 i;

  for (i = 0; i < ARRAY_SIZE (KnownGPUS); i++) {
    if (KnownGPUS[i].device == id) {
      return KnownGPUS[i].name;
    }
  }

  return KnownGPUS[0].name;
}

BOOLEAN
setup_gma_devprop (
  pci_dt_t * gma_dev
)
{
  DevPropDevice *device;
  CHAR8 *model;
  UINT8 BuiltIn;
  UINT16 DualLink;
  UINT8 ClassFix[4] = { 0x00, 0x00, 0x03, 0x00 };

  BuiltIn = 0;

  model = get_gma_model ((gma_dev->vendor_id << 16) | gma_dev->device_id);

  if (string == NULL) {
    string = devprop_create_string ();
  }

  device = devprop_add_device_pci (string, gma_dev);

  if (device == NULL) {
    return FALSE;
  }

  DualLink = gSettings.DualLink;
  devprop_add_value (device, "model", (UINT8 *) model,
                     (UINT32) (AsciiStrLen (model) + 1));
  devprop_add_value (device, "device_type", (UINT8 *) "display\0", 8);
  devprop_add_value (device, "subsystem-vendor-id", GMAX3100_vals[21], 4);
  devprop_add_value (device, "hda-gfx", (UINT8 *) "onboard-1\0", 10);

  switch (gma_dev->device_id) {
  case 0x0102:
    devprop_add_value (device, "class-code", ClassFix, 4);
  case 0x0112:
  case 0x0116:
  case 0x0122:
  case 0x0126:
  case 0x0152:
  case 0x0156:
  case 0x0162:
  case 0x0166:
  case 0x016a:
  case 0x0412:
  case 0x0416:
    if ((gma_dev->device_id == 0x162) || (gma_dev->device_id == 0x16a)) {
      devprop_add_value (device, "AAPL,ig-platform-id", GMAX3100_vals[23], 4);
      devprop_add_value (device, "class-code", ClassFix, 4);
    }
    else if (gma_dev->device_id == 0x166)
      devprop_add_value (device, "AAPL,ig-platform-id", GMAX3100_vals[25], 4);
    else if (gma_dev->device_id == 0x152)
      devprop_add_value (device, "AAPL,ig-platform-id", GMAX3100_vals[24], 4);
    else if (gma_dev->device_id == 0x156)
      devprop_add_value (device, "AAPL,ig-platform-id", GMAX3100_vals[25], 4);
    else if (gma_dev->device_id == 0x412)
      devprop_add_value (device, "AAPL,ig-platform-id", GMAX3100_vals[26], 4);
    else if (gma_dev->device_id == 0x416)
      devprop_add_value (device, "AAPL,ig-platform-id", GMAX3100_vals[26], 4);

  case 0xA011:
  case 0xA012:
    if (DualLink != 0) {
      devprop_add_value (device, "AAPL00,DualLink", (UINT8 *) &DualLink, 1);
    }
  case 0x2582:
  case 0x2592:
  case 0x27A2:
  case 0x27AE:
    devprop_add_value (device, "AAPL,HasPanel", reg_TRUE, 4);
    devprop_add_value (device, "built-in", &BuiltIn, 1);

  case 0x2772:
  case 0x29C2:
  case 0x0044:
  case 0x0046:
  case 0xA002:
    devprop_add_value (device, "built-in", &BuiltIn, 1);
    devprop_add_value (device, "AAPL00,DualLink", (UINT8 *) &DualLink, 1);
    break;

  case 0x2A02:
  case 0x2A12:
  case 0x2A42:
    devprop_add_value (device, "AAPL,HasPanel", GMAX3100_vals[0], 4);
    devprop_add_value (device, "AAPL,SelfRefreshSupported", GMAX3100_vals[1],
                       4);
    devprop_add_value (device, "AAPL,aux-power-connected", GMAX3100_vals[2], 4);
    devprop_add_value (device, "AAPL,backlight-control", GMAX3100_vals[3], 4);
    devprop_add_value (device, "AAPL00,blackscreen-preferences",
                       GMAX3100_vals[4], 4);
    devprop_add_value (device, "AAPL01,BacklightIntensity", GMAX3100_vals[5],
                       4);
    devprop_add_value (device, "AAPL01,blackscreen-preferences",
                       GMAX3100_vals[6], 4);
    devprop_add_value (device, "AAPL01,DataJustify", GMAX3100_vals[7], 4);
#if 0
    devprop_add_value (device, "AAPL01,Depth", GMAX3100_vals[8], 4);
#endif
    devprop_add_value (device, "AAPL01,Dither", GMAX3100_vals[9], 4);
    devprop_add_value (device, "AAPL01,DualLink", (UINT8 *) &DualLink, 1);
#if 0
    devprop_add_value (device, "AAPL01,Height", GMAX3100_vals[10], 4);
#endif
    devprop_add_value (device, "AAPL01,Interlace", GMAX3100_vals[11], 4);
    devprop_add_value (device, "AAPL01,Inverter", GMAX3100_vals[12], 4);
    devprop_add_value (device, "AAPL01,InverterCurrent", GMAX3100_vals[13], 4);
#if 0
    devprop_add_value (device, "AAPL01,InverterCurrency", GMAX3100_vals[15], 4);
#endif
    devprop_add_value (device, "AAPL01,LinkFormat", GMAX3100_vals[14], 4);
    devprop_add_value (device, "AAPL01,LinkType", GMAX3100_vals[15], 4);
    devprop_add_value (device, "AAPL01,Pipe", GMAX3100_vals[16], 4);
#if 0
    devprop_add_value (device, "AAPL01,PixelFormat", GMAX3100_vals[17], 4);
#endif
    devprop_add_value (device, "AAPL01,Refresh", GMAX3100_vals[18], 4);
    devprop_add_value (device, "AAPL01,Stretch", GMAX3100_vals[19], 4);
    devprop_add_value (device, "AAPL01,InverterFrequency", GMAX3100_vals[20],
                       4);
#if 0
    devprop_add_value (device, "class-code", ClassFix, 4);
    devprop_add_value (device, "subsystem-vendor-id", GMAX3100_vals[21], 4);
#endif
    devprop_add_value (device, "subsystem-id", GMAX3100_vals[22], 4);
    devprop_add_value (device, "built-in", &BuiltIn, 1);
    break;

  default:
    DBG ("Intel card id=%x unsupported\n", gma_dev->device_id);
    return FALSE;
  }

  return TRUE;
}

VOID
SetDevices (
  VOID
)
{
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00 Pci;
  UINTN HandleCount;
  UINTN ArrayCount;
  UINTN HandleIndex;
  UINTN ProtocolIndex;
  EFI_HANDLE *HandleBuffer;
  EFI_GUID **ProtocolGuidArray;
  pci_dt_t PCIdevice;
  UINTN Segment;
  UINTN Bus;
  UINTN Device;
  UINTN Function;
  BOOLEAN StringDirty = FALSE;
  BOOLEAN TmpDirty = FALSE;

  /* Read Pci Bus for GFX */
  Status =
    gBS->LocateHandleBuffer (AllHandles, NULL, NULL, &HandleCount,
                             &HandleBuffer);

  if (!EFI_ERROR (Status)) {
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      Status =
        gBS->ProtocolsPerHandle (HandleBuffer[HandleIndex], &ProtocolGuidArray,
                                 &ArrayCount);

      if (!EFI_ERROR (Status)) {
        for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {
          if (CompareGuid
              (&gEfiPciIoProtocolGuid, ProtocolGuidArray[ProtocolIndex])) {
            Status =
              gBS->OpenProtocol (HandleBuffer[HandleIndex],
                                 &gEfiPciIoProtocolGuid, (VOID **) &PciIo,
                                 gImageHandle, NULL,
                                 EFI_OPEN_PROTOCOL_GET_PROTOCOL);

            if (EFI_ERROR (Status)) {
              continue;
            }

            ZeroMem ((UINT8 *) &Pci, sizeof (PCI_TYPE00));  //paranoia
            Status =
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0,
                               sizeof (Pci) / sizeof (UINT32), &Pci);

            if (EFI_ERROR (Status)) {
              continue;
            }

            Status =
              PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);
            PCIdevice.DeviceHandle = HandleBuffer[HandleIndex];
            PCIdevice.dev.addr = (UINT32) PCIADDR (Bus, Device, Function);
            PCIdevice.vendor_id = Pci.Hdr.VendorId;
            PCIdevice.device_id = Pci.Hdr.DeviceId;
            PCIdevice.revision = Pci.Hdr.RevisionID;
            PCIdevice.subclass = Pci.Hdr.ClassCode[0];
            PCIdevice.class_id = *((UINT16 *) (Pci.Hdr.ClassCode + 1));
            PCIdevice.subsys_id.subsys.vendor_id = Pci.Device.SubsystemVendorID;
            PCIdevice.subsys_id.subsys.device_id = Pci.Device.SubsystemID;

            // GFX
            if (gSettings.GraphicsInjector &&
                (Pci.Hdr.ClassCode[2] == PCI_CLASS_DISPLAY) &&
                (Pci.Hdr.ClassCode[1] == PCI_CLASS_DISPLAY_VGA)) {
              gGraphics.DeviceID = Pci.Hdr.DeviceId;

              switch (Pci.Hdr.VendorId) {
              case 0x1002:
                DBG ("Device Inject: ATI\n");
                gGraphics.Vendor = Ati;
                TmpDirty = setup_ati_devprop (&PCIdevice);
                StringDirty |= TmpDirty;
                break;

              case 0x8086:
                DBG ("Device Inject: Intel GMA\n");
                gGraphics.Vendor = Intel;
                TmpDirty = setup_gma_devprop (&PCIdevice);
                StringDirty |= TmpDirty;
                break;

              case 0x10de:
                DBG ("Device Inject: nVidia\n");
                gGraphics.Vendor = Nvidia;
                TmpDirty = setup_nvidia_devprop (&PCIdevice);
                StringDirty |= TmpDirty;
                break;

              default:
                break;
              }
            }
            //LAN
            else if (gSettings.ETHInjection &&
                     (Pci.Hdr.ClassCode[2] == PCI_CLASS_NETWORK) &&
                     (Pci.Hdr.ClassCode[1] == PCI_CLASS_NETWORK_ETHERNET)) {
              DBG ("Device Inject: LAN\n");
              TmpDirty = set_eth_props (&PCIdevice);
              StringDirty |= TmpDirty;
            }
            //USB
            else if (gSettings.USBInjection &&
                     (Pci.Hdr.ClassCode[2] == PCI_CLASS_SERIAL) &&
                     (Pci.Hdr.ClassCode[1] == PCI_CLASS_SERIAL_USB)) {
              DBG ("Device Inject: USB\n");
              TmpDirty = set_usb_props (&PCIdevice);
              StringDirty |= TmpDirty;
            }
            // HDA
            else if ((gSettings.HDALayoutId != 0) &&
                     (Pci.Hdr.ClassCode[2] == PCI_CLASS_MEDIA) &&
                     (Pci.Hdr.ClassCode[1] == PCI_CLASS_MEDIA_HDA)) {
              DBG ("Device Inject: HDA LayoutId = %d\n", gSettings.HDALayoutId);
              TmpDirty = set_hda_props (PciIo, &PCIdevice);
              StringDirty |= TmpDirty;
            }
          }
        }
      }
    }
  }

  if (StringDirty) {
    stringlength = string->length * 2;
    gDevProp = AllocateAlignedPages (EFI_SIZE_TO_PAGES (stringlength + 1), 64);
    CopyMem (gDevProp, (VOID *) devprop_generate_string (string), stringlength);
    gDevProp[stringlength] = 0;
    StringDirty = FALSE;
    DBG ("Device Inject: gDevProp = <%a>\n", gDevProp);
  }
}
