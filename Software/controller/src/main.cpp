#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <cstdio>

#include "commands.h"
#include "display.h"
#include "targets.h"
#include "inputs.h"

#include "screens/connection.h"
#include "screens/threshold.h"
#include "screens/game.h"

static const char *TAG = "CONTROLLER";
static uint8_t g_target_address = 0x04;

static Targets g_targets;
static Inputs g_inputs;

static void cycle_target()
{
    esp_err_t err = g_targets.enable_target(0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send CMD_SET_COLOR=1: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Sent CMD_SET_COLOR=1");
}

static void send_zero()
{
    esp_err_t err = g_targets.disable_target(0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send CMD_SET_COLOR=0: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Sent CMD_SET_COLOR=0");
}

static void update_targets()
{
    g_targets.update_targets();
    g_targets.set_threshold_all(80);
}

static void get_target_data()
{
    if (g_targets.connected_targets.empty())
    {
        ESP_LOGW(TAG, "No targets connected, cannot get data");
        return;
    }

    Target target_0 = g_targets.connected_targets[0];
    uint8_t vibration = target_0.get_vibration();
    ESP_LOGI(TAG, "Target 0 vibration: %d", vibration);
}

static void encoder_cw()
{
    ESP_LOGI(TAG, "Encoder CW");
}

static void encoder_ccw()
{
    ESP_LOGI(TAG, "Encoder CCW");
}

void setup()
{
    gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);

    esp_err_t err = g_targets.init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Targets init failed: %s", esp_err_to_name(err));
        return;
    }
    g_targets.update_targets();

    err = display_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Display init failed: %s", esp_err_to_name(err));
    }

    g_inputs.set_sw1_callback(cycle_target);
    g_inputs.set_sw2_callback(send_zero);
    g_inputs.set_sw3_callback(update_targets);
    g_inputs.set_sw4_callback(get_target_data);
    g_inputs.set_ec_cw_callback(encoder_cw);
    g_inputs.set_ec_ccw_callback(encoder_ccw);
    g_inputs.init();
}

void loop()
{
    ConnectionScreen::show(g_targets, g_inputs, display_get_u8g2());
    
    threshold:
    if (!ThresholdScreen::show(g_targets, g_inputs, display_get_u8g2()))
    {
        ESP_LOGI(TAG, "User chose to return to connection screen");
        return;
    }

    if (!GameScreen::show(g_targets, g_inputs, display_get_u8g2()))
    {
        ESP_LOGI(TAG, "User chose to return to threshold screen");
        goto threshold;
    }
    
    vTaskDelay(pdMS_TO_TICKS(5000));
}

extern "C" void app_main()
{
    setup();
    while (1)
    {
        loop();
    }
}