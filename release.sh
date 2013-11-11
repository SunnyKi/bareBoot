#!/bin/sh
export BB_SOURCE_PATH=~/src/edk2/bareBoot
export BB_ARCHIVE_PATH=~/src/bb
export BUILD_PATH=~/src/edk2/Build

if [[ $# -gt 0 ]];
then
ARCHIVE_NAME=bb_$1
else
ARCHIVE_NAME=bb_`git tag | tail -n 1`
fi
echo $ARCHIVE_NAME

#SPEEDUP
cd $BB_SOURCE_PATH
source cbuild.sh -s -ia32
cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC47/FV/boot3 $BB_ARCHIVE_PATH/for\ HDD/ata/32/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh -s
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC47/FV/boot $BB_ARCHIVE_PATH/for\ HDD/ata/64/
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC47/FV/Efildgpt $BB_ARCHIVE_PATH/for\ HDD/ata/64/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh -s -ia32 -b
cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC47/FV/boot3 $BB_ARCHIVE_PATH/for\ HDD/BlockIo/32/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh -s -b
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC47/FV/boot $BB_ARCHIVE_PATH/for\ HDD/BlockIo/64/
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC47/FV/Efildgpt $BB_ARCHIVE_PATH/for\ HDD/BlockIo/64/
rm -rf $BUILD_PATH/bareboot/

# no SPEEDUP
cd $BB_SOURCE_PATH
source cbuild.sh -ia32
cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC47/FV/boot3 $BB_ARCHIVE_PATH/standard/ata/32/
cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC47/FV/Efildr20 $BB_ARCHIVE_PATH/standard/ata/32/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh 
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC47/FV/boot $BB_ARCHIVE_PATH/standard/ata/64/
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC47/FV/Efildr20 $BB_ARCHIVE_PATH/standard/ata/64/
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC47/FV/Efildgpt $BB_ARCHIVE_PATH/standard/ata/64/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh -b -ia32
cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC47/FV/boot3 $BB_ARCHIVE_PATH/standard/BlockIo/32/
cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC47/FV/Efildr20 $BB_ARCHIVE_PATH/standard/BlockIo/32/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh -b
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC47/FV/boot $BB_ARCHIVE_PATH/standard/BlockIo/64/
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC47/FV/Efildr20 $BB_ARCHIVE_PATH/standard/BlockIo/64/
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC47/FV/Efildgpt $BB_ARCHIVE_PATH/standard/BlockIo/64/
rm -rf $BUILD_PATH/bareboot/

# archive
cd $BB_ARCHIVE_PATH/..
zip -rv $ARCHIVE_NAME bb -x "*.DS_Store"
