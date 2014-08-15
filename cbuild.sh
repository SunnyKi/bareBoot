#!/bin/sh

## MAIN ARGUMENT PART##

export TARGET_TOOLS=GCC49
export PROCESSOR=X64
export TARGET=RELEASE
export DEF=""

while [[ $# -gt 0 ]]; do
  case $1 in
        "-ia32")    PROCESSOR=IA32;;
        "-x64")     PROCESSOR=X64;;
        "-debug")   TARGET=DEBUG;;
        "-release") TARGET=RELEASE;;
        "-clean")   export ARG=claen;;
        "-cleanall") export ARG=claenall;;
#use BlockIoDxe instead SataControllerDxe and Co
        "-b") DEF="$DEF -D BLOCKIO" ;;
#don't use usb drivers and oem config dir
        "-s") DEF="$DEF -D SPEEDUP" ;;
#use VBoxFsDxe instead HFSPlus
        "-vhfs") DEF="$DEF -D VBOXHFS" ;;
        "-ohci") DEF="$DEF -D OHCI" ;;
        "-ps2") DEF="$DEF -D PS2" ;;
        "-m2s") DEF="$DEF -D MEMLOG2SERIAL" ;;
        "-*")
         echo $"ERROR!"
        exit 1
    esac
    shift
done

fnMainBuildScript ()
# Function MAIN DUET BUILD SCRIPT
{
set -e
shopt -s nocasematch
if [ -z "$WORKSPACE" ]
then
  echo Initializing workspace
  if [ ! -e `pwd`/edksetup.sh ]
  then
    cd ..
  fi
  export EDK_TOOLS_PATH=`pwd`/BaseTools
  echo $EDK_TOOLS_PATH
  source edksetup.sh BaseTools
else
  echo Building from: $WORKSPACE
fi


BUILD_ROOT_ARCH=$WORKSPACE/Build/bareBoot/$PROCESSOR/"$TARGET"_"$TARGET_TOOLS"/$PROCESSOR

if  [[ ! -f `which build` || ! -f `which GenFv` ]];
then
  echo Building tools as they are not in the path
  make -C $WORKSPACE/BaseTools
elif [[ ( -f `which build` ||  -f `which GenFv` )  && ! -d  $EDK_TOOLS_PATH/Source/C/bin ]];
then
  echo Building tools no $EDK_TOOLS_PATH/Source/C/bin directory
  make -C $WORKSPACE/BaseTools
else
  echo using prebuilt tools
fi

if [[ $ARG == cleanall ]]; then
  make -C $WORKSPACE/BaseTools clean
  build -p $WORKSPACE/bareBoot/bareBoot.dsc -a $PROCESSOR -b $TARGET -t $TARGET_TOOLS -n 3 clean
exit $?
fi

if [[ $ARG == clean ]]; then
  build -p $WORKSPACE/bareBoot/bareBoot.dsc -a $PROCESSOR -b $TARGET -t $TARGET_TOOLS -n 3 clean
exit $?
fi

# Build the edk2 bareBoot
echo Running edk2 build for bareBoot$PROCESSOR with $DEF
VERFILE=$WORKSPACE/bareBoot/Version.h
echo "#define FIRMWARE_VERSION L\"2.31\"" > $VERFILE
echo "#define FIRMWARE_BUILDDATE L\"`LC_ALL=C date \"+%Y-%m-%d %H:%M:%S\"`\"" >> $VERFILE
echo "#define FIRMWARE_REVISION L\"`cd $WORKSPACE/bareBoot; git tag | tail -n 1`\"" >> $VERFILE
echo "#define FIRMWARE_BUILDDATE_ASCII \" (`LC_ALL=C date \"+%Y-%m-%d %H:%M:%S\"`)\"" >> $VERFILE
echo "#define FIRMWARE_REVISION_ASCII \"bareBoot `cd $WORKSPACE/bareBoot; git tag | tail -n 1`\"" >> $VERFILE

build -p $WORKSPACE/bareBoot/bareBoot.dsc -a $PROCESSOR -b $TARGET -t $TARGET_TOOLS -n 3 $DEF

}


fnMainPostBuildScript ()
{
if [ -z "$EDK_TOOLS_PATH" ]
then
  export BASETOOLS_DIR=$WORKSPACE/BaseTools/Source/C/bin
else
  export BASETOOLS_DIR=$EDK_TOOLS_PATH/Source/C/bin
fi

export BOOTSECTOR_BIN_DIR=$WORKSPACE/bareBoot/BootSector/bin
export BUILD_DIR=$WORKSPACE/Build/bareBoot/$PROCESSOR/"$TARGET"_"$TARGET_TOOLS"

echo Compressing DUETEFIMainFv.FV ...
$BASETOOLS_DIR/LzmaCompress -e -o $BUILD_DIR/FV/DUETEFIMAINFV.z $BUILD_DIR/FV/DUETEFIMAINFV.Fv

echo Compressing DxeMain.efi ...
$BASETOOLS_DIR/LzmaCompress -e -o $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/$PROCESSOR/DxeCore.efi

echo Compressing DxeIpl.efi ...
$BASETOOLS_DIR/LzmaCompress -e -o $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/$PROCESSOR/DxeIpl.efi	

echo Generate Loader Image ...

if [ $PROCESSOR = IA32 ]
then
  $BASETOOLS_DIR/GenFw --rebase 0x10000 -o $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/$PROCESSOR/EfiLoader.efi
  $BASETOOLS_DIR/EfiLdrImage -o $BUILD_DIR/FV/Efildr32 $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/DUETEFIMAINFV.z
  cat $BOOTSECTOR_BIN_DIR/Start16.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/Efildr16
  cat $BOOTSECTOR_BIN_DIR/Start32.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/Efildr20
  #cat $BOOTSECTOR_BIN_DIR/Start32.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/boot
  dd if=$BUILD_DIR/FV/Efildr20 of=$BUILD_DIR/FV/boot3 bs=512 skip=1

  echo Done!
fi

if [ $PROCESSOR = X64 ]
then
  $BASETOOLS_DIR/GenFw --rebase 0x10000 -o $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/$PROCESSOR/EfiLoader.efi
  $BASETOOLS_DIR/EfiLdrImage -o $BUILD_DIR/FV/Efildr64 $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/DUETEFIMAINFV.z

  cat $BOOTSECTOR_BIN_DIR/St32_64m.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/Efildr20Pure
  $BASETOOLS_DIR/GenPage -f 0x70000 $BUILD_DIR/FV/Efildr20Pure -o $BUILD_DIR/FV/Efildr20
  dd if=$BUILD_DIR/FV/Efildr20 of=$BUILD_DIR/FV/boot bs=512 skip=1

  cat $BOOTSECTOR_BIN_DIR/St_gpt_iT.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/Efildr20Pure
  $BASETOOLS_DIR/GenPage -f 0x70000 $BUILD_DIR/FV/Efildr20Pure -o $BUILD_DIR/FV/Efildgpt

  echo Done!
fi
}

fnMainBuildScript
fnMainPostBuildScript

