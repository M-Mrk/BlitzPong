#include "target.h"
#include <driver/i2c.h>
static const char *TAG = "TARGETS";

Target::Target(uint8_t addr)
{
    address = addr;
}

bool Target::get_hit()
{
    uint8_t raw_value = send_get_cmd(CMD_GET_HIT);
    bool hit = raw_value != 0;
    if (hit && auto_disable)
    {
        enabled = false; // Hit automatically disabled target, as auto_disable was on
    }
    return hit;
}

uint8_t Target::get_version()
{
    return send_get_cmd(CMD_GET_VERSION);
    // return VERSION_CMD;
}

uint8_t Target::get_threshold()
{
    return send_get_cmd(CMD_GET_THRESHOLD);
}

esp_err_t Target::set_threshold(uint8_t threshold)
{
    return send_set_cmd(CMD_SET_THRESHOLD, threshold);
}

uint8_t Target::get_vibration()
{
    return send_get_cmd(CMD_GET_VIBRATION);
}

esp_err_t Target::set_auto_disable(bool auto_disable)
{
    this->auto_disable = auto_disable;
    return send_set_cmd(CMD_SET_AUTO_TURN_OFF, auto_disable ? 1 : 0);
}

esp_err_t Target::enable()
{
    get_hit(); // Clear any existing hit state before enabling
    enabled = true;
    return send_set_cmd(CMD_SET_COLOR, 1);
}

esp_err_t Target::disable()
{
    enabled = false;
    return send_set_cmd(CMD_SET_COLOR, 0);
}

esp_err_t Target::send_set_cmd(uint8_t cmd, uint8_t value)
{
    uint8_t payload[2] = {cmd, value};

    for (int attempt = 0; attempt < 2; attempt++)
    {
        esp_err_t err = i2c_master_write_to_device(
            I2C_MASTER_PORT,
            address,
            payload,
            sizeof(payload),
            pdMS_TO_TICKS(CMD_PROCESSING_TIME_MS));
        
        if (err != ESP_ERR_TIMEOUT || attempt == 1)
        {
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send set command 0x%02X with value 0x%02X to target at 0x%02X: %s", cmd, value, address, esp_err_to_name(err));
            }
            return err;
        }
    }
    return ESP_FAIL; // Should not reach here
}

uint8_t Target::send_get_cmd(uint8_t cmd)
{
    uint8_t value = 0;

    for (int attempt = 0; attempt < 2; attempt++)
    {
        esp_err_t err = i2c_master_write_to_device(
            I2C_MASTER_PORT,
            address,
            &cmd,
            1,
            pdMS_TO_TICKS(CMD_PROCESSING_TIME_MS));
        if (err != ESP_OK)
        {
            if (err != ESP_ERR_TIMEOUT || attempt == 1)
            {
                ESP_LOGE(TAG, "Failed to send get command 0x%02X to target at 0x%02X: %s", cmd, address, esp_err_to_name(err));
                return 0;
            }
            continue; // Retry on timeout
        }

        // Give target time to process command and prepare response
        vTaskDelay(pdMS_TO_TICKS(1));

        err = i2c_master_read_from_device(
            I2C_MASTER_PORT,
            address,
            &value,
            1,
            pdMS_TO_TICKS(CMD_PROCESSING_TIME_MS));
        if (err != ESP_OK)
        {
            if (err != ESP_ERR_TIMEOUT || attempt == 1)
            {
                ESP_LOGE(TAG, "Failed to read response for command 0x%02X from target at 0x%02X: %s", cmd, address, esp_err_to_name(err));
                return 0;
            }
            continue; // Retry on timeout
        }

        return value; // Success
    }
    return 0; // Should not reach here
}