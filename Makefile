#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

export TARGET := $(shell basename $(CURDIR))
export TOPDIR := $(CURDIR)

# GMAE_ICON is the image used to create the game icon, leave blank to use default rule
GAME_ICON :=

# specify a directory which contains the nitro filesystem
# this is relative to the Makefile
NITRO_FILES :=

# These set the information text in the nds file
GAME_TITLE     := acekard
GAME_SUBTITLE1 := Real Play Gear
GAME_SUBTITLE2 := www.acekard.com

include $(DEVKITARM)/ds_rules

.PHONY: checkarm7 checkarm9 checkarm9_dsi clean

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: checkarm7 checkarm9 checkarm9_dsi $(TARGET).nds $(TARGET).dsi

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
	-b $(GAME_ICON) "$(GAME_TITLE);$(GAME_SUBTITLE1);$(GAME_SUBTITLE2)" \
	$(_ADDFILES)

#---------------------------------------------------------------------------------
$(TARGET).dsi : $(NITRO_FILES) arm7/$(TARGET).elf arm9_dsi/$(TARGET).elf
	ndstool	-c $@ -7 arm7/$(TARGET).elf -9 arm9_dsi/$(TARGET).elf \
	-b $(GAME_ICON) "$(GAME_TITLE);$(GAME_SUBTITLE1);$(GAME_SUBTITLE2)" \
	$(_ADDFILES)

#---------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	$(MAKE) -C arm7

#---------------------------------------------------------------------------------
arm9/$(TARGET).elf:
	$(MAKE) -C arm9

#---------------------------------------------------------------------------------
arm9_dsi/$(TARGET).elf:
	$(MAKE) -C arm9_dsi

#---------------------------------------------------------------------------------
clean:
	$(MAKE) -C arm9 clean
	$(MAKE) -C arm9_dsi clean
	$(MAKE) -C arm7 clean
	rm -f *.nds *.dsi
