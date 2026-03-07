#include <Arduino.h>
#include <Wire.h>
#include "../shared/commands.h" // Include shared commands

constexpr int MAX_DEVICES = 5;

int getAddresses(int *targets)
{
  int numDevice = 0;
  for (int address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);
    int error = Wire.endTransmission();
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      Serial.println(address, HEX);
      targets[numDevice] = address;
      numDevice += 1;
      if (numDevice >= MAX_DEVICES)
      {
        break;
      }
    }
  }
  return numDevice;
}

void testI2C(int address)
{
  Serial.println("Testing I2C communication with device at address: " + String(address));
  Serial.println("Sending CMD_SET_BRIGHTNESS");
  Wire.beginTransmission(address);
  Wire.write(CMD_SET_BRIGHTNESS);
  Wire.write(128);
  Wire.endTransmission();
  delay(CMD_PROCESSING_TIME_MS);

  Serial.println("Sending CMD_GET_BRIGHTNESS");
  Wire.beginTransmission(address);
  Wire.write(CMD_GET_BRIGHTNESS);
  Wire.endTransmission();

  Wire.requestFrom(address, 1); // Request the actual data
  if (Wire.available() > 0)
  {
    uint8_t response = Wire.read(); // Read the actual data as uint8_t
    Serial.println("\tReceived from Slave: " + String(response));
  }
  else
  {
    Serial.println("Error: Insufficient data received for CMD_GET_BRIGHTNESS");
  }

  Serial.println("Sending CMD_GET_HIT");
  Wire.beginTransmission(address);
  Wire.write(CMD_GET_HIT);
  Wire.endTransmission();
  delay(CMD_PROCESSING_TIME_MS); // Short delay to ensure the slave has time to process the command
  Wire.requestFrom(address, 1);  // Request the actual data
  if (Wire.available() > 0)
  {
    uint8_t response = Wire.read(); // Read the actual data
    Serial.println("\tReceived from Slave: " + String(response));
  }

  Serial.println("Sending CMD_GET_DEBUG");
  Wire.beginTransmission(address);
  Wire.write(CMD_GET_DEBUG);
  Wire.endTransmission();
  delay(CMD_PROCESSING_TIME_MS); // Short delay to ensure the slave has time to process the command
  Wire.requestFrom(address, 1);  // Request the actual data
  if (Wire.available() > 0)
  {
    uint8_t response = Wire.read(); // Read the actual data
    Serial.println("\tReceived from Slave: " + String(response));
  }
}

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  int targets[MAX_DEVICES]; // Array to hold device addresses
  int numDevices = getAddresses(targets);
  for (int i = 0; i < numDevices; i++)
  {
    testI2C(targets[i]);
  }
}

void loop()
{
  delay(500);
}