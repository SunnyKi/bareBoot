@REM ## @file
@REM #
@REM #  Currently, Build system does not provide post build mechanism for module 
@REM #  and platform building, so just use a bat file to do post build commands.
@REM #  Originally, following post building command is for EfiLoader module.
@REM #
@REM #  Copyright (c) 2010 - 2016, Intel Corporation. All rights reserved.<BR>
@REM #
@REM #  This program and the accompanying materials
@REM #  are licensed and made available under the terms and conditions of the BSD License
@REM #  which accompanies this distribution. The full text of the license may be found at
@REM #  http://opensource.org/licenses/bsd-license.php
@REM #  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM #  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM ##

@echo off

set PKGNAME=bareBoot

set TARGET_ARCH=%1
set TARGET=%2
set TOOL_CHAIN_TAG=%3

for %%i in (BootSector2\bin) do set BOOTSECTOR_BIN_DIR=%%~$PACKAGES_PATH:i

set BUILD_DIR=%WORKSPACE%\Build\%PKGNAME%\%TARGET_ARCH%\%TARGET%_%TOOL_CHAIN_TAG%

set FV_DIR=%BUILD_DIR%\FV
set FV_NAME=bareBootEFIMAINFV

set BOOTFILE=%WORKSPACE%\stage\boot%TARGET_ARCH%-%TARGET%-%TOOL_CHAIN_TAG%

echo Compressing bareBootEFIMainFv.FV ...
LzmaCompress -e -o %FV_DIR%\%FV_NAME%.z %FV_DIR%\%FV_NAME%.Fv

echo Compressing DxeMain.efi ...
LzmaCompress -e -o %FV_DIR%\DxeMain.z %BUILD_DIR%\%TARGET_ARCH%\DxeCore.efi

echo Compressing DxeIpl.efi ...
LzmaCompress -e -o %FV_DIR%\DxeIpl.z %BUILD_DIR%\%TARGET_ARCH%\DxeIpl.efi

echo Generate Loader Image ...
EfiLdrImage.exe -o %FV_DIR%\Efildr%TARGET_ARCH% %BUILD_DIR%\%TARGET_ARCH%\EfiLoader.efi %FV_DIR%\DxeIpl.z %FV_DIR%\DxeMain.z %FV_DIR%\%FV_NAME%.z

copy /b %BOOTSECTOR_BIN_DIR%\EfiLdrPrelude%TARGET_ARCH%+%FV_DIR%\Efildr%TARGET_ARCH% %BOOTFILE%

echo Created bootfile %BOOTFILE%

goto end

:Help
echo Usage: "PostBuild IA32|X64 RELEASE|DEBUG|NOOPT VS2017|...."
:end
