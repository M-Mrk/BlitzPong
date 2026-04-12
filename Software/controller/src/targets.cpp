#include "targets.h"
static const char *TAG = "TARGETS";

esp_err_t Targets::probe_address(uint8_t address)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    return err;
}

esp_err_t Targets::init()
{
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = SDA_PIN;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = SCL_PIN;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    esp_err_t err = i2c_param_config(I2C_MASTER_PORT, &conf);
    if (err != ESP_OK)
    {
        return err;
    }

    return i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);
}

void Targets::add_target(uint8_t address)
{
    for (Target &target : connected_targets)
    {
        if (target.address == address)
        {
            ESP_LOGW(TAG, "Target at address 0x%02X already exists, skipping", address);
            return;
        }
    }

    Target new_target = Target(address);
    connected_targets.push_back(new_target);
    ESP_LOGI(TAG, "Added target at address 0x%02X", address);
}

void Targets::remove_target(uint8_t address)
{
    for (int index = 0; index < connected_targets.size(); index++)
    {
        if (connected_targets[index].address == address)
        {
            connected_targets.erase(connected_targets.begin() + index);
            ESP_LOGI(TAG, "Removed target at address 0x%02X", address);
            return;
        }
    }
    ESP_LOGW(TAG, "Target at address 0x%02X not found, cannot remove", address);
}

std::vector<uint8_t> Targets::scan_targets()
{
    std::vector<uint8_t> address_list;
    int found_count = 0;
    ESP_LOGI(TAG, "Scanning I2C bus for targets...");

    for (int addr = 1; addr < 127; ++addr)
    {
        if (addr == SCREEN_ADDRESS)
        {
            continue;
        }
        esp_err_t err = probe_address(addr);
        if (err == ESP_OK)
        {
            found_count++;
            address_list.push_back(addr);
            ESP_LOGI(TAG, "Found I2C target at 0x%02X", addr);
        }
    }

    if (found_count == 0)
    {
        ESP_LOGW(TAG, "No targets found on the bus");
    }
    else
    {
        ESP_LOGI(TAG, "Scan complete: %d target(s) found", found_count);
    }
    return address_list;
}

void Targets::update_targets()
{
    std::vector<uint8_t> current_connected_targets = scan_targets();

    // Add newly discovered targets.
    for (uint8_t address : current_connected_targets)
    {
        add_target(address);
    }

    // Remove targets that are no longer present in the latest scan.
    connected_targets.erase(
        std::remove_if(
            connected_targets.begin(),
            connected_targets.end(),
            [&](const Target &target)
            {
                bool currently_connected = std::find(
                                               current_connected_targets.begin(),
                                               current_connected_targets.end(),
                                               target.address) != current_connected_targets.end();
                if (!currently_connected)
                {
                    ESP_LOGW(TAG, "Target (0x%02X) not connected anymore, removing...", target.address);
                }
                return !currently_connected;
            }),
        connected_targets.end());

    // Remove targets with incompatible firmware version.
    connected_targets.erase(
        std::remove_if(
            connected_targets.begin(),
            connected_targets.end(),
            [&](Target &target)
            {
                uint8_t version = target.get_version();
                if (version != VERSION_CMD)
                {
                    ESP_LOGE(TAG, "Target at 0x%02X has unexpected version 0x%02X, expected 0x%02X", target.address, version, VERSION_CMD);
                    return true;
                }

                ESP_LOGI(TAG, "Target at 0x%02X is running expected version 0x%02X", target.address, version, VERSION_CMD);
                return false;
            }),
        connected_targets.end());
}

void Targets::set_threshold_all(uint8_t threshold)
{
    for (Target &target : connected_targets)
    {
        target.set_threshold(threshold);
    }
}

esp_err_t Targets::enable_target(int target_index)
{
    if (target_index < 0 || target_index >= connected_targets.size())
    {
        return ESP_ERR_INVALID_ARG;
    }
    return connected_targets[target_index].enable();
}

esp_err_t Targets::disable_target(int target_index)
{
    if (target_index < 0 || target_index >= connected_targets.size())
    {
        return ESP_ERR_INVALID_ARG;
    }
    return connected_targets[target_index].disable();
}

std::vector<int> Targets::check_enabled_hit()
{
    std::vector<int> hit_targets;
    for (int index = 0; index < connected_targets.size(); index++)
    {
        Target &target = connected_targets[index];
        if (target.enabled && target.get_hit())
        {
            hit_targets.push_back(index);
        }
    }
    return hit_targets;
}
