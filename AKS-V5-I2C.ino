#include "LoRa_E22.h"
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include "RingBuffer.h"
#include <mcp2515.h>
#include <Wire.h>

LoRa_E22 e22ttl(&Serial3);
SoftwareSerial nextion(18, 19);
RingBuffer ringbuffer(1000);

// I2C KODLARI

const int slaveAddress = 0x08;
uint8_t receivedSOC;
uint16_t receivedCapacity;

// I2C END

struct can_frame Received_Data;
struct can_frame Transmitted_Data;
MCP2515 mcp2515(53);

// Data structures to store BMS information
struct {
  uint8_t temps[5];
  uint8_t maxTemp;
  uint16_t sumVoltage;
  uint16_t soc;
  uint16_t powerWatt;
  uint16_t cellVoltages[20];
  int16_t current;
} bmsData;


const int chipSelect = 13;
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 600;
const int analogPin = A8;
const int relayPin = 8;
unsigned long previousMillis = 0;
const long interval = 60000;
int dosyaSayaci = 0;
const int PRESARJ_ROLE_PIN = 22;
const int MOTOR_SURUCU_ROLE_PIN = 23;
const int SARJ_ROLE_PIN = 24;
const int SARJ_DURUM_PIN = 32;  // 25. pin olması gerekiyor. Düzeltilecek!
unsigned long presarjBaslangicZamani = 0;
const unsigned long PRESARJ_SURESI = 3000;  // 3 saniye
bool sarjDurumu = false;
const int sagRolePin = 3;  // IN1
const int solRolePin = 4;  // IN2
const int buton1Pin = 6;
const int buton2Pin = 7;
bool buton1State = LOW;
bool buton2State = LOW;
bool sagSinyalAktif = false;
bool solSinyalAktif = false;
char mevcutDosyaAdi[13];



File veriDosyasi;

struct Veriler {
  int arac_hizi;
  float hucreVoltajlari[20];
  float toplamVoltaj;
  int kalanEnerjiMiktari;
  float sicaklik;
  float tahminiSarjSuresi;
  byte temp[4];
} data;


struct Veriler2 {
  byte dizi[24];
} data2;

struct Alici {
  byte temp[4];
} data3;



float hucreVoltajlari[20];
float toplamVoltaj, sicaklik;

bool loraConnected = false;
unsigned long lastLoraDataTime = 0;
const unsigned long loraTimeout = 2000;
const unsigned long loraWarningTime = 10000;


bool requestData(uint8_t requestId) {
  unsigned long startTime = millis();

  while (millis() - startTime < 3000) {
    Transmitted_Data.data[0] = requestId;
    mcp2515.sendMessage(&Transmitted_Data);

    delay(50);

    if (mcp2515.readMessage(&Received_Data) == MCP2515::ERROR_OK) {
      for (int i = 0; i < 8; i++) {
        // Serial.println(Received_Data.data[i]);
      }
      if (Received_Data.data[0] == requestId + 100) {
        processReceivedData(requestId + 100);
        return true;
      }
    }
  }

  Serial.println("Timeout waiting for response");
  return false;
}


void processReceivedData(uint8_t responseId) {
  switch (responseId) {
    case 101:
      for (int i = 0; i < 4; i++) {
        bmsData.temps[i] = Received_Data.data[i + 1];
      }
      bmsData.maxTemp = Received_Data.data[5];
      break;
    case 102:
      bmsData.sumVoltage = (Received_Data.data[1] << 8) | Received_Data.data[2];
      bmsData.soc = (Received_Data.data[3] << 8) | Received_Data.data[4];
      bmsData.powerWatt = (Received_Data.data[5] << 8) | Received_Data.data[6];
      break;
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
    case 108:
      for (int i = 0; i < 3; i++) {
        int index = (responseId - 103) * 3 + i;
        bmsData.cellVoltages[index] = (Received_Data.data[i * 2 + 1] << 8) | Received_Data.data[i * 2 + 2];
      }
      break;
    case 109:
      bmsData.cellVoltages[18] = (Received_Data.data[1] << 8) | Received_Data.data[2];
      bmsData.cellVoltages[19] = (Received_Data.data[3] << 8) | Received_Data.data[4];
      break;
  }

  bmsData.maxTemp = max(bmsData.temps[0], bmsData.temps[1]);
  bmsData.maxTemp = max(bmsData.maxTemp, bmsData.temps[2]);
  bmsData.maxTemp = max(bmsData.maxTemp, bmsData.temps[3]);
  bmsData.temps[4] = bmsData.maxTemp - 40;
  bmsData.maxTemp -= 40;
}


