#pragma once
#include <nds.h>

#ifdef __cplusplus
extern "C" {
#endif

void my_i2cDelay();
void my_i2cStop(u8 arg0);
u8 my_i2cGetResult();
u8 my_i2cGetData();
void my_i2cSetDelay(u8 device);
u8 my_i2cSelectDevice(u8 device);
u8 my_i2cSelectRegister(u8 reg);
u8 my_i2cWriteRegister(u8 device, u8 reg, u8 data);
u8 my_i2cReadRegister(u8 device, u8 reg);
#ifdef __cplusplus
}
#endif