#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

export TARGET := $(shell basename $(CURDIR))
export TOPDIR := $(CURDIR)

# GMAE_ICON is the image used to create the game icon, leave blank to use default rule
GAME_ICON := icon.bmp

# specify a directory which contains the nitro filesystem
# this is relative to the Makefile
NITRO_FILES :=

# These set the information text in the nds file
GAME_TITLE     := akmenu-next
GAME_SUBTITLE1 := nds-bootstrap
GAME_SUBTITLE2 := github.com/coderkei

include $(DEVKITARM)/ds_rules

#.PHONY: checkarm7 checkarm9 checkarm9_ak2 checkarm9_dsi checkarm9_m3 checkarm9_tt clean
.PHONY: nds-bootloader checkarm7 checkarm9 checkarm9_dsi clean

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all:	nds-bootloader checkarm7 checkarm9 checkarm9_dsi \
		$(TARGET).nds $(TARGET).dsi
	@$(MAKE) organize_files

data:
	@mkdir -p data

nds-bootloader: data
	$(MAKE) -C nds-bootloader LOADBIN=$(CURDIR)/data/load.bin

#---------------------------------------------------------------------------------
checkarm7:
	$(MAKE) -C arm7

#---------------------------------------------------------------------------------
checkarm9:
	$(MAKE) -C arm9

#---------------------------------------------------------------------------------
checkarm9_dsi:
	$(MAKE) -C arm9_dsi

#---------------------------------------------------------------------------------
$(TARGET).nds : $(NITRO_FILES) arm7/$(TARGET).elf arm9/$(TARGET).elf
	ndstool	-c $(TARGET).nds -7 arm7/$(TARGET).elf -9 arm9/$(TARGET).elf \
	-h 0x200 -t banner.bin \
	$(_ADDFILES)

#---------------------------------------------------------------------------------
$(TARGET).dsi : $(NITRO_FILES) arm7/$(TARGET).elf arm9_dsi/$(TARGET).elf
	ndstool	-c $@ -7 arm7/$(TARGET).elf -9 arm9_dsi/$(TARGET).elf \
	-t banner.bin \
	-g AKGE 01 "AKMENU" -z 80040407 -u 00030004 -a 00000138 -p 0001 \
	$(_ADDFILES)

#---------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	$(MAKE) -C arm7

#---------------------------------------------------------------------------------
arm9/$(TARGET).elf: nds-bootloader
	$(MAKE) -C arm9

#---------------------------------------------------------------------------------
arm9_dsi/$(TARGET).elf:
	$(MAKE) -C arm9_dsi

#---------------------------------------------------------------------------------
organize_files:
	@mkdir -p build
	@mv -f $(TARGET).nds $(TARGET).dsi build/
	@cd build && \
	cp $(TARGET).nds launcher_$(TARGET).nds && \
	cp $(TARGET).dsi launcher_$(TARGET).dsi

#---------------------------------------------------------------------------------
clean:
	$(MAKE) -C arm9 clean
	$(MAKE) -C arm9_dsi clean
	$(MAKE) -C nds-bootloader clean
	$(MAKE) -C arm7 clean
	rm -rf data build
	rm -f *.nds *.dsi
