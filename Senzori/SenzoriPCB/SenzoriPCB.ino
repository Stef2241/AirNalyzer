#include <Wire.h>
#include <SPI.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_BME680.h>
#include "BluetoothSerial.h"
#include <ArduinoJson.h>

#define MQ3_PIN      19
#define MQ136_PIN    15
#define RX_PIN       16  // RX ESP32 <- TX MH-Z19B
#define TX_PIN       17  // TX ESP32 -> RX MH-Z19B
#define MQ135_PIN    14
#define MQ137_PIN    26
#define MQ138_PIN    33
#define MQ7_PIN      35

Adafruit_SGP30 sgp;
Adafruit_BME680 bme;           // I2C
HardwareSerial co2Serial(2);   // UART2 pentru MH-Z19B
BluetoothSerial SerialBT;

// MH-Z19B citire robustă
byte readCmd[9] = {0xFF,0x01,0x86,0,0,0,0,0,0x79};
const int MAX_RETRIES = 3;
const int AVG_SIZE = 5;
int readings[AVG_SIZE];
int readIndex = 0;
int readCount = 0;

byte calcChecksum(byte *buf){
  int sum = 0;
  for(int i=1;i<=7;i++) sum += buf[i];
  return (0xFF - (sum & 0xFF) + 1) & 0xFF;
}

int readCO2Once(int timeoutMs=200){
  for(int attempt=0; attempt<MAX_RETRIES; attempt++){
    while (co2Serial.available()) co2Serial.read();
    co2Serial.write(readCmd, 9);
    unsigned long start = millis();
    while (millis() - start < (unsigned long)timeoutMs){
      if (co2Serial.available() >= 9){
        byte resp[9];
        co2Serial.readBytes(resp, 9);
        if (resp[0] == 0xFF && resp[1] == 0x86 && resp[8] == calcChecksum(resp)){
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

// FUNCȚII UTILE
float averageFromPins(int pins[], int n) {
  float sum = 0;
  for (int i = 0; i < n; i++) sum += analogRead(pins[i]);
  return sum / n;
}

float mapAndCalibrate(float raw, float inMin, float inMax, float outMin, float outMax, float offset=0, float scale=1.0){
  if (inMax == inMin) return (raw + offset) * scale;
  float mapped = ((raw - inMin)/(inMax - inMin))*(outMax - outMin) + outMin;
  return (mapped + offset) * scale;
}

bool prevBTclient = false;

void setup() {
  delay(10000);
  Serial.begin(115200);
  SerialBT.begin("ESP32_Airnalyzer"); // Numele vizibil pentru pairing
  Serial.println("\n=== Initializare dispozitiv ===");
  SerialBT.println("Merge cica");

  // I2C pe pinii specificați (SDA = 21, SCL = 22)
  Wire.begin(21, 22);
  delay(50);

  // --- SGP30 ---
  Serial.println("Init SGP30...");
  if (!sgp.begin()) {
    Serial.println("✖ Eroare init SGP30");
    while (1) { delay(10); }
  }
  sgp.IAQinit();
  Serial.println("✔ SGP30 OK (I2C 0x58)");

  // --- BME68x pe I2C (încercăm 0x76/0x77) ---
  Serial.println("Init BME68x (I2C)...");
  bool bme_ok = false;
  uint8_t bme_addr_tried = 0;
  const uint8_t addr_try[] = {0x76, 0x77};
  for (uint8_t i=0; i < sizeof(addr_try); i++) {
    uint8_t addr = addr_try[i];
    Serial.print("Încearcă BME la 0x");
    if (addr < 16) Serial.print("0");
    Serial.println(addr, HEX);
    if (bme.begin(addr)) {
      bme_ok = true;
      bme_addr_tried = addr;
      break;
    }
  }
  if (!bme_ok) {
    Serial.println("✖ Eroare init BME68x (nu s-a găsit la 0x76/0x77)");
    while (1) { delay(10); }
  }
  Serial.print("✔ BME68x OK la adresa I2C 0x");
  if (bme_addr_tried < 16) Serial.print("0");
  Serial.println(bme_addr_tried, HEX);

  // setări BME
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);

  // --- MH-Z19B UART ---
  co2Serial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  for(int i=0;i<AVG_SIZE;i++) readings[i]=0;

  Serial.println("Așteptare stabilizare MH-Z19B...");
  delay(10000); // stabilizare

  Serial.println("============================\n");
}

void loop() {
  // --- Detectare conectare/deconectare BT (doar pentru log pe Serial Monitor) ---
  bool curClient = SerialBT.hasClient();
  if (curClient && !prevBTclient) {
    Serial.println(">>> Bluetooth client conectat.");
    prevBTclient = true;
  } else if (!curClient && prevBTclient) {
    Serial.println(">>> Bluetooth client deconectat.");
    prevBTclient = false;
  }

  // --- Citire senzori analogici MQ ---
  int mq3 = analogRead(MQ3_PIN);
  int mq136 = analogRead(MQ136_PIN);
  int mq135 = analogRead(MQ135_PIN);
  int mq137 = analogRead(MQ137_PIN);
  int mq138 = analogRead(MQ138_PIN);
  int mq7 = analogRead(MQ7_PIN);

  // --- Citire senzori digitali/BME/SGP ---
  float temperature = 36.5;   // valoare fallback
  float humidity = 45.0;      // valoare fallback
  float acetone = mq3;
  float ammonia = mq136;
  float co = mq7;
  float co2 = (float)readCO2Once();
  float h2s = mq135;

  // BME: actualizează temperature/humidity dacă e disponibil
  if(bme.performReading()){
    temperature = bme.temperature; // °C
    humidity = bme.humidity;       // %RH
  }

  // SGP30: actualizează TVOC/eCO2; combinăm TVOC cu acetone (logicul tău original)
  if(sgp.IAQmeasure()){
    acetone = (acetone + (float)sgp.TVOC) / 2.0;
  }

  // --- Calibrare și mapare ---
//  temperature = mapAndCalibrate(temperature, -40.0, 85.0, 34.0, 40.0);
//  humidity    = mapAndCalibrate(humidity, 0.0, 100.0, 30.0, 95.0);
//  acetone     = mapAndCalibrate(acetone, 0.0, 4095.0, 0.2, 2.0);
//  ammonia     = mapAndCalibrate((float)ammonia, 0.0, 4095.0, 0.0, 2.0);
//  co          = mapAndCalibrate((float)co, 0.0, 4095.0, 0.0, 5.0);
//  if (co2 < 0) {
//    co2 = -1.0;
//  } else {
//    co2 = mapAndCalibrate(co2, 0.0, 10000.0, 350.0, 5000.0);
//  }
//  h2s = mapAndCalibrate((float)h2s, 0.0, 4095.0, 0.0, 0.1);

  // --- JSON strict ---
  StaticJsonDocument<256> doc;
  doc["Temperature"] = temperature;
  doc["Humidity"]    = humidity;
  doc["Acetone"]     = acetone;
  doc["Ammonia"]     = ammonia;
  doc["CO"]          = co;
  doc["CO2"]         = co2;
  doc["H2S"]         = h2s;

  char buffer[256];
  serializeJson(doc, buffer);

  // --- Transmitere date (Bluetooth + Serial) ---
  // Trimitem oricând; clientul va primi doar dacă e conectat.
  SerialBT.println(buffer);
  Serial.println(buffer);

  delay(8000); // citire la 8s
}