void printBmsData() {
  Serial.println("BMS Data:");
  Serial.print("Temperatures: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(bmsData.temps[i]);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("Max Temperature: ");
  Serial.println(bmsData.maxTemp);
  Serial.print("Sum Voltage: ");
  Serial.println(bmsData.sumVoltage);
  Serial.print("SOC: ");
  Serial.println(bmsData.soc);
  Serial.print("Power: ");
  Serial.println(bmsData.powerWatt);

  Serial.println("Cell Voltages:");
  for (int i = 0; i < 20; i++) {
    Serial.print(bmsData.cellVoltages[i]);
    Serial.print(" ");
    if ((i + 1) % 5 == 0) {
      Serial.println();
    }
  }

  //  Serial.print("Current: ");
  //  Serial.println(bmsData.current);
  //  Serial.println();
}


void setup() {

  SPI.begin();
  Serial.begin(9600);

  Wire.begin(slaveAddress);
  Wire.onReceive(receiveEvent);

  e22ttl.begin();
  delay(500);
  nextion.begin(9600);
  randomSeed(analogRead(0));
  if (!SD.begin(chipSelect)) {
    //  Serial.println("SD kart başlatılamadı!");
    return;
  }
  //  Serial.println("SD kart başlatıldı.");
  yeniDosyaAc();
  pinMode(PRESARJ_ROLE_PIN, OUTPUT);
  pinMode(MOTOR_SURUCU_ROLE_PIN, OUTPUT);
  pinMode(SARJ_ROLE_PIN, OUTPUT);
  pinMode(SARJ_DURUM_PIN, INPUT);
  digitalWrite(PRESARJ_ROLE_PIN, HIGH);
  presarjBaslangicZamani = millis();
  pinMode(sagRolePin, OUTPUT);
  pinMode(solRolePin, OUTPUT);

  pinMode(buton1Pin, INPUT_PULLUP);  //  Dahili pull-up direnç kullanımı
  pinMode(buton2Pin, INPUT_PULLUP);  // Dahili pull-up direnç kullanımı

  digitalWrite(sagRolePin, HIGH);  // Röleleri başlangıçta kapalı yap
  digitalWrite(solRolePin, HIGH);  // Röleleri başlangıçta kapalı yap

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Transmitted_Data.can_id = 123;
  Transmitted_Data.can_dlc = 1;
}


void receiveEvent(int bytesReceived) {
  Serial.println("******************* OnReceive *************************");
  receivedSOC = Wire.read();

  uint8_t highByte = Wire.read();

  uint8_t lowByte = Wire.read();

  receivedCapacity = (highByte << 8) | lowByte;

  Serial.print("Received SOC: ");
  Serial.print(receivedSOC);
  Serial.println(" %");

  Serial.print("Received Capacity: ");
  Serial.print(receivedCapacity);
  Serial.println(" mAh");
  Serial.println();
}


void loop() {
  for (int i = 1; i <= 9; i++) {
    requestData(i);
  }


  printBmsData();

  unsigned long currentTime = millis();
  bool tempDataReceived = false;

  // ****************************************************************************************
  while (e22ttl.available() > 1) {
    ResponseStructContainer rsc = e22ttl.receiveMessage(sizeof(Veriler));
    struct Alici receivedData = *(Alici*)rsc.data;
    float tempValue = *(float*)(receivedData.temp);
    if (!isnan(tempValue)) {
      tempDataReceived = true;
      lastLoraDataTime = currentTime;
      // Serial.print("Temp verisi alındı: ");
      // Serial.println(tempValue);

      if (!loraConnected) {
        loraConnected = true;
        // Serial.println("LoRa bağlantısı kuruldu");
      }
    }
    rsc.close();
  }
  // ****************************************************************************************

  unsigned long timeSinceLastData = currentTime - lastLoraDataTime;
  if (loraConnected) {
    if (timeSinceLastData > loraWarningTime && timeSinceLastData <= loraTimeout) {
      // Serial.println("Uyarı: LoRa verisi gecikiyor");
    } else if (timeSinceLastData > loraTimeout) {
      loraConnected = false;
      // Serial.println("LoRa bağlantısı kesildi");
    }
  }
  veriKaydetSD();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
  }
  delay(500);

  if (millis() - presarjBaslangicZamani >= PRESARJ_SURESI) {
    digitalWrite(PRESARJ_ROLE_PIN, LOW);  // Preşarj rölesini kapat
    if (digitalRead(MOTOR_SURUCU_ROLE_PIN) == LOW) {
      digitalWrite(MOTOR_SURUCU_ROLE_PIN, HIGH);  // Motor sürücü rölesini aç
    }
  }

  // Şarj durumunu kontrol et
  bool yeniSarjDurumu = digitalRead(SARJ_DURUM_PIN) == HIGH;
  if (yeniSarjDurumu != sarjDurumu) {
    sarjDurumu = yeniSarjDurumu;
    if (sarjDurumu) {
      // Serial.println("Araç şarj oluyor");
      digitalWrite(MOTOR_SURUCU_ROLE_PIN, LOW);  // Motor sürücü rölesini kapat
      digitalWrite(SARJ_ROLE_PIN, HIGH);         // Şarj rölesini aç
    } else {
      // Serial.println("Araç şarjı bitti");
      digitalWrite(SARJ_ROLE_PIN, LOW);           // Şarj rölesini kapat
      digitalWrite(MOTOR_SURUCU_ROLE_PIN, HIGH);  // Motor sürücü rölesini aç
    }
  }

  if (data.sicaklik >= 60) {
    if (sarjDurumu) {
      digitalWrite(SARJ_ROLE_PIN, LOW);  // Şarj rölesini kapat
      // Serial.println("Yüksek sıcaklık nedeniyle şarj durduruldu");
    } else {
      digitalWrite(MOTOR_SURUCU_ROLE_PIN, LOW);  // Motor sürücü rölesini kapat
      // Serial.println("Yüksek sıcaklık nedeniyle motor sürücü kapatıldı");
    }
  }
  // Düşük voltaj röle kontrolü
  if (data.toplamVoltaj <= 75.0) {
    digitalWrite(MOTOR_SURUCU_ROLE_PIN, LOW);  // Motor sürücü rölesini kapat
    // Serial.println("Düşük batarya voltajı! Motor sürücü kapatıldı.");
  } else if (!sarjDurumu && data.sicaklik < 60) {
    digitalWrite(MOTOR_SURUCU_ROLE_PIN, HIGH);  // Normal durumlarda motor sürücü rölesini aç
  }
  delay(1000);
  //*****************************************************************************


  int geciciDizi1[24];
  geciciDizi1[23] = bmsData.maxTemp;
  data.sicaklik = bmsData.maxTemp;

  geciciDizi1[20] = random(0, 120);
  data.arac_hizi = random(0, 120);

  geciciDizi1[21] = 0;
  data.toplamVoltaj = 0;

  data.toplamVoltaj = (float)(bmsData.sumVoltage / 10.0);

  for (int i = 0; i < 20; i++) {
    geciciDizi1[i] = bmsData.cellVoltages[i];
    data.hucreVoltajlari[i] = bmsData.cellVoltages[i];

    geciciDizi1[21] = data.toplamVoltaj;
    data.toplamVoltaj = data.toplamVoltaj;
  }
  geciciDizi1[22] = random(1000, 4000);
  data.kalanEnerjiMiktari = random(1000, 4000);

  for (int i = 0; i < 24; i++) {
    ringbuffer.write(geciciDizi1[i]);
  }

  // for (int i = 0; i < 20; i++) {
  //   ringbuffer.write(data.hucreVoltajlari[i]);
  // }
  // ringbuffer.write(data.arac_hizi); // 20
  // ringbuffer.write(data.toplamVoltaj); // 21
  // ringbuffer.write(data.kalanEnerjiMiktari); // 22
  // ringbuffer.write(data.sicaklik); // 23


  if (currentTime - lastUpdateTime >= updateInterval) {
    updateValues();
    updateDisplay();
    lastUpdateTime = currentTime;
  }

  buton1State = digitalRead(buton1Pin);
  buton2State = digitalRead(buton2Pin);

  if (buton1State == LOW && buton2State == HIGH) {  // Sadece buton 1 basılı
    sagSinyalAktif = true;
    solSinyalAktif = false;
  } else if (buton2State == LOW && buton1State == HIGH) {  // Sadece buton 2 basılı
    sagSinyalAktif = false;
    solSinyalAktif = true;
  } else {  // İki tuşa da basılmadığında veya ikisine de aynı anda basıldığında
    sagSinyalAktif = false;
    solSinyalAktif = false;
  }

  if (sagSinyalAktif) {
    sagSinyalTikTak();
  } else if (solSinyalAktif) {
    solSinyalTikTak();
  } else {
    digitalWrite(sagRolePin, HIGH);  // Röleleri kapat
    digitalWrite(solRolePin, HIGH);  // Röleleri kapat
  }


  if (loraConnected) {
    for (int i = 0; i < 24; i++) {
      int tempData = ringbuffer.read();
      data2.dizi[i] = tempData;
    }
  }
  ResponseStatus rs = e22ttl.sendFixedMessage(0, 196, 12, &data2, sizeof(Veriler2));
  delay(1500);
}



