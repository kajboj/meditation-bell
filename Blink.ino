#include <TM1637Display.h>

static const int BELL_PIN   = 2;

void setup() {
  pinMode(BELL_PIN, OUTPUT);
  digitalWrite(BELL_PIN, LOW);
}

void loop() {
  digitalWrite(BELL_PIN, LOW);
  delay(1000);
  digitalWrite(BELL_PIN, HIGH);
  delay(100);
}
