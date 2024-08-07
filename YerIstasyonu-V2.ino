// Ring Buffer ile yeni sistem için yazdığım kod.

#include "LoRa_E22.h"
#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11);

/*
   Pinler     Arduino Nano    Lora E32 433T20d
                  11                3
                  10                4
*/

LoRa_E22 e22ttl(&mySerial);

struct Signal {
  int dizi[79];
  byte temp[4];
} data;

void setup() {
  Serial.begin(9600);
  e22ttl.begin();
  delay(500);
}

void loop() {
  while (e22ttl.available() > 1) {
    // Gelen mesaj okunuyor
    ResponseStructContainer rsc = e22ttl.receiveMessage(sizeof(Signal));
    struct Signal data = *(Signal*)rsc.data;

    Serial.print("Gelen Mesaj: ");
    for (int i = 0; i < 78; i++) {
      Serial.print(data.dizi[i]);
      Serial.print(" ");
    }
    Serial.println();

    rsc.close();

    // Gönderilecek paket veri hazırlanıyor
    struct Signal {
      byte temp[4];
    } data2;

    // Örnek veri doldurma
    data2.temp[0] = 12;
    data2.temp[1] = 34;
    data2.temp[2] = 56;
    data2.temp[3] = 78;

    if (data.dizi[78] == 155) {
      delay(300);
      ResponseStatus rs = e22ttl.sendFixedMessage(0, 195, 12, &data2, sizeof(Signal));
      Serial.println("Feedback Gönderildi.");
    }
  }
}
