#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "pins.h"

class Inputs
{
public:
    using Callback = void (*)();

    Inputs() = default;

    void init();
    void clear_all();

    void set_sw1_callback(Callback callback) { sw1_callback_ = callback; }
    void set_sw2_callback(Callback callback) { sw2_callback_ = callback; }
    void set_sw3_callback(Callback callback) { sw3_callback_ = callback; }
    void set_sw4_callback(Callback callback) { sw4_callback_ = callback; }
    void set_ec_cw_callback(Callback callback) { ec_cw_callback_ = callback; }
    void set_ec_ccw_callback(Callback callback) { ec_ccw_callback_ = callback; }

    Callback get_sw1_callback() const { return sw1_callback_; }
    Callback get_sw2_callback() const { return sw2_callback_; }
    Callback get_sw3_callback() const { return sw3_callback_; }
    Callback get_sw4_callback() const { return sw4_callback_; }
    Callback get_ec_cw_callback() const { return ec_cw_callback_; }
    Callback get_ec_ccw_callback() const { return ec_ccw_callback_; }

private:
    Callback sw1_callback_ = nullptr;
    Callback sw2_callback_ = nullptr;
    Callback sw3_callback_ = nullptr;
    Callback sw4_callback_ = nullptr;
    Callback ec_cw_callback_ = nullptr;
    Callback ec_ccw_callback_ = nullptr;

    volatile bool sw1_pressed_ = false;
    volatile bool sw2_pressed_ = false;
    volatile bool sw3_pressed_ = false;
    volatile bool sw4_pressed_ = false;
    volatile bool cw_pressed_ = false;
    volatile bool ccw_pressed_ = false;

    volatile int64_t sw1_last_interrupt_time_ = 0;
    volatile int64_t sw2_last_interrupt_time_ = 0;
    volatile int64_t sw3_last_interrupt_time_ = 0;
    volatile int64_t sw4_last_interrupt_time_ = 0;
    volatile int64_t cw_last_interrupt_time_ = 0;
    volatile int64_t ccw_last_interrupt_time_ = 0;
    volatile int64_t encoder_last_transition_time_ = 0;
    volatile int8_t encoder_step_accumulator_ = 0;
    volatile uint8_t encoder_state_ = 0;

    static void IRAM_ATTR sw1_isr_handler(void *arg);
    static void IRAM_ATTR sw2_isr_handler(void *arg);
    static void IRAM_ATTR sw3_isr_handler(void *arg);
    static void IRAM_ATTR sw4_isr_handler(void *arg);
    static void IRAM_ATTR handle_debounced_button(volatile bool &pressed_flag, volatile int64_t &last_interrupt_time, int debounce_time_ms);
    static void IRAM_ATTR ec_cw_isr_handler(void *arg);
    static void IRAM_ATTR ec_ccw_isr_handler(void *arg);
    static void IRAM_ATTR handle_encoder_interrupt(Inputs *inputs);
    static uint8_t IRAM_ATTR read_encoder_state();
    static int8_t IRAM_ATTR decode_encoder_step(uint8_t previous_state, uint8_t current_state);
    static void input_task(void *pvParameters);
};
