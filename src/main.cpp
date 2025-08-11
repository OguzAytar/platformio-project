/*
 * ESP8266 NodeMCU ile P10 32x16 LED Panel Kontrolü + Modbus RTU
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
 * Modbus RTU Bağlantısı:
 * ┌─────────────────────────────────────────────────────────┐
 * │ Modbus RTU    │ NodeMCU Pin │ GPIO  │ Açıklama          │
 * ├───────────────┼─────────────┼───────┼───────────────────┤
 * │ TX            │ D3          │ GPIO0 │ Modbus TX         │
 * │ RX            │ D4          │ GPIO2 │ Modbus RX         │
 * │ DE/RE         │ D8          │ GPIO15│ RS485 Direction   │
 * └─────────────────────────────────────────────────────────┘
 * 
 * Modbus Registers:
 * - Holding Register 0: Display Mode (0=Off, 1=Welcome Text, 2=Price Display, 3=Time Display)
 * - Holding Register 1: Scroll Speed (50-500ms) - for welcome text
 * - Holding Register 2: Price Value (for mode 2) - will show as "XXXX TL"
 * - Holding Register 3: Time Value (for mode 3) - will show as "XXXX sn"
 */

#include <Arduino.h>
#include <SPI.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>
#include <SoftwareSerial.h>
#include <ModbusRTU.h>

// Panel boyutları (1 panel = 32x16 piksel)
SPIDMD dmd(1, 1);  // genişlik, yükseklik (panel sayısı)

// Modbus RTU yapılandırması
#define MODBUS_SLAVE_ID 1
#define RS485_TX_PIN 0   // D3
#define RS485_RX_PIN 2   // D4  
#define RS485_DE_PIN 15  // D8

SoftwareSerial modbusSerial(RS485_RX_PIN, RS485_TX_PIN);
ModbusRTU mb;

// Display değişkenleri
String welcomeText = "Welcome";
int displayMode = 1;  // 0=Off, 1=Welcome Text, 2=Price Display, 3=Time Display
int scrollSpeed = 100;
int scrollX = 32;
unsigned long lastScrollTime = 0;

// Price display değişkenleri
int16_t priceValue = 0;

// Time display değişkenleri
int16_t timeValue = 0;

// Sabit pozisyon değerleri
const int TEXT_POS_X = 2;
const int TEXT_POS_Y = 4;

// Modbus register'larından display parametrelerini güncelle
void updateDisplayFromModbus() {
    // Display mode (Register 0)
    displayMode = mb.Hreg(0);
    
    // Scroll speed (Register 1)
    if (mb.Hreg(1) >= 50 && mb.Hreg(1) <= 500) {
        scrollSpeed = mb.Hreg(1);
    }
    
    // Price değeri (Register 2)
    priceValue = (int16_t)mb.Hreg(2);
    
    // Time değeri (Register 3)
    timeValue = (int16_t)mb.Hreg(3);
    
    // Welcome modunda scroll pozisyonunu sıfırla
    if (displayMode == 1) {
        scrollX = 32;
    }
}

// Modbus callback fonksiyonu
bool modbusCallback(Modbus::ResultCode event, uint16_t transactionId, void* data) {
    if (event == Modbus::EX_SUCCESS) {
        // Register değerleri güncellendi
        updateDisplayFromModbus();
    }
    return true;
}

void setup() {
    Serial.begin(115200);
    Serial.println("P10 LED Panel + Modbus RTU Test Başladı");
    
    // DMD2'yi başlat
    dmd.begin();
    dmd.selectFont(SystemFont5x7);
    dmd.clearScreen();
    
    // Modbus RTU setup
    modbusSerial.begin(9600);
    mb.begin(&modbusSerial, RS485_DE_PIN);
    mb.slave(MODBUS_SLAVE_ID);
    
    // Holding register'ları ekle (0-3)
    for (int i = 0; i < 4; i++) {
        mb.addHreg(i);
    }
    
    // Başlangıç değerleri
    mb.Hreg(0, 1);        // Welcome mode
    mb.Hreg(1, 100);      // 100ms scroll speed
    mb.Hreg(2, 1500);     // 1500 TL örnek fiyat
    mb.Hreg(3, 60);       // 60 sn örnek zaman
    
    updateDisplayFromModbus();
    
    Serial.println("Panel hazır, Modbus RTU Slave ID: " + String(MODBUS_SLAVE_ID));
    Serial.println("Baud Rate: 9600, Parity: None, Stop Bits: 1");
}

void loop() {
    // Modbus iletişimini işle
    mb.task();
    
    // Register'ları güncelle
    displayMode = mb.Hreg(0);
    scrollSpeed = mb.Hreg(1);
    priceValue = (int16_t)mb.Hreg(2);
    timeValue = (int16_t)mb.Hreg(3);
    
    // Display mode'a göre işlem yap
    switch (displayMode) {
        case 0: // Off
            dmd.clearScreen();
            break;
            
        case 1: // Welcome Text (scrolling)
            if (millis() - lastScrollTime >= (unsigned long)scrollSpeed) {
                dmd.clearScreen();
                dmd.drawString(scrollX, TEXT_POS_Y, welcomeText.c_str());
                scrollX--;
                if (scrollX < -((int)welcomeText.length() * 6)) {  // Metin tamamen soldan çıktığında
                    scrollX = 32;  // Sağdan başlat
                }
                lastScrollTime = millis();
            }
            break;
            
        case 2: // Price Display
            {
                String priceStr = String(priceValue) + " TL";
                dmd.clearScreen();
                dmd.drawString(TEXT_POS_X, TEXT_POS_Y, priceStr.c_str());
            }
            break;
            
        case 3: // Time Display
            {
                String timeStr = String(timeValue) + " sn";
                dmd.clearScreen();
                dmd.drawString(TEXT_POS_X, TEXT_POS_Y, timeStr.c_str());
            }
            break;
            
        default:
            // Geçersiz mode, hata göster
            dmd.clearScreen();
            dmd.drawString(2, 4, "MODE ERROR");
            delay(1000);
            break;
    }
    
    delay(10); // CPU yükünü azalt
}
