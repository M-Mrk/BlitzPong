#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "U8g2lib.h"
#include <string>

#include "display.h"
#include "targets.h"
#include "inputs.h"

namespace ThresholdScreen {
    bool show(Targets &targets, Inputs &inputs, u8g2_t *u8g2);
}