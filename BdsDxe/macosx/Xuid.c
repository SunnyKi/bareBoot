#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

EFI_GUID gNil_Guid                    = { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};

VOID
EraseGuid (
  OUT EFI_GUID *Guid
)
{
  ZeroMem (Guid, sizeof (*Guid));
}

BOOLEAN
IsGuidValid (
  IN EFI_GUID *Guid
)
{
  return CompareMem (Guid, &gNil_Guid, sizeof (EFI_GUID)) == 0 ? FALSE : TRUE;
}

/*
 * RFC4112 was used
 */

/*
 * Returns TRUE if Str is in format xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
 */

BOOLEAN
IsValidUuidAsciiString (
  IN CHAR8 *Str
)
{
  UINTN i;
  CHAR8 c;
  
  if (Str == NULL || AsciiStrLen(Str) != 36) {
    return FALSE;
  }
  
  for (i = 0; i < 36; i++, Str++) {
    c = *Str;
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (c != '-') { return FALSE; }
      continue;
    }
    if ((c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F')
       ) {
        continue;
    }
    return FALSE;
  }
  return TRUE;
}

/**
 Converts ascii UUID string to binary value.
 UUID text format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
                     012345678901234567890123456789012345

 For uuid the three first fields are big endian.

 @param Str              UUID text format string that contains the UUID value.
 @param Guid             A pointer to the memory for binary value.

 @retval EFI_SUCCESS     The UUID string was successfully converted.
 @retval EFI_UNSUPPORTED The input(s) is not in right format.
 **/

EFI_STATUS
AsciiStrUuidToBinary (
  IN CHAR8 *Str,
  OUT EFI_GUID *Guid
)
{
  static const unsigned char ub[16] = { 3, 2, 1, 0, /*-*/ 5,  4, /*-*/  7,  6, /*-*/  8,  9, /*-*/ 10, 11, 12, 13, 14, 15 };
  static const unsigned char sp[16] = { 0, 2, 4, 6, /*-*/ 9, 11, /*-*/ 14, 16, /*-*/ 19, 21, /*-*/ 24, 26, 28, 30, 32, 34 };

  UINTN i;
  UINT8 *up;
  CHAR8 cb[4];

  if (Guid == NULL) {
    return EFI_UNSUPPORTED;
  }

  EraseGuid (Guid);

  if (!IsValidUuidAsciiString (Str)) {
    return EFI_UNSUPPORTED;
  }

  up = (UINT8 *) Guid;
  cb[2] = '\0';

  for (i = 0; i < 16; i++) {
    CopyMem (cb, Str + sp[i], 2);
    up[ub[i]] = (UINT8) AsciiStrHexToUintn (cb);
  }
  return EFI_SUCCESS;
}

/**
 Converts UUID like string to binary value.
 UUID text format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx

 @param Str              UUID text format string that contains
                         linear byte hex dump of the UUID value.
 @param Guid             A pointer to the memory for binary value.

 @retval EFI_SUCCESS     The conversion was successful.
 @retval EFI_UNSUPPORTED The input(s) is not in right format.
 **/

EFI_STATUS
AsciiStrXuidToBinary (
  IN CHAR8 *Str,
  OUT EFI_GUID *Guid
)
{
  EFI_STATUS Status;

  Status = AsciiStrUuidToBinary (Str, Guid);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Guid->Data1 = SwapBytes32 (Guid->Data1);
  Guid->Data2 = SwapBytes16 (Guid->Data2);
  Guid->Data3 = SwapBytes16 (Guid->Data3);

  return EFI_SUCCESS;
}
