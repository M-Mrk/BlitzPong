#include "screens/game.h"
#include "esp_random.h"
#include <cstdio>

static const char *TAG = "SCREEN/GAME";

namespace GameScreen
{
    struct TaskParams
    {
        Targets *targets;
        u8g2_t *u8g2;
    };

    static volatile bool running = true;
    static volatile bool game_running = false;
    static volatile bool continue_after_screen = true;
    static volatile int score = 0;
    static volatile bool timer_enabled = true;
    static volatile int remaining_seconds = 120;
    static volatile int configured_duration_seconds = 120;
    static volatile int runtime_duration_offset_seconds = 0;
    static volatile int pending_duration_step_seconds = 0;
    static volatile bool reset_requested = false;
    static volatile bool stop_requested = false;
    static volatile int active_target_index = -1;

    static constexpr int DURATION_STEP_SECONDS = 30;
    static constexpr int MIN_GAME_DURATION_SECONDS = 30;
    static constexpr int MAX_GAME_DURATION_SECONDS = 1800;

    static int clamp_duration(int value)
    {
        if (value < MIN_GAME_DURATION_SECONDS)
        {
            return MIN_GAME_DURATION_SECONDS;
        }
        if (value > MAX_GAME_DURATION_SECONDS)
        {
            return MAX_GAME_DURATION_SECONDS;
        }
        return value;
    }

