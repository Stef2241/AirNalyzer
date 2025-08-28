#include <Wire.h>
#include <SPI.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_BME680.h>
#include "BluetoothSerial.h"
#include <ArduinoJson.h>

#define VCC       5.0       // alimentare MQ
#define RL        10000.0   // 10k rezistență de sarcină

// pini MQ
#define MQ3_PIN      19
#define MQ136_PIN    15
#define MQ135_PIN    14
#define MQ137_PIN    26
#define MQ138_PIN    33
#define MQ7_PIN      35

// MH-Z19B UART
#define RX_PIN       16
#define TX_PIN       17

Adafruit_SGP30 sgp;
Adafruit_BME680 bme;
HardwareSerial co2Serial(2);
BluetoothSerial SerialBT;

// --- Parametrii MQ (A, B din datasheet, Ro calibrat în aer curat)
struct MQParams {
  float A;
  float B;
  float Ro;
};

MQParams mq3   = { 0.3934, -1.504, 10000 };  // alcool / acetonă
MQParams mq136 = { 101.9,  -2.074, 608407 };  // H2S
MQParams mq135 = { 110.47, -2.862, 75513 };  // CO2 / NH3
MQParams mq137 = { 38.0,   -3.0,   40504 };  // NH3
MQParams mq138 = { 20.0,   -2.5,   62636 };  // VOC
MQParams mq7   = { 99.042, -1.518, 80480.5 };  // CO

// --- funcții MQ ---
float readADCavg(int pin, int n=10) {
  long sum = 0;
  for (int i=0; i<n; i++) {
    sum += analogRead(pin);
    delay(5);
  }
  return (float)sum / n;
}

float adcToRs(float adc) {
  if (adc <= 0) return 1e9;
  float vout = adc * (VCC / 4095.0);
  return (VCC * RL / vout) - RL;
}

float mqGetPPM(int pin, MQParams params) {
  float adc = readADCavg(pin);
  float Rs = adcToRs(adc);
  float ratio = Rs / params.Ro;
  return params.A * pow(ratio, params.B);
}

// --- MH-Z19B ---
byte readCmd[9] = {0xFF,0x01,0x86,0,0,0,0,0,0x79};
const int MAX_RETRIES = 3;

byte calcChecksum(byte *buf) {
  int sum = 0;
  for (int i=1; i<=7; i++) sum += buf[i];
  return (0xFF - (sum & 0xFF) + 1) & 0xFF;
}

int readCO2Once(int timeoutMs=200) {
  for(int attempt=0; attempt<MAX_RETRIES; attempt++) {
    while (co2Serial.available()) co2Serial.read();
    co2Serial.write(readCmd, 9);
    unsigned long start = millis();
    while (millis() - start < (unsigned long)timeoutMs) {
      if (co2Serial.available() >= 9) {
        byte resp[9];
        co2Serial.readBytes(resp, 9);
        if (resp[0] == 0xFF && resp[1] == 0x86 && resp[8] == calcChecksum(resp)) {
          int ppm = resp[2]*256 + resp[3];
          if (ppm>=0 && ppm<=10000) return ppm;
        }
        break;
      }
    }
    delay(100);
  }
  return -1;
}

// --- setup ---
void setup() {
  delay(2000);
  Serial.begin(115200);
  SerialBT.begin("ESP32_Airnalyzer");
  Serial.println("\n=== Initializare dispozitiv ===");

  // I2C
  Wire.begin(21, 22);

  // SGP30
  Serial.println("Init SGP30...");
  if (!sgp.begin()) {
    Serial.println("✖ Eroare init SGP30");
    while (1) { delay(10); }
  }
  sgp.IAQinit();
  Serial.println("✔ SGP30 OK");

  // BME680
  Serial.println("Init BME680...");
  bool bme_ok = false;
  const uint8_t addr_try[] = {0x76, 0x77};
  for (uint8_t i=0; i < sizeof(addr_try)/sizeof(addr_try[0]); i++) {
    if (bme.begin(addr_try[i])) {
      bme_ok = true;
      break;
    }
  }
  if (!bme_ok) {
    Serial.println("✖ Eroare init BME680");
    while (1) { delay(10); }
  }
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);
  Serial.println("✔ BME680 OK");

  // MH-Z19B
  co2Serial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  Serial.println("Așteptare stabilizare MH-Z19B...");
  delay(5000);
}

// --- loop ---
void loop() {
  // MQ
  float acetone = mqGetPPM(MQ3_PIN, mq3);
  float h2s     = mqGetPPM(MQ136_PIN, mq136);
  float mq135v  = mqGetPPM(MQ135_PIN, mq135);
  float ammonia = mqGetPPM(MQ137_PIN, mq137);
  float voc     = mqGetPPM(MQ138_PIN, mq138);
  float co      = mqGetPPM(MQ7_PIN, mq7);

  // BME680
  float temperature = NAN;
  float humidity = NAN;
  if(bme.performReading()) {
    temperature = bme.temperature;
    humidity = bme.humidity;
  }

  // MH-Z19B
  float co2 = readCO2Once();
  if(co2 < 0) co2 = NAN;

  // SGP30
  if(sgp.IAQmeasure()) {
    voc = (voc + (float)sgp.TVOC) / 2.0;
  }

  // JSON
  StaticJsonDocument<512> doc;
  doc["Temperature"] = isnan(temperature) ? nullptr : temperature;
  doc["Humidity"]    = isnan(humidity) ? nullptr : humidity;
  doc["Acetone"]     = isnan(acetone) ? nullptr : acetone;
  doc["Ammonia"]     = isnan(ammonia) ? nullptr : ammonia;
  doc["CO"]          = isnan(co) ? nullptr : co;
  doc["CO2"]         = isnan(co2) ? nullptr : co2;
  doc["H2S"]         = isnan(h2s) ? nullptr : h2s;
  doc["MQ135_CO2"]   = isnan(mq135v) ? nullptr : mq135v;
  doc["MQ138_VOC"]   = isnan(voc) ? nullptr : voc;

  char buffer[512];
  serializeJson(doc, buffer);

  if (SerialBT.hasClient()) {
    SerialBT.println(buffer);
  }
  Serial.println(buffer);

  delay(8000);
}
