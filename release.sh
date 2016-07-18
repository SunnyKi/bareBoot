#!/bin/sh
export BB_SOURCE_PATH=~/src/edk2/bareBoot
export BB_ARCHIVE_PATH=~/src/bb
export BUILD_PATH=~/src/edk2/Build

if [[ $# -gt 0 ]];
then
ARCHIVE_NAME=bb_$1.zip
else
ARCHIVE_NAME=bb_`git tag | tail -n 1`.zip
fi
echo $ARCHIVE_NAME

#SPEEDUP
cd $BB_SOURCE_PATH
source cbuild.sh -s -ia32 -old_cpudxe
cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC49/FV/boot $BB_ARCHIVE_PATH/for\ HDD/ata/32/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh -s -old_cpudxe
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC49/FV/boot $BB_ARCHIVE_PATH/for\ HDD/ata/64/
#cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC49/FV/Efildgpt $BB_ARCHIVE_PATH/for\ HDD/ata/64/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh -s -ia32 -b -old_cpudxe
cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC49/FV/boot $BB_ARCHIVE_PATH/for\ HDD/BlockIo/32/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh -s -b -old_cpudxe
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC49/FV/boot $BB_ARCHIVE_PATH/for\ HDD/BlockIo/64/
#cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC49/FV/Efildgpt $BB_ARCHIVE_PATH/for\ HDD/BlockIo/64/
rm -rf $BUILD_PATH/bareboot/

# no SPEEDUP
cd $BB_SOURCE_PATH
source cbuild.sh -ia32 -old_cpudxe
cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC49/FV/boot $BB_ARCHIVE_PATH/standard/ata/32/
#cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC49/FV/Efildr20 $BB_ARCHIVE_PATH/standard/ata/32/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh  -old_cpudxe
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC49/FV/boot $BB_ARCHIVE_PATH/standard/ata/64/
#cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC49/FV/Efildr20 $BB_ARCHIVE_PATH/standard/ata/64/
#cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC49/FV/Efildgpt $BB_ARCHIVE_PATH/standard/ata/64/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh -b -ia32 -old_cpudxe
cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC49/FV/boot $BB_ARCHIVE_PATH/standard/BlockIo/32/
#cp -vf $BUILD_PATH/bareboot/IA32/RELEASE_GCC49/FV/Efildr20 $BB_ARCHIVE_PATH/standard/BlockIo/32/
rm -rf $BUILD_PATH/bareboot/

cd $BB_SOURCE_PATH
source cbuild.sh -b -old_cpudxe
cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC49/FV/boot $BB_ARCHIVE_PATH/standard/BlockIo/64/
#cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC49/FV/Efildr20 $BB_ARCHIVE_PATH/standard/BlockIo/64/
#cp -vf $BUILD_PATH/bareboot/X64/RELEASE_GCC49/FV/Efildgpt $BB_ARCHIVE_PATH/standard/BlockIo/64/
rm -rf $BUILD_PATH/bareboot/

# archive
cd $BB_ARCHIVE_PATH/..
zip -rv $ARCHIVE_NAME bb -x "*.DS_Store"
