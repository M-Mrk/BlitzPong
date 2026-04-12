#pragma once

#include "esp_err.h"
#include "U8g2lib.h"
#include "pins.h"

constexpr int BUTTON_Y = 63;

esp_err_t display_init();
u8g2_t *display_get_u8g2();
