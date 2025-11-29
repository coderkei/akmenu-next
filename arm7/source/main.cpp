/*
    main.cpp
    Copyright (C) 2008-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <nds.h>
#include <string.h>
#include "../../share/fifotool.h"
#include "../../share/memtool.h"
#include "picoLoader7.h"

#ifdef __cplusplus
extern "C" {
#endif
extern void swiSwitchToGBAModeFixed(); // This is broken in libnds since 2008. Not fixed until libnds2 which this project isn't using yet.
void __libnds_exit(int rc) {}
#ifdef __cplusplus
}
#endif

void VblankHandler(void) {
    inputGetAndSend();
}

#define PM_NDSLITE_ADR (4)
#define PM_CONTROL2_REG (16)
#define PM_CONTROL2_RESET (1)
#define PM_NDSLITE_ISLITE BIT(6)
#define PM_NDSLITE_BRIGHTNESS(x) ((x & 0x03) << 0)
#define PM_NDSLITE_BRIGHTNESS_MASK (PM_NDSLITE_BRIGHTNESS(3))

static u32 getSystem(void) {
    static u32 dsGen = 0;
    if (!dsGen) {
        dsGen = 1;                                                             // default ds phat
        if (readPowerManagement(PM_NDSLITE_ADR) & PM_NDSLITE_ISLITE) dsGen++;  // dslite and dsi
        if (readPowerManagement(PM_NDSLITE_ADR) == readPowerManagement(5)) dsGen++;  // dslite
    }
    return dsGen;
}

static void prepairResetTT() {
    memcpy(__NDSHeader->arm7destination, *(void* volatile*)0x02FFFE00, __NDSHeader->arm7binarySize);
}

typedef void (*pico_loader_7_func_t)(void);

volatile bool reset_pico = false;

static void resetDSPico() {
    zeroMemory((void*)0x40000B0, 0x30);

    REG_IME = IME_DISABLE;
    REG_IE = 0;
    REG_IF = ~0;

    auto header7 = (pload_header7_t*)0x06000000;
    // header7->dldiDriver = (void*)0x037F8000;
    ((pico_loader_7_func_t)header7->entryPoint)();
}

static void prepairReset() {
    // enable sound
    if (2 == getSystem())
        writePowerManagement(PM_CONTROL_REG,
                             (readPowerManagement(PM_CONTROL_REG) & ~PM_SOUND_MUTE) | PM_SOUND_AMP);

    // disble rtc irq, on some games enabled rtc irq caused tearing.
    uint8 command[2];
    command[0] = WRITE_STATUS_REG2;
    command[1] = 0x00;
    rtcTransaction(command, 2, 0, 0);

    REG_POWERCNT |= POWER_SOUND;

    // reset DMA
    zeroMemory((void*)0x40000B0, 0x30);

    REG_IME = IME_DISABLE;
    REG_IE = 0;
    REG_IF = ~0;

    // instruct arm9 to reset
    *((vu32*)0x02FFFE04) = MENU_MSG_ARM7_READY_BOOT;
    swiDelay(1);
}

static void brightnessNext(void) {
    u8 data = readPowerManagement(PM_NDSLITE_ADR);
    if (0 == (data & PM_NDSLITE_ISLITE))  // this is not a DS Lite machine
        return;
    u8 level = data & PM_NDSLITE_BRIGHTNESS_MASK;
    level++;
    data &= ~PM_NDSLITE_BRIGHTNESS_MASK;
    data |= PM_NDSLITE_BRIGHTNESS(level);
    writePowerManagement(PM_NDSLITE_ADR, data);
}

static void brightnessSet(u8 level) {
    u8 data = readPowerManagement(PM_NDSLITE_ADR);
    if (0 == (data & PM_NDSLITE_ISLITE))  // this is not a DS Lite machine
        return;
    data &= ~PM_NDSLITE_BRIGHTNESS_MASK;
    data |= PM_NDSLITE_BRIGHTNESS(level);
    writePowerManagement(PM_NDSLITE_ADR, data);
}
static u8 brightnessGet(void) {
    u8 data = readPowerManagement(PM_NDSLITE_ADR);
    if (0 == (data & PM_NDSLITE_ISLITE))  // this is not a DS Lite machine
        return 0;
    return data & PM_NDSLITE_BRIGHTNESS_MASK;
}

static void menuValue32Handler(u32 value, void* data) {
    switch (value) {
        case MENU_MSG_GBA: {
            u32 ii = 0;
            if (PersonalData->gbaScreen)
                ii = (1 * PM_BACKLIGHT_BOTTOM) | PM_SOUND_AMP;
            else
                ii = (1 * PM_BACKLIGHT_TOP) | PM_SOUND_AMP;
            writePowerManagement(PM_CONTROL_REG, ii);
            swiChangeSoundBias(0, 0x400);
            swiSwitchToGBAModeFixed();
        } break;
        case MENU_MSG_ARM7_REBOOT_TT:
            prepairResetTT();
        case MENU_MSG_ARM7_REBOOT:
            prepairReset();
            swiSoftReset();
            break;
        case MENU_MSG_ARM7_REBOOT_PICOLOADER:
            reset_pico = true;
            break;
        case MENU_MSG_BRIGHTNESS_NEXT:
            brightnessNext();
            break;
        case MENU_MSG_SYSTEM:
            fifoSendValue32(FIFO_USER_02, getSystem());
            break;
        case MENU_MSG_BRIGHTNESS_GET:
            fifoSendValue32(FIFO_USER_01, brightnessGet());
            break;
        case MENU_MSG_BRIGHTNESS_SET0:
        case MENU_MSG_BRIGHTNESS_SET1:
        case MENU_MSG_BRIGHTNESS_SET2:
        case MENU_MSG_BRIGHTNESS_SET3:
            brightnessSet(value & 3);
            break;
        case MENU_MSG_SHUTDOWN:
            if (2 == getSystem())
                writePowerManagement(PM_CONTROL2_REG,
                                     readPowerManagement(PM_CONTROL2_REG) | PM_CONTROL2_RESET);
            else
                systemShutDown();
        default:
            break;
    }
}

int main() {
    // switch on backlight on both screens
    writePowerManagement(PM_CONTROL_REG, readPowerManagement(PM_CONTROL_REG) | PM_BACKLIGHT_BOTTOM |
                                                 PM_BACKLIGHT_TOP);

    // read User Settings from firmware
    readUserSettings();

    irqInit();
    fifoInit();
	touchInit();

    // Start the RTC tracking IRQ
    initClockIRQ();

    fifoSetValue32Handler(FIFO_USER_01, menuValue32Handler, 0);

    installSystemFIFO();

    irqSet(IRQ_VBLANK, VblankHandler);

    irqEnable(IRQ_VBLANK | IRQ_NETWORK);

	if (isDSiMode() && REG_SNDEXTCNT != 0) {
		i2cWriteRegister(0x4A, 0x12, 0x00);	// Press power-button for auto-reset
		i2cWriteRegister(0x4A, 0x70, 0x01);	// Bootflag = Warmboot/SkipHealthSafety
	}
       
    while (true) {
        if (*(u32*)(0x2FFFD0C) == 0x454D4D43) {
            sdmmc_get_cid(true, (u32*)0x2FFD7BC);    // Get eMMC CID
            *(u32*)(0x2FFFD0C) = 0;
        }
        swiWaitForVBlank();
        if(reset_pico)
            resetDSPico();
        swiWaitForVBlank();
    }
}

