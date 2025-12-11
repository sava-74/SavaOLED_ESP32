#include "SavaOLED_ESP32.h"
///#include "Fonts\SavaFont_MyFont_Proportional_16px.h"
#include "Fonts\SavaFont_ilya_Pro_8px.h"
#include "Fonts\SavaFont_cuper_Pro_16px.h"
// Старые шрифты временно ОТКЛЮЧАЕМ!
// #include "Fonts/SavaFont_Arialdb_Latin_33x36.h"  SavaFont_Proportional_Num_8px SavaFont_MyFont_Num_32px


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 5
#define OLED_SCL 4

SavaOLED_ESP32 oled(SCREEN_WIDTH, SCREEN_HEIGHT, I2C_NUM_0);

uint32_t tim = 0;
bool flag = false;

void setup() {
    Serial.begin(115200);
    oled.init(800000, OLED_SDA, OLED_SCL);
    oled.setScrollSpeed(4);
    oled.setRotation(false);
}

void loop() {
    if (millis() >= tim + 15000) {
        flag = !flag;
        tim = millis();
    }

    oled.clearBuffer();

    oled.rect(0, 0, 127, 20, INV_AUTO, FILL);

     //Тест 1: Скроллинг с русским и английским
    oled.setCursor(10, 2, StrScroll,117);
    oled.setScroll(flag); // Включаем скролл
    oled.setDrawMode(INV_AUTO);
    oled.setCharSpacing(1); // Интервал 2 пикселя
    oled.setFont(SavaFont_ilya_Pro_8px);
    
    // Внимание: Текст должен быть в UTF-8 (обычно это дефолт в Arduino IDE)
    oled.addPrint("ЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮйцукенгшщзхъфывапролджэячсмитьбю.!нгш"); 
    oled.drawPrint();

    // Тест 2: Статичный текст по центру
    oled.setCursor(0, 30, StrScroll); // Координата X=64 (центр экрана 128/2)
    oled.setScroll(!flag);
    oled.setCharSpacing(1);
    oled.setDrawMode(ADD_UP);
    oled.addPrint("Привет мир! Это Я Мальчик Илья");
    oled.drawPrint();

    // Тест 3: Плотный текст (интервал 1)
    oled.setCursor(0, 50, StrCenter);
   // oled.setScroll(false);
    oled.setCharSpacing(1);
    oled.setDrawMode(INV_AUTO);
    oled.setFont(SavaFont_cuper_Pro_16px);
    oled.addPrint(":-");
    oled.addPrint(millis(),8);
    oled.drawPrint();

    // ЕДИНСТВЕННЫЙ display() в конце!
    oled.display();
}
