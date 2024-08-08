/*
Kontrol verileri eklendi.

*/

#include "LoRa_E22.h"
#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11);  // Arduino RX <-- e22 TX, Arduino TX --> e22 RX
LoRa_E22 e22ttl(&mySerial);

int ucluFlag = 0;
int sayac = 0;

struct Veriler {
  byte dizi[79];
  byte temp[4];
} data;

struct Veriler2 {
  int arac_hizi;
  float hucreVoltajlari[20];
  float toplamVoltaj;
  int kalanEnerjiMiktari;
  float sicaklik;
  byte tahminiSarjSuresi;
  float temp[4];
} data2;

void setup() {
  Serial.begin(9600);
  e22ttl.begin();
  delay(500);
}

ResponseStructContainer rsc;

struct Veriler receivedData;
struct Veriler2 veriler2obj;

void loop() {
  while (e22ttl.available() > 1) {
    rsc = e22ttl.receiveMessage(sizeof(Veriler));
    receivedData = *(Veriler*)rsc.data;  // `data` yerine `receivedData` kullanıldı.

    // Serial.print("Gelen Mesaj: ");
    // for (int i = 0; i < 79; i++) {
    //   Serial.print(receivedData.dizi[i]);
    //   Serial.print(" ");
    // }
    Serial.println();
    rsc.close();

    sayac++;
    if (receivedData.dizi[27] == 0) {
      // PAKET BU KADARDI
      ucluFlag = 1;
    } else {
      // IKINCI PAKETE GEC
      ucluFlag = 2;
      if (receivedData.dizi[54] != 0) {
        ucluFlag = 3;
      }
    }

    if (ucluFlag == 1) {
      fonkisyon(0);
      yazdir();
    }
    delay(25);

    if (ucluFlag == 2) {
      fonkisyon(26);
      yazdir();
    }
    delay(25);

    if (ucluFlag == 3) {
      fonkisyon(52);
      yazdir();
    }
    delay(25);

    struct SendFeedback {
      byte temp[4];
    } data3;

    data3.temp[0] = 12;
    data3.temp[1] = 12;
    data3.temp[2] = 12;
    data3.temp[3] = 78;

    if (receivedData.dizi[78] == 155) {
      delay(340);
      ResponseStatus rs = e22ttl.sendFixedMessage(0, 195, 12, &data3, sizeof(SendFeedback));
      // Serial.println("Feedback Gönderildi.");
    }
  }
}

void fonkisyon(int param) {
  for (int i = 0; i < 20; i++) {
    veriler2obj.hucreVoltajlari[i] = float(receivedData.dizi[param + i]) / 10.0;
  }
  veriler2obj.arac_hizi = receivedData.dizi[param + 20];
  veriler2obj.toplamVoltaj = receivedData.dizi[param + 21];
  veriler2obj.kalanEnerjiMiktari = receivedData.dizi[param + 22];  // IPTAL
  veriler2obj.sicaklik = receivedData.dizi[param + 23];
  veriler2obj.kalanEnerjiMiktari = (receivedData.dizi[param + 24] << 8) | receivedData.dizi[param + 25];
}




void yazdir() {

  bool validData = true;


  if (veriler2obj.arac_hizi > 180) validData = false;
  if (veriler2obj.toplamVoltaj < 1.0 || veriler2obj.toplamVoltaj > 100.0) validData = false;
  if (veriler2obj.kalanEnerjiMiktari < 100 || veriler2obj.kalanEnerjiMiktari > 5000) validData = false;
  for (int i = 0; i < 20; i++) {
    if (veriler2obj.hucreVoltajlari[i] < 1.0 || veriler2obj.hucreVoltajlari[i] > 5.0) {
      validData = false;
      break;
    }
  }
  if (veriler2obj.sicaklik < 1 || veriler2obj.sicaklik > 220) validData = false;

  if (validData) {

    char sicaklikBuffer[10];
    dtostrf(veriler2obj.sicaklik, 4, 2, sicaklikBuffer);  // 4 basamaklı ve 2 ondalık hane

    // Virgül yerine nokta kullanarak veriyi gönder
    String sicaklikStr = String(sicaklikBuffer);
    sicaklikStr.replace(',', '.');

    Serial.print(veriler2obj.arac_hizi);
    Serial.print(",");
    Serial.print(sicaklikStr);

    // 20 voltaj değerini gönder
    for (int i = 0; i < 20; i++) {
      char voltajBuffer[10];
      dtostrf(veriler2obj.hucreVoltajlari[i], 4, 2, voltajBuffer);  // 4 basamaklı ve 2 ondalık hane
      String voltajStr = String(voltajBuffer);
      voltajStr.replace(',', '.');
      Serial.print(",");
      Serial.print(voltajStr);
    }

    char toplamVoltajBuffer[10];
    dtostrf(veriler2obj.toplamVoltaj, 4, 2, toplamVoltajBuffer);  // 4 basamaklı ve 2 ondalık hane
    String toplamVoltajStr = String(toplamVoltajBuffer);
    toplamVoltajStr.replace(',', '.');
    Serial.print(",");
    Serial.print(toplamVoltajStr);
    Serial.print(",");
    Serial.print(veriler2obj.kalanEnerjiMiktari);
    Serial.println();  // Satırı sonlandır
  }
}
