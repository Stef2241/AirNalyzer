#include <BluetoothSerial.h>

#define VCC       5.0
#define RL        10000.0

#define MQ3_PIN      19
#define MQ136_PIN    15
#define MQ135_PIN    14
#define MQ137_PIN    26
#define MQ138_PIN    33
#define MQ7_PIN      32
BluetoothSerial SerialBT;

// --- citire medie ADC ---
float readADCavg(int pin, int n=50){
  long sum = 0;
  for(int i=0;i<n;i++){
    sum += analogRead(pin);
    delay(10); // mic delay între citiri
  }
  return (float)sum / n;
}

// --- calcul Rs ---
float adcToRs(float adc){
  if(adc <= 0) return 1e9;
  float vout = adc * (VCC / 4095.0);
  return (VCC * RL / vout) - RL;
}

// --- variabile pentru medie cumulativă ---
long count = 0;
double sum_MQ3 = 0, sum_MQ136 = 0, sum_MQ135 = 0, sum_MQ137 = 0, sum_MQ138 = 0, sum_MQ7 = 0;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_MQ_Calib");
  Serial.println("=== Calibrare MQ - Aer Curat ===");
  Serial.println("Se recomanda lasarea senzorilor in aer curat cateva ore...");
  delay(2000);
  delay(60000); // citire la 1 minut 
}

void loop() {
  // --- Citire curenta ---
  float Rs3   = adcToRs(readADCavg(MQ3_PIN));
  float Rs136 = adcToRs(readADCavg(MQ136_PIN));
  float Rs135 = adcToRs(readADCavg(MQ135_PIN));
  float Rs137 = adcToRs(readADCavg(MQ137_PIN));
  float Rs138 = adcToRs(readADCavg(MQ138_PIN));
  float Rs7   = adcToRs(readADCavg(MQ7_PIN));

  // --- Update sum si count ---
  sum_MQ3   += Rs3;
  sum_MQ136 += Rs136;
  sum_MQ135 += Rs135;
  sum_MQ137 += Rs137;
  sum_MQ138 += Rs138;
  sum_MQ7   += Rs7;
  count++;

  // --- Calcul medie ---
  double avg_MQ3   = sum_MQ3 / count;
  double avg_MQ136 = sum_MQ136 / count;
  double avg_MQ135 = sum_MQ135 / count;
  double avg_MQ137 = sum_MQ137 / count;
  double avg_MQ138 = sum_MQ138 / count;
  double avg_MQ7   = sum_MQ7 / count;

  // --- Afisare pe Serial ---
  Serial.print("MQ3 Rs curent = "); Serial.print(Rs3); Serial.print(" | avg = "); Serial.println(avg_MQ3);
  Serial.print("MQ136 Rs curent = "); Serial.print(Rs136); Serial.print(" | avg = "); Serial.println(avg_MQ136);
  Serial.print("MQ135 Rs curent = "); Serial.print(Rs135); Serial.print(" | avg = "); Serial.println(avg_MQ135);
  Serial.print("MQ137 Rs curent = "); Serial.print(Rs137); Serial.print(" | avg = "); Serial.println(avg_MQ137);
  Serial.print("MQ138 Rs curent = "); Serial.print(Rs138); Serial.print(" | avg = "); Serial.println(avg_MQ138);
  Serial.print("MQ7 Rs curent = "); Serial.print(Rs7); Serial.print(" | avg = "); Serial.println(avg_MQ7);
  Serial.println("-------------------------------");

  // --- Trimitere prin BT ---
  SerialBT.print("MQ3 Rs curent = "); SerialBT.print(Rs3); SerialBT.print(" | avg = "); SerialBT.println(avg_MQ3);
  SerialBT.print("MQ136 Rs curent = "); SerialBT.print(Rs136); SerialBT.print(" | avg = "); SerialBT.println(avg_MQ136);
  SerialBT.print("MQ135 Rs curent = "); SerialBT.print(Rs135); SerialBT.print(" | avg = "); SerialBT.println(avg_MQ135);
  SerialBT.print("MQ137 Rs curent = "); SerialBT.print(Rs137); SerialBT.print(" | avg = "); SerialBT.println(avg_MQ137);
  SerialBT.print("MQ138 Rs curent = "); SerialBT.print(Rs138); SerialBT.print(" | avg = "); SerialBT.println(avg_MQ138);
  SerialBT.print("MQ7 Rs curent = "); SerialBT.print(Rs7); SerialBT.print(" | avg = "); SerialBT.println(avg_MQ7);
  SerialBT.println("-------------------------------");

  delay(60000); // citire la 1 minut
}
