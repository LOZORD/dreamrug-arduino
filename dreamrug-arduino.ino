#include <ArduinoJson.h>

const int BAUD = 9600;
const int SENSOR_PINS[] = {1,2,3};
const int NUM_SENSOR_PINS = sizeof(SENSOR_PINS) / sizeof(int);
const int LOOP_DELAY_MILLIS = 100;

void setup() {
  Serial.begin(BAUD);
}

typedef struct {
  int fromPin;
  int readingValue;
} PinReading;


void loop() {
 PinReading readings[NUM_SENSOR_PINS];
  for (int i = 0; i < NUM_SENSOR_PINS; i++) {
    int pin = SENSOR_PINS[i];
    int value = analogRead(pin);
    readings[i] = PinReading{
      .fromPin = pin,
      .readingValue = value,
    };
  }

  JsonDocument doc;
  doc["upt"] = millis();
  for (int i = 0; i < NUM_SENSOR_PINS; i++) {
    PinReading reading = readings[i];
    String label = String("pin_");
    label += String(reading.fromPin);
    doc[label] = reading.readingValue;
  }
  serializeJson(doc, Serial);
  Serial.println();
  delay(LOOP_DELAY_MILLIS);
}


