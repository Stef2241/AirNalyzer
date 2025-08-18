#include <BluetoothSerial.h> // Biblioteca Bluetooth Classic

// --- DEFINIȚII PINI ---
#define MQ135_PIN  34
#define MQ137_PIN  35
#define MQ138_PIN  32
#define MQ7_PIN    33

// --- INSTANȚIERE BLUETOOTH ---
BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Senzoei"); // Nume dispozitiv Bluetooth

  Serial.println("\n=== Inițializare senzori MQ ===");
  Serial.println("Bluetooth activat. Caută 'ESP32_GasSensors' și conectează-te.");
  Serial.println("===============================\n");

  delay(2000); // timp scurt pentru stabilizare
}

void loop() {
  String data = ""; // Vom construi mesajul pentru BT

  // --- MQ135 ---
  int mq135 = analogRead(MQ135_PIN);
  data += "MQ-135 (CO₂, NH₃, benzen, alcool, fum): " + String(mq135) + "\n";

  // --- MQ137 ---
  int mq137 = analogRead(MQ137_PIN);
  data += "MQ-137 (NH₃ - amoniac) " + String(mq137) + "\n";

  // --- MQ138 ---
  int mq138 = analogRead(MQ138_PIN);
  data += "MQ-138 (benzen, toluen, alcool, acetonă, fum) " + String(mq138) + "\n";

  // --- MQ7 ---
  int mq7 = analogRead(MQ7_PIN);
  data += "MQ-7 (CO - monoxid de carbon) " + String(mq7) + "\n";

  // --- Trimitem prin Serial și Bluetooth ---
  Serial.print(data);
  SerialBT.print(data);

  Serial.println("----------------------------\n");
  SerialBT.println("----------------------------\n");

  delay(5000); // citire la 2 secunde
}
