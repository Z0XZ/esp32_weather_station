#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <cppQueue.h>
#include "secrets.h"

#define BATTERY_ADC_PIN 35
#define BATTERY_GND_CTRL_PIN 32
#define SEALEVELPRESSURE_HPA (1013.25)
#define LIGHT_SENSOR_PIN 36
#define SDA_PIN 21
#define SCL_PIN 22
#define WIND_SENSOR_PIN 4
#define ROTOR_RADIUS 0.1  // meter – juster etter din rotordiameter
#define CALIBRATION_FACTOR_WIND 1.3  // juster basert på kalibrering

// Dvale
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 300  // Hvert 5. minutt

// Sensorer
Adafruit_BME280 bme;

// cppQueue for vindmåling
#define MAX_LENGTH 20
cppQueue timeStamps(sizeof(int), MAX_LENGTH, FIFO, true); // Overwrite=true
int lastWindVal = 1;
float windFreq = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);

  // I2C + BME280
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!bme.begin(0x76, &Wire)) {
    Serial.println("Fant ikke BME280!");
    while (1);
  }

  // Lys
  pinMode(LIGHT_SENSOR_PIN, INPUT);

  // WiFi
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

  // Mål sensordata
  float temp = bme.readTemperature();
  float humid = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;
  int light = analogRead(LIGHT_SENSOR_PIN);
  float battery_voltage = readBatteryVoltage();
  float windFreq = measureWind();
  float windSpeed = getWindSpeed(windFreq);

  Serial.println("Sender til backend:");
  Serial.printf("Temp: %.1f C\n", temp);
  Serial.printf("Fukt: %.1f %%\n", humid);
  Serial.printf("Trykk: %.1f hPa\n", pressure);
  Serial.printf("Lys: %d\n", light);
  Serial.printf("Batteri: %.2f V\n", battery_voltage);
  Serial.printf("Vindfrekvens: %.2f Hz\n", windFreq);
  Serial.printf("Vindhastighet: %.2f m/s\n", windSpeed);

  // Send til backend
  sendToBackend(temp, location, humid, pressure, light, battery_voltage, windFreq, windSpeed);

  // Deep sleep
  sleepNow();
}

void loop() {} // Ikke i bruk – kreves av Arduino

// Batterimåling
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

  float battery_voltage = (rawADC / 4095.0) * 3.3 * 2 * 1.134; // ADC → Volt (x2 pga. spenningsdeler) (1,134 er en korreksjonsfaktor lagt inn basert på faktiske målinger.)
  return battery_voltage;
}

float measureWind() {
  int val = 0;
  int samples = 100;  // ~100 ms
  for (int i = 0; i < samples; i++) {
    val = digitalRead(WIND_SENSOR_PIN);
    if (val == 0 && lastWindVal == 1) {
      int timeStamp = millis();
      if (!timeStamps.isFull()) {
        timeStamps.push(&timeStamp);
      } else {
        int oldest;
        timeStamps.pop(&oldest);
        timeStamps.push(&timeStamp);

        int first, last;
        timeStamps.peekIdx(&first, 0);
        timeStamps.peekIdx(&last, MAX_LENGTH - 1);
        int deltaTime = last - first;
        if (deltaTime > 0) {
          windFreq = (MAX_LENGTH - 1) * 1000.0 / (deltaTime * 2);  // 2 magneter
        }
      }
    }
    lastWindVal = val;
    delay(1);  // gir ca. 100 ms totalt
  }
  return windFreq;
}

float getWindSpeed(float freqHz) {
  float circumference = 2 * PI * ROTOR_RADIUS;
  float tip_speed = freqHz * circumference;
  return CALIBRATION_FACTOR_WIND * tip_speed;
}


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

void sleepNow() {
  Serial.println("Går i dvale...");
  Serial.flush();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}