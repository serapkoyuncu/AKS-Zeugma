/*
ringbuffer.write kodları açılmalı.
SD kart bağlandığında döngü hızlandı mı diye kontrol edilmeli.
Serial.print kapatıldı. Açılabilir.

*/

#include "LoRa_E22.h"
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include "RingBuffer.h"
#include <mcp2515.h>
#include <Wire.h>


const int slaveAddressA = 0x08;
const int slaveAddressB = 0x09;
int receivedValueA = 0;
int sendValueA = 100;

uint8_t receivedSOC;
uint16_t receivedCapacity;
const int chipSelect = 13;
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 600;
const int analogPin = A8;
const int relayPin = 8;
unsigned long previousMillis = 0;
const long interval = 60000;
int dosyaSayaci = 0;
int chargeState = 0;

// const int flasorRole = 22;

// const int MOTOR_SURUCU_ROLE_PIN = 23;

// const int SARJ_ROLE_PIN = 24;


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
float hucreVoltajlari[20];
float toplamVoltaj, sicaklik;
int loraConnected = 0;
unsigned long lastLoraDataTime = 0;
const unsigned long loraTimeout = 2000;
const unsigned long loraWarningTime = 10000;
const unsigned long requestInterval = 100;

byte counterr = 0;


unsigned long anlikZaman;
int gecici = 1;

int soc = 0;
int baslikFLag = 1;
int kalanEnerji = 0;


struct can_frame Received_Data;
struct can_frame Transmitted_Data;


MCP2515 mcp2515(53);

SoftwareSerial mySerial(10, 11);

LoRa_E22 e22ttl(&mySerial);
SoftwareSerial nextion(18, 19);
RingBuffer ringbuffer(2000);
File veriDosyasi;
ResponseStatus rs;


struct {
  uint8_t temps[5];
  uint8_t maxTemp;
  uint16_t sumVoltage;
  uint16_t soc;
  uint16_t powerWatt;
  uint16_t cellVoltages[20];
  int16_t current;
} bmsData;


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
  byte dizi[79];
} data2;

struct Alici {
  byte temp[4];
} data3;

struct Signal {
  int dizi[79];
  byte temp[4];
} data4;

void requestFromB() {
  Wire.requestFrom(slaveAddressB, 1);
  while (Wire.available()) {
    receivedValueA = Wire.read();
    // Serial.print("Arduino A received: ");
    // Serial.println(receivedValueA);
  }
}

void receiveEventA(int howMany) {
  while (Wire.available()) {
    receivedValueA = Wire.read();
    receivedValueA = Wire.read();

    // Serial.print("Arduino A received via interrupt: ");
    // Serial.println(receivedValueA);
  }
}

void requestEventA() {
  Wire.write(sendValueA);  // İstek gelince veriyi gönder
}


void setup() {

  Serial.begin(9600);

  SPI.begin();
  delay(200);

  Wire.begin();  // Slave olarak başla

  nextion.begin(9600);
  randomSeed(analogRead(0));

  DDRD |= (1 << PD7);  // PD7'yi (38. pin) çıkış olarak ayarla
  DDRD |= (1 << PD6);  // PD7'yi (38. pin) çıkış olarak ayarla
  DDRD |= (1 << PD5);  // PD7'yi (38. pin) çıkış olarak ayarla

  PORTD &= ~(1 << PD5);
  PORTD &= ~(1 << PD6);
  PORTD &= ~(1 << PD7);

  pinMode(SARJ_DURUM_PIN, INPUT);

  presarjBaslangicZamani = millis();
  pinMode(sagRolePin, OUTPUT);
  pinMode(solRolePin, OUTPUT);

  pinMode(buton1Pin, INPUT_PULLUP);
  pinMode(buton2Pin, INPUT_PULLUP);

  digitalWrite(sagRolePin, HIGH);
  digitalWrite(solRolePin, HIGH);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Transmitted_Data.can_id = 123;
  Transmitted_Data.can_dlc = 8;

  e22ttl.begin();

  if (!SD.begin(chipSelect)) {
    // Serial.println("SD kart başlatılamadı!");
    return;
  }
  // Serial.println("SD kart başlatıldı.");
  yeniDosyaAc();
}






