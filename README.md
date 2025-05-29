# ESP32 Værstasjon med Vindmåling

Dette prosjektet er et ESP32-basert system for innsamling av lokale værdata. Systemet måler temperatur, luftfuktighet, trykk, lysstyrke, batterispenning og vindhastighet, og sender dataene til en backend via HTTP POST.

Prosjektet er utviklet med fokus på lavt strømforbruk og stabil drift over lang tid.

## Innhold

- [Funksjonalitet](#funksjonalitet)
- [Maskinvare](#maskinvare)
- [Oppsett og opplasting](#oppsett-og-opplasting)
- [Konfigurasjon med secrets.h](#konfigurasjon-med-secretsh)
- [Backend-integrasjon](#backend-integrasjon)
- [Programstruktur og dokumentasjon](#programstruktur-og-dokumentasjon)
- [Forslag til utvidelser](#forslag-til-utvidelser)
- [Lisens](#lisens)

---

## Funksjonalitet

- Leser sensordata fra:
  - BME280 (temp, fukt, trykk)
  - Lyssensor (analog)
  - Spenningsdeler for batteri
  - Hall-effektsensor for vind
- Sender data som JSON til REST API
- Går i deep sleep i 5 minutter mellom hver måling
- Lavt strømforbruk, egnet for batteridrift

---

## Maskinvare

| Komponent           | ESP32-pin  | Beskrivelse                                 |
|---------------------|------------|---------------------------------------------|
| BME280 (I2C)        | 21 (SDA)   | I2C datalinje                               |
|                     | 22 (SCL)   | I2C klokkelinje                             |
| Lyssensor           | 36 (ADC)   | Analog inngang                              |
| Spenningsdeler      | 35 (ADC)   | Analog inngang for batterispenning          |
| GND-kontroll pin    | 32         | Slår på jordforbindelse til spenningsdeler  |
| Vindsensor (Hall)   | 4          | Digital inngang fra vindsensor              |

---

## Oppsett og opplasting

1. Installer Arduino IDE
2. Legg til ESP32-støtte via Board Manager:
   - URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Installer bibliotekene:
   - `Adafruit BME280`
   - `Adafruit Unified Sensor`
   - `WiFi.h` (standard i ESP32)
   - `HTTPClient.h` (standard i ESP32)
   - `cppQueue` (installer manuelt fra https://github.com/SMFSW/cppQueue)
4. Koble til ESP32
5. Velg riktig port og board (`ESP32 Dev Module`)
6. Kompiler og last opp programmet

---

## Konfigurasjon med secrets.h

Sensitive data som WiFi-passord og API-nøkler holdes utenfor kildekoden ved å bruke en egen headerfil kalt `secrets.h`.

Denne fila er **ikke lastet opp til GitHub**, men det følger med en mal (`secrets_example.h`) som du kan bruke.

### 1. Kopier fila

Lag en kopi av `secrets_example.h` og gi den navnet `secrets.h`:

```sh
cp secrets_example.h secrets.h
```

### 2. Fyll inn dine verdier

Åpne `secrets.h` og fyll inn dine egne innstillinger:

```cpp
#pragma once

// WiFi
const char* ssid = "MittNettverk";
const char* password = "MittPassord";

// Backend
const char* serverUrl = "https://din-backend-url.no/api/sensor-data/";
const char* espId = "esp32-a1b2c3";
const char* apiKey = "hemmelig-api-nokkel";
const char* location = "Sandnes";
```

### 3. Ikke last opp secrets.h

`secrets.h` er lagt til i `.gitignore` og vil aldri lastes opp til GitHub. På denne måten kan prosjektet deles trygt uten at sensitiv informasjon eksponeres.

---

## Backend-integrasjon

Dette prosjektet sender data til en Django REST API. Du finner backend-repoet her:

➡️ [lenke til backend-repo eller URL til API]

Eksempel på JSON som sendes:

```json
{
  "esp_id": "unik-valgt-esp-id",
  "location": "Sandnes",
  "temperature": 21.5,
  "humidity": 42.3,
  "pressure": 1008.6,
  "light": 289,
  "battery_voltage": 3.74,
  "wind_freq": 2.54,
  "wind_speed": 1.97
}
```

---

## Programstruktur og dokumentasjon

### Konstanter og pin-definisjoner

Øverst i filen finner du `#define`-konstanter for pinvalg, kalibrering og tidsintervaller.

### setup()

Kjører én gang ved oppstart:
- Starter sensorer og WiFi
- Leser alle verdier
- Sender data til backend
- Går i dvale i 5 minutter

### loop()

Tom funksjon – brukes ikke fordi programmet går i deep sleep.

### Funksjoner

- `readBatteryVoltage()` – Leser batterispenning via ADC
- `measureWind()` – Leser vindpuls og beregner frekvens
- `getWindSpeed()` – Konverterer frekvens til m/s
- `sendToBackend()` – Sender alle data via HTTP POST
- `sleepNow()` – Setter ESP32 i dvale

### Forkortelser og begreper

| Begrep               | Forklaring                                                                 |
|----------------------|----------------------------------------------------------------------------|
| ADC                  | Analog-til-digital konverter – leser analoge spenninger                    |
| I2C                  | Kommunikasjonsprotokoll mellom mikrokontroller og sensorer                |
| BME280               | Sensor for temperatur, fuktighet og lufttrykk                              |
| GND                  | Jordforbindelse i kretsen                                                  |
| VCC                  | Spenningstilførsel til sensorer                                            |
| HTTP POST            | Internettprotokoll for å sende data til en webserver                      |
| Hz (Hertz)           | Enhet for frekvens – antall svingninger/pulser per sekund                  |
| JSON                 | Tekstformat for strukturert datautveksling                                 |
| Deep Sleep           | Strømsparende dvalemodus for ESP32                                         |
| cppQueue             | C++-bibliotek som brukes for å holde orden på måletidspunktene             |

---

## Forslag til utvidelser

- Lagring til SD-kort ved manglende WiFi
- MQTT-støtte for sanntidsdata
- Flere sensorer (regn, UV, osv.)
- Kalibreringsmodus for vindmåling
- Varsling ved lav batterispenning

---

## Lisens

Dette prosjektet kan brukes fritt i private og utdanningsprosjekter. For kommersiell bruk, ta kontakt.
