#include <Arduino.h>
#include "SavaOLED_ESP32.h"

// 1. ПОДКЛЮЧАЕМ ТВОЙ ВЕРТИКАЛЬНЫЙ ШРИФТ
// (Убедись, что в этом файле буквы лежат "на боку", 
// а первый байт массива - это их высота в пикселях)  SavaFont_ilya_Pro_8px
#include "Fonts/SavaFont_vert_Pro_8px.h" 
#include "Fonts/SavaFont_ilya_Pro_8px.h" 
#include "Fonts/SavaFont_icon_Num_Pro_8px.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 5
#define OLED_SCL 4

uint8_t counter = 0;
String simvol;
uint32_t oldTime = 0;

// Создаем объект дисплея
SavaOLED_ESP32 oled(SCREEN_WIDTH, SCREEN_HEIGHT, I2C_NUM_0);

void setup() {
    Serial.begin(115200);
    
    // Инициализация (частота, SDA, SCL)
    oled.init(800000, OLED_SDA, OLED_SCL);
    
    // Очистка экрана перед стартом
    oled.clear();
    oled.ScrollSpeed(8);
    oled.ScrollSpeedVert(5);

    oled.display();
}

void loop() {

    uint32_t start = micros();

    oled.clear();

    // --- ПРИМЕР 1: Левое меню (Столбик 1) ---
    // Ставим курсор в левый верхний угол
    oled.Cursor(2, 10, StrScroll); 
    oled.Scroll(true);
    oled.Font(SavaFont_ilya_Pro_8px); 
    oled.DrawMode(ADD_UP);
    oled.CharSpacing(1); 
    oled.print("MenuМеню"); 
    //oled.Font(SavaFont_vert_Pro_8px);
    oled.print(8856);
    oled.drawPrintVert(); 


    // --- ПРИМЕР 2: Правый столбик (Статус) ---
    // Сдвигаемся вправо на 120 пикселей, отступ сверху 10
    oled.Cursor(119, 0, StrCenter); 
    oled.Font(SavaFont_vert_Pro_8px); 
    oled.DrawMode(INV_AUTO); // Перезапись
    oled.print(8888);      // Выводим цифры
    
    oled.drawPrintVert();


    // --- ПРИМЕР 3: Графика (для проверки, что всё работает вместе) ---
    // Рисуем рамку вокруг текста слева (просто для красоты)
    // X=0, Y=0, Ширина=10 (примерно), Высота=64
    oled.rect(0, 0, 11, 64, ADD_UP, false);
    oled.rect(118, 0, 9, 64, INV_AUTO, true);

    oled.rect(13, 0, 92, 11,INV_AUTO,FILL);

    oled.Cursor(14, 2, StrScroll, 90);
    oled.Scroll(true);
    oled.DrawMode(INV_AUTO);
    oled.CharSpacing(1);
    oled.Font(SavaFont_ilya_Pro_8px);
    oled.print("ЦЫПЛЁНОК ТАФИКЮРА должен щенку аЩеНОК молчит");
    oled.drawPrint();

    if(millis()>oldTime+1000){
        counter++;
        oldTime = millis();
        if(counter>=4)counter = 0;
    }

    switch (counter) {
        case 0: simvol = "8"; break;
        case 1: simvol = "9"; break;
        case 2: simvol = ":"; break;
        case 3: simvol = "-"; break;
    }

    oled.Cursor(12, 12, StrCenter); 
    oled.Font(SavaFont_icon_Num_Pro_8px); 
    oled.DrawMode(INV_AUTO); // Перезапись
    oled.print(simvol);      // Выводим цифры
    oled.drawPrint();

    // ФИНАЛЬНЫЙ ВЫВОД НА ЭКРАН
    oled.display();
    
    // Пауза, чтобы не молотить впустую (для статики)
    //delay(100);
    Serial.println(micros() - start);
}
