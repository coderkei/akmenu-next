#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
export BLOCKSDS			?= /opt/blocksds/core
export BLOCKSDSEXT		?= /opt/blocksds/external
export WONDERFUL_TOOLCHAIN	?= /opt/wonderful

ifeq ($(strip $(BLOCKSDS)),)
$(error "Please set BLOCKSDS in your environment. export BLOCKSDS=<path to>blocksds/core")
endif
ifeq ($(wildcard $(BLOCKSDS)/sys/crts/ds_arm9.specs),)
$(error "$(BLOCKSDS)/sys/crts/ds_arm9.specs not found. Check BLOCKSDS")
endif

export TARGET := $(shell basename $(CURDIR))
export TOPDIR := $(CURDIR)

# GMAE_ICON is the image used to create the game icon, leave blank to use default rule
GAME_ICON := icon.bmp

# specify a directory which contains the nitro filesystem
# this is relative to the Makefile
NITRO_FILES :=

# These set the information text in the nds file
GAME_TITLE     := AKMenu-Next
GAME_SUBTITLE1 := github.com/coderkei

ifeq ($(OS),Windows_NT)
    MAKE_CIA = ./tools/make_cia.exe
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        MAKE_CIA = ./tools/make_cia
    endif
endif

.PHONY: nds-bootloader checkarm7 checkarm9 clean organize_files make_cia

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all:$(TARGET).nds $(TARGET).dsi
	@$(MAKE) organize_files

data:
	@mkdir -p data

nds-bootloader: data
	$(MAKE) -C nds-bootloader LOADBIN=$(CURDIR)/data/load.bin

#---------------------------------------------------------------------------------
checkarm7:
	+$(MAKE) -f Makefile.arm7 --no-print-directory

#---------------------------------------------------------------------------------
checkarm9: nds-bootloader
	+$(MAKE) -f Makefile.arm9 --no-print-directory

#---------------------------------------------------------------------------------

$(TARGET).nds : $(NITRO_FILES) checkarm7 checkarm9
	$(BLOCKSDS)/tools/ndstool/ndstool	-c $(TARGET).nds -7 build/arm7.elf -9 build/arm9.elf \
	-h 0x200 -t banner.bin \
	$(_ADDFILES)

$(TARGET).dsi : $(NITRO_FILES) checkarm7 checkarm9
	$(BLOCKSDS)/tools/ndstool/ndstool	-c $@ -7 build/arm7.elf -9 build/arm9.elf \
	-t banner.bin \
	-g NEXT 01 "AKMENU" -z 80040407 -u 00030004 -a 00000138 -p 0001 \
	$(_ADDFILES)

#---------------------------------------------------------------------------------
organize_files:
	@mv -f $(TARGET).nds $(TARGET).dsi package/
	cp package/$(TARGET).dsi package/title/00030004/4e455854/content/00000000.app
	cp package/$(TARGET).nds package/boot.nds
	cp package/$(TARGET).dsi package/boot.dsi
	@$(MAKE) make_cia
	rm -f package/$(TARGET).nds package/$(TARGET).dsi
	cp -r language package/_nds/akmenunext/

#---------------------------------------------------------------------------------
make_cia:
	$(MAKE_CIA) --srl=$(CURDIR)/package/$(TARGET).dsi -o $(CURDIR)/package/$(TARGET).cia

#---------------------------------------------------------------------------------
clean:
	$(MAKE) -f Makefile.arm9 clean --no-print-directory
	$(MAKE) -C nds-bootloader clean
	$(MAKE) -f Makefile.arm7 clean --no-print-directory
	rm -rf build
	rm -rf data
	rm -f package/*.nds package/*.dsi package/*.cia
	rm -f *.nds *.dsi
	rm -rf package/_nds/akmenunext/language
