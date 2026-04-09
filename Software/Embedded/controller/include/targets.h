#pragma once

#include "driver/i2c.h"
#include "esp_log.h"
#include <vector>
#include "pins.h"
#include "target.h"

class Targets {
public:
    std::vector<Target> connected_targets;
    bool auto_disable = false;

    esp_err_t init();
    void update_targets();

    void set_threshold_all(uint8_t threshold);
    esp_err_t enable_target(int target_index);
    esp_err_t disable_target(int target_index);

    std::vector<int> check_enabled_hit();
private:
    std::vector<uint8_t> scan_targets();
    void add_target(uint8_t address);
    void remove_target(uint8_t address);
    esp_err_t probe_address(uint8_t address);
};
