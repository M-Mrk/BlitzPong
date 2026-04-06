#include <Arduino.h>
#include <Wire.h>
#define FASTLED_INTERNAL
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
volatile unsigned long timer_millis = 0;

#include "../shared/commands.h"

#define SLAVE_ADDRESS 0x04
#define NUM_LEDS 4
constexpr uint8_t FAKE_NUM_LEDS = (NUM_LEDS*2)+1; // Leave space so we can change an immaginary LED to trick FastLED into updating even though no real change was made
#define LED_PIN PIN_PA1
#define SENSOR_PIN PIN_PA5

template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
class SK6812B_Controller : public ClocklessController<
                               DATA_PIN,
                               NS(300), // T1: T0H, initial high pulse
                               NS(900), // T2: transition (T1H − T0H)
                               NS(300), // T3: T1L, final low
                               RGB_ORDER,
                               0, false, 200>
{
};

CRGB leds[FAKE_NUM_LEDS];

byte g_lastCMD = 0x00;
uint8_t g_debug = 0;
bool g_auto_turn_off = 0;
uint8_t g_color = 0;
uint8_t g_brightness = 255;

uint8_t g_hit = 1;
uint8_t g_threshold = 255;

void requestEvent()
{
  switch (g_lastCMD)
  {
  case CMD_GET_DEBUG:
    Wire.write(g_debug);
    break;

  case CMD_GET_COLOR:
    Wire.write(g_color);
    break;

  case CMD_GET_HIT:
    Wire.write(g_hit);
    g_hit = 0; // Hit has been read, now reset to unhit state
    break;

  case CMD_GET_THRESHOLD:
    Wire.write(g_threshold);
    break;

  case CMD_GET_VIBRATION:
    uint8_t vibration = analogRead(SENSOR_PIN);
    Wire.write(vibration);
    break;

  default:
    Wire.write(0);
    break;
  }
}

void receiveEvent(int numBytes)
{
  if (numBytes > 1)
  {
    g_lastCMD = Wire.read();
    byte data = Wire.read();
    switch (g_lastCMD)
    {
    case CMD_SET_COLOR:
      g_color = (uint8_t)data;
      break;

    case CMD_SET_THRESHOLD:
      g_threshold = (uint8_t)data;
      break;

    case CMD_SET_DEBUG:
      g_debug = (uint8_t)data;
      break;
    
    case CMD_SET_AUTO_TURN_OFF:
      uint8_t received_value = (uint8_t)data;
      if (received_value > 1) {
        received_value = 1;
      }
      if (received_value == 1){
        g_auto_turn_off = true;
      } else {
        g_auto_turn_off = false;
      }
      break;

    default:
      break;
    }
  }
  else
  {
    g_lastCMD = Wire.read();
  }
}

void setup()
{
  Wire.begin(SLAVE_ADDRESS);

  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  pinMode(SENSOR_PIN, INPUT);

  FastLED.addLeds<SK6812B_Controller, LED_PIN, RGB>(leds, FAKE_NUM_LEDS);
  FastLED.setDither(false);
  FastLED.setMaxRefreshRate(400);
  fill_solid(leds, FAKE_NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void set_color(uint8_t color, uint8_t brightness)
{
  const CRGB cycleColors[] = {CRGB::Red, CRGB::Green1, CRGB::Blue};
  static uint8_t g_current_color_index = 0;
  static long last_run = 0;
  if (millis() - last_run < 500 && color != 0)
  {
    return;
  }
  last_run = millis();

  CRGB c;
  switch (color)
  {
  case 0:
    c = CRGB::Black;
    break;

  default:
    c = cycleColors[g_current_color_index];
    g_current_color_index = (g_current_color_index + 1) % 3;
    break;
  }
  fill_solid(leds, (FAKE_NUM_LEDS), c);
  leds[0] = cycleColors[g_current_color_index];
  FastLED.setBrightness(brightness);
  FastLED.show();
}

void check_hit()
{
  uint8_t sensor_value = analogRead(SENSOR_PIN);
  if (sensor_value > g_threshold)
  {
    g_hit = 1;
    if (g_auto_turn_off) {
      set_color(0, 0); // Turn off LEDs immediately when hit is detected
    }
  }
}

void loop()
{
  set_color(g_color, g_brightness);
  check_hit();
}