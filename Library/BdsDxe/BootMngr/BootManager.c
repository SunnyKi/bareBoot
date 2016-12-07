/** @file
  The platform boot manager reference implementation

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/DxeServicesLib.h>

#include "BootManager.h"
#include "Graphics.h"
#include "Version.h"

UINT16                          mKeyInput;
BDS_COMMON_OPTION               *gOption;
EFI_FORM_BROWSER2_PROTOCOL      *gFormBrowser2;
BOOLEAN                         gFronPage;
UINTN                           gCallbackKey;

CHAR8   *mAVersion;
CHAR8   *mABiosVersion;
CHAR8   *mAProductName;
CHAR8   *mAProcessorVersion;
CHAR8   *mAProcessorSpeed;
CHAR8   *mAMemorySize;

CHAR16             *mDeviceTypeStr[] = {
  L"Legacy BEV",
  L"Legacy Floppy",
  L"Legacy Hard Drive",
  L"Legacy CD ROM",
  L"Legacy PCMCIA",
  L"Legacy USB",
  L"Legacy Embedded Network",
  L"Legacy Unknown Device"
};


HII_VENDOR_DEVICE_PATH  mBootManagerHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    BOOT_MANAGER_FORMSET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { 
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

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

BOOT_MANAGER_CALLBACK_DATA  gBootManagerPrivate = {
  BOOT_MANAGER_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    BootManagerCallback
  }
};

/**
  This call back function is registered with Boot Manager formset.
  When user selects a boot option, this call back function will
  be triggered. The boot option is saved for later processing.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
BootManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  BDS_COMMON_OPTION       *Option;
  LIST_ENTRY              *Link;
  UINT16                  KeyCount;

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((Value == NULL) || (ActionRequest == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Initialize the key count
    //

    gCallbackKey = QuestionId;

    KeyCount = 0;

    switch (QuestionId) {
      default:
        gCallbackKey = 0;
        break;
    }

    for (Link = GetFirstNode (&gBootOptionList); !IsNull (&gBootOptionList, Link); Link = GetNextNode (&gBootOptionList, Link)) {
      Option = CR (Link, BDS_COMMON_OPTION, Link, BDS_LOAD_OPTION_SIGNATURE);

      KeyCount++;

      gOption = Option;

      //
      // Is this device the one chosen?
      //
      if (KeyCount == QuestionId) {
        //
        // Assigning the returned Key to a global allows the original routine to know what was chosen
        //
        mKeyInput = QuestionId;

        //
        // Request to exit SendForm(), so that we could boot the selected option
        //
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
        break;
      }
    }
    

    return EFI_SUCCESS;
  }

  //
  // All other action return unsupported.
  //
  return EFI_UNSUPPORTED;
}

/**

  Registers HII packages for the Boot Manger to HII Database.
  It also registers the browser call back function.

  @retval  EFI_SUCCESS           HII packages for the Boot Manager were registered successfully.
  @retval  EFI_OUT_OF_RESOURCES  HII packages for the Boot Manager failed to be registered.

**/
EFI_STATUS
InitializeBootManager (
  VOID
  )
{
  EFI_STATUS                  Status;

  gCallbackKey  = 0;
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &gFormBrowser2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gBootManagerPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mBootManagerHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gBootManagerPrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  gBootManagerPrivate.HiiHandle = HiiAddPackages (
                                    &gBootManagerFormSetGuid,
                                    gBootManagerPrivate.DriverHandle,
                                    BootManagerVfrBin,
                                    BdsDxeStrings,
                                    NULL
                                    );
  if (gBootManagerPrivate.HiiHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    Status = EFI_SUCCESS;
  }
  return Status;
}

/**
 Convert Memory Size to a string.

 @param MemorySize      The size of the memory to process
 @param String          The string that is created

 **/
VOID
ConvertMemorySizeToString (
  IN  UINT32          MemorySize,
  OUT CHAR16          **String
  )
{
  CHAR16  *StringBuffer;

  StringBuffer = AllocateZeroPool (0x20);
  ASSERT (StringBuffer != NULL);
  UnicodeValueToString (StringBuffer, LEFT_JUSTIFY, MemorySize, 6);
  StrCat (StringBuffer, L" MB RAM");
  
  *String = (CHAR16 *) StringBuffer;
  
  return ;
}

