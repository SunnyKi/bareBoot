#!/bin/sh

EDK2HOME=../edk2; export EDK2HOME

exec cppcheck \
	--library=$EDK2HOME/cppcheck/edk2.cfg \
	-I DxeIpl \
	-I EfiLdr \
	-I Include \
	-I Library/BdsDxe \
	-I Library/BdsDxe/Graphics \
	-I Library/BdsDxe/GenericBds \
	-I Library/PListLib \
	-I Library/PListLib/b64 \
	-I VBoxFsDxe \
	-I $EDK2HOME/MdePkg/Include \
	-I $EDK2HOME/MdePkg/Include/Ia32 \
	-I $EDK2HOME/MdeModulePkg/Include \
	-I $EDK2HOME/IntelFrameworkPkg/Include \
	-I $EDK2HOME/IntelFrameworkModulePkg/Include \
	-I $EDK2HOME/UefiCpuPkg/Include \
	$*
