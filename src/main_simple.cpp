/*
 * ESP8266 NodeMCU ile P10 32x16 LED Panel Kontrolü - Sabit Yazı Versiyonu
 * 
 * Pin Bağlantı Şeması:
 * ┌─────────────────────────────────────────────────────────┐
 * │ P10 Panel Pin │ NodeMCU Pin │ GPIO  │ Açıklama          │
 * ├───────────────┼─────────────┼───────┼───────────────────┤
 * │ A             │ D0          │ GPIO16│ Row Address A     │
 * │ B             │ D1          │ GPIO5 │ Row Address B     │
 * │ OE            │ D2          │ GPIO4 │ Output Enable     │
 * │ CLK           │ D5          │ GPIO14│ Clock             │
 * │ STB           │ D6          │ GPIO12│ Strobe/Latch      │
 * │ R             │ D7          │ GPIO13│ Red Data          │
 * │ GND           │ GND         │ GND   │ Ground            │
 * │ VCC           │ 5V          │ 5V    │ Power Supply      │
 * └─────────────────────────────────────────────────────────┘
 * 
 * Bu versiyon Modbus haberleşmesi kullanmadan sabit yazı gösterir.
 */

#include <Arduino.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>
#include <fonts/Arial_Black_16.h>

// P10 Panel Pin Tanımlamaları
#define DMD_PIN_A      16  // D0 - Row Address A
#define DMD_PIN_B      5   // D1 - Row Address B  
#define DMD_PIN_SCLK   14  // D5 - Clock
#define DMD_PIN_SDATA  13  // D7 - Red Data
#define DMD_PIN_nOE    4   // D2 - Output Enable (aktif düşük)
#define DMD_PIN_STB    12  // D6 - Strobe/Latch

// Panel Boyutları
#define DISPLAYS_WIDE 2   // Yatayda kaç panel (32x16 için 2 panel = 64 piksel genişlik)
#define DISPLAYS_HIGH 1   // Dikeyde kaç panel (16 piksel yükseklik)

// DMD2 Objesi
SPIDMD dmd(DISPLAYS_WIDE, DISPLAYS_HIGH, DMD_PIN_nOE, DMD_PIN_A, DMD_PIN_B, DMD_PIN_STB);

// Global değişkenler
String currentText = "MERHABA DUNYA!";
String scrollText = "*** PlatformIO ESP8266 P10 LED Panel Projesi *** ";
bool isScrolling = false;
unsigned long lastUpdate = 0;
unsigned long textChangeTimer = 0;
int scrollPosition = 0;
int textMode = 0;  // 0: Sabit yazı, 1: Kayan yazı, 2: Saat

// Sabit yazılar listesi
String staticTexts[] = {
    "MERHABA",
    "DUNYA!",
    "ESP8266",
    "P10 LED",
    "PANEL",
    "PROJESI"
};
int staticTextCount = 6;
int currentStaticIndex = 0;

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("P10 LED Panel - Sabit Yazı Versiyonu Başlatılıyor...");
    
    // DMD2 başlatma
    dmd.setBrightness(50);  // Parlaklık ayarı (0-255)
    dmd.selectFont(SystemFont5x7);  // Varsayılan font
    dmd.begin();
    
    // Başlangıç ekranı
    dmd.clearScreen();
    dmd.drawString(0, 0, "BASLIYOR...");
    delay(2000);
    
    Serial.println("P10 LED Panel hazır!");
    Serial.println("Gösterilecek sabit yazılar:");
    for(int i = 0; i < staticTextCount; i++) {
        Serial.println("- " + staticTexts[i]);
    }
}

