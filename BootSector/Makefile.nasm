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
  $(OUTPUT_DIR)/Gpt.com \
  $(OUTPUT_DIR)/Mbr.com \
  $(OUTPUT_DIR)/boot1_12.com \
  $(OUTPUT_DIR)/boot1_16.com \
  $(OUTPUT_DIR)/boot1_32.com \
  $(OUTPUT_DIR)/start12.com \
  $(OUTPUT_DIR)/start16.com \
  $(OUTPUT_DIR)/start32.com \
  $(OUTPUT_DIR)/start64.com \
  $(OUTPUT_DIR)/st16_64.com \
  $(OUTPUT_DIR)/st32_64.com \
  $(OUTPUT_DIR)/efi32.com2 \
  $(OUTPUT_DIR)/efi64.com2

all: $(TARGET_FILES)

#=============

$(OUTPUT_DIR)/Gpt.com: $(MODULE_DIR)/Gpt.nasmb
	nasm -i $(MODULE_DIR)/ -f bin -o $@ $?

$(OUTPUT_DIR)/Mbr.com: $(MODULE_DIR)/Mbr.nasmb
	nasm -i $(MODULE_DIR)/ -f bin -o $@ $?

#=============

$(OUTPUT_DIR)/boot1_12.com: $(MODULE_DIR)/boot1_XX.nasmb
	nasm -i $(MODULE_DIR)/ -DFAT=12 -f bin -o $@ $?

$(OUTPUT_DIR)/boot1_16.com: $(MODULE_DIR)/boot1_XX.nasmb
	nasm -i $(MODULE_DIR)/ -DFAT=16 -f bin -o $@ $?

$(OUTPUT_DIR)/boot1_32.com: $(MODULE_DIR)/boot1_XX.nasmb
	nasm -i $(MODULE_DIR)/ -DFAT=32 -f bin -o $@ $?

#=============

$(OUTPUT_DIR)/start12.com: $(MODULE_DIR)/start12.nasmb
	nasm -i $(MODULE_DIR)/ -f bin -o $@ $?

$(OUTPUT_DIR)/start16.com: $(MODULE_DIR)/start16.nasmb
	nasm -i $(MODULE_DIR)/ -f bin -o $@ $?

$(OUTPUT_DIR)/start32.com: $(MODULE_DIR)/start32.nasmb
	nasm -i $(MODULE_DIR)/ -f bin -o $@ $?

$(OUTPUT_DIR)/start64.com: $(MODULE_DIR)/start64.nasmb
	nasm -i $(MODULE_DIR)/ -f bin -o $@ $?

#=============

$(OUTPUT_DIR)/st16_64.com: $(MODULE_DIR)/st16_64.nasmb
	nasm -i $(MODULE_DIR)/ -f bin -o $@ $?

$(OUTPUT_DIR)/st32_64.com: $(MODULE_DIR)/st32_64.nasmb
	nasm -i $(MODULE_DIR)/ -f bin -o $@ $?

#=============

$(OUTPUT_DIR)/efi32.com: $(MODULE_DIR)/efi32.nasmb
	nasm -i $(MODULE_DIR)/ -f bin -o $@ $?

$(OUTPUT_DIR)/efi64.com: $(MODULE_DIR)/efi64.nasmb
	nasm -i $(MODULE_DIR)/ -f bin -o $@ $?

#=============

$(OUTPUT_DIR)/efi32.com2: $(OUTPUT_DIR)/efi32.com
	Split -f $(OUTPUT_DIR)/efi32.com -t $(OUTPUT_DIR)/efi32.com2 -s 135168

$(OUTPUT_DIR)/efi64.com2: $(OUTPUT_DIR)/efi64.com
	Split -f $(OUTPUT_DIR)/efi64.com -t $(OUTPUT_DIR)/efi64.com2 -s 135168
