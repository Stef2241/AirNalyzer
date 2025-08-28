#include <Wire.h>
#include <SPI.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_BME680.h>
#include "BluetoothSerial.h"

// --- DEFINIȚII PINI ---
#define MQ3_PIN      19
#define MQ136_PIN    15
#define RX_PIN       16  // RX ESP32 <- TX MH-Z19B
#define TX_PIN       17  // TX ESP32 -> RX MH-Z19B
#define MQ135_PIN    14
#define MQ137_PIN    26
#define MQ138_PIN    33
#define MQ7_PIN      35

// --- INSTANȚIERI SENZORI ---
// SGP30 (I2C, adresa fixă 0x58)
Adafruit_SGP30 sgp;

// BME680/BME688 - pentru I2C instanțiem fără CS
Adafruit_BME680 bme; // I2C

HardwareSerial co2Serial(2); // UART2 pentru MH-Z19B
BluetoothSerial SerialBT;

// --- MH-Z19B citire robustă ---
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
    while (millis() - start < timeoutMs){
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

void setup() {
  Serial.begin(115200);
  delay(10000);
  Serial.println("incepem in 5");
  delay(5000);
  SerialBT.begin("ESP32_Senzori");
  Serial.println("\n=== Inițializare senzori ===");
  Serial.println("Bluetooth activat. Caută 'ESP32_Senzori' și conectează-te.");

  // I2C pe pinii specificați (SDA = 21, SCL = 22)
  Wire.begin(21, 22);
  delay(50);

  // --- SGP30 ---
  if (!sgp.begin()) {
    Serial.println("✖ Eroare la init SGP30");
    while (1) delay(10);
  }
  sgp.IAQinit();
  Serial.println("✔ SGP30 OK (I2C 0x58)");

  // --- BME688 prin I2C ---
  // încercăm adresele comune 0x76 și 0x77
  bool bme_ok = false;
  uint8_t bme_addr_tried = 0;
  const uint8_t addr_try[] = {0x76, 0x77};
  for (uint8_t i=0; i < sizeof(addr_try); i++) {
    uint8_t addr = addr_try[i];
    Serial.print("Încearcă BME la I2C 0x");
    if (addr < 16) Serial.print("0");
    Serial.println(addr, HEX);
    if (bme.begin(addr)) {
      bme_ok = true;
      bme_addr_tried = addr;
      break;
    }
  }

  if (!bme_ok) {
    Serial.println("✖ Eroare la init BME688 (nu s-a găsit la 0x76/0x77)");
    while (1) delay(10);
  }

  Serial.print("✔ BME688 OK la adresa I2C 0x");
  if (bme_addr_tried < 16) Serial.print("0");
  Serial.println(bme_addr_tried, HEX);

  // setări BME
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C pentru 150 ms

  // --- MH-Z19B ---
  co2Serial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(2000);
  for(int i=0;i<AVG_SIZE;i++) readings[i]=0;

  Serial.println("Așteptare stabilizare MH-Z19B...");
  delay(10000);

  Serial.println("============================\n");
}

void loop() {
  String data = "";

  // --- SGP30 ---
  if (sgp.IAQmeasure()) {
    data += "SGP30 → eCO2: " + String(sgp.eCO2) + " ppm, TVOC: " + String(sgp.TVOC) + " ppb\n";
  } else {
    data += "SGP30 → Eroare citire\n";
  }

  // --- BME688 ---
  if (bme.performReading()) {
    data += "BME688 → Temp: " + String(bme.temperature) +
            " °C, Hum: " + String(bme.humidity) +
            " %, Press: " + String(bme.pressure / 100.0) +
            " hPa, Gas: " + String(bme.gas_resistance / 1000.0) + " KΩ\n";
  } else {
    data += "BME688 → Eroare citire\n";
  }

  // --- MQ3 & MQ136 ---
  int mq3 = analogRead(MQ3_PIN);
  int mq136 = analogRead(MQ136_PIN);
  data += "MQ-3 analog: " + String(mq3) + "\n";
  data += "MQ-136 analog: " + String(mq136) + "\n";

  // --- MH-Z19B ---
  int ppm = readCO2Once(200);
  if (ppm >= 0){
    readings[readIndex] = ppm;
    readIndex = (readIndex + 1) % AVG_SIZE;
    if (readCount < AVG_SIZE) readCount++;
    long sum = 0;
    for(int i=0;i<readCount;i++) sum += readings[i];
    int avg = sum / readCount;
    data += "MH-Z19B → CO2 raw: " + String(ppm) + " ppm | avg: " + String(avg) + " ppm\n";
  } else {
    data += "MH-Z19B → Eroare citire CO2\n";
  }

  // --- MQ135, MQ137, MQ138, MQ7 ---
  int mq135 = analogRead(MQ135_PIN);
  int mq137 = analogRead(MQ137_PIN);
  int mq138 = analogRead(MQ138_PIN);
  int mq7 = analogRead(MQ7_PIN);

  data += "MQ-135: " + String(mq135) + "\n";
  data += "MQ-137: " + String(mq137) + "\n";
  data += "MQ-138: " + String(mq138) + "\n";
  data += "MQ-7: " + String(mq7) + "\n";

  // --- Trimite date ---
  Serial.print(data);
  SerialBT.print(data);
  Serial.println("----------------------------\n");
  SerialBT.println("----------------------------\n");

  delay(2000);
}
