#include <FastLED.h>
#define NUM_LEDS1 35
#define led_pini 12
#define Buton A2  // Butonun bağlı olduğu pin A2 olarak değiştirildi

CRGB leds1[NUM_LEDS1];

void setup() {
  FastLED.addLeds<WS2812B, led_pini, GRB>(leds1, NUM_LEDS1);
  pinMode(Buton, INPUT_PULLUP);  // A2 pinini buton girişi olarak ayarla
}

void loop() {
  if (digitalRead(Buton) == LOW) {  // Butona basıldığında
    for (int i = 0; i < NUM_LEDS1; i++) {
      leds1[i] = CRGB::Red;  // Tüm LED'leri kırmızı yap
    }
  } else {  // Buton bırakıldığında
    for (int i = 0; i < NUM_LEDS1; i++) {
      leds1[i] = CRGB::Black;  // Tüm LED'leri kapat
    }
  }
  FastLED.show();  // LED'lerin durumunu güncelle
}
