#include "screens/threshold.h"

static const char *TAG = "SCREEN/THRESH";

namespace ThresholdScreen
{
    struct TaskParams
    {
        Targets *targets;
        u8g2_t *u8g2;
    };

    constexpr int threshold_encoder_change = 5;
    static volatile bool running = true;
    static volatile bool continue_after_screen = true;
    static volatile uint8_t current_threshold = 125;

    static void draw(Targets &targets, u8g2_t *u8g2)
    {
        u8g2_SetFont(u8g2, u8g2_font_4x6_tr);

        int start_x = 1;
        int current_x = start_x;
        for (size_t i = 0; i < targets.connected_targets.size(); ++i)
        {
            u8g2_SetDrawColor(u8g2, 0);
            u8g2_DrawBox(u8g2, current_x, 20, 16, 8);
            u8g2_DrawBox(u8g2, 45, 39, 35, 7);
            u8g2_SetDrawColor(u8g2, 1);

            uint8_t target_vibration = targets.connected_targets[i].get_vibration();
            u8g2_DrawBox(u8g2, current_x, 4, 16, 16);
            u8g2_DrawStr(u8g2, current_x + 2, 26, std::to_string(target_vibration).c_str());
            if (target_vibration > current_threshold)
            {
                u8g2_DrawLine(u8g2, current_x + 1, 27, current_x + 13, 27);
            }
            current_x += 20;
        }

        // Buttons
        u8g2_DrawStr(u8g2, 2, BUTTON_Y, "Adjust");  // Encoder
        u8g2_DrawStr(u8g2, 50, BUTTON_Y, "Back"); // SW1
        u8g2_DrawStr(u8g2, 110, BUTTON_Y, "Next");  // SW3

        // Threshold
        u8g2_DrawStr(u8g2, 45, 38, "Threshold");
        u8g2_DrawStr(u8g2, 56, 45, std::to_string(current_threshold).c_str());

        u8g2_SendBuffer(u8g2);
    }

    static void screen_task(void *pvParameters)
    {
        auto *params = static_cast<TaskParams *>(pvParameters);
        Targets &targets = *params->targets;
        u8g2_t *u8g2 = params->u8g2;
        delete params;

        u8g2_ClearBuffer(u8g2);
        u8g2_ClearDisplay(u8g2);

        while (running)
        {
            draw(targets, u8g2);
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        vTaskDelete(nullptr);
    }

    bool show(Targets &targets, Inputs &inputs, u8g2_t *u8g2)
    {
        ESP_LOGI(TAG, "Showing Threshold Screen");
        running = true;
        current_threshold = 125;
        continue_after_screen = true;

        auto *params = new TaskParams{&targets, u8g2};
        BaseType_t result = xTaskCreate(screen_task, "ThresholdScreen", 4096, params, 10, nullptr);
        if (result != pdPASS)
        {
            delete params;
            ESP_LOGE(TAG, "Failed to create threshold screen task");
            inputs.clear_all();
            return false;
        }

        inputs.set_ec_cw_callback([]() {
            current_threshold = (current_threshold + threshold_encoder_change) % 256;
        });
        inputs.set_ec_ccw_callback([]() {
            current_threshold = (current_threshold - threshold_encoder_change) % 256;
        });
        inputs.set_sw1_callback([]() {
            continue_after_screen = false;
            running = false;
        });
        inputs.set_sw3_callback([]() {
            running = false;
        });

        while (running)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        vTaskDelay(pdMS_TO_TICKS(10));

        inputs.clear_all();
        targets.set_threshold_all(current_threshold);
        ESP_LOGI(TAG, "Exiting Threshold Screen");
        return continue_after_screen;
    }
}