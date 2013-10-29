/** @file
  Default instance of ShiftKeys library.
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/ShiftKeysLib.h>

UINT32
EFIAPI
ShiftKeyPressed (
  VOID
  )
{
  EFI_TPL                           OldTpl;
  UINT8                             KbFlag1;  // 0040h:0017h - KEYBOARD - STATUS FLAGS 1
  UINT8                             KbFlag2;  // 0040h:0018h - KEYBOARD - STATUS FLAGS 2
  UINT32                            KeyShiftState;
  
  KeyShiftState = EFI_SHIFT_STATE_VALID;
  
  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  
  KbFlag1 = *((UINT8 *) (UINTN) 0x417); // read STATUS FLAGS 1
  KbFlag2 = *((UINT8 *) (UINTN) 0x418); // read STATUS FLAGS 2
  //
  // Check shift state
  //
  if ((KbFlag1 & KB_ALT_PRESSED) == KB_ALT_PRESSED) {
    KeyShiftState  |= ((KbFlag2 & KB_LEFT_ALT_PRESSED) == KB_LEFT_ALT_PRESSED) ? EFI_LEFT_ALT_PRESSED : EFI_RIGHT_ALT_PRESSED;
  }
  if ((KbFlag1 & KB_CTRL_PRESSED) == KB_CTRL_PRESSED) {
    KeyShiftState  |= ((KbFlag2 & KB_LEFT_CTRL_PRESSED) == KB_LEFT_CTRL_PRESSED) ? EFI_LEFT_CONTROL_PRESSED : EFI_RIGHT_CONTROL_PRESSED;
  }
  if ((KbFlag1 & KB_LEFT_SHIFT_PRESSED) == KB_LEFT_SHIFT_PRESSED) {
    KeyShiftState  |= EFI_LEFT_SHIFT_PRESSED;
  }
  if ((KbFlag1 & KB_RIGHT_SHIFT_PRESSED) == KB_RIGHT_SHIFT_PRESSED) {
    KeyShiftState  |= EFI_RIGHT_SHIFT_PRESSED;
  }
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
  
  return KeyShiftState;
}