/**
 Convert Processor Frequency Data to a string.

 @param ProcessorFrequency The frequency data to process
 @param Base10Exponent     The exponent based on 10
 @param String             The string that is created

 **/
VOID
ConvertProcessorToString (
  IN  UINT16                               ProcessorFrequency,
  IN  UINT16                               Base10Exponent,
  OUT CHAR16                               **String
  )
{
  CHAR16  *StringBuffer;
  UINTN   Index;
  UINT32  FreqMhz;

  if (Base10Exponent >= 6) {
    FreqMhz = ProcessorFrequency;
    for (Index = 0; Index < (UINTN) (Base10Exponent - 6); Index++) {
      FreqMhz *= 10;
    }
  } else {
    FreqMhz = 0;
  }

  StringBuffer = AllocateZeroPool (0x20);
  ASSERT (StringBuffer != NULL);
  Index = UnicodeValueToString (StringBuffer, LEFT_JUSTIFY, FreqMhz / 1000, 3);
  StrCat (StringBuffer, L".");
  UnicodeValueToString (StringBuffer + Index + 1, PREFIX_ZERO, (FreqMhz % 1000) / 10, 2);
  StrCat (StringBuffer, L" GHz");
  *String = (CHAR16 *) StringBuffer;
  return ;
}

/**

 Acquire the string associated with the Index from smbios structure and return it.
 The caller is responsible for free the string buffer.

 @param    OptionalStrStart  The start position to search the string
 @param    Index             The index of the string to extract
 @param    String            The string that is extracted

 @retval   EFI_SUCCESS       The function returns EFI_SUCCESS always.

 **/
EFI_STATUS
GetOptionalStringByIndex (
  IN      CHAR8                   *OptionalStrStart,
  IN      UINT8                   Index,
  OUT     CHAR16                  **String
  )
{
  UINTN          StrSize;

  if (Index == 0) {
    *String = AllocateZeroPool (sizeof (CHAR16));
    return EFI_SUCCESS;
  }

  StrSize = 0;
  do {
    Index--;
    OptionalStrStart += StrSize;
    StrSize           = AsciiStrSize (OptionalStrStart);
  } while (OptionalStrStart[StrSize] != 0 && Index != 0);

  if ((Index != 0) || (StrSize == 1)) {
    //
    // Meet the end of strings set but Index is non-zero, or
    // Find an empty string
    //
    *String = GetStringById (STRING_TOKEN (STR_MISSING_STRING));
  } else {
    *String = AllocatePool (StrSize * sizeof (CHAR16));
    AsciiStrToUnicodeStr (OptionalStrStart, *String);
  }
  
  return EFI_SUCCESS;
}

