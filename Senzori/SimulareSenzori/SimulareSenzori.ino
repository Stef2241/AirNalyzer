#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

float sgp_eCO2;
float sgp_TVOC;
float bme_temperature;
float bme_humidity;
float bme_pressure;
float bme_gas_resistance;
int mq3;
int mq136;
int mq135;
int mq137;
int mq138;
int mq7;
int mhz19_ppm;
float mhz19_avg;

float clampf(float v, float a, float b) { if (v < a) return a; if (v > b) return b; return v; }
long clampl(long v, long a, long b) { if (v < a) return a; if (v > b) return b; return v; }

float smoothFloat(float prev, float minV, float maxV, float maxDelta, uint16_t jumpPercent) {
  float r = (random(-1000, 1001) / 1000.0) * maxDelta;
  float next = prev + r;
  if (random(0, 100) < jumpPercent) {
    next = random((long)(minV*100), (long)(maxV*100)+1) / 100.0;
  }
  return clampf(next, minV, maxV);
}

int smoothInt(int prev, int minV, int maxV, int maxDelta, uint8_t jumpPercent) {
  int delta = random(-maxDelta, maxDelta + 1);
  int next = prev + delta;
  if (random(0,100) < jumpPercent) {
    next = random(minV, maxV+1);
  }
  return (int)clampl(next, minV, maxV);
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Senzori");

  // seed (folosim analogRead(0) ^ millis() pentru mai mult entropie)
  randomSeed(analogRead(0) ^ (uint32_t)millis());

  sgp_eCO2 = 700.0;      // ppm
  sgp_TVOC = 80.0;       // ppb
  bme_temperature = 21.5;  // °C
  bme_humidity = 45.0;     // %
  bme_pressure = 1013.0;   // hPa
  bme_gas_resistance = 30000.0; // ohm (-> 30 KΩ)
  // analog ADC baseline (ESP32 ADC 0..4095)
  mq3   = 180;
  mq136 = 120;
  mq135 = 160;
  mq137 = 140;
  mq138 = 150;
  mq7   = 130;

  mhz19_ppm = 600;    // ppm
  mhz19_avg = (float)mhz19_ppm;
}

void loop() {
  // Update simulated sensors smoothly

  // SGP30: eCO2 (400-1200 ppm typical indoors), small step, occasional spike
  sgp_eCO2 = smoothFloat(sgp_eCO2, 400.0, 2000.0, 30.0, /*jumpPercent*/ 3); // occasional jump (3%)

  // TVOC: baseline small, occasionally larger if "activity"
  sgp_TVOC = smoothFloat(sgp_TVOC, 0.0, 2000.0, 20.0, /*jumpPercent*/ 5);

  // BME688: temp/hum/press/gas_resistance
  bme_temperature = smoothFloat(bme_temperature, 15.0, 30.0, 0.15, 1); // +/-0.15°C per loop
  bme_humidity    = smoothFloat(bme_humidity, 20.0, 70.0, 0.6, 2);     // +/-0.6% RH
  bme_pressure    = smoothFloat(bme_pressure, 980.0, 1035.0, 0.6, 1);  // +/-0.6 hPa
  // gas resistance in ohms - simulate in range ~5kΩ..1MΩ (report later as KΩ)
  bme_gas_resistance = smoothFloat(bme_gas_resistance, 5000.0, 1000000.0, bme_gas_resistance * 0.02, 2); // up to 2% step

  // MQ sensors (analog 0..4095), baseline and occasional spikes
  mq3   = smoothInt(mq3, 0, 4095, 40, 5);    // alcohol sensor - occasional higher spikes
  mq136 = smoothInt(mq136, 0, 4095, 30, 2);  // H2S/SO2 like sensor (normally low)
  mq135 = smoothInt(mq135, 0, 4095, 35, 3);  // general air quality
  mq137 = smoothInt(mq137, 0, 4095, 30, 2);  // NH3 - usually low
  mq138 = smoothInt(mq138, 0, 4095, 30, 3);  // organic vapours
  mq7   = smoothInt(mq7, 0, 4095, 20, 1);    // CO sensor analog baseline

  // MH-Z19B CO2 (ppm) - simulate realistic indoor for 4 people: baseline 450-1200, occasional peak up to 2000
  // small step changes + occasional jump
  mhz19_ppm = (int)smoothFloat((float)mhz19_ppm, 400.0, 3000.0, 25.0, 4);

  // update exponential moving average for CO2 (alpha small -> smooth avg)
  const float alpha = 0.08; // smoothing factor
  mhz19_avg = alpha * (float)mhz19_ppm + (1.0 - alpha) * mhz19_avg;

  String data = "";
  data += "SGP30 → eCO2: " + String((int)sgp_eCO2) + " ppm, TVOC: " + String((int)sgp_TVOC) + " ppb\n";
  data += "BME688 → Temp: " + String(bme_temperature, 2) +
          " °C, Hum: " + String(bme_humidity, 1) +
          " %, Press: " + String(bme_pressure, 2) +
          " hPa, Gas: " + String(bme_gas_resistance / 1000.0, 2) + " KΩ\n";
  data += "MQ-3 analog: " + String(mq3) + "\n";
  data += "MQ-136 analog: " + String(mq136) + "\n";
  data += "MH-Z19B → CO2 raw: " + String(mhz19_ppm) + " ppm | avg: " + String(mhz19_avg, 1) + " ppm\n";
  data += "MQ-135: " + String(mq135) + "\n";
  data += "MQ-137: " + String(mq137) + "\n";
  data += "MQ-138: " + String(mq138) + "\n";
  data += "MQ-7: " + String(mq7) + "\n";

  Serial.print(data);
  SerialBT.print(data);
  Serial.println("----------------------------\n");
  SerialBT.println("----------------------------\n");
  delay(5000);
}