void sagSinyalTikTak() {
  digitalWrite(solRolePin, HIGH);  // Sol LED kapalı
  digitalWrite(sagRolePin, LOW);   // Sağ LED açık
  delay(500);
  digitalWrite(sagRolePin, HIGH);  // Sağ LED kapalı
  delay(500);
}

void solSinyalTikTak() {
  digitalWrite(sagRolePin, HIGH);  // Sağ LED kapalı
  digitalWrite(solRolePin, LOW);   // Sol LED açık
  delay(500);
  digitalWrite(solRolePin, HIGH);  // Sol LED kapalı
  delay(500);
}

void updateValues() {

  gonderText("t8", String(data.kalanEnerjiMiktari) + " Wh");
  gonderGosterge("z0", map(data.kalanEnerjiMiktari, 800, 4444, 0, 180));
  gonderText("t7", String(data.arac_hizi));
  gonderGosterge("z1", map(data.arac_hizi, 0, 180, 0, 100));

  gonderText("t10", String(data.toplamVoltaj, 2) + " V");
  gonderText("t12", String(data.sicaklik, 1) + " °C");

  for (int i = 0; i < 20; i++) {
    gonderText("k" + String(i + 9), String(data.hucreVoltajlari[i], 2) + " V");
    gonderGosterge("j" + String(i), data.hucreVoltajlari[i] * 25);
  }
}
void updateDisplay() {
  static int updateCounter = 0;
  updateCounter = (updateCounter + 1) % 4;

  switch (updateCounter) {
    case 0:
      updateGaugesAndMainInfo();
      break;
    case 1:
      updateTemperatures();
      break;
    case 2:
      updateVoltages(0, 9);
      break;
    case 3:
      updateVoltages(9, 19);
      break;
  }
}

