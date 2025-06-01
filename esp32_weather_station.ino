// === Biblioteker for sensorer, nettverk og køhåndtering ===
#include <Wire.h>                  // I2C-kommunikasjon
#include <WiFi.h>                  // WiFi-tilkobling
#include <HTTPClient.h>            // HTTP POST til backend
#include <Adafruit_Sensor.h>       // Adafruit baseklasse for sensorer
#include <Adafruit_BME280.h>       // Bibliotek for BME280-sensoren
#include <cppQueue.h>              // FIFO-kø til vindmålinger
#include "secrets.h"               // Inneholder blant annet WiFi og API-nøkler

// === Definisjoner av tilkoblinger og konstanter ===
#define BATTERY_ADC_PIN 35         // Analog inngang for batterispenning
#define BATTERY_GND_CTRL_PIN 32    // Kontrollerer "jord" til spenningsdeler
#define LIGHT_SENSOR_PIN 36        // Analog inngang for lysmåling
#define SDA_PIN 21                 // I2C-data
#define SCL_PIN 22                 // I2C-klokke
#define WIND_SENSOR_PIN 4          // Digital inngang for Hall-effektsensor

#define SEALEVELPRESSURE_HPA (1013.25)     // Referanse for høydeberegning
#define ROTOR_RADIUS 0.12                  // Radius på vindrotor i meter
#define CALIBRATION_FACTOR_WIND 2          // Kalibreringsfaktor for vindhastighet
#define MAX_LENGTH 20                      // Maks antall vindpulser i kø

// === Dvale-innstillinger ===
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 300          // 5 minutter i dvale mellom målinger

// === Sensorobjekter ===
Adafruit_BME280 bme;               // Temperatur, trykk og fuktighetssensor



void setup() {
  Serial.begin(9600);
  delay(1000);

  // === Initialiserer I2C og BME280 ===
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!bme.begin(0x76, &Wire)) {
    Serial.println("Fant ikke BME280!");
    sleepNow();
  }

  // === Initialiserer lysmåling ===
  pinMode(LIGHT_SENSOR_PIN, INPUT);

  // === Kobler til WiFi ===
  connectToWiFi();

  // === Måler sensordata ===
  float temp = bme.readTemperature();
  float humid = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;
  int light = analogRead(LIGHT_SENSOR_PIN);
  float battery_voltage = readBatteryVoltage();
  float windFreq = measureWind();
  float windSpeed = getWindSpeed(windFreq);

  // === Debug print til serieport ===
  Serial.println("Sender til backend:");
  Serial.printf("Temp: %.1f C\n", temp);
  Serial.printf("Fukt: %.1f %%\n", humid);
  Serial.printf("Trykk: %.1f hPa\n", pressure);
  Serial.printf("Lys: %d\n", light);
  Serial.printf("Batteri: %.2f V\n", battery_voltage);
  Serial.printf("Vindfrekvens: %.2f Hz\n", windFreq);
  Serial.printf("Vindhastighet: %.2f m/s\n", windSpeed);

  // === Send data til backend ===
  sendToBackend(temp, location, humid, pressure, light, battery_voltage, windFreq, windSpeed);

  // === Sett mikrokontroller i deep sleep ===
  sleepNow();
}

void loop() {
  // Ikke i bruk – programmet kjører kun én gang per oppvåkning, men funksjonen kreves av Arduino
}

// === FUNKSJON FOR Å MÅLE BATTERISPENNING ===
float readBatteryVoltage() {
  pinMode(BATTERY_GND_CTRL_PIN, OUTPUT);
  digitalWrite(BATTERY_GND_CTRL_PIN, LOW); // Slå på "jord" for spenningsdeler
  delay(100); // La spenningen stabilisere seg

  int rawADC = 0;
  for (int i = 0; i < 10; i++) {
    rawADC += analogRead(BATTERY_ADC_PIN);
    delay(10);
  }
  rawADC /= 10;
  digitalWrite(BATTERY_GND_CTRL_PIN, INPUT); // Koble fra for å spare strøm

  // Konverter til volt (faktor 2 pga. spenningsdeler, 1.134 er kalibreringsfaktor basert på målinger)
  float battery_voltage = (rawADC / 4095.0) * 3.3 * 2 * 1.134;
  return battery_voltage;
}

// === FUNKSJON FOR Å MÅLE VINDFREKVENS OVER 10 SEKUNDER ===
float measureWind() {
  const unsigned long duration_ms = 10000; // måletid: 10 sekunder
  unsigned long startTime = millis();

  int val = 0;
  int pulseCount = 0;

  cppQueue timeStamps(sizeof(int), MAX_LENGTH, FIFO, true); // Overwrite=true
  int lastWindVal = 1;
  timeStamps.flush(); // tøm tidligere målinger

  while (millis() - startTime < duration_ms) {
    val = digitalRead(WIND_SENSOR_PIN);
    if (val == 0 && lastWindVal == 1) {
      int timeStamp = millis();
      pulseCount++;

      if (!timeStamps.isFull()) {
        timeStamps.push(&timeStamp);
      } else {
        int oldest;
        timeStamps.pop(&oldest);
        timeStamps.push(&timeStamp);
      }
    }
    lastWindVal = val;
    delay(1);
  }

  if (timeStamps.nbRecs() < 2) {
    return 0.0; // ikke nok data til å beregne frekvens
  }

  int first, last;
  timeStamps.peekIdx(&first, 0);
  timeStamps.peekIdx(&last, timeStamps.nbRecs() - 1);

  int deltaTime = last - first;
  if (deltaTime <= 0) {
    return 0.0;
  }

  // Beregn frekvens (Hz) basert på antall pulser og tid
  float windFreq = (timeStamps.nbRecs() - 1) * 1000.0 / (deltaTime * 2);  // 2 magneter
  return windFreq;
}

// === FUNKSJON FOR Å BEREGNE VINDHASTIGHET I M/S ===
float getWindSpeed(float freqHz) {
  float circumference = 2 * PI * ROTOR_RADIUS;
  float tip_speed = freqHz * circumference;
  return CALIBRATION_FACTOR_WIND * tip_speed;
}

// === FUNKSJON FOR Å SENDE DATA TIL BACKEND ===
void sendToBackend(float temp, const char* location, float humid, float pressure, int light, float battery_voltage, float windFreq, float windSpeed) {
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-KEY", apiKey);

  String payload = "{";
  payload += "\"esp_id\": \"" + String(espId) + "\",";
  payload += "\"location\": \"" + String(location) + "\",";
  payload += "\"temperature\": " + String(temp, 1) + ",";
  payload += "\"humidity\": " + String(humid, 1) + ",";
  payload += "\"pressure\": " + String(pressure, 1) + ",";
  payload += "\"light\": " + String(light) + ",";
  payload += "\"battery_voltage\": " + String(battery_voltage, 2) + ",";
  payload += "\"wind_freq\": " + String(windFreq, 2) + ",";
  payload += "\"wind_speed\": " + String(windSpeed, 2);
  payload += "}";

  int httpResponseCode = http.POST(payload);

  Serial.print("HTTP response code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Respons:");
    Serial.println(response);
  }

  http.end();
}

// === FUNKSJON FOR Å KOBLE TIL WIFI ===
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Kobler til WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi-feil, går i dvale");
    sleepNow();
  }

  Serial.println("\nWiFi tilkoblet!");
}

// === FUNKSJON FOR Å STARTE DEEP SLEEP ===
void sleepNow() {
  Serial.println("Går i dvale...");
  Serial.flush();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}