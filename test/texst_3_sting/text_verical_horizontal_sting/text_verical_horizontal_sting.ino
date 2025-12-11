#include <Arduino.h>
#include "SavaOLED_ESP32.h"

// 1. ПОДКЛЮЧАЕМ ТВОЙ ВЕРТИКАЛЬНЫЙ ШРИФТ
// (Убедись, что в этом файле буквы лежат "на боку", 
// а первый байт массива - это их высота в пикселях)  SavaFont_ilya_Pro_8px
#include "Fonts/SavaFont_vert_Pro_8px.h" 
#include "Fonts/SavaFont_ilya_Pro_8px.h" 


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 5
#define OLED_SCL 4

// Создаем объект дисплея
SavaOLED_ESP32 oled(SCREEN_WIDTH, SCREEN_HEIGHT, I2C_NUM_0);

void setup() {
    Serial.begin(115200);
    
    // Инициализация (частота, SDA, SCL)
    oled.init(800000, OLED_SDA, OLED_SCL);
    
    // Очистка экрана перед стартом
    oled.clearBuffer();
    oled.setScrollSpeed(8);

    oled.display();
}

void loop() {

    uint32_t start = micros();

    oled.clearBuffer();

    // --- ПРИМЕР 1: Левое меню (Столбик 1) ---
    // Ставим курсор в левый верхний угол
    oled.setCursor(2, 2, StrUp,57); 
    
    // Подключаем вертикальный шрифт
    oled.setFont(SavaFont_ilya_Pro_8px); 
    
    // Режим наложения (чтобы не стирать фон, если он будет)
    oled.setDrawMode(ADD_UP);
    
    // Интервал между буквами по вертикали (например, 2 пикселя)
    oled.setCharSpacing(1); 
    
    // Добавляем текст в буфер
    oled.addPrint("MenuМеню"); 
    
    // РИСУЕМ ВЕРТИКАЛЬНО
    oled.drawPrintVert(); 


    // --- ПРИМЕР 2: Правый столбик (Статус) ---
    // Сдвигаемся вправо на 120 пикселей, отступ сверху 10
    oled.setCursor(119, 0, StrCenter); 
    oled.setFont(SavaFont_vert_Pro_8px); 
    oled.setDrawMode(INV_AUTO); // Перезапись
    oled.addPrint("1234!88");      // Выводим цифры
    
    oled.drawPrintVert();


    // --- ПРИМЕР 3: Графика (для проверки, что всё работает вместе) ---
    // Рисуем рамку вокруг текста слева (просто для красоты)
    // X=0, Y=0, Ширина=10 (примерно), Высота=64
    oled.rect(0, 0, 11, 64, ADD_UP, false);
    oled.rect(118, 0, 9, 64, INV_AUTO, true);

    oled.rect(13, 0, 103, 11,INV_AUTO,FILL);
    oled.setCursor(14, 2, StrScroll, 115);
    oled.setScroll(true);
    oled.setDrawMode(INV_AUTO);
    oled.setCharSpacing(1);
    oled.setFont(SavaFont_ilya_Pro_8px);
    oled.addPrint("ЦЫПЛЁНОК ТАФИКЮРА должен щенку аЩеНОК молчит");
    oled.drawPrint();

    // ФИНАЛЬНЫЙ ВЫВОД НА ЭКРАН
    oled.display();
    
    // Пауза, чтобы не молотить впустую (для статики)
    //delay(100);
    Serial.println(micros() - start);
}
