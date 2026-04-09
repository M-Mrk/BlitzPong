#include "inputs.h"

constexpr int button_debounce_time_ms = 200;
constexpr int ec_transition_guard_time_ms = 2;

void IRAM_ATTR Inputs::handle_debounced_button(volatile bool &pressed_flag, volatile int64_t &last_interrupt_time, int debounce_time_ms)
{
    int64_t interrupt_time = esp_timer_get_time() / 1000;
    if (interrupt_time - last_interrupt_time > debounce_time_ms)
    {
        last_interrupt_time = interrupt_time;
        pressed_flag = true;
    }
}

void IRAM_ATTR Inputs::sw1_isr_handler(void *arg)
{
    auto *inputs = static_cast<Inputs *>(arg);
    handle_debounced_button(inputs->sw1_pressed_, inputs->sw1_last_interrupt_time_, button_debounce_time_ms);
}

void IRAM_ATTR Inputs::sw2_isr_handler(void *arg)
{
    auto *inputs = static_cast<Inputs *>(arg);
    handle_debounced_button(inputs->sw2_pressed_, inputs->sw2_last_interrupt_time_, button_debounce_time_ms);
}

void IRAM_ATTR Inputs::sw3_isr_handler(void *arg)
{
    auto *inputs = static_cast<Inputs *>(arg);
    handle_debounced_button(inputs->sw3_pressed_, inputs->sw3_last_interrupt_time_, button_debounce_time_ms);
}

void IRAM_ATTR Inputs::sw4_isr_handler(void *arg)
{
    auto *inputs = static_cast<Inputs *>(arg);
    handle_debounced_button(inputs->sw4_pressed_, inputs->sw4_last_interrupt_time_, button_debounce_time_ms);
}

void IRAM_ATTR Inputs::ec_cw_isr_handler(void *arg)
{
    auto *inputs = static_cast<Inputs *>(arg);
    handle_encoder_interrupt(inputs);
}

void IRAM_ATTR Inputs::ec_ccw_isr_handler(void *arg)
{
    auto *inputs = static_cast<Inputs *>(arg);
    handle_encoder_interrupt(inputs);
}

uint8_t IRAM_ATTR Inputs::read_encoder_state()
{
    int a = gpio_get_level(ENCODER_A_PIN);
    int b = gpio_get_level(ENCODER_B_PIN);
    return static_cast<uint8_t>((a << 1) | b);
}

int8_t IRAM_ATTR Inputs::decode_encoder_step(uint8_t previous_state, uint8_t current_state)
{
    uint8_t transition = static_cast<uint8_t>((previous_state << 2) | current_state);
    switch (transition)
    {
    case 0b0001:
    case 0b0111:
    case 0b1110:
    case 0b1000:
        return -1;
    case 0b0010:
    case 0b0100:
    case 0b1101:
    case 0b1011:
        return 1;
    default:
        return 0;
    }
}

void IRAM_ATTR Inputs::handle_encoder_interrupt(Inputs *inputs)
{
    int64_t interrupt_time = esp_timer_get_time() / 1000;
    if (interrupt_time - inputs->encoder_last_transition_time_ <= ec_transition_guard_time_ms)
    {
        return;
    }

    uint8_t previous_state = inputs->encoder_state_;
    uint8_t current_state = read_encoder_state();
    if (current_state == previous_state)
    {
        return;
    }

    int8_t step = decode_encoder_step(previous_state, current_state);
    inputs->encoder_state_ = current_state;
    if (step == 0)
    {
        return;
    }

    inputs->encoder_last_transition_time_ = interrupt_time;
    inputs->encoder_step_accumulator_ += step;

    if (inputs->encoder_step_accumulator_ >= 4)
    {
        inputs->encoder_step_accumulator_ = 0;
        inputs->cw_last_interrupt_time_ = interrupt_time;
        inputs->cw_pressed_ = true;
    }
    else if (inputs->encoder_step_accumulator_ <= -4)
    {
        inputs->encoder_step_accumulator_ = 0;
        inputs->ccw_last_interrupt_time_ = interrupt_time;
        inputs->ccw_pressed_ = true;
    }
}

void Inputs::clear_all()
{
    sw1_pressed_ = false;
    sw2_pressed_ = false;
    sw3_pressed_ = false;
    sw4_pressed_ = false;
    cw_pressed_ = false;
    ccw_pressed_ = false;

    sw1_callback_ = nullptr;
    sw2_callback_ = nullptr;
    sw3_callback_ = nullptr;
    sw4_callback_ = nullptr;
    ec_cw_callback_ = nullptr;
    ec_ccw_callback_ = nullptr;
}

void Inputs::input_task(void *pvParameters)
{
    auto *inputs = static_cast<Inputs *>(pvParameters);
    while (true)
    {
        if (inputs->sw1_pressed_)
        {
            inputs->sw1_pressed_ = false;
            if (inputs->sw1_callback_)
            {
                inputs->sw1_callback_();
            }
        }

        if (inputs->sw2_pressed_)
        {
            inputs->sw2_pressed_ = false;
            if (inputs->sw2_callback_)
            {
                inputs->sw2_callback_();
            }
        }

        if (inputs->sw3_pressed_)
        {
            inputs->sw3_pressed_ = false;
            if (inputs->sw3_callback_)
            {
                inputs->sw3_callback_();
            }
        }

        if (inputs->sw4_pressed_)
        {
            inputs->sw4_pressed_ = false;
            if (inputs->sw4_callback_)
            {
                inputs->sw4_callback_();
            }
        }

        if (inputs->cw_pressed_)
        {
            inputs->cw_pressed_ = false;
            if (inputs->ec_cw_callback_)
            {
                inputs->ec_cw_callback_();
            }
        }

        if (inputs->ccw_pressed_)
        {
            inputs->ccw_pressed_ = false;
            if (inputs->ec_ccw_callback_)
            {
                inputs->ec_ccw_callback_();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Inputs::init()
{
    gpio_config_t button_conf = {};
    button_conf.intr_type = GPIO_INTR_NEGEDGE;
    button_conf.mode = GPIO_MODE_INPUT;
    button_conf.pin_bit_mask = (1ULL << SW1_PIN) | (1ULL << SW2_PIN) | (1ULL << SW3_PIN) | (1ULL << SW4_PIN);
    button_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    button_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    gpio_config(&button_conf);

    gpio_config_t encoder_conf = {};
    encoder_conf.intr_type = GPIO_INTR_ANYEDGE;
    encoder_conf.mode = GPIO_MODE_INPUT;
    encoder_conf.pin_bit_mask = (1ULL << ENCODER_A_PIN) | (1ULL << ENCODER_B_PIN);
    encoder_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    encoder_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    gpio_config(&encoder_conf);

    encoder_step_accumulator_ = 0;
    encoder_last_transition_time_ = 0;
    encoder_state_ = read_encoder_state();

    gpio_install_isr_service(0);
    gpio_isr_handler_add(SW1_PIN, sw1_isr_handler, this);
    gpio_isr_handler_add(SW2_PIN, sw2_isr_handler, this);
    gpio_isr_handler_add(SW3_PIN, sw3_isr_handler, this);
    gpio_isr_handler_add(SW4_PIN, sw4_isr_handler, this);
    gpio_isr_handler_add(ENCODER_A_PIN, ec_cw_isr_handler, this);
    gpio_isr_handler_add(ENCODER_B_PIN, ec_ccw_isr_handler, this);

    xTaskCreate(input_task, "InputTask", 2048, this, 10, nullptr);
}