#include <Arduino.h>
#include <ArduinoJson.h>
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

// Variabile pentru senzori
float Temperature = 36.5;
float Humidity = 45.0;
float Acetone = 0.5;
float Ammonia = 0.3;
float CO = 0.4;
float CO2 = 400;
float H2S = 0.01;

// Boală simulată
int disease = 0;

void setup() {
  Serial.begin(115200);

  // Pornire Bluetooth
  if(!SerialBT.begin("ESP32_Airnalyzer")) {
    Serial.println("Eroare pornire Bluetooth");
  } else {
    Serial.println("Bluetooth pornit. Aștept conexiune...");
  }

  randomSeed(analogRead(0));
}

void loop() {
  // Alege o boală random
  disease = random(0, 7);

  // --- Set valori de bază și noise ---
  float base_temp = 36.7 + random(-2,3)/10.0;
  float base_hum = 40 + random(-5,6);
  float base_acetone = 0.3 + random(-5,6)/100.0;
  float base_ammonia = 0.02 + random(-2,3)/100.0;
  float base_co = 0.4 + random(-5,6)/100.0;
  float base_co2 = 400 + random(-10,11);
  float base_h2s = 0.01 + random(-3,4)/1000.0;

  // --- Modificare valori în funcție de boală ---
  switch(disease) {
    case 1: // Diabetes
      Acetone = base_acetone + 1.5 + random(-5,5)/100.0;
      Ammonia = base_ammonia;
      CO = base_co;
      CO2 = base_co2;
      H2S = base_h2s;
      Temperature = base_temp;
      Humidity = base_hum;
      break;
    case 2: // Lung Infection
      Temperature = base_temp + random(5,20)/10.0;
      Humidity = base_hum + random(-5,5);
      CO2 = base_co2 + random(50,150);
      CO = base_co + random(20,30)/100.0;
      H2S = base_h2s + random(5,15)/1000.0;
      Acetone = base_acetone;
      Ammonia = base_ammonia + random(3,6)/100.0;
      break;
    case 3: // Asthma
      Temperature = base_temp;
      Humidity = base_hum + random(5,10);
      CO2 = base_co2 + random(20,80);
      CO = base_co + random(5,10)/100.0;
      Acetone = base_acetone;
      Ammonia = base_ammonia;
      H2S = base_h2s;
      break;
    case 4: // Liver Dysfunction
      Temperature = base_temp;
      Humidity = base_hum;
      Acetone = base_acetone;
      Ammonia = base_ammonia + random(30,60)/100.0;
      CO = base_co;
      CO2 = base_co2;
      H2S = base_h2s + random(10,20)/1000.0;
      break;
    case 5: // COPD
      Temperature = base_temp + random(0,5)/10.0;
      Humidity = base_hum + random(-5,5);
      CO = base_co + random(20,40)/100.0;
      CO2 = base_co2 + random(100,200);
      Acetone = base_acetone;
      Ammonia = base_ammonia;
      H2S = base_h2s;
      break;
    case 6: // Unclear
      Temperature = base_temp + random(-5,10)/10.0;
      Humidity = base_hum + random(-5,10);
      Acetone = base_acetone + random(-10,40)/100.0;
      Ammonia = base_ammonia + random(-10,40)/100.0;
      CO = base_co + random(-10,50)/100.0;
      CO2 = base_co2 + random(-20,100);
      H2S = base_h2s + random(-5,20)/1000.0;
      break;
    default: // Normal
      Temperature = base_temp;
      Humidity = base_hum;
      Acetone = base_acetone;
      Ammonia = base_ammonia;
      CO = base_co;
      CO2 = base_co2;
      H2S = base_h2s;
      break;
  }

  // --- Trimite JSON ---
  StaticJsonDocument<256> doc;
  doc["Temperature"] = Temperature;
  doc["Humidity"] = Humidity;
  doc["Acetone"] = Acetone;
  doc["Ammonia"] = Ammonia;
  doc["CO"] = CO;
  doc["CO2"] = CO2;
  doc["H2S"] = H2S;

  char buffer[256];
  serializeJson(doc, buffer);

  // Trimite prin Bluetooth dacă e client
  if(SerialBT.hasClient()) SerialBT.println(buffer);

  // Debug în serial monitor
  Serial.println(buffer);

  delay(8000); // trimite la fiecare 8 secunde
}
