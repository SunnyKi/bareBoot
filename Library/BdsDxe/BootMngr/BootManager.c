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

#include "BootManager.h"
#include "../../../Version.h"

UINT16             mKeyInput;
BDS_COMMON_OPTION  *gOption;
EFI_FORM_BROWSER2_PROTOCOL      *gFormBrowser2;

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
      NewString = AllocateZeroPool (StrSize (TmpString) + StrSize (L"    "));
      StrCat (NewString, L"    ");
      StrCat (NewString, TmpString);
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
  EFI_STATUS                  Status;
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
  if (!gConnectAllHappened) {
    BdsLibConnectAllDriversToAllControllers ();
    gConnectAllHappened = TRUE;
  }

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
                       MIN (DeviceType & 0xF, sizeof (mDeviceTypeStr) / sizeof (mDeviceTypeStr[0]) - 1)
                       ],
                     NULL
                     );
      HiiCreateSubTitleOpCode (StartOpCodeHandle, Token, 0, 0, 1);
      NeedEndOp = TRUE;
    }

    ASSERT (Option->Description != NULL);
    
    Token = HiiSetString (HiiHandle, 0, Option->Description, NULL);

    /* XXX: shell we use more human representation? */
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
  Status = gFormBrowser2->SendForm (
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
  Status = BdsLibBootViaBootOption (gOption, gOption->LoadOptions, gOption->DevicePath, &ExitDataSize, &ExitData);
#if 0
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
