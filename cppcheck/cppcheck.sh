#!/bin/sh

exec cppcheck \
	--library=$EDK2CFG \
	--inconclusive \
	\
	-I $EDK2HOME/MdePkg/Include \
	-I $EDK2HOME/MdePkg/Include/Ia32 \
	-I $EDK2HOME/MdeModulePkg/Include \
	-I $EDK2HOME/IntelFrameworkPkg/Include \
	-I $EDK2HOME/IntelFrameworkModulePkg/Include \
	-I $EDK2HOME/UefiCpuPkg/Include \
	\
	$*
