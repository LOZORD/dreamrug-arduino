#include <ArduinoJson.h>

const int NO_MULTIPLEXER = -1;

typedef struct {
  // If non-negative, the multiplexer pin to read from.
  int multiplexerPin;
  // The analog pin to read from.
  // If the multiplexer pin is present, this is the pin on the multiplexer to use.
  int analogPin;
  // The arbitrary ID assigned to this input for use by downstream programs.
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
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = 4,
  .id = 1004,
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = 5,
  .id = 1005
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = 6,
  .id = 1006,
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = 7,
  .id = 1007,
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = 0,
  .id = 1000,
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
    if (input.multiplexerPin >= 0) {
      // TODO: Handle reading from multiplexers.
      continue;
    }
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


