#pragma once

#include "driver/i2c.h"

#define SDA_PIN GPIO_NUM_22
#define SCL_PIN GPIO_NUM_23
#define SCREEN_ADDRESS 0x3C

#define SW1_PIN GPIO_NUM_0
#define SW2_PIN GPIO_NUM_1
#define SW3_PIN GPIO_NUM_21
#define SW4_PIN GPIO_NUM_19
#define ENCODER_A_PIN GPIO_NUM_18
#define ENCODER_B_PIN GPIO_NUM_20

constexpr i2c_port_t I2C_MASTER_PORT = I2C_NUM_0;
constexpr uint32_t I2C_MASTER_FREQ_HZ = 100000;