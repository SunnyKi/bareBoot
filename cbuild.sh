#!/bin/sh

## FUNCTIONS ##

fnGCC46 ()
# Function: Xcode chainload
{
[ ! -f /usr/bin/xcodebuild ] && \
echo "ERROR: Install Xcode Tools from Apple before using this script." && exit
echo "CHAINLOAD: GCC46"
export TARGET_TOOLS=GCC46
}

fnArchIA32 ()
# Function: IA32 Arch function
{
echo "ARCH: IA32"
export PROCESSOR=IA32
export Processor=Ia32
}

fnArchX64 ()
# Function: X64 Arch function
{
echo "ARCH: X64"
export PROCESSOR=X64
export Processor=X64
}

fnDebug ()
# Function: Debug version of compiled source
{
echo "TARGET: DEBUG"
export TARGET=DEBUG
export VTARGET=DEBUG
}

fnRelease ()
# Function: Release version of compiled source
{
echo "TARGET: RELEASE"
export TARGET=RELEASE
export VTARGET=RELEASE
}


fnHelpArgument ()
# Function: Help with arguments
{
echo "ERROR!"
echo "Example: ./cbuild.sh -xcode -ia32 -release"
echo "Example: ./cbuild.sh -gcc46 -x64 -release"
}

## MAIN ARGUMENT PART##

    fnGCC46
    case "$1" in
        '')
        fnArchX64
        ;;
        '-ia32')
         fnArchIA32
        ;;
        '-x64')
         fnArchX64
        ;;
        *)
         echo $"ERROR!"
         echo $"ARCH: {-ia32|-x64}"
        exit 1
    esac
    case "$2" in
        '')
         fnRelease
        ;;
        '-debug')
         fnDebug
        ;;
        '-release')
         fnRelease
        ;;
        *)
         echo $"ERROR!"
         echo $"TYPE: {-debug|-release}"
        exit 1
    esac
    case "$3" in
        '-clean')
        export ARG=clean
        ;;
        '-cleanall')
        export ARG=cleanall
        ;;
    esac

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


BUILD_ROOT_ARCH=$WORKSPACE/Build/miniClover$PROCESSOR/"$VTARGET"_"$TARGET_TOOLS"/$PROCESSOR

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
build -p $WORKSPACE/miniClover/miniClover$Processor.dsc -a $PROCESSOR -b $VTARGET -t $TARGET_TOOLS -n 3 clean
exit $?
fi

if [[ $ARG == clean ]]; then
build -p $WORKSPACE/miniClover/miniClover$Processor.dsc -a $PROCESSOR -b $VTARGET -t $TARGET_TOOLS -n 3 clean
exit $?
fi

# Build the edk2 miniClover
echo Running edk2 build for miniClover$Processor
echo "#define FIRMWARE_VERSION \"2.31\"" > $WORKSPACE/miniClover/Version.h
echo "#define FIRMWARE_BUILDDATE L\"`date \"+%Y-%m-%d %H:%M:%S\"`\"" >> $WORKSPACE/miniClover/Version.h
echo "#define FIRMWARE_REVISION L\"`cd $WORKSPACE/miniClover; git tag | head -n 1`\"" >> $WORKSPACE/miniClover/Version.h

build -p $WORKSPACE/miniClover/miniClover$Processor.dsc -a $PROCESSOR -b $VTARGET -t $TARGET_TOOLS -n 3 $*

}


fnMainPostBuildScript ()
{
if [ -z "$EDK_TOOLS_PATH" ]
then
export BASETOOLS_DIR=$WORKSPACE/BaseTools/Source/C/bin
else
export BASETOOLS_DIR=$EDK_TOOLS_PATH/Source/C/bin
fi
export BOOTSECTOR_BIN_DIR=$WORKSPACE/miniClover/BootSector/bin
export BUILD_DIR=$WORKSPACE/Build/miniClover$PROCESSOR/"$VTARGET"_"$TARGET_TOOLS"

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

cat $BOOTSECTOR_BIN_DIR/start32.com $BOOTSECTOR_BIN_DIR/efi32.com3 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/Efildr20	
cat $BOOTSECTOR_BIN_DIR/start32H.com2 $BOOTSECTOR_BIN_DIR/efi32.com3 $BUILD_DIR/FV/Efildr32 > $BUILD_DIR/FV/boot
echo Done!
fi

if [ $PROCESSOR = X64 ]
then
$BASETOOLS_DIR/GenFw --rebase 0x10000 -o $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/$PROCESSOR/EfiLoader.efi
$BASETOOLS_DIR/EfiLdrImage -o $BUILD_DIR/FV/Efildr64 $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/DUETEFIMAINFV.z
cat $BOOTSECTOR_BIN_DIR/St32_64.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/Efildr20Pure
#cat $BOOTSECTOR_BIN_DIR/St_20_iT.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/Efildr20Pure
$BASETOOLS_DIR/GenPage $BUILD_DIR/FV/Efildr20Pure -o $BUILD_DIR/FV/Efildr20
cat $BOOTSECTOR_BIN_DIR/St_gpt_iT.com $BOOTSECTOR_BIN_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 > $BUILD_DIR/FV/Efildr20Pure
$BASETOOLS_DIR/GenPage $BUILD_DIR/FV/Efildr20Pure -o $BUILD_DIR/FV/Efildgpt
dd if=$BUILD_DIR/FV/Efildr20 of=$BUILD_DIR/FV/boot bs=512 skip=1

echo Done!
fi

}

fnMainBuildScript
fnMainPostBuildScript