void loop() {



  // Serial.println(millis());
  for (int i = 1; i <= 9; i++) {
    requestData(i);
  }


  unsigned long startTime = millis();  // Zamanı başlat
  Wire.requestFrom(8, 4);              // request 2 bytes from slave device #8
  while (Wire.available() < 4) {
    if (millis() - startTime >= 200) {
      // Serial.println("Timeout: Data not received in time.");
      break;
    }
  }

  soc = Wire.read();  // receive high byte
  int soc2 = Wire.read();
  soc = (soc << 8) | soc2;



  int highByte = Wire.read();
  int lowByte = Wire.read();  // receive low byte
  int receivedData = (highByte << 8) | lowByte;

  // Serial.print("Soccccccccccccc: ");
  // Serial.println(soc);

  // Serial.print("Currenttttttttttt: ");
  // Serial.println(receivedData);


  printBmsData();

  unsigned long currentTime = millis();
  bool tempDataReceived = false;

  // // ****************************************************************************************

  // // ****************************************************************************************

  // unsigned long timeSinceLastData = millis() - lastLoraDataTime;
  // if (loraConnected) {
  //   if (timeSinceLastData > loraWarningTime && timeSinceLastData <= loraTimeout) {
  //     // Serial.println("Uyarı: LoRa verisi gecikiyor (DİKKATE ALMA)");
  //   } else if (timeSinceLastData > loraTimeout) {
  //     loraConnected += 1;
  //     // Serial.println("LoRa bağlantısı kesildi (DİKKATE ALMA)");
  //   }
  // }

  veriKaydetSD();



  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
  }


  if (bmsData.maxTemp >= 55) {
    PORTD |= (1 << PD7);
  } else {
    PORTD &= ~(1 << PD7);
  }

  if (bmsData.maxTemp >= 70) {
    PORTD |= (1 << PD6);
  } else {
    PORTD &= ~(1 << PD6);
  }

  if (soc <= 20) {  // DUZENLENECEK
    PORTD |= (1 << PD5);
  } else {
    PORTD &= ~(1 << PD5);
  }

  // // delay(750);
  // //*****************************************************************************


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


  // ******
  // for (int i = 0; i < 24; i++) {
  //   ringbuffer.write(geciciDizi1[i]);
  // }

  // for (int i = 0; i < 20; i++) {
  //   ringbuffer.write(bmsData.cellVoltages[i] / 100);
  // }
  // ringbuffer.write(data.arac_hizi);           // 20
  // ringbuffer.write(bmsData.sumVoltage / 10);  // 21
  // ringbuffer.write(soc);                      // 22
  // ringbuffer.write(bmsData.maxTemp);          // 23
  // ringbuffer.write(highByte(kalanEnerji));    // 23
  // ringbuffer.write(lowByte(kalanEnerji));     // 23

  for (int i = 0; i < 20; i++) {
    ringbuffer.write(37);
  }
  ringbuffer.write(80);              // 20
  ringbuffer.write(75);              // 21
  ringbuffer.write(99);              // 22
  ringbuffer.write(37);              // 23
  ringbuffer.write(highByte(2450));  // 23
  ringbuffer.write(counterr);        // 23 // lowByte(2450)
  counterr++;
  if (counterr == 254) counterr = 0;

  // *******



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


  kalanEnerji = ((56 * soc) / 1000) * bmsData.sumVoltage;
  // Serial.print("************ KALAN ENERJI ****************");
  // Serial.println(kalanEnerji);



  if (gecici) {  // LORA KODLARI
    anlikZaman = millis();

    if (ilkPaketHazirla() == 1) {
      e22ttl.sendFixedMessage(0, 196, 12, &data2, sizeof(Veriler2));
      delay(80);
      if (ilkPaketHazirla() == 1) {
        e22ttl.sendFixedMessage(0, 196, 12, &data2, sizeof(Veriler2));
        delay(80);
        if (ilkPaketHazirla() == 1) {
          e22ttl.sendFixedMessage(0, 196, 12, &data2, sizeof(Veriler2));
          delay(80);
        } else {
          e22ttl.sendFixedMessage(0, 196, 12, &data2, sizeof(Veriler2));
          delay(80);
        }

      } else {
        e22ttl.sendFixedMessage(0, 196, 12, &data2, sizeof(Veriler2));
        delay(80);
      }
    } else {
      e22ttl.sendFixedMessage(0, 196, 12, &data2, sizeof(Veriler2));
      delay(80);
    }
  
    // KONTROL PAKETI
    data2.dizi[78] = 155;
    // Serial.println("Kontrol Verisi gidiyor...");
    e22ttl.sendFixedMessage(0, 196, 12, &data2, sizeof(Veriler2));
    Serial.println("Veri gönderildi.");

    gecici = 0;
  }

  unsigned long eski = millis();
  unsigned long yeni = millis();

  Serial.print("başla: ");
  Serial.println(millis());
  ResponseStructContainer rsc;
  int baskaFlag = 0;

  while (yeni - eski < 1000) {
    yeni = millis();
    while (e22ttl.available() > 1) {
      rsc = e22ttl.receiveMessage(sizeof(Signal));
      struct Signal data = *(Signal*)rsc.data;
      Serial.println("Veri Geldi..");
      gecici = 1;
      rsc.close();
      baskaFlag = 1;
    }
    if (baskaFlag)
      break;
  }

  Serial.print("bitir: ");
  Serial.println(millis());

  Serial.println("Veri gelmedi.");

  if (millis() - anlikZaman > 5000) {
    Serial.println("Zaman asimi! Uyandırma paketi gidiyor...");
    kontrolSync();
    e22ttl.sendFixedMessage(0, 196, 12, &data2, sizeof(Veriler2));
    anlikZaman = millis();
  }

  loraConnected += 1;
}


