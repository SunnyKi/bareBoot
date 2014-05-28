#!/bin/sh

echo "#define FIRMWARE_VERSION L\"2.31\"" 
echo "#define FIRMWARE_BUILDDATE L\"`LC_ALL=C date \"+%Y-%m-%d %H:%M:%S\"`\"" 
echo "#define FIRMWARE_REVISION L\"`git tag | tail -n 1`\"" 
echo "#define FIRMWARE_BUILDDATE_ASCII \" (`LC_ALL=C date \"+%Y-%m-%d %H:%M:%S\"`)\"" 
echo "#define FIRMWARE_REVISION_ASCII \"bareBoot `git tag | tail -n 1`\"" 
