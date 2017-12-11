#!/bin/sh

#	-D HOST_EFI \
#	-D VBOX \
#	-D FSTYPE=hfs \
#	-D FSW_DEBUG_LEVEL=3 \
#	\
#	-i $BBHOME/Library/PListLib/main.c \
#	-i $BBHOME/Library/PListLib/plist_helpers_os.c \
#	-i $BBHOME/VBoxFsDxe/msvc/ \
#	-i $BBHOME/VBoxFsDxe/posix/ \


EDK2HOME=$BBHOME/../edk2; export EDK2HOME
EDK2CFG=$BBHOME/cppcheck/edk2.cfg; export EDK2CFG

exec $BBHOME/cppcheck/cppcheck.sh \
	--library=$BBHOME/cppcheck/bareBoot.cfg \
	--inconclusive \
	\
	-D MDE_CPU_X64 \
	\
	-I $BBHOME/DxeIpl \
	-I $BBHOME/EfiLdr \
	-I $BBHOME/Include \
	-I $BBHOME/BdsDxe \
	-I $BBHOME/BdsDxe/Graphics \
	-I $BBHOME/BdsDxe/GenericBds \
	-I $BBHOME/Library/PListLib \
	-I $BBHOME/Library/PListLib/b64 \
	-I $BBHOME/VBoxFsDxe \
	\
	-I $EDK2HOME/MdePkg/Include \
	-I $EDK2HOME/MdePkg/Include/X64 \
	-I $EDK2HOME/MdeModulePkg/Include \
	-I $EDK2HOME/IntelFrameworkPkg/Include \
	-I $EDK2HOME/IntelFrameworkModulePkg/Include \
	-I $EDK2HOME/UefiCpuPkg/Include \
	\
	$*
