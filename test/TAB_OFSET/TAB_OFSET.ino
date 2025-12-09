#include <Arduino.h>
#include "SavaOLED_ESP32.h"
#include "Fonts\SavaFont_Proportional_Num_8px.h" 

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    delay(1000); 

    Serial.println("\n\n=== ГЕНЕРАТОР ТАБЛИЦЫ СМЕЩЕНИЙ (FIXED 161 chars) ===\n");
    Serial.print("const uint16_t SavaFont_Proportional_Num_8px_Offsets[] = {");

    uint32_t currentOffset = 0;
    
    // !!! БЫЛО 160, СТАЛО 161 (чтобы захватить 'ё') !!!
    const int totalChars = 13; // сколько элментов массива

    for (int i = 0; i < totalChars; i++) {
        if (i % 12 == 0) {
            Serial.println();
            Serial.print("    ");
        }
        Serial.print(currentOffset);
        if (i < totalChars - 1) {
            Serial.print(", ");
        }
        
        // Защита от выхода за пределы массива данных при чтении ширины
        // (на случай если Data короче чем мы думаем)
        if (currentOffset < sizeof(SavaFont_Proportional_Num_8px_Data)) {
             uint8_t charWidth = SavaFont_Proportional_Num_8px_Data[currentOffset];
             currentOffset += (charWidth + 1);
        }
    }

    Serial.println("\n};");
}

void loop() {}