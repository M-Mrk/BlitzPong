#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <cstdio>

#include "commands.h"
#include "display.h"
#include "targets.h"
#include "inputs.h"

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

static void render_testing_text()
{
    u8g2_t *u8g2 = display_get_u8g2();
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(u8g2, 18, 36, "Testing");
    u8g2_SendBuffer(u8g2);
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

void draw(void *pvParameters)
{
    (void)pvParameters;

    static int frame = 0;

    while (true)
    {
        frame++;
        u8g2_t *u8g2 = display_get_u8g2();
        u8g2_ClearBuffer(u8g2);
        u8g2_ClearDisplay(u8g2);
        u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
        u8g2_DrawStr(u8g2, 18, 36, "Hello World!");
        u8g2_DrawStr(u8g2, 18, 50, "Frame: ");

        char frame_text[16];
        snprintf(frame_text, sizeof(frame_text), "%d", frame);
        u8g2_DrawStr(u8g2, 60, 50, frame_text);

        u8g2_SendBuffer(u8g2);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
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
    else
    {
        render_testing_text();
        ESP_LOGI(TAG, "Display text rendered: Testing");
    }

    g_inputs.set_sw1_callback(cycle_target);
    g_inputs.set_sw2_callback(send_zero);
    g_inputs.set_sw3_callback(update_targets);
    g_inputs.set_sw4_callback(get_target_data);
    g_inputs.set_ec_cw_callback(encoder_cw);
    g_inputs.set_ec_ccw_callback(encoder_ccw);
    g_inputs.init();

    xTaskCreatePinnedToCore(
        draw,
        "DrawTask",
        4096,
        nullptr,
        1,
        nullptr,
        tskNO_AFFINITY);
}

void loop()
{
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