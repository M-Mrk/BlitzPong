#include "screens/connection.h"

static const char *TAG = "SCREEN/CONN";

namespace ConnectionScreen
{
    void rescan(Targets &targets);

    struct TaskParams
    {
        Targets *targets;
        Inputs *inputs;
        u8g2_t *u8g2;
    };

    static bool running = false;
    static bool stop_requested = false;
    static bool first_draw = true;
    static int last_number_targets = 0;
    static std::string screen_message;
    static std::string last_drawn_message;
    static TickType_t screen_message_set_at = 0;
    static Targets *active_targets = nullptr;
    static Inputs *active_inputs = nullptr;
    static Inputs::Callback previous_sw2_callback = nullptr;
    static Inputs::Callback previous_sw3_callback = nullptr;

    static void set_screen_message(const std::string &message)
    {
        screen_message = message;
        screen_message_set_at = xTaskGetTickCount();
    }

    static void draw(Targets &targets, u8g2_t *u8g2)
    {
        (void)targets;
        u8g2_SetFont(u8g2, u8g2_font_4x6_tr);

        // Buttons
        if (first_draw)
        {
            u8g2_DrawStr(u8g2, 80, BUTTON_Y, "Rescan");
            u8g2_DrawStr(u8g2, 113, BUTTON_Y, "Next");
        }

        // Target
        int start_x = 1;
        int current_x = start_x;
        size_t max_visible_targets = 5;
        size_t visible_count = targets.connected_targets.size();
        if (visible_count > max_visible_targets)
        {
            visible_count = max_visible_targets;
        }

        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawBox(u8g2, 0, 0, 127, 35);
        u8g2_SetDrawColor(u8g2, 1);

        for (size_t i = 0; i < visible_count; ++i)
        {
            Target &target = targets.connected_targets[i];
            u8g2_DrawBox(u8g2, current_x, 5, 16, 16);
            char addr_str[5];
            snprintf(addr_str, sizeof(addr_str), "0x%02X", target.address);
            u8g2_DrawStr(u8g2, current_x, 29, addr_str);
            current_x += 20;
        }

        if (targets.connected_targets.size() > max_visible_targets)
        {
            u8g2_DrawStr(u8g2, 95, 31, "and more");
        }

        // Message Box
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawBox(u8g2, 1, 51, 53, 11);
        u8g2_SetDrawColor(u8g2, 1);
        if (!screen_message.empty())
        {
            u8g2_DrawFrame(u8g2, 0, 50, 55, 13);
            u8g2_DrawStr(u8g2, 2, 59, screen_message.c_str());
        }

        u8g2_SendBuffer(u8g2);
        first_draw = false;
    }

    static void screen_task(void *pvParameters)
    {
        auto *params = static_cast<TaskParams *>(pvParameters);
        Targets &targets = *params->targets;
        u8g2_t *u8g2 = params->u8g2;

        u8g2_ClearBuffer(u8g2);
        u8g2_ClearDisplay(u8g2);
        first_draw = true;
        last_number_targets = 0;
        last_drawn_message.clear();
        screen_message_set_at = 0;

        while (!stop_requested)
        {
            if (!screen_message.empty() &&
                (xTaskGetTickCount() - screen_message_set_at) >= pdMS_TO_TICKS(2000))
            {
                screen_message.clear();
            }

            // Check if target count changed to trigger redraw
            int current_count = targets.connected_targets.size();
            if (current_count != last_number_targets || first_draw || screen_message != last_drawn_message)
            {
                last_number_targets = current_count;
                last_drawn_message = screen_message;
                draw(targets, u8g2);
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }

        delete params;
        running = false;
        vTaskDelete(nullptr);
    }

    void rescan(Targets &targets)
    {
        set_screen_message("Rescanning...");
        targets.update_targets();
        if (targets.connected_targets.empty())
        {
            ESP_LOGI(TAG, "No targets found during rescan");
            set_screen_message("None found...");
        }
        else if (targets.connected_targets.size() == last_number_targets)
        {
            set_screen_message("No change...");
        }
        else
        {
            set_screen_message("Updated...");
        }
    }

    static void sw2_rescan_callback()
    {
        if (active_targets != nullptr)
        {
            rescan(*active_targets);
        }
    }

    static void sw3_next_callback()
    {
        stop_requested = true;
    }

    void show(Targets &targets, Inputs &inputs, u8g2_t *u8g2)
    {
        ESP_LOGI(TAG, "Showing Connection Screen");
        if (running || active_inputs != nullptr)
        {
            ESP_LOGW(TAG, "Connection screen is already running");
            return;
        }

        running = true;
        stop_requested = false;
        first_draw = true;
        last_number_targets = 0;
        screen_message.clear();
        last_drawn_message.clear();
        screen_message_set_at = 0;
        active_targets = &targets;
        active_inputs = &inputs;
        previous_sw2_callback = inputs.get_sw2_callback();
        previous_sw3_callback = inputs.get_sw3_callback();

        auto *params = new TaskParams{&targets, &inputs, u8g2};
        BaseType_t result = xTaskCreate(screen_task, "ConnectionScreen", 4096, params, 10, nullptr);
        if (result != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to start ConnectionScreen task");
            delete params;
            running = false;
            stop_requested = false;
            active_targets = nullptr;
            active_inputs = nullptr;
            previous_sw2_callback = nullptr;
            previous_sw3_callback = nullptr;
            return;
        }

        inputs.set_sw2_callback(sw2_rescan_callback);
        inputs.set_sw3_callback(sw3_next_callback);

        while (running)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        if (active_inputs != nullptr)
        {
            active_inputs->set_sw2_callback(previous_sw2_callback);
            active_inputs->set_sw3_callback(previous_sw3_callback);
        }

        active_targets = nullptr;
        active_inputs = nullptr;
        previous_sw2_callback = nullptr;
        previous_sw3_callback = nullptr;
        stop_requested = false;
        inputs.clear_all();
        ESP_LOGI(TAG, "Exiting Connection Screen");
    }
} // namespace ConnectionScreen