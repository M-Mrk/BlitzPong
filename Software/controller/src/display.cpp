#include "display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_rom_sys.h"

static u8g2_t g_u8g2;
static i2c_cmd_handle_t g_u8x8_i2c_cmd = nullptr;
static uint8_t g_u8x8_dc = 0;

static uint8_t u8x8_esp32_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    (void)u8x8;
    (void)arg_ptr;

    switch (msg)
    {
    case U8X8_MSG_DELAY_MILLI:
        vTaskDelay(pdMS_TO_TICKS(arg_int));
        break;
    case U8X8_MSG_DELAY_NANO:
        if (arg_int > 0)
        {
            esp_rom_delay_us((arg_int + 999) / 1000);
        }
        break;
    case U8X8_MSG_DELAY_10MICRO:
        esp_rom_delay_us(10);
        break;
    case U8X8_MSG_DELAY_100NANO:
        break;
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
    case U8X8_MSG_GPIO_I2C_CLOCK:
    case U8X8_MSG_GPIO_I2C_DATA:
        break;
    default:
        return 0;
    }

    return 1;
}

static uint8_t u8x8_esp32_i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    (void)u8x8;

    switch (msg)
    {
    case U8X8_MSG_BYTE_INIT:
        return 1;

    case U8X8_MSG_BYTE_SET_DC:
        (void)arg_int;
        return 1;

    case U8X8_MSG_BYTE_START_TRANSFER:
        g_u8x8_i2c_cmd = i2c_cmd_link_create();
        if (g_u8x8_i2c_cmd == nullptr)
        {
            return 0;
        }

        i2c_master_start(g_u8x8_i2c_cmd);
        i2c_master_write_byte(g_u8x8_i2c_cmd, (SCREEN_ADDRESS << 1) | I2C_MASTER_WRITE, true);
        return 1;

    case U8X8_MSG_BYTE_SEND:
        if (g_u8x8_i2c_cmd == nullptr)
        {
            return 0;
        }
        i2c_master_write(g_u8x8_i2c_cmd, (uint8_t *)arg_ptr, arg_int, true);
        return 1;

    case U8X8_MSG_BYTE_END_TRANSFER:
        if (g_u8x8_i2c_cmd == nullptr)
        {
            return 0;
        }

        i2c_master_stop(g_u8x8_i2c_cmd);
        {
            esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_PORT, g_u8x8_i2c_cmd, pdMS_TO_TICKS(100));
            i2c_cmd_link_delete(g_u8x8_i2c_cmd);
            g_u8x8_i2c_cmd = nullptr;
            return err == ESP_OK ? 1 : 0;
        }

    default:
        return 0;
    }
}

esp_err_t display_init()
{
    u8g2_Setup_ssd1309_i2c_128x64_noname0_f(&g_u8g2, U8G2_R2, u8x8_esp32_i2c_byte_cb, u8x8_esp32_gpio_and_delay_cb);
    u8g2_SetI2CAddress(&g_u8g2, SCREEN_ADDRESS << 1);

    u8g2_InitDisplay(&g_u8g2);
    u8g2_SetPowerSave(&g_u8g2, 0);
    return ESP_OK;
}

u8g2_t *display_get_u8g2()
{
    return &g_u8g2;
}
