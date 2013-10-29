/** @file
    Provides simple log services to memory buffer.
**/

#ifndef __SHIFTKEYS_LIB_H__
#define __SHIFTKEYS_LIB_H__

#define EFI_SHIFT_STATE_VALID     0x80000000
#define EFI_RIGHT_SHIFT_PRESSED   0x00000001
#define EFI_LEFT_SHIFT_PRESSED    0x00000002
#define EFI_RIGHT_CONTROL_PRESSED 0x00000004
#define EFI_LEFT_CONTROL_PRESSED  0x00000008
#define EFI_RIGHT_ALT_PRESSED     0x00000010
#define EFI_LEFT_ALT_PRESSED      0x00000020
#define KB_ALT_PRESSED            (0x1 << 3)
#define KB_CTRL_PRESSED           (0x1 << 2)
#define KB_LEFT_SHIFT_PRESSED     (0x1 << 1)
#define KB_RIGHT_SHIFT_PRESSED    (0x1 << 0)
#define KB_LEFT_ALT_PRESSED       (0x1 << 1)
#define KB_LEFT_CTRL_PRESSED      (0x1 << 0)

UINT32
EFIAPI
ShiftKeyPressed (
  VOID
  );


#endif // __SHIFTKEYS_LIB_H__
