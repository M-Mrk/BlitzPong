#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

int spike = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(A0, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int current_read = analogRead(A0);
  if (current_read > spike) {
    spike = current_read;
  }

  static unsigned long lastReset = 0;
  static int twoSecSpike = 0;

  if (millis() - lastReset >= 2000) {
    twoSecSpike = 0;
    lastReset = millis();
  }
  if (current_read > twoSecSpike) {
    twoSecSpike = current_read;
  }

  static unsigned long last500msReset = 0;
  static int fiveHundredMsSpike = 0;

  if (millis() - last500msReset >= 500) {
    fiveHundredMsSpike = 0;
    last500msReset = millis();
  }
  if (current_read > fiveHundredMsSpike) {
    fiveHundredMsSpike = current_read;
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 100) {
    Serial.print("Total spike: ");
    Serial.print(spike);
    Serial.print(", 2s spike: ");
    Serial.print(twoSecSpike);
    Serial.print(", 500ms spike: ");
    Serial.println(fiveHundredMsSpike);
    lastPrint = millis();
  }
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}