void gonderText(String id, String veri) {
  String komut = id + ".txt=\"" + veri + "\"";
  nextion.print(komut);
  nextion.write(0xff);  // Nextion komut sonu baytları
  nextion.write(0xff);
  nextion.write(0xff);
}

void gonderGosterge(String id, int deger) {
  String komut = id + ".val=" + String(deger);
  nextion.print(komut);
  nextion.write(0xff);  // Nextion komut sonu baytları
  nextion.write(0xff);
  nextion.write(0xff);
}
void updateGaugesAndMainInfo() {
}

void updateTemperatures() {
}

void updateVoltages(int startIdx, int endIdx) {
}

void yeniDosyaAc() {
  if (veriDosyasi) {
    veriDosyasi.close();
  }

  do {
    dosyaSayaci++;
    snprintf(mevcutDosyaAdi, sizeof(mevcutDosyaAdi), "VLR%03d.TXT", dosyaSayaci);
  } while (SD.exists(mevcutDosyaAdi));

  veriDosyasi = SD.open(mevcutDosyaAdi, FILE_WRITE);
  if (veriDosyasi) {
    // Serial.println("Yeni dosya açıldı: " + String(mevcutDosyaAdi));
  } else {
    // Serial.println("Yeni dosya açılamadı: " + String(mevcutDosyaAdi));
    while (true)
      ;
  }
}

void dosyayaYazdir() {
  veriDosyasi.print(data.arac_hizi);
  veriDosyasi.print(",");
  veriDosyasi.print(data.kalanEnerjiMiktari, 2);
  veriDosyasi.print(",");
  veriDosyasi.print(data.toplamVoltaj, 2);
  veriDosyasi.print(",");
  veriDosyasi.print(bmsData.maxTemp, 2);

  for (int i = 0; i < 20; i++) {
    veriDosyasi.print(", ");
    veriDosyasi.print(bmsData.cellVoltages[i], 2);
  }
  veriDosyasi.println();
}

void veriKaydetSD() {
  if (veriDosyasi) {

    // Serial.print("Araç Hızı: ");
    // Serial.println(data.arac_hizi);
    // Serial.print("Kalan Enerji: ");
    // Serial.println(data.kalanEnerjiMiktari);
    // Serial.print("Toplam Voltaj: ");
    // Serial.println(data.toplamVoltaj);
    // Serial.print("Max Sıcaklık: ");
    // Serial.println(bmsData.maxTemp);

    if (data.kalanEnerjiMiktari == 0 && data.toplamVoltaj == 0 && bmsData.maxTemp == 0) {
      return;
    }

    dosyayaYazdir();

    veriDosyasi.flush();
    // Serial.println("Veri SD karta yazıldı.");
  } else {
    // Serial.println("SD karta yazma hatası!");
    if (SD.begin(chipSelect)) {
      yeniDosyaAc();
    } else {
      // Serial.println("SD kart yeniden başlatılamadı!");
    }
  }
}