void loop() {
    unsigned long currentTime = millis();
    
    // Her 3 saniyede bir yazıyı değiştir
    if(currentTime - textChangeTimer >= 3000) {
        textChangeTimer = currentTime;
        
        switch(textMode) {
            case 0: // Sabit yazı modu
                showStaticText();
                break;
                
            case 1: // Kayan yazı modu
                if(!isScrolling) {
                    startScrolling();
                }
                break;
                
            case 2: // Saat modu
                showTime();
                break;
        }
        
        // 5 cycle sonra modu değiştir
        static int cycleCount = 0;
        cycleCount++;
        if(cycleCount >= 5) {
            cycleCount = 0;
            textMode = (textMode + 1) % 3;
            isScrolling = false;
            scrollPosition = 0;
        }
    }
    
    // Kayan yazı güncelleme
    if(isScrolling && (currentTime - lastUpdate >= 100)) {
        lastUpdate = currentTime;
        updateScrolling();
    }
    
    // Panel güncelleme
    dmd.scanDisplayBySPI();
    
    // Serial monitor için bilgi
    if(currentTime % 5000 < 50) {  // Her 5 saniyede bir
        Serial.println("Aktif mod: " + String(textMode) + 
                      " | Yazı: " + getCurrentDisplayText());
    }
    
    delay(10);
}

void showStaticText() {
    dmd.clearScreen();
    
    String text = staticTexts[currentStaticIndex];
    
    // Yazıyı ortalamak için pozisyon hesapla
    int textWidth = text.length() * 6;  // Yaklaşık karakter genişliği
    int x = (dmd.width() - textWidth) / 2;
    int y = (dmd.height() - 7) / 2;  // Font yüksekliği 7
    
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    
    dmd.drawString(x, y, text);
    
    // Sıradaki yazıya geç
    currentStaticIndex = (currentStaticIndex + 1) % staticTextCount;
    
    Serial.println("Sabit yazı gösteriliyor: " + text);
}

void startScrolling() {
    isScrolling = true;
    scrollPosition = dmd.width();
    dmd.clearScreen();
    Serial.println("Kayan yazı başlatıldı: " + scrollText);
}

void updateScrolling() {
    dmd.clearScreen();
    
    // Yazıyı kayan pozisyonda göster
    dmd.drawString(scrollPosition, 4, scrollText);
    
    // Pozisyonu güncelle
    scrollPosition -= 2;
    
    // Yazı tamamen kaybolduğunda döngüyü başlat
    int textWidth = scrollText.length() * 6;
    if(scrollPosition < -textWidth) {
        scrollPosition = dmd.width();
    }
}

void showTime() {
    dmd.clearScreen();
    
    // Basit bir saat simülasyonu (gerçek RTC olmadan)
    unsigned long seconds = millis() / 1000;
    int hours = (seconds / 3600) % 24;
    int minutes = (seconds / 60) % 60;
    int secs = seconds % 60;
    
    String timeStr = String(hours) + ":" + 
                    (minutes < 10 ? "0" : "") + String(minutes) + ":" +
                    (secs < 10 ? "0" : "") + String(secs);
    
    // Saati ortalayarak göster
    int x = (dmd.width() - (timeStr.length() * 6)) / 2;
    dmd.drawString(x, 4, timeStr);
    
    Serial.println("Saat gösteriliyor: " + timeStr);
}

String getCurrentDisplayText() {
    switch(textMode) {
        case 0:
            return staticTexts[currentStaticIndex];
        case 1:
            return scrollText;
        case 2:
            return "SAAT";
        default:
            return "BILINMEYEN";
    }
}

/*
 * KULLANIM TALİMATLARI:
 * 
 * 1. Bu kod sabit yazıları döngüsel olarak gösterir
 * 2. 3 farklı mod vardır:
 *    - Sabit yazı modu: Önceden tanımlı yazıları sırayla gösterir
 *    - Kayan yazı modu: Uzun bir yazıyı kayar şekilde gösterir
 *    - Saat modu: Basit bir saat gösterir
 * 
 * 3. Yazıları değiştirmek için:
 *    - staticTexts[] dizisini düzenleyin
 *    - scrollText değişkenini değiştirin
 * 
 * 4. Zamanlamaları ayarlamak için:
 *    - textChangeTimer kontrolündeki 3000 değerini değiştirin (ms)
 *    - scrolling update süresini 100ms'den farklı yapmak için değiştirin
 * 
 * 5. Panel boyutlarını ayarlamak için:
 *    - DISPLAYS_WIDE ve DISPLAYS_HIGH değerlerini değiştirin
 * 
 * 6. Parlaklığı ayarlamak için:
 *    - dmd.setBrightness() değerini 0-255 arasında değiştirin
 */
