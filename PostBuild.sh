#!/bin/env bash

## @file
#
#  Currently, Build system does not provide post build mechanism for module
#  and platform building, so just use a sh file to do post build commands.
#  Originally, following post building command is for EfiLoader module.
#
#  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution. The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

if [ $# = 2 ]
then
  PROCESSOR=$1
  TOOLTAG=$2
fi

source $WORKSPACE/Conf/target.txt

if [ -z "$PROCESSOR" ]
then
  PROCESSOR=$TARGET_ARCH
fi

if [ -z "$TOOLTAG" ]
then
  TOOLTAG=$TOOL_CHAIN_TAG
fi

if [ -z "$PROJECTNAME" ]
then
  PROJECTNAME=$(basename $ACTIVE_PLATFORM .dsc)
fi

BOOTSECTOR_BIN_DIR=$WORKSPACE/$PROJECTNAME/BootSector/bin

BUILD_DIR=$WORKSPACE/Build/$PROJECTNAME/$PROCESSOR/${TARGET}_$TOOLTAG

FVNAME=$(echo ${PROJECTNAME}EFIMAINFV | tr '[a-z]' '[A-Z]')

# Boot sector module could only be built under IA32 tool chain

echo Compressing ${FVNAME}.FV ...
LzmaCompress -e -o $BUILD_DIR/FV/${FVNAME}.z $BUILD_DIR/FV/${FVNAME}.Fv

echo Compressing DxeMain.efi ...
LzmaCompress -e -o $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/$PROCESSOR/DxeCore.efi

echo Compressing DxeIpl.efi ...
LzmaCompress -e -o $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/$PROCESSOR/DxeIpl.efi

echo Generate Loader Image ...

case "$PROCESSOR" in
  IA32)
    GenFw --rebase 0x10000 -o $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/$PROCESSOR/EfiLoader.efi
    EfiLdrImage -o $BUILD_DIR/FV/Efildr32 $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/${FVNAME}.z
    cat $BOOTSECTOR_BIN_DIR/start.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32   > $BUILD_DIR/FV/Efildr
    # It is safe to use "bcat" to cat following binary file, if bcat command is avaiable for your system
    #bcat -o $BUILD_DIR/FV/Efildr.bcat $BOOTSECTOR_BIN_DIR/start.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32
    cat $BOOTSECTOR_BIN_DIR/start16.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/Efildr16
    #bcat -o $BUILD_DIR/FV/Efildr16.bcat $BOOTSECTOR_BIN_DIR/start16.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32
    cat $BOOTSECTOR_BIN_DIR/start32.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/Efildr20
    #bcat -o $BUILD_DIR/FV/Efildr20.bcat $BOOTSECTOR_BIN_DIR/start32.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32
    ;;

  X64)
    GenFw --rebase 0x10000 -o $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/$PROCESSOR/EfiLoader.efi
    EfiLdrImage -o $BUILD_DIR/FV/Efildr64 $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/${FVNAME}.z
    cat $BOOTSECTOR_BIN_DIR/start64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/EfildrPure
    #bcat -o $BUILD_DIR/FV/EfildrPure $BOOTSECTOR_BIN_DIR/start64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64
    GenPage $BUILD_DIR/FV/EfildrPure -o $BUILD_DIR/FV/Efildr
    cat $BOOTSECTOR_BIN_DIR/st16_64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/Efildr16Pure
    #bcat -o $BUILD_DIR/FV/Efildr16Pure $BOOTSECTOR_BIN_DIR/st16_64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64
    GenPage $BUILD_DIR/FV/Efildr16Pure -o $BUILD_DIR/FV/Efildr16
    cat $BOOTSECTOR_BIN_DIR/st32_64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/Efildr20Pure
    #bcat -o $BUILD_DIR/FV/Efildr20Pure $BOOTSECTOR_BIN_DIR/st32_64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64
    GenPage $BUILD_DIR/FV/Efildr20Pure -o $BUILD_DIR/FV/Efildr20
    ;;
esac

echo Done!
exit 0
