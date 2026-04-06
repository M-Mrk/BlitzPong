#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "commands.h"

#define target_address 0x04
#define I2C_MASTER_PORT I2C_NUM_0
#define I2C_MASTER_SDA_PIN GPIO_NUM_6
#define I2C_MASTER_SCL_PIN GPIO_NUM_7
#define I2C_MASTER_FREQ_HZ 100000

#define SCREEN_ADDRESS 0x3C
#define GPIO_ALT_CYCLE GPIO_NUM_0
#define GPIO_SEND_ZERO GPIO_NUM_1

static const char *TAG = "CONTROLLER";
static uint8_t g_target_address = target_address;
static uint8_t g_found_addresses[128];
static size_t g_found_count = 0;
static volatile bool g_cycle_requested = false;
static volatile bool g_send_zero_requested = false;
static uint8_t g_next_cycle_color = 1;
static int g_prev_gpio0_level = 1;
static int g_prev_gpio1_level = 1;

static esp_err_t init_i2c_master()
{
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 22;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = 23;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    esp_err_t err = i2c_param_config(I2C_MASTER_PORT, &conf);
    if (err != ESP_OK)
    {
        return err;
    }

    return i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);
}

static esp_err_t probe_address(uint8_t address)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    return err;
}

static void scan_i2c_devices()
{
    g_found_count = 0;
    ESP_LOGI(TAG, "Scanning I2C bus for devices...");

    for (int addr = 1; addr < 127; ++addr)
    {
        if (addr == SCREEN_ADDRESS) {
            // continue;
        }
        esp_err_t err = probe_address(addr);
        if (err == ESP_OK)
        {
            g_found_addresses[g_found_count++] = addr;
            ESP_LOGI(TAG, "Found I2C device at 0x%02X", addr);
        }
    }

    if (g_found_count == 0)
    {
        ESP_LOGW(TAG, "No I2C devices found on the bus");
    }
    else
    {
        ESP_LOGI(TAG, "Scan complete: %d device(s) found", g_found_count);
    }
}

void blink()
{
    gpio_set_level(GPIO_NUM_15, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(GPIO_NUM_15, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(GPIO_NUM_15, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(GPIO_NUM_15, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(GPIO_NUM_15, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
}

static esp_err_t send_set_color(uint8_t color)
{
    blink();
    uint8_t payload[2] = {CMD_SET_COLOR, color};
    return i2c_master_write_to_device(
        I2C_MASTER_PORT,
        g_target_address,
        payload,
        sizeof(payload),
        pdMS_TO_TICKS(CMD_PROCESSING_TIME_MS));
}

static void IRAM_ATTR gpio0_isr_handler(void *arg)
{
    (void)arg;
    g_cycle_requested = true;
}

static void IRAM_ATTR gpio1_isr_handler(void *arg)
{
    (void)arg;
    g_send_zero_requested = true;
}

static esp_err_t init_button_interrupts()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_ALT_CYCLE) | (1ULL << GPIO_SEND_ZERO);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK)
    {
        return err;
    }

    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        return err;
    }

    err = gpio_isr_handler_add(GPIO_ALT_CYCLE, gpio0_isr_handler, nullptr);
    if (err != ESP_OK)
    {
        return err;
    }

    return gpio_isr_handler_add(GPIO_SEND_ZERO, gpio1_isr_handler, nullptr);
}


void setup()
{
    gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);

    esp_err_t err = init_i2c_master();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(err));
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(500));
    scan_i2c_devices();

    err = init_button_interrupts();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "GPIO interrupt init failed: %s", esp_err_to_name(err));
        return;
    }
}

void loop()
{
    if (g_send_zero_requested)
    {
        g_send_zero_requested = false;
        g_cycle_requested = false;

        esp_err_t err = send_set_color(0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send CMD_SET_COLOR=0: %s", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "Sent CMD_SET_COLOR=0");
        }
    }

    if (g_cycle_requested)
    {
        g_cycle_requested = false;
        esp_err_t err = send_set_color(1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send CMD_SET_COLOR=%u: %s", g_next_cycle_color, esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI(TAG, "Sent CMD_SET_COLOR=%u", g_next_cycle_color);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(200));
}

extern "C" void app_main()
{
    setup();
    while (1)
    {
        loop();
    }
}