VOID
UpdateBootStrings (
  VOID
  )
{
  UINT8                             StrIndex;
  CHAR16                            *NewString;
  CHAR16                            *TmpString;
  BOOLEAN                           Find[5];
  EFI_STATUS                        Status;
  EFI_STRING_ID                     TokenToUpdate;
  EFI_SMBIOS_HANDLE                 SmbiosHandle;
  EFI_SMBIOS_PROTOCOL               *Smbios;
  SMBIOS_TABLE_TYPE0                *Type0Record;
  SMBIOS_TABLE_TYPE1                *Type1Record;
  SMBIOS_TABLE_TYPE4                *Type4Record;
  SMBIOS_TABLE_TYPE19               *Type19Record;
  EFI_SMBIOS_TABLE_HEADER           *Record;

  ZeroMem (Find, sizeof (Find));

  //
  // Update Front Page strings
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **) &Smbios
                  );
  ASSERT_EFI_ERROR (Status);

  NewString = AllocateZeroPool (
                StrSize (FIRMWARE_REVISION) +
                StrSize (L"    ") +
                StrSize (L" (") +
                StrSize (FIRMWARE_BUILDDATE) +
                StrSize (L")")
                );
  StrCat (NewString, L"    ");
  StrCat (NewString, FIRMWARE_REVISION);
  StrCat (NewString, L" (");
  StrCat (NewString, FIRMWARE_BUILDDATE);
  StrCat (NewString, L")");
  mAVersion = AllocateZeroPool (StrLen (NewString) + 1);
  UnicodeStrToAsciiStr (NewString, mAVersion);
  TokenToUpdate = STRING_TOKEN (STR_MINI_CLOVER_VERSION);
  HiiSetString (gBootManagerPrivate.HiiHandle, TokenToUpdate, NewString, NULL);
  FreePool (NewString);

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  do {
    Status = Smbios->GetNext (Smbios, &SmbiosHandle, NULL, &Record, NULL);
    if (EFI_ERROR(Status)) {
      break;
    }

    if (Record->Type == EFI_SMBIOS_TYPE_BIOS_INFORMATION) {
      Type0Record = (SMBIOS_TABLE_TYPE0 *) Record;
      StrIndex = Type0Record->BiosVersion;
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)Type0Record + Type0Record->Hdr.Length), StrIndex, &TmpString);
      NewString = AllocateZeroPool (StrSize (TmpString) + StrSize (L"  "));
      StrCat (NewString, L"  ");
      StrCat (NewString, TmpString);
      mABiosVersion = AllocateZeroPool (StrLen (NewString) + 1);
      UnicodeStrToAsciiStr (NewString, mABiosVersion);
      TokenToUpdate = STRING_TOKEN (STR_BOOT_BIOS_VERSION);
      HiiSetString (gBootManagerPrivate.HiiHandle, TokenToUpdate, NewString, NULL);
      FreePool (NewString);
      Find[0] = TRUE;
    }

    if (Record->Type == EFI_SMBIOS_TYPE_SYSTEM_INFORMATION) {
      Type1Record = (SMBIOS_TABLE_TYPE1 *) Record;
      StrIndex = Type1Record->ProductName;
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)Type1Record + Type1Record->Hdr.Length), StrIndex, &TmpString);
      NewString = AllocateZeroPool (StrSize (TmpString) + StrSize (L"  "));
      StrCat (NewString, L"  ");
      StrCat (NewString, TmpString);
      mAProductName = AllocateZeroPool (StrLen (NewString) + 1);
      UnicodeStrToAsciiStr (NewString, mAProductName);
      TokenToUpdate = STRING_TOKEN (STR_BOOT_COMPUTER_MODEL);
      HiiSetString (gBootManagerPrivate.HiiHandle, TokenToUpdate, NewString, NULL);
      FreePool (NewString);
      Find[1] = TRUE;
    }

    if (Record->Type == EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION) {
      Type4Record = (SMBIOS_TABLE_TYPE4 *) Record;
      StrIndex = Type4Record->ProcessorVersion;
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)Type4Record + Type4Record->Hdr.Length), StrIndex, &TmpString);
      NewString = AllocateZeroPool (StrSize (TmpString) + StrSize (L"  "));
      StrCat (NewString, L"  ");
      StrCat (NewString, TmpString);
      mAProcessorVersion = AllocateZeroPool (StrLen (NewString) + 1);
      UnicodeStrToAsciiStr (NewString, mAProcessorVersion);
      TokenToUpdate = STRING_TOKEN (STR_BOOT_CPU_MODEL);
      HiiSetString (gBootManagerPrivate.HiiHandle, TokenToUpdate, NewString, NULL);
      FreePool (NewString);
      Find[2] = TRUE;
    }

    if (Record->Type == EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION) {
      Type4Record = (SMBIOS_TABLE_TYPE4 *) Record;
      ConvertProcessorToString(Type4Record->CurrentSpeed, 6, &TmpString);
      NewString = AllocateZeroPool (StrSize (TmpString) + StrSize (L"    "));
      StrCat (NewString, L"    ");
      StrCat (NewString, TmpString);
      mAProcessorSpeed = AllocateZeroPool (StrLen (NewString) + 1);
      UnicodeStrToAsciiStr (NewString, mAProcessorSpeed);
      TokenToUpdate = STRING_TOKEN (STR_BOOT_CPU_SPEED);
      HiiSetString (gBootManagerPrivate.HiiHandle, TokenToUpdate, NewString, NULL);
      FreePool (NewString);
      Find[3] = TRUE;
    }

    if ( Record->Type == EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS ) {
      Type19Record = (SMBIOS_TABLE_TYPE19 *) Record;
      ConvertMemorySizeToString (
        (UINT32)(RShiftU64((Type19Record->EndingAddress - Type19Record->StartingAddress + 1), 10)),
        &TmpString
        );
      NewString = AllocateZeroPool (StrSize (TmpString) + StrSize (L"    Memory "));
      StrCat (NewString, L"    Memory ");
      StrCat (NewString, TmpString);
      mAMemorySize = AllocateZeroPool (StrLen (NewString) + 1);
      UnicodeStrToAsciiStr (NewString, mAMemorySize);
      TokenToUpdate = STRING_TOKEN (STR_BOOT_MEMORY_SIZE);
      HiiSetString (gBootManagerPrivate.HiiHandle, TokenToUpdate, NewString, NULL);
      FreePool (NewString);
      Find[4] = TRUE;
    }
  } while ( !(Find[0] && Find[1] && Find[2] && Find[3] && Find[4]));
  return ;
}

