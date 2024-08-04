#include <EEPROM.h>

const double fullCapacity = 56000.0; // mAh cinsinden toplam batarya kapasitesi (56 Ah)
double soc = 100.0; // Başlangıçta %100 SOC
const unsigned long interval = 1000; // 1 saniyede bir ölçüm al
int potPin = A2; // Potansiyometre bağlı olduğu pin

unsigned long lastTime = 0;

void setup() {
  Serial.begin(9600);
  
  // EEPROM'dan başlangıç SOC'yi oku
  EEPROM.get(0, soc);
  
  // EEPROM'dan geçerli SOC'yi okunamıyorsa varsayılan olarak %100 yükle
  if (soc <= 0 || soc > 100) {
    soc = 100.0;
  }
  
  lastTime = millis(); // lastTime başlangıçta ayarla
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastTime >= interval) {
    // Geçen süreyi saat cinsine çevir
    double elapsedHours = (currentTime - lastTime) / 3600000.0;
    lastTime = currentTime; // lastTime'ı burada güncelle
    
    // Potansiyometreden akım değerini oku (0-1023 arasında bir değer)
    int potValue = analogRead(potPin);
    // Akım değeri (mA) hesapla (Örnek: 0-1023 arasında değeri -5000 mA ile 5000 mA aralığına ölçekleyelim)
    double current_mA = (potValue - 512) * 5000.0 / 512.0;
    
    // Akımı zamanla entegre et (Q = I * t)
    double deltaCapacity = current_mA * elapsedHours;
    
    // Bataryanın kapasitesini güncelle
    double currentCapacity = (soc / 100.0) * fullCapacity;
    currentCapacity += deltaCapacity; // Şarj edilme ve deşarj edilme durumları
    
    // Kapasitenin sınırlarını kontrol et
    if (currentCapacity > fullCapacity) {
      currentCapacity = fullCapacity;
    } else if (currentCapacity < 0) {
      currentCapacity = 0;
    }
    
    // Yeni SOC'yi hesapla
    soc = (currentCapacity / fullCapacity) * 100.0;
    
    // Verileri ekrana yazdır
    Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
    Serial.print("Remaining Capacity: "); Serial.print(currentCapacity); Serial.println(" mAh");
    Serial.print("State of Charge (SOC): "); Serial.print(soc); Serial.println(" %");
    Serial.println("");
    
    // SOC'yi EEPROM'a kaydet
    EEPROM.put(0, soc);
  }
}
