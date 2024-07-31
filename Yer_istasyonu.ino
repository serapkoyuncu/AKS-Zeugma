#include "LoRa_E22.h"
#include <SoftwareSerial.h>

SoftwareSerial mySerial(12, 11);  // Arduino RX <-- e22 TX, Arduino TX --> e22 RX
LoRa_E22 e22ttl(&mySerial);

struct Veriler {
  byte dizi[24];
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

void loop() {

  while (e22ttl.available() > 1) {
    ResponseStructContainer rsc = e22ttl.receiveMessage(sizeof(Veriler));
    struct Veriler receivedData = *(Veriler*)rsc.data;
    struct Veriler2 veriler2obj;

    for (int i = 0; i < 20; i++) {
      veriler2obj.hucreVoltajlari[i] = receivedData.dizi[i];
    }
    veriler2obj.arac_hizi = receivedData.dizi[20];
    veriler2obj.toplamVoltaj = receivedData.dizi[21];
    veriler2obj.kalanEnerjiMiktari = receivedData.dizi[22];
    veriler2obj.sicaklik = receivedData.dizi[23];

    bool validData = true;

    // Araç hızı kontrolü
    if (veriler2obj.arac_hizi < 0 || veriler2obj.arac_hizi > 130) {
      validData = false;
    }

    // Toplam voltaj kontrolü
    if (veriler2obj.toplamVoltaj < 0 || veriler2obj.toplamVoltaj > 90) {
      validData = false;
    }

    // Kalan enerji miktarı kontrolü
    if (veriler2obj.kalanEnerjiMiktari < 0 || veriler2obj.kalanEnerjiMiktari > 5000) {
      validData = false;
    }

    // Hücre voltajlarını kontrol et
    for (int i = 0; i < 20; i++) {
      if (veriler2obj.hucreVoltajlari[i] < 0 || veriler2obj.hucreVoltajlari[i] > 5) {
        validData = false;
        break;
      }
    }

    // Sıcaklık kontrolü (isteğe bağlı, örneğin -50 ile 150 derece arası)
    if (veriler2obj.sicaklik < 0 || veriler2obj.sicaklik > 150) {
      validData = false;
    }

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
    rsc.close();

    struct SendFeedback {
      byte temp[4];
    } data3;

    data3.temp[0] = 12;
    data3.temp[1] = 12;
    data3.temp[2] = 12;
    data3.temp[3] = 12;

    ResponseStatus rs = e22ttl.sendFixedMessage(0, 195, 12, &data3, sizeof(SendFeedback));
  }
}
