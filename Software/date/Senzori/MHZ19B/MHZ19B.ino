// Robust MH-Z19B read + checksum + retry + moving average
#define RX_PIN 16
#define TX_PIN 17
HardwareSerial co2Serial(2);

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
    while (co2Serial.available()) co2Serial.read(); // clear
    co2Serial.write(readCmd, 9);
    unsigned long start = millis();
    while (millis() - start < timeoutMs){
      if (co2Serial.available() >= 9){
        byte resp[9];
        co2Serial.readBytes(resp, 9);
        if (resp[0] == 0xFF && resp[1] == 0x86 && resp[8] == calcChecksum(resp)){
          int ppm = resp[2]*256 + resp[3];
          if (ppm>=0 && ppm<=10000) return ppm; // sanity
        }
        // dacă ajunge aici înseamnă răspuns invalid -> retry
        break;
      }
    }
    delay(100); // mic pauză înainte retry
  }
  return -1; // eroare
}

void setup(){
  Serial.begin(115200);
  co2Serial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(2000);
  for(int i=0;i<AVG_SIZE;i++) readings[i]=0;
}

void loop(){
  int ppm = readCO2Once(200);
  if (ppm >= 0){
    // moving average
    readings[readIndex] = ppm;
    readIndex = (readIndex + 1) % AVG_SIZE;
    if (readCount < AVG_SIZE) readCount++;
    long sum = 0;
    for(int i=0;i<readCount;i++) sum += readings[i];
    int avg = sum / readCount;
    Serial.print("CO2 raw: "); Serial.print(ppm);
    Serial.print(" ppm  | avg: "); Serial.print(avg); Serial.println(" ppm");
  } else {
    Serial.println("Eroare citire CO2 (retry esuat). Verifica cablajul/alim.");
  }
  delay(2000);
}
