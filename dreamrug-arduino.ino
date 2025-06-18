#include <ArduinoJson.h>

const int NO_MULTIPLEXER = -1;

typedef struct {
  int multiplexerPin;
  int analogPin;
  int id;
} SensorInput;

const int BAUD = 9600;
const SensorInput SENSOR_INPUTS[] = {{
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = 1,
  .id = 1001,
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = 2,
  .id = 1002,
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = 3,
  .id = 1003,
}};
const int NUM_SENSOR_INPUTS = sizeof(SENSOR_INPUTS) / sizeof(SensorInput);
const int LOOP_DELAY_MILLIS = 100;

void setup() {
  Serial.begin(BAUD);
}

typedef struct {
  SensorInput fromInput;
  int readingValue;
} InputReading;


void loop() {
  InputReading readings[NUM_SENSOR_INPUTS];
  for (int i = 0; i < NUM_SENSOR_INPUTS; i++) {
    SensorInput input = SENSOR_INPUTS[i];
    // TODO: Handle reading from multiplexers.
    int value = analogRead(input.analogPin);
    readings[i] = InputReading{
      .fromInput = input,
      .readingValue = value,
    };
  }

  JsonDocument doc;
  doc["upt"] = millis();
  for (int i = 0; i < NUM_SENSOR_INPUTS; i++) {
    InputReading reading = readings[i];
    doc["sensorData"][i]["name"] = String("input_") + String(reading.fromInput.id);
    doc["sensorData"][i]["value"] = reading.readingValue;
  }
  serializeJson(doc, Serial);
  Serial.println();
  delay(LOOP_DELAY_MILLIS);
}


