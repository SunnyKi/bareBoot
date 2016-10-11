#!/bin/sh

EDK2HOME=$BBHOME/../edk2; export EDK2HOME
EDK2CFG=$BBHOME/cppcheck/edk2.cfg; export EDK2HOME

exec $BBHOME/cppcheck/cppcheck.sh \
	--library=$BBHOME/cppcheck/bareBoot.cfg \
	--inconclusive \
	\
	-D HOST_EFI \
	-D VBOX \
	-D FSTYPE=hfs \
	-D FSW_DEBUG_LEVEL=3 \
	\
	-I $BBHOME/DxeIpl \
	-I $BBHOME/EfiLdr \
	-I $BBHOME/Include \
	-I $BBHOME/Library/BdsDxe \
	-I $BBHOME/Library/BdsDxe/Graphics \
	-I $BBHOME/Library/BdsDxe/GenericBds \
	-I $BBHOME/Library/PListLib \
	-I $BBHOME/Library/PListLib/b64 \
	-I $BBHOME/VBoxFsDxe \
	\
	-I $EDK2HOME/MdePkg/Include \
	-I $EDK2HOME/MdePkg/Include/Ia32 \
	-I $EDK2HOME/MdeModulePkg/Include \
	-I $EDK2HOME/IntelFrameworkPkg/Include \
	-I $EDK2HOME/IntelFrameworkModulePkg/Include \
	-I $EDK2HOME/UefiCpuPkg/Include \
	\
	-i $BBHOME/Library/PListLib/main.c \
	-i $BBHOME/Library/PListLib/plist_helpers_os.c \
	-i $BBHOME/VBoxFsDxe/msvc/ \
	-i $BBHOME/VBoxFsDxe/posix/ \
	$*
