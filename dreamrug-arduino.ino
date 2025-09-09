#define BAUD 115200
#define LOOP_DELAY_MILLIS 100
#define READING_MAX 4095
#define WIFI_POLL_MILLIS 500
#define WIFI_PORT 80
#define NUM_WIFI_CLIENTS 4

// To have clients connect, I recommend using the command:
// $ stdbuf -oL nc <ARDUINO_IP> <WIFI_PORT>
// That way, netcat won't do "efficient buffering", which will stagger the output
// and make it less interactive.
// By using stdbuf, the output will be line-buffered instead, which is in-line
// (pun intended) with the lines of JSON that we're sending.

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

// The global array of WiFi clients, allowing us to broadcast to multiple clients.
// Clients are added to the array wherever a spot is available.
WiFiClient clients[NUM_WIFI_CLIENTS];

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

  int added = scanForNewClients();
  if (added > 0) {
    Serial.print("# Successfully added ");
    Serial.print(added);
    Serial.println(" clients.");
  } else if (added == 0) {
    // No new clients added. Don't log spam.
  } else {
    Serial.print("# Failed to add ");
    Serial.print(-added);
    Serial.println(" clients. Client list likely full.");
  }

  sendToClients(doc);

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

int scanForNewClients() {
  int added = 0;
  int rejected = 0;
  WiFiClient newClient;
  // It is a little dubious to assume that multiple clients will connect within
  // the same main Arduino polling loop, but this feels like the right way to
  // do it for now.
  while (newClient = server.available()) {
    Serial.print("# Attempting to add new client: ");
    Serial.print(newClient.remoteIP());
    Serial.println();

    bool addedClient = false;

    for (int i = 0; i < NUM_WIFI_CLIENTS; i++) {
      if (!clients[i].connected()) {
        // Found an empty slot. Assign, increment added and move on.
        clients[i] = newClient;
        added++;
        addedClient = true;
        Serial.println("# Client added.");
        break;
      }
    }

    if (!addedClient) {
      // If we finished the loop and never added the client, the list is full.
      // Reject the client and count it.
      Serial.println("# Client list full. Rejecting client.");
      newClient.stop();
      rejected++;
    }
  }

  // Prioritize reporting failures. If we rejected anyone, return the negative count.
  if (rejected > 0) {
    return -rejected;
  }
  
  // Otherwise, just return the number of clients we successfully added.
  return added; 
}

void sendToClients(const JsonDocument& doc) {
  for (int i = 0; i < NUM_WIFI_CLIENTS; i++) {
    WiFiClient& client = clients[i];
    if (client.connected()) {
      serializeJson(doc, client);
      client.println();
    }
  }
}