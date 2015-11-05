## @file
#
#  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution. The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
##

OUTPUT_DIR=$(MODULE_DIR)/bin

TARGET_FILES = \
  $(OUTPUT_DIR)/bootsect.com \
  $(OUTPUT_DIR)/bs16.com \
  $(OUTPUT_DIR)/bs32.com \
  $(OUTPUT_DIR)/Gpt.com \
  $(OUTPUT_DIR)/Mbr.com \
  $(OUTPUT_DIR)/St16_64.com \
  $(OUTPUT_DIR)/St32_64.com \
  $(OUTPUT_DIR)/Start.com \
  $(OUTPUT_DIR)/Start16.com \
  $(OUTPUT_DIR)/Start32.com \
  $(OUTPUT_DIR)/Start64.com \
  $(OUTPUT_DIR)/efi32.com2 \
  $(OUTPUT_DIR)/efi64.com2 

all: $(TARGET_FILES)
          
#=============

$(OUTPUT_DIR)/bootsect.com: $(MODULE_DIR)/bootsect.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/bs16.com:     $(MODULE_DIR)/bs16.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/bs32.com:     $(MODULE_DIR)/bs32.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/Gpt.com:      $(MODULE_DIR)/Gpt.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/Mbr.com:      $(MODULE_DIR)/Mbr.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/Start.com:    $(MODULE_DIR)/Start.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/Start16.com:  $(MODULE_DIR)/Start16.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/Start32.com:  $(MODULE_DIR)/Start32.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/Start64.com:  $(MODULE_DIR)/Start64.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/St16_64.com:  $(MODULE_DIR)/St16_64.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/St32_64.com:  $(MODULE_DIR)/St32_64.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/efi32.com:    $(MODULE_DIR)/efi32.nasmb
  nasm -f bin -o $@ $?

$(OUTPUT_DIR)/efi64.com:    $(MODULE_DIR)/efi64.nasmb
  nasm -f bin -o $@ $?

#=============

$(OUTPUT_DIR)/efi32.com2:$(OUTPUT_DIR)/efi32.com
  Split -f $(OUTPUT_DIR)/efi32.com -t $(OUTPUT_DIR)/efi32.com2 -s 135168

$(OUTPUT_DIR)/efi64.com2:$(OUTPUT_DIR)/efi64.com
  Split -f $(OUTPUT_DIR)/efi64.com -t $(OUTPUT_DIR)/efi64.com2 -s 135168
