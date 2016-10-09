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

for %%i in (bareBoot\BootSector2\bin) do set BOOTSECTOR_BIN_DIR=%%~$PACKAGES_PATH:i
for %%i in (bareBoot\GetVariables.bat) do set GETVARSSCRIPT=%%~$PACKAGES_PATH:i

call %GETVARSSCRIPT%

set PROCESSOR=""

if NOT "%1"=="" @set TARGET_ARCH=%1
if "%TARGET_ARCH%"=="IA32" set PROCESSOR=IA32
if "%TARGET_ARCH%"=="X64" set PROCESSOR=X64
if %PROCESSOR%=="" goto WrongArch

set BUILD_DIR=%WORKSPACE%\Build\%PKGNAME%\%TARGET_ARCH%\%TARGET%_%TOOL_CHAIN_TAG%

set FV_DIR=%BUILD_DIR%\FV
set FV_NAME=bareBootEFIMAINFV

set BOOTFILE=%WORKSPACE%\stage\boot%PROCESSOR%-%TARGET%-%TOOL_CHAIN_TAG%

echo Compressing bareBootEFIMainFv.FV ...
LzmaCompress -e -o %FV_DIR%\%FV_NAME%.z %FV_DIR%\%FV_NAME%.Fv

echo Compressing DxeMain.efi ...
LzmaCompress -e -o %FV_DIR%\DxeMain.z %BUILD_DIR%\%PROCESSOR%\DxeCore.efi

echo Compressing DxeIpl.efi ...
LzmaCompress -e -o %FV_DIR%\DxeIpl.z %BUILD_DIR%\%PROCESSOR%\DxeIpl.efi

echo Generate Loader Image ...
EfiLdrImage.exe -o %FV_DIR%\Efildr%PROCESSOR% %BUILD_DIR%\%PROCESSOR%\EfiLoader.efi %FV_DIR%\DxeIpl.z %FV_DIR%\DxeMain.z %FV_DIR%\%FV_NAME%.z

copy /b %BOOTSECTOR_BIN_DIR%\EfiLdrPrelude%PROCESSOR%+%FV_DIR%\Efildr%PROCESSOR% %BOOTFILE%

echo Created bootfile %BOOTFILE%

goto end

:WrongArch
echo Error! Wrong architecture.
goto Help

:Help
echo Usage: "PostBuild [IA32|X64]"
:end
