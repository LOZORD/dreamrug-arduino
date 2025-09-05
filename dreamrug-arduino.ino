#define BAUD 115200
#define LOOP_DELAY_MILLIS 100
#define READING_MAX 4095
#define WIFI_POLL_MILLIS 500
#define WIFI_PORT 80
#include <ArduinoJson.h>
#include <WiFi.h>

// This `secret.h` file includes `#defines` for secret values.
// Namely the following values:
// - WIFI_SSID
// - WIFI_PASS
#include "secret.h"

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

const SensorInput SENSOR_INPUTS[] = {{
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = A0,
  .id = 1000,
},
#ifndef SOLO_PIN
{
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = A1,
  .id = 1001,
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = A2,
  .id = 1002,
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = A3,
  .id = 1003,
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = A4,
  .id = 1004,
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = A5,
  .id = 1005
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = GPIO_NUM_5,
  .id = 1006,
}, {
  .multiplexerPin = NO_MULTIPLEXER,
  .analogPin = GPIO_NUM_6,
  .id = 1007,
}
#endif
};
const int NUM_SENSOR_INPUTS = sizeof(SENSOR_INPUTS) / sizeof(SensorInput);

// Our server instance.
// IDEA: instead of having clients poll this server, maybe we should push updates to them instead?
WiFiServer server(WIFI_PORT);
IPAddress localIP;

void setup() {
  Serial.begin(BAUD);
  while (!Serial) {
    // Wait for it to become ready.
  }

  configureWifi();
  startServer();
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
    // TODO: consider mapping the reading's value to a [0, 1024) range (or similar).
    // This is useful since we have the #def'd constant now.
    // It might be worth it to do _nothing_ since using [0, 4095) gives higher fidelity.
    int value = analogRead(input.analogPin);
    readings[i] = InputReading{
      .fromInput = input,
      .readingValue = value,
    };
  }

  JsonDocument doc;
  doc["upt"] = millis();
  doc["lip"] = localIP;
  for (int i = 0; i < NUM_SENSOR_INPUTS; i++) {
    InputReading reading = readings[i];
    doc["sensorData"][i]["name"] = String("input_") + String(reading.fromInput.id);
    doc["sensorData"][i]["value"] = reading.readingValue;
  }
  serializeJson(doc, Serial);
  // This println seems to be necessary even with the flush call.
  // Otherwise, `cat /dev/...` seems to hang.
  // The downstream handler should just skip any blank lines.
  Serial.println();
  Serial.flush();

  WiFiClient client;
  while(client = server.available()) {
    Serial.print("# Got client: ");
    Serial.print(client.remoteIP());
    Serial.println();
    // TODO: update this to send the JSON payload.
    client.println("Howdy there friend!\n\n");
    client.stop();
  }

  delay(LOOP_DELAY_MILLIS);
}

// Sets up this board on a WiFi router.
// WiFi code takes inspiration from https://learn.adafruit.com/adafruit-esp32-feather-v2/wifi-test.
void configureWifi() {
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("# No WiFi capability present on the board.");
    while (true) { 
      // Loop forever :(
    }
  }

  Serial.print("# Attempting to connect to: ");
  Serial.print(WIFI_SSID);
  Serial.println();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("# ");
  while(WiFi.status() != WL_CONNECTED) {
    delay(WIFI_POLL_MILLIS);
    Serial.print('.');
  }
  Serial.println();
  
  Serial.println("# Successfully connected to WiFi.");
 
  Serial.print("# Connected to SSID: ");
  Serial.print(WiFi.SSID());
  Serial.println();

  Serial.print("# Local IP Address: ");
  localIP = WiFi.localIP();
  Serial.print(localIP);
  Serial.println();

  Serial.print("# Signal strength (RSSI) in dBm: ");
  Serial.print(WiFi.RSSI());
  Serial.println();
}

// Starts up the server to allow clients to connect.
void startServer() {
  Serial.println("# Starting up server");
  server.begin();
  Serial.println("# Server started, waiting for clients");
}