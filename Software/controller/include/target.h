#pragma once

#include "driver/i2c.h"
#include "esp_log.h"
#include "commands.h"
#include "pins.h"

class Target
{
public:
    uint8_t address;
    bool enabled = false;

    Target(uint8_t addr);

    bool get_hit();
    uint8_t get_version();
    uint8_t get_threshold();
    esp_err_t set_threshold(uint8_t threshold);
    uint8_t get_vibration();
    esp_err_t set_auto_disable(bool auto_disable);
    esp_err_t enable();
    esp_err_t disable();

private:
    bool auto_disable = false;
    esp_err_t send_set_cmd(uint8_t cmd, uint8_t value);
    uint8_t send_get_cmd(uint8_t cmd);
};