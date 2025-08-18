#include <Wire.h>
#include <SPI.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_BME680.h>
#include "BluetoothSerial.h" // Biblioteca Bluetooth Classic

// --- DEFINIȚII PINI ---
#define MQ3_PIN      34
#define MQ136_PIN    35
#define RX_PIN       16  // RX ESP32 <- TX MH-Z19B
#define TX_PIN       17  // TX ESP32 -> RX MH-Z19B
#define BME680_CS    5

// --- INSTANȚIERI SENZORI ---
Adafruit_SGP30 sgp;
Adafruit_BME680 bme(BME680_CS);
HardwareSerial co2Serial(2); // UART2 pentru MH-Z19B
BluetoothSerial SerialBT;    // Obiect Bluetooth

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
  byte ch = (0xFF - (sum & 0xFF) + 1) & 0xFF;
  return ch;
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
  SerialBT.begin("ESP32_Senzori"); // Nume dispozitiv Bluetooth
  Serial.println("\n=== Inițializare senzori ===");
  Serial.println("Bluetooth activat. Caută 'ESP32_Senzori' și conectează-te.");

  // SGP30
  Wire.begin(21, 22);
  if (!sgp.begin()) {
    Serial.println("✖ Eroare la init SGP30");
    while (1);
  }
  sgp.IAQinit();
  Serial.println("✔ SGP30 OK");

  // BME688
  SPI.begin();
  if (!bme.begin()) {
    Serial.println("✖ Eroare la init BME688");
    while (1);
  }
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);
  Serial.println("✔ BME688 OK");

  // MH-Z19B (cu metoda robustă)
  co2Serial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(2000);
  for(int i=0;i<AVG_SIZE;i++) readings[i]=0;

  Serial.println("Așteptare stabilizare MH-Z19B...");
  delay(10000);

  Serial.println("============================\n");
}

void loop() {
  String data = ""; // Vom construi aici mesajul pentru BT

  // --- SGP30 ---
  if (sgp.IAQmeasure()) {
    data += "SGP30 → eCO2: " + String(sgp.eCO2) + " ppm, TVOC: " + String(sgp.TVOC) + " ppb\n";
  }

  // --- BME688 ---
  if (bme.performReading()) {
    data += "BME688 → Temp: " + String(bme.temperature) +
            " °C, Hum: " + String(bme.humidity) +
            " %, Press: " + String(bme.pressure / 100.0) +
            " hPa, Gas: " + String(bme.gas_resistance / 1000.0) + " KΩ\n";
  }

  // --- MQ3 & MQ136 ---
  int mq3 = analogRead(MQ3_PIN);
  int mq136 = analogRead(MQ136_PIN);
  data += "MQ-3 analog: " + String(mq3) + "\n";
  data += "MQ-136 analog: " + String(mq136) + "\n";

  // --- MH-Z19B robust ---
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

  // --- Trimite datele prin Serial și Bluetooth ---
  Serial.print(data);
  SerialBT.print(data);

  Serial.println("----------------------------\n");
  SerialBT.println("----------------------------\n");

  delay(2000);
}
