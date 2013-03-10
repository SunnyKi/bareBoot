/** @file
  FrontPage routines to handle the callbacks and browser calls

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Bds.h"
#include "FrontPage.h"
#include "BootManager.h"
#if 0
#include "Hotkey.h"
#endif

BOOLEAN   mSetupModeInitialized = FALSE;
UINT32    mSetupTextModeColumn;
UINT32    mSetupTextModeRow;
UINT32    mSetupHorizontalResolution;
UINT32    mSetupVerticalResolution;

BOOLEAN   gConnectAllHappened = FALSE;
UINTN     gCallbackKey;
/**
 This function allows a caller to extract the current configuration for one
 or more named elements from the target driver.
 
 
 @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
 @param Request         A null-terminated Unicode string in <ConfigRequest> format.
 @param Progress        On return, points to a character in the Request string.
 Points to the string's null terminator if request was successful.
 Points to the most recent '&' before the first failing name/value
 pair (or the beginning of the string if the failure is in the
 first name/value pair) if the request was not successful.
 @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
 has all values filled in for the names in the Request string.
 String to be allocated by the called function.
 
 @retval  EFI_SUCCESS            The Results is filled with the requested values.
 @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
 @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
 @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.
 
 **/
EFI_STATUS
EFIAPI
FakeExtractConfig (
                   IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
                   IN  CONST EFI_STRING                       Request,
                   OUT EFI_STRING                             *Progress,
                   OUT EFI_STRING                             *Results
                   )
{
  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
 This function processes the results of changes in configuration.
 
 
 @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
 @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
 @param Progress        A pointer to a string filled in with the offset of the most
 recent '&' before the first failing name/value pair (or the
 beginning of the string if the failure is in the first
 name/value pair) or the terminating NULL if all was successful.
 
 @retval  EFI_SUCCESS            The Results is processed successfully.
 @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
 @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.
 
 **/
EFI_STATUS
EFIAPI
FakeRouteConfig (
                 IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
                 IN  CONST EFI_STRING                       Configuration,
                 OUT EFI_STRING                             *Progress
                 )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  *Progress = Configuration;
//  if (!HiiIsConfigHdrMatch (Configuration, &gBootMaintFormSetGuid, mBootMaintStorageName)
//      && !HiiIsConfigHdrMatch (Configuration, &gFileExploreFormSetGuid, mFileExplorerStorageName)) {
//    return EFI_NOT_FOUND;
//  }
  
  *Progress = Configuration + StrLen (Configuration);
  return EFI_SUCCESS;
}

/**
  Function waits for a given event to fire, or for an optional timeout to expire.

  @param   Event              The event to wait for
  @param   Timeout            An optional timeout value in 100 ns units.

  @retval  EFI_SUCCESS      Event fired before Timeout expired.
  @retval  EFI_TIME_OUT     Timout expired before Event fired..

**/
EFI_STATUS
WaitForSingleEvent (
  IN EFI_EVENT                  Event,
  IN UINT64                     Timeout OPTIONAL
  )
{
  UINTN       Index;
  EFI_STATUS  Status;
  EFI_EVENT   TimerEvent;
  EFI_EVENT   WaitList[2];

  if (Timeout != 0) {
    //
    // Create a timer event
    //
    Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
    if (!EFI_ERROR (Status)) {
      //
      // Set the timer event
      //
      gBS->SetTimer (
             TimerEvent,
             TimerRelative,
             Timeout
             );

      //
      // Wait for the original event or the timer
      //
      WaitList[0] = Event;
      WaitList[1] = TimerEvent;
      Status      = gBS->WaitForEvent (2, WaitList, &Index);
      gBS->CloseEvent (TimerEvent);

      //
      // If the timer expired, change the return to timed out
      //
      if (!EFI_ERROR (Status) && Index == 1) {
        Status = EFI_TIMEOUT;
      }
    }
  } else {
    //
    // No timeout... just wait on the event
    //
    Status = gBS->WaitForEvent (1, &Event, &Index);
    ASSERT (!EFI_ERROR (Status));
    ASSERT (Index == 0);
  }

  return Status;
}

/**
  Function show progress bar to wait for user input.


  @param   TimeoutDefault  The fault time out value before the system continue to boot.

  @retval  EFI_SUCCESS       User pressed some key except "Enter"
  @retval  EFI_TIME_OUT      Timeout expired or user press "Enter"

**/
EFI_STATUS
ShowProgress (
  IN UINT16                       TimeoutDefault
  )
{
#if 0
  CHAR16                        *TmpStr;
#endif
  UINT16                        TimeoutRemain;
  EFI_STATUS                    Status;
  EFI_INPUT_KEY                 Key;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;

  if (TimeoutDefault == 0) {
    return EFI_TIMEOUT;
  }

  SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
  SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
  SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
  Print(L".");
#if 0
  TmpStr = GetStringById (STRING_TOKEN (STR_START_BOOT_OPTION));
  TmpStr = NULL;
  if (TmpStr != NULL) {
    PlatformBdsShowProgress (Foreground, Background, TmpStr, Color, 0, 0);
  }
#endif
  TimeoutRemain = TimeoutDefault;
  while (TimeoutRemain != 0) {
    Status = WaitForSingleEvent (gST->ConIn->WaitForKey, ONE_SECOND);
    if (Status != EFI_TIMEOUT) {
      break;
    }
    Print(L".");
    TimeoutRemain--;
#if 0
    if (TmpStr != NULL) {
      PlatformBdsShowProgress (
        Foreground,
        Background,
        TmpStr,
        Color,
        ((TimeoutDefault - TimeoutRemain) * 100 / TimeoutDefault),
        0
        );
    }
#endif
  }
#if 0
  gBS->FreePool (TmpStr);
#endif
  if (TimeoutRemain == 0) {
    return EFI_TIMEOUT;
  }
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
    //
    // User pressed enter, equivalent to select "continue"
    //
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  This function is the main entry of the platform setup entry.
  The function will present the main menu of the system setup,
  this is the platform reference part and can be customize.


  @param TimeoutDefault     The fault time out value before the system
                            continue to boot.
  @param ConnectAllHappened The indicater to check if the connect all have
                            already happened.

**/
VOID
PlatformBdsEnterFrontPage (
  IN UINT16                       TimeoutDefault,
  IN BOOLEAN                      ConnectAllHappened
  )
{
  EFI_STATUS                    Status = EFI_SUCCESS;

//  PERF_START (NULL, "BdsTimeOut", "BDS", 0);
  if (ConnectAllHappened) {
    gConnectAllHappened = TRUE;
  }
  if (TimeoutDefault != 0xffff) {
    Status = ShowProgress (TimeoutDefault);
    if (EFI_ERROR (Status)) goto Exit;
  }
  Status = EFI_SUCCESS;
  UpdateBootStrings ();
  do {

    gCallbackKey = 0;
    CallBootManager ();

  } while (Status == EFI_SUCCESS);

Exit:
  //
  // Automatically load current entry
  // Note: The following lines of code only execute when Auto boot
  // takes affect
  //
  //PERF_END (NULL, "BdsTimeOut", "BDS", 0);
  ;
}
