#!/usr/bin/env bash

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

function findInPackages
{
  if [ -z "$PACKAGES_PATH" ]
  then
    echo $WORKSPACE/$1
    return
  fi
  for d in ${PACKAGES_PATH//:/ }
  do
    fn=$d/$1
    if [ -e $fn ]
    then
      echo $fn
      return
    fi
  done
  echo $1
}

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

BOOTSECTOR_BIN_DIR=$(findInPackages bareBoot/BootSector2/bin)

BUILD_DIR=$WORKSPACE/Build/$PROJECTNAME/$PROCESSOR/${TARGET}_$TOOLTAG

FV_DIR=$BUILD_DIR/FV
FVNAME=$(echo ${PROJECTNAME}EFIMAINFV | tr '[a-z]' '[A-Z]')

BOOTFILE=$WORKSPACE/stage/boot$PROCESSOR-$TARGET-$TOOLTAG

echo Compressing ${FVNAME}.FV ...
LzmaCompress -e -o $FV_DIR/${FVNAME}.z $FV_DIR/${FVNAME}.Fv

echo Compressing DxeMain.efi ...
LzmaCompress -e -o $FV_DIR/DxeMain.z $BUILD_DIR/$PROCESSOR/DxeCore.efi

echo Compressing DxeIpl.efi ...
LzmaCompress -e -o $FV_DIR/DxeIpl.z $BUILD_DIR/$PROCESSOR/DxeIpl.efi

echo Generate Loader Image ...

GenFw --rebase 0x10000 -o $FV_DIR/EfiLoader.efi $BUILD_DIR/$PROCESSOR/EfiLoader.efi
EfiLdrImage -o $FV_DIR/Efildr$PROCESSOR $FV_DIR/EfiLoader.efi $FV_DIR/DxeIpl.z $FV_DIR/DxeMain.z $FV_DIR/${FVNAME}.z
cat $BOOTSECTOR_BIN_DIR/EfiLdrPrelude$PROCESSOR $FV_DIR/Efildr$PROCESSOR > $BOOTFILE

echo $BOOTFILE is made!
exit 0
