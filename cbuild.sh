#!/bin/sh

set -e

export WORKSPACE=/Volumes/Data/Workspace
export PACKAGES_PATH=$WORKSPACE/bareBoot:$WORKSPACE/edk2
export EDK_TOOLS_PATH=$WORKSPACE/edk2/BaseTools
export GCC5_BIN=$WORKSPACE/opt/local/cross/bin/x86_64-clover-linux-gnu-
export NASM_PREFIX=$WORKSPACE/opt/local/bin/

echo NASM_PREFIX: $NASM_PREFIX
echo GCC5_BIN: $GCC5_BIN

RECONFIG=FALSE
. $EDK_TOOLS_PATH/BuildEnv


## MAIN ARGUMENT PART##

export TARGET_TOOLS=GCC5
export PROCESSOR=X64
export TARGET=RELEASE
export DEF="-D NO_LOGO -D NO_FONT"

while [[ $# -gt 0 ]]; do
  case $1 in
        "-ia32")    PROCESSOR=IA32;;
        "-x64")     PROCESSOR=X64;;
        "-debug")   TARGET=DEBUG;;
        "-release") TARGET=RELEASE;;
        "-clean")   export ARG=clean;;
        "-cleanall") export ARG=cleanall;;
        "-b") DEF="$DEF -D BLOCKIO" ;;
        "-s") DEF="$DEF -D SPEEDUP" ;;
        "-vhfs") DEF="$DEF -D VBOXHFS" ;;
        "-ohci") DEF="$DEF -D OHCI" ;;
        "-edk2_cpudxe") DEF="$DEF -D EDK2_CPUDXE" ;;
        "-ps2") DEF="$DEF -D PS2" ;;
        "-m2s") DEF="$DEF -D MEMLOG2SERIAL" ;;
        "-ion") DEF="$DEF -D ION" ;;
        "-xcode5")    TARGET_TOOLS=XCODE5;;
        "-*")
         echo $"ERROR!"
        exit 1
    esac
    shift
done

fnMainBuildScript ()
{
echo Running edk2 build for bareBoot$PROCESSOR with $DEF

VERFILE=Version.h
echo "#define FIRMWARE_VERSION L\"2.31\"" > $VERFILE
echo "#define FIRMWARE_BUILDDATE L\"`LC_ALL=C date \"+%Y-%m-%d %H:%M:%S\"`\"" >> $VERFILE
echo "#define FIRMWARE_REVISION L\"`git tag | tail -n 1`\"" >> $VERFILE
echo "#define FIRMWARE_BUILDDATE_ASCII \" (`LC_ALL=C date \"+%Y-%m-%d %H:%M:%S\"`)\"" >> $VERFILE
echo "#define FIRMWARE_REVISION_ASCII \"bareBoot `git tag | tail -n 1`\"" >> $VERFILE
if ! [ -d $WORKSPACE/Build ]; then
  mkdir $WORKSPACE/Build
fi

build -j $WORKSPACE/Build/build.log -p bareBoot.dsc -a $PROCESSOR -b $TARGET -t $TARGET_TOOLS -n 3 $DEF
}


fnMainPostBuildScript ()
{
export BASETOOLS_DIR=$EDK_TOOLS_PATH/Source/C/bin
export BOOTSECTOR_BIN_DIR=BootSector/bin
export BOOTSECTOR2_BIN_DIR=BootSector2/bin
export BUILD_DIR=$WORKSPACE/Build/bareBoot/$PROCESSOR/"$TARGET"_"$TARGET_TOOLS"

echo Compressing bareBootEFIMainFv.FV ...
$BASETOOLS_DIR/LzmaCompress -e -o $BUILD_DIR/FV/bareBootEFIMAINFV.z $BUILD_DIR/FV/BAREBOOTEFIMAINFV.Fv

echo Compressing DxeMain.efi ...
$BASETOOLS_DIR/LzmaCompress -e -o $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/$PROCESSOR/DxeCore.efi

echo Compressing DxeIpl.efi ...
$BASETOOLS_DIR/LzmaCompress -e -o $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/$PROCESSOR/DxeIpl.efi	

echo Generate Loader Image ...
$BASETOOLS_DIR/GenFw --rebase 0x10000 -o $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/$PROCESSOR/EfiLoader.efi
$BASETOOLS_DIR/EfiLdrImage -o $BUILD_DIR/FV/Efildr$PROCESSOR $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/bareBootEFIMAINFV.z
cat $BOOTSECTOR2_BIN_DIR/EfiLdrPrelude$PROCESSOR $BUILD_DIR/FV/Efildr$PROCESSOR > $BUILD_DIR/FV/boot

if [ $PROCESSOR = IA32_ ]
then
  $BASETOOLS_DIR/GenFw --rebase 0x10000 -o $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/$PROCESSOR/EfiLoader.efi
  $BASETOOLS_DIR/EfiLdrImage -o $BUILD_DIR/FV/Efildr32 $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/bareBootEFIMAINFV.z
  cat $BOOTSECTOR_BIN_DIR/start16.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/Efildr16
  cat $BOOTSECTOR_BIN_DIR/start32.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/Efildr20
  #cat $BOOTSECTOR_BIN_DIR/start32.com $BOOTSECTOR_BIN_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/boot
  dd if=$BUILD_DIR/FV/Efildr20 of=$BUILD_DIR/FV/boot3 bs=512 skip=1

  echo Done!
fi

if [ $PROCESSOR = X64_ ]
then
  $BASETOOLS_DIR/GenFw --rebase 0x10000 -o $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/$PROCESSOR/EfiLoader.efi
  $BASETOOLS_DIR/EfiLdrImage -o $BUILD_DIR/FV/Efildr64 $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/bareBootEFIMAINFV.z

  cat $BOOTSECTOR_BIN_DIR/st32_64m.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/Efildr20Pure
  $BASETOOLS_DIR/GenPage -f 0x70000 $BUILD_DIR/FV/Efildr20Pure -o $BUILD_DIR/FV/Efildr20
  dd if=$BUILD_DIR/FV/Efildr20 of=$BUILD_DIR/FV/boot bs=512 skip=1

  cat $BOOTSECTOR_BIN_DIR/st_gpt_it.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/Efildr20Pure
  $BASETOOLS_DIR/GenPage -f 0x70000 $BUILD_DIR/FV/Efildr20Pure -o $BUILD_DIR/FV/Efildgpt

  echo Done!
fi
}

fnMainBuildScript
fnMainPostBuildScript
