#include "SavaOLED_ESP32.h"
///#include "Fonts\SavaFont_MyFont_Proportional_16px.h"
//#include "Fonts\SavaFont_ilya_Pro_8px.h"
#include "Fonts\SavaFont_test_ttf_Pro_16px.h"
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
    oled.scrollSpeed(4);
    oled.rotation(false);
}

void loop() {
   // if (millis() >= tim + 15000) {
   //     flag = !flag;
   //     tim = millis();
   // }

    oled.clear();

    oled.rect(0, 0, 127, 25, INV_AUTO, FILL);

     //Тест 1: Скроллинг с русским и английским
    oled.cursor(10, 2, StrScroll,117);
    oled.scroll(true); // Включаем скролл
    oled.drawMode(INV_AUTO);
    oled.charSpacing(1); // Интервал 2 пикселя
    oled.font(SavaFont_test_ttf_Pro_16px);
    
    oled.print("ЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮйцукенгшщзхъфывапролджэячсмитьбю.!нгш"); 
    oled.drawPrint();


    oled.display();
}