    static void disable_active_target(Targets &targets)
    {
        int target_index = active_target_index;
        if (target_index < 0)
        {
            return;
        }

        esp_err_t err = targets.disable_target(target_index);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to disable target %d: %s", target_index, esp_err_to_name(err));
        }
        active_target_index = -1;
    }

    static void enable_next_random_target(Targets &targets)
    {
        int target_count = static_cast<int>(targets.connected_targets.size());
        if (target_count <= 0)
        {
            active_target_index = -1;
            return;
        }

        int random_index = static_cast<int>(esp_random() % static_cast<uint32_t>(target_count));
        esp_err_t err = targets.enable_target(random_index);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to enable target %d: %s", random_index, esp_err_to_name(err));
            active_target_index = -1;
            return;
        }

        active_target_index = random_index;
    }

    static void draw(Targets &targets, u8g2_t *u8g2)
    {
        (void)targets;
        const char *game_state_text = game_running ? "Stop" : "Start";
        std::string score_text = std::to_string(score);
        int seconds = remaining_seconds;
        if (seconds < 0)
        {
            seconds = 0;
        }
        char timer_text[6];
        std::snprintf(timer_text, sizeof(timer_text), "%d:%02d", seconds / 60, seconds % 60);
        u8g2_SetFont(u8g2, u8g2_font_4x6_tr);

        // Buttons
        u8g2_DrawStr(u8g2, 2, BUTTON_Y - 6, "Disable");     // SW4
        u8g2_DrawStr(u8g2, 2, BUTTON_Y, "Timer");           // "
        u8g2_DrawStr(u8g2, 50, BUTTON_Y, "Back");           // SW1
        u8g2_DrawStr(u8g2, 80, BUTTON_Y, "Reset");          // SW2
        u8g2_DrawStr(u8g2, 109, BUTTON_Y, game_state_text); // SW3

        // Timer
        u8g2_SetFont(u8g2, u8g2_font_t0_14b_tr);
        u8g2_DrawStr(u8g2, 2, 15, "Time");
        u8g2_SetFont(u8g2, u8g2_font_profont12_tr);
        u8g2_DrawStr(u8g2, 5, 34, timer_text);
        if (!game_running)
        {
            u8g2_DrawLine(u8g2, 4, 40, 28, 40);
        }
        if (!timer_enabled)
        {
            u8g2_DrawLine(u8g2, 0, 42, 31, 17);
            u8g2_DrawLine(u8g2, 0, 41, 31, 16);
            u8g2_DrawLine(u8g2, 0, 43, 31, 18);
        }

        u8g2_DrawLine(u8g2, 38, 5, 38, 51); // Seperator

        // Score
        u8g2_SetFont(u8g2, u8g2_font_t0_14b_tr);
        u8g2_DrawStr(u8g2, 50, 15, "Targets hit");
        int score_start_x = 78;
        int score_x = score_start_x - (score_text.length() - 1) * 5;
        u8g2_DrawStr(u8g2, score_x, 39, score_text.c_str());

        u8g2_SendBuffer(u8g2);
    }

    static void screen_task(void *pvParameters)
    {
        auto *params = static_cast<TaskParams *>(pvParameters);
        Targets &targets = *params->targets;
        u8g2_t *u8g2 = params->u8g2;
        delete params;

        TickType_t game_start_tick = 0;
        bool game_timer_started = false;

        u8g2_ClearBuffer(u8g2);
        u8g2_ClearDisplay(u8g2);

        while (running)
        {
            if (reset_requested)
            {
                disable_active_target(targets);
                score = 0;
                runtime_duration_offset_seconds = 0;
                pending_duration_step_seconds = 0;
                remaining_seconds = configured_duration_seconds;
                game_running = false;
                game_timer_started = false;
                stop_requested = false;
                reset_requested = false;
            }

            int duration_step = pending_duration_step_seconds;
            if (duration_step != 0)
            {
                pending_duration_step_seconds = 0;

                if (game_running)
                {
                    int effective_duration = configured_duration_seconds + runtime_duration_offset_seconds + duration_step;
                    effective_duration = clamp_duration(effective_duration);
                    runtime_duration_offset_seconds = effective_duration - configured_duration_seconds;
                }
                else
                {
                    configured_duration_seconds = clamp_duration(configured_duration_seconds + duration_step);
                    remaining_seconds = configured_duration_seconds;
                }
            }

            if (stop_requested)
            {
                disable_active_target(targets);
                stop_requested = false;
            }

            if (game_running)
            {
                if (!game_timer_started)
                {
                    game_start_tick = xTaskGetTickCount();
                    game_timer_started = true;
                    runtime_duration_offset_seconds = 0;
                }

                TickType_t now = xTaskGetTickCount();
                TickType_t elapsed_ticks = now - game_start_tick;
                uint32_t elapsed_seconds = static_cast<uint32_t>(elapsed_ticks / configTICK_RATE_HZ);
                int effective_duration_seconds = clamp_duration(configured_duration_seconds + runtime_duration_offset_seconds);

                if (!timer_enabled)
                {
                    remaining_seconds = effective_duration_seconds;
                }
                else if (elapsed_seconds >= static_cast<uint32_t>(effective_duration_seconds))
                {
                    remaining_seconds = 0;
                }
                else
                {
                    remaining_seconds = static_cast<int>(effective_duration_seconds - elapsed_seconds);
                }

                if (timer_enabled && (now - game_start_tick) >= pdMS_TO_TICKS(effective_duration_seconds * 1000))
                {
                    game_running = false;
                    disable_active_target(targets);
                }
                else
                {
                    if (active_target_index < 0 || active_target_index >= static_cast<int>(targets.connected_targets.size()))
                    {
                        enable_next_random_target(targets);
                    }
                    else
                    {
                        std::vector<int> hit_targets = targets.check_enabled_hit();
                        for (int hit_target : hit_targets)
                        {
                            if (hit_target == active_target_index)
                            {
                                score += 50;
                                disable_active_target(targets);
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                game_timer_started = false;
                runtime_duration_offset_seconds = 0;
                remaining_seconds = configured_duration_seconds;
            }

            u8g2_ClearBuffer(u8g2);
            draw(targets, u8g2);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        disable_active_target(targets);
        vTaskDelete(nullptr);
    }

    bool show(Targets &targets, Inputs &inputs, u8g2_t *u8g2)
    {
        ESP_LOGI(TAG, "Showing Game Screen");
        running = true;
        game_running = false;
        continue_after_screen = true;
        score = 0;
        timer_enabled = true;
        configured_duration_seconds = 120;
        runtime_duration_offset_seconds = 0;
        pending_duration_step_seconds = 0;
        remaining_seconds = configured_duration_seconds;
        reset_requested = false;
        stop_requested = false;
        active_target_index = -1;
        u8g2_ClearBuffer(u8g2);
        u8g2_ClearDisplay(u8g2);

        auto *params = new TaskParams{&targets, u8g2};
        BaseType_t result = xTaskCreate(screen_task, "GameScreen", 4096, params, 10, nullptr);
        if (result != pdPASS)
        {
            delete params;
            ESP_LOGE(TAG, "Failed to create game screen task");
            inputs.clear_all();
            return false;
        }

        inputs.set_sw1_callback([]()
                                {
            game_running = false;
            stop_requested = true;
            running = false;
            continue_after_screen = false; });

        inputs.set_sw2_callback([]()
                                {
            score = 0;
            reset_requested = true; });

        inputs.set_sw3_callback([]()
                                {
            game_running = !game_running;
            if (!game_running)
            {
                stop_requested = true;
            } });

        inputs.set_sw4_callback([]()
                                { timer_enabled = !timer_enabled; });

        inputs.set_ec_cw_callback([]()
                                  { pending_duration_step_seconds += DURATION_STEP_SECONDS; });

        inputs.set_ec_ccw_callback([]()
                                   { pending_duration_step_seconds -= DURATION_STEP_SECONDS; });

        while (running)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
        inputs.clear_all();
        ESP_LOGI(TAG, "Exiting Game Screen");
        return continue_after_screen;
    }
}