void loraSync() {
  if (loraConnected < 5) {
    for (int i = 0; i < 79; i++) {
      int tempData = ringbuffer.read();
      if (tempData == -1) {
        tempData = 0;
      }
      data2.dizi[i] = tempData;
    }
  }
}

bool requestData(uint8_t requestId) {
  unsigned long startTime = millis();

  while (millis() - startTime < 10) {  // BURASI 3000 ms OLACAK
    Transmitted_Data.data[0] = requestId;
    mcp2515.sendMessage(&Transmitted_Data);

    delay(40);

    if (mcp2515.readMessage(&Received_Data) == MCP2515::ERROR_OK) {
      for (int i = 0; i < 8; i++) {
        // // Serial.println(Received_Data.data[i]);
      }
      if (Received_Data.data[0] == requestId + 100) {
        processReceivedData(requestId + 100);
        return true;
      }
    }
  }

  // Serial.println("Timeout waiting for response");
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
  // Serial.println("BMS Data:");
  // Serial.print("Temperatures: ");
  for (int i = 0; i < 4; i++) {
    // Serial.print(bmsData.temps[i]);
    // Serial.print(" ");
  }
  // Serial.println();

  // Serial.print("Max Temperature: ");
  // Serial.println(bmsData.maxTemp);
  // Serial.print("Sum Voltage: ");
  // Serial.println(bmsData.sumVoltage);
  // Serial.print("SOC: ");
  // Serial.println(soc);
  // Serial.print("Power: ");
  // Serial.println(bmsData.powerWatt);

  // Serial.println("Cell Voltages:");
  for (int i = 0; i < 20; i++) {
    // Serial.print(bmsData.cellVoltages[i]);
    // Serial.print(" ");
    if ((i + 1) % 5 == 0) {
      // Serial.println();
    }
  }
}





void receiveEvent(int bytesReceived) {
  // Serial.println("******************* OnReceive *************************");
  receivedSOC = Wire.read();

  uint8_t highByte = Wire.read();

  uint8_t lowByte = Wire.read();

  receivedCapacity = (highByte << 8) | lowByte;

  // Serial.print("Received SOC: ");
  // Serial.print(receivedSOC);
  // Serial.println(" %");

  // Serial.print("Received Capacity: ");
  // Serial.print(receivedCapacity);
  // Serial.println(" mAh");
  // Serial.println();
}



void updateValues() {
  gonderText("k5", String(bmsData.temps[0] - 40));
  gonderText("k6", String(bmsData.temps[0] - 40));
  gonderText("k7", String(bmsData.temps[0] - 40));
  gonderText("k8", String(bmsData.temps[0] - 40));

  gonderText("t11", String(soc) + " %");

  if (1) {  // DÜZENLENECEK
    gonderText("t9", "Şarj Ediliyor");
  } else {
    gonderText("t9", "Şarj Edilmiyor");
  }

  if (soc <= 20) {
    gonderText("t14", "Düşük Batarya Voltajı!");
  } else {
    gonderText("t14", " ");
  }

  if (bmsData.maxTemp >= 55) {
    gonderText("t12", "Yüksek Sıcaklık!");
  } else {
    gonderText("t14", " ");
  }


  gonderText("t12", String(bmsData.maxTemp) + " °C");

  gonderText("t8", String(kalanEnerji) + " Wh");

  gonderGosterge("z0", map(kalanEnerji, 800, 4444, 0, 180));

  gonderText("t7", String(data.arac_hizi));

  gonderGosterge("z1", map(data.arac_hizi, 0, 180, 0, 100));

  gonderText("t10", String(float(bmsData.sumVoltage) / 10.0, 2) + " V");

  for (int i = 0; i < 20; i++) {
    gonderText("k" + String(i + 9), String(float(bmsData.cellVoltages[i]) / 1000.0, 2) + " V");
    gonderGosterge("j" + String(i), float(bmsData.cellVoltages[i]) / 1000.0 * 25);
  }
  delay(15);
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

  if (baslikFLag) {
    veriDosyasi.println("Arac_Hizi;SOC;Toplam_Voltaj;Maks_Sicaklik;Kalan_Enerji;HucreVoltajlari");
    baslikFLag = 0;
  }

  veriDosyasi.print(data.arac_hizi);
  veriDosyasi.print(",");
  veriDosyasi.print(soc);
  veriDosyasi.print(",");
  veriDosyasi.print((float)(bmsData.sumVoltage) / 10);
  veriDosyasi.print(",");
  veriDosyasi.print(bmsData.maxTemp);
  veriDosyasi.print(",");
  veriDosyasi.print(kalanEnerji);

  for (int i = 0; i < 20; i++) {
    veriDosyasi.print(", ");
    veriDosyasi.print((float)(bmsData.cellVoltages[i]) / 1000);
  }
  veriDosyasi.println();
}

void veriKaydetSD() {


  if (veriDosyasi) {

    // // Serial.print("Araç Hızı: ");
    // // Serial.println(data.arac_hizi);
    // // Serial.print("Kalan Enerji: ");
    // // Serial.println(data.kalanEnerjiMiktari);
    // // Serial.print("Toplam Voltaj: ");
    // // Serial.println(data.toplamVoltaj);
    // // Serial.print("Max Sıcaklık: ");
    // // Serial.println(bmsData.maxTemp);

    if (data.kalanEnerjiMiktari == 0 && data.toplamVoltaj == 0 && bmsData.maxTemp == 0) {
      return;
    }

    dosyayaYazdir();

    veriDosyasi.flush();
    // // Serial.println("Veri SD karta yazıldı.");
  } else {
    // // Serial.println("SD karta yazma hatası!");
    if (SD.begin(chipSelect)) {
      yeniDosyaAc();
    } else {
      // // Serial.println("SD kart yeniden başlatılamadı!");
    }
  }
}


void kontrolSync() {
  for (int i = 0; i < 79; i++) {
    data2.dizi[i] = 124;
  }
  data2.dizi[78] = 155;
}

void sifirla() {
  for (int i = 0; i < 79; i++) {
    data2.dizi[i] = 0;
  }
}

int ilkPaketHazirla() {
  sifirla();

  for (int i = 0; i < 78; i++) {
    int tempData = ringbuffer.read();
    if (tempData != -1) {
      data2.dizi[i] = tempData;
    } else {
      return 0;
    }
  }
  return 1;
}
