#include <Arduino.h>
#include <Wire.h>
#include "../shared/commands.h"

#define SLAVE_ADDRESS 0x04
byte lastCMD = 0x00;
uint8_t brightness = 25;
uint8_t hit = 1;
uint8_t debug = 2;
uint8_t threshold = 0;

void requestEvent()
{
  switch (lastCMD)
  {
  case CMD_GET_DEBUG:
    Wire.write(debug);
    break;

  case CMD_GET_BRIGHTNESS:
    Wire.write(brightness);
    break;

  case CMD_GET_HIT:
    Wire.write(hit);
    break;
  
  case CMD_GET_THRESHOLD:
    Wire.write(threshold);
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
    debug = 3;
    lastCMD = Wire.read();
    byte data = Wire.read();
    switch (lastCMD)
    {
    case CMD_SET_BRIGHTNESS:
      brightness = (uint8_t)data;
      break;

    case CMD_SET_THRESHOLD:
      threshold = (uint8_t)data;
      break;

    default:
      break;
    }
  }
  else
  {
    lastCMD = Wire.read();
  }
}

void setup()
{
  Wire.begin(SLAVE_ADDRESS);

  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);
}

void loop()
{
  delay(50);
}