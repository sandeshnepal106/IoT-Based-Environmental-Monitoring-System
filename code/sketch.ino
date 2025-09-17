#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"

// ----- DHT22 CONFIG -----
#define DHTPIN 4        // DHT22 data pin → GPIO4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ----- Sensor Pins -----
const int LDR_PIN = 34;   // LDR AO → GPIO34
const int MQ_PIN  = 35;   // Potentiometer wiper → GPIO35

// ----- WiFi + ThingSpeak -----
const char* ssid = "Wokwi-GUEST";
const char* password = "";
String apiKey = "DOVDZPTBBR0G7NNU";
const char* server = "http://api.thingspeak.com/update";

// ----- Upload Interval -----
unsigned long lastTime = 0;
unsigned long timerDelay = 15000; // ThingSpeak requires ≥15s between updates

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // ----- Read Sensors -----
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int ldrRaw = analogRead(LDR_PIN);
    int mqRaw = analogRead(MQ_PIN);

    if (isnan(temp) || isnan(hum)) {
      Serial.println("Failed to read from DHT22!");
      return;
    }

    // Scale LDR & MQ to human-readable values
    float lightLevel = map(ldrRaw, 0, 4095, 0, 100);   // 0-100 %
    float airQuality = map(mqRaw, 0, 4095, 0, 500);   // 0-500 scale

    // ----- Print Locally -----
    Serial.printf("Temp: %.2f °C, Hum: %.2f %%, Air: %.2f, Light: %.2f %%\n",
                  temp, hum, airQuality, lightLevel);

    // ----- Send to ThingSpeak -----
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = server;
      url += "?api_key=" + apiKey;
      url += "&field1=" + String(temp);
      url += "&field2=" + String(hum);
      url += "&field3=" + String(airQuality);
      url += "&field4=" + String(lightLevel);

      http.begin(url);
      int httpCode = http.GET();
      if (httpCode > 0) {
        Serial.printf("ThingSpeak response: %d\n", httpCode);
      } else {
        Serial.printf("Error sending data: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }

    lastTime = millis();
  }
}
