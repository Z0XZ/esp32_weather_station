# ESP32 Værstasjon med Vindmåling

Dette prosjektet er et ESP32-basert system for innsamling av lokale værdata. Systemet måler temperatur, luftfuktighet, trykk, lysstyrke, batterispenning og vindhastighet, og sender dataene til en backend via HTTP POST.

Prosjektet er utviklet med fokus på lavt strømforbruk og stabil drift over lang tid.

## Innhold

- [Funksjonalitet](#funksjonalitet)
- [Maskinvare](#maskinvare)
- [Oppsett og opplasting](#oppsett-og-opplasting)
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
   - `cppQueue` (installer Queue av SMFSW i ESP32)
4. Koble til ESP32
5. Velg riktig port og board (`ESP32 Dev Module`)
6. Kompiler og last opp programmet

---

## Backend-integrasjon

Dette prosjektet sender data til en Django REST API. Du finner backend-repoet her:

➡️ [lenke til backend-repo eller URL til API]

Eksempel på JSON som sendes:

```json
{
  "esp_id": "din-esp-id",
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