/**
  This function invokes Boot Manager. If all devices have not a chance to be connected,
  the connect all will be triggered. It then enumerate all boot options. If 
  a boot option from the Boot Manager page is selected, Boot Manager will boot
  from this boot option.
  
**/
VOID
CallBootManager (
  VOID
  )
{
#if 0
  EFI_STATUS                  Status;
#endif
  BDS_COMMON_OPTION           *Option;
  LIST_ENTRY                  *Link;
  CHAR16                      *ExitData;
  UINTN                       ExitDataSize;
  EFI_STRING_ID               Token;
#if 0
  EFI_INPUT_KEY               Key;
#endif
  CHAR16                      *HelpString;
  EFI_STRING_ID               HelpToken;
  UINT16                      *TempStr;
  EFI_HII_HANDLE              HiiHandle;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;
  UINTN                       TempSize;
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *EndLabel;
  UINT16                      DeviceType;
  BOOLEAN                     IsLegacyOption;
  BOOLEAN                     NeedEndOp;

  DeviceType = (UINT16) -1;
  gOption    = NULL;

  //
  // Connect all prior to entering the platform setup menu.
  //
#if 0
  if (!gConnectAllHappened) {
    BdsLibConnectAllDriversToAllControllers ();
    gConnectAllHappened = TRUE;
  }
#endif
  
  if (IsListEmpty (&gBootOptionList)) {
    DBG ("BootManager: BdsLibEnumerateAllBootOption\n");
    InitializeListHead (&gBootOptionList);
    BdsLibEnumerateAllBootOption (&gBootOptionList);
  }

  HiiHandle = gBootManagerPrivate.HiiHandle;

  //
  // Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_BOOT_OPTION;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_BOOT_OPTION_END;

  mKeyInput = 0;
  NeedEndOp = FALSE;
  for (Link = GetFirstNode (&gBootOptionList); !IsNull (&gBootOptionList, Link); Link = GetNextNode (&gBootOptionList, Link)) {
    Option = CR (Link, BDS_COMMON_OPTION, Link, BDS_LOAD_OPTION_SIGNATURE);

    //
    // At this stage we are creating a menu entry, thus the Keys are reproduceable
    //
    mKeyInput++;

    //
    // Don't display the boot option marked as LOAD_OPTION_HIDDEN
    //
    if ((Option->Attribute & LOAD_OPTION_HIDDEN) != 0) {
      continue;
    }

    //
    // Group the legacy boot option in the sub title created dynamically
    //
    IsLegacyOption = (BOOLEAN) (
                       (DevicePathType (Option->DevicePath) == BBS_DEVICE_PATH) &&
                       (DevicePathSubType (Option->DevicePath) == BBS_BBS_DP)
                       );

    if (!IsLegacyOption && NeedEndOp) {
      NeedEndOp = FALSE;
      HiiCreateEndOpCode (StartOpCodeHandle);
    }
    
    if (IsLegacyOption && DeviceType != ((BBS_BBS_DEVICE_PATH *) Option->DevicePath)->DeviceType) {
      if (NeedEndOp) {
        HiiCreateEndOpCode (StartOpCodeHandle);
      }

      DeviceType = ((BBS_BBS_DEVICE_PATH *) Option->DevicePath)->DeviceType;
      Token      = HiiSetString (
                     HiiHandle,
                     0,
                     mDeviceTypeStr[
                       MIN (DeviceType & 0xF, ARRAY_SIZE (mDeviceTypeStr) - 1)
                       ],
                     NULL
                     );
      HiiCreateSubTitleOpCode (StartOpCodeHandle, Token, 0, 0, 1);
      NeedEndOp = TRUE;
    }

    ASSERT (Option->Description != NULL);
    
    Token = HiiSetString (HiiHandle, 0, Option->Description, NULL);

    /* XXX: do we need more human representation for device path? */
    TempStr = ConvertDevicePathToText (Option->DevicePath, FALSE, FALSE);
    TempSize = StrSize (TempStr);
    HelpString = AllocateZeroPool (TempSize + StrSize (L"Device Path : "));
    ASSERT (HelpString != NULL);
    StrCat (HelpString, L"Device Path : ");
    StrCat (HelpString, TempStr);

    HelpToken = HiiSetString (HiiHandle, 0, HelpString, NULL);

    HiiCreateActionOpCode (
      StartOpCodeHandle,
      mKeyInput,
      Token,
      HelpToken,
      EFI_IFR_FLAG_CALLBACK,
      0
      );
  }

  if (NeedEndOp) {
    HiiCreateEndOpCode (StartOpCodeHandle);
  }

  HiiUpdateForm (
    HiiHandle,
    &gBootManagerFormSetGuid,
    BOOT_MANAGER_FORM_ID,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
#if 1
  (void)
#else
  Status =
#endif
	  gFormBrowser2->SendForm (
                           gFormBrowser2,
                           &HiiHandle,
                           1,
                           &gBootManagerFormSetGuid,
                           0,
                           NULL,
                           &ActionRequest
                           );
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  if (gOption == NULL) {
    return ;
  }

  //
  // Will leave browser, check any reset required change is applied? if yes, reset system
  //
#if 0
  SetupResetReminder ();
#endif
  //
  // parse the selected option
  //
#if 1
  (void) BdsLibBootViaBootOption (gOption, gOption->LoadOptions, gOption->DevicePath, &ExitDataSize, &ExitData);
#else
  Status = BdsLibBootViaBootOption (gOption, gOption->LoadOptions, gOption->DevicePath, &ExitDataSize, &ExitData);
  if (!EFI_ERROR (Status)) {
    gOption->StatusString = GetStringById (STRING_TOKEN (STR_BOOT_SUCCEEDED));
    PlatformBdsBootSuccess (gOption);
  } else {
    gOption->StatusString = GetStringById (STRING_TOKEN (STR_BOOT_FAILED));
    PlatformBdsBootFail (gOption, Status, ExitData, ExitDataSize);
    gST->ConOut->OutputString (
                  gST->ConOut,
                  GetStringById (STRING_TOKEN (STR_ANY_KEY_CONTINUE))
                  );
    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  }
#endif
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
  UINT16                        TimeoutRemain;
  EFI_STATUS                    Status;
  EFI_INPUT_KEY                 Key;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *Pixel;
  EFI_GRAPHICS_OUTPUT_PROTOCOL      *GraphicsOutput;
  UINTN                          BlockHeight;
  UINTN                          BlockWidth;
  UINTN                          PosX;
  UINTN                          PosY;
  INTN                          DestX;
  INTN                          DestY;
  UINT8                         *ImageData;
  UINTN                         ImageSize;
  UINTN                         BltSize;
  UINTN                         Height;
  UINTN                         Width;
  UINTN                         FontHeight;
  UINTN                         FontWidth;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  
  GraphicsOutput = NULL;
  Pixel = NULL;
  PosX = 0;
  PosY = 0;
  BlockWidth  = 0;
  BlockHeight = 0;
  
  if (TimeoutDefault == 0) {
    return EFI_TIMEOUT;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                );

  if (!EFI_ERROR (Status)) {
    Pixel = AllocateZeroPool (sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    Pixel->Blue = 0;
    Pixel->Green = 0;
    Pixel->Red = 191;
    Pixel->Reserved = 0;

    BlockWidth  = (GraphicsOutput->Mode->Info->HorizontalResolution) / TimeoutDefault;
    BlockHeight = 5;
    PosY        = GraphicsOutput->Mode->Info->VerticalResolution -  10;

    Blt = NULL;
    ImageData = NULL;
    ImageSize = 0;
    FontHeight = 16;
    FontWidth = 10;

    //
    // Get the specified image from FV.
    //

    if (!gSettings.YoBlack) {
      Status = GetSectionFromAnyFv (
                 PcdGetPtr(PcdBBLogoFile),
                 EFI_SECTION_RAW,
                 0,
                 (VOID **) &ImageData,
                 &ImageSize
               );
      if (EFI_ERROR (Status)) {
        goto Down;
      }
      
      Status = ConvertBmpToGopBlt (
                 ImageData,
                 ImageSize,
                 (VOID **) &Blt,
                 &BltSize,
                 &Height,
                 &Width
               );
      
      if (EFI_ERROR (Status)) {
        goto Down;
      }

      DestX = (GraphicsOutput->Mode->Info->HorizontalResolution - Width) / 2;
      DestY = (GraphicsOutput->Mode->Info->VerticalResolution - Height) / 2;

      if ((DestX >= 0) && (DestY >= 0)) {
        Status = BltWithAlpha (
                   GraphicsOutput,
                   Blt,
                   EfiBltBufferToVideo,
                   0,
                   0,
                   (UINTN) DestX,
                   (UINTN) DestY,
                   Width,
                   Height,
                   Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL),
                   FALSE
                 );
      }
      
      DestX = GraphicsOutput->Mode->Info->HorizontalResolution -  AsciiStrLen (mAVersion) * FontWidth - 40;
      DestY = FontHeight * 2;
      ShowAString (GraphicsOutput, mAVersion, DestX, DestY, TRUE);
      
      DestX = 40;
      DestY += FontHeight * 2;
      ShowAString (GraphicsOutput, mAProductName, DestX, DestY, TRUE);
      
      DestX += AsciiStrLen (mAProductName) * FontWidth;
      ShowAString (GraphicsOutput, mABiosVersion, DestX, DestY, TRUE);
      
      DestX = 40;
      DestY += FontHeight * 2;
      ShowAString (GraphicsOutput, mAProcessorVersion, DestX, DestY, TRUE);
      
      DestX += AsciiStrLen (mAProcessorVersion) * FontWidth;
      ShowAString (GraphicsOutput, mAProcessorSpeed, DestX, DestY, TRUE);
      
      DestX = 40 - 2 * FontWidth;
      DestY += FontHeight * 2;
      ShowAString (GraphicsOutput, mAMemorySize, DestX, DestY, TRUE);
    }


#if 0
    ShowPngFile (GraphicsOutput, L"\\EFI\\bareboot\\banner.png", DestX, DestY, TRUE);
#endif

Down:
    if (ImageData != NULL) {
      FreePool (ImageData);
    }
    if (Blt != NULL) {
      FreePool (Blt);
    }
  } else {
    Print(L".");
  }
    
  TimeoutRemain = TimeoutDefault;
  while (TimeoutRemain != 0) {
    Status = WaitForSingleEvent (gST->ConIn->WaitForKey, ONE_SECOND);
    if (Status != EFI_TIMEOUT) {
      break;
    }
    if (GraphicsOutput != NULL) {
      GraphicsOutput->Blt (
                       GraphicsOutput,
                       Pixel,
                       EfiBltVideoFill,
                       0, 0, PosX, PosY,
                       BlockWidth,
                       BlockHeight,
                       0
                      );
      PosX += BlockWidth;
    } else {
      Print(L".");
    }
    TimeoutRemain--;
  }
  if (TimeoutRemain == 0) {
    if (Pixel != NULL) {
      FreePool (Pixel);
    }
    return EFI_TIMEOUT;
  }
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  if (EFI_ERROR (Status)) {
    if (Pixel != NULL) {
      FreePool (Pixel);
    }
    return Status;
  }
  if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
    //
    // User pressed enter, equivalent to select "continue"
    //
    if (Pixel != NULL) {
      FreePool (Pixel);
    }
    return EFI_TIMEOUT;
  }
  
  if (Pixel != NULL) {
    FreePool (Pixel);
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
  IN UINT16                       TimeoutDefault
  )
{
  EFI_STATUS                    Status = EFI_SUCCESS;

  gFronPage = FALSE;

  UpdateBootStrings ();

  if (TimeoutDefault != 0xffff) {
    Status = ShowProgress (TimeoutDefault);
    if (EFI_ERROR (Status)) goto Exit;
  }
  
  Status = EFI_SUCCESS;
  gFronPage = TRUE;
  ClearScreen (0x00, NULL);
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
  ;
}

