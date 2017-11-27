#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Uefi/UefiBaseType.h>

VOID
EraseGuid (
  IN OUT EFI_GUID *Guid
)
{
  ZeroMem (Guid, sizeof (*Guid));
}

/*
 * RFC4112 was used
 */

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

  Status = AsciiStrToGuid (Str, Guid);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Guid->Data1 = SwapBytes32 (Guid->Data1);
  Guid->Data2 = SwapBytes16 (Guid->Data2);
  Guid->Data3 = SwapBytes16 (Guid->Data3);

  return EFI_SUCCESS;
}
