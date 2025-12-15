#include "SavaOLED_ESP32.h"

// 1. ПОДКЛЮЧАЕМ ВАШИ ШРИФТЫ

//#include "Fonts/SavaFont_MyFont_Latin_17x25.h"
#include "Fonts/SavaFont_90grad_Pro_15px.h"
//#include "Fonts/SavaFont_segments_Numbers_10x14.h"
//SavaFont_Tahoma_Latin_29x32.h
//SavaFont_Arialdb_Latin_33x36.h


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 5
#define OLED_SCL 4

// Объект дисплея
SavaOLED_ESP32 oled(SCREEN_WIDTH, SCREEN_HEIGHT, I2C_NUM_0);

uint32_t StartFps = 0;
uint32_t tim = 0;
bool flag = false;

void setup() {
    oled.init(800000, OLED_SDA, OLED_SCL);
    oled.setScrollSpeed(5);


}

void loop() {

  if(millis()>=tim + 15000){
    flag = !flag;
    tim = millis();
  }


  //if(millis()>=StartFps+50){
  //  StartFps = millis();

  oled.clearBuffer();

    oled.clearBuffer();
    
    oled.rect(0, 0, 128, 16,INV_AUTO,!flag);
    oled.setCursor(2, 1, StrScroll, 125);
    oled.setScroll(!flag);
    oled.setDrawMode(INV_AUTO);
    oled.setCharSpacing(1);
    oled.setFont(SavaFont_90grad_Pro_15px);
    oled.addPrint("ЦЫПЛЁНОК ТАФИКЮРА должен щенку аЩеНОК молчит");
    oled.drawPrint();


    oled.setCursor(0, 17, StrScroll);
    oled.setScroll(flag);
    oled.setDrawMode(INV_AUTO);
    oled.setCharSpacing(1);
    oled.setFont(SavaFont_90grad_Pro_15px);
    oled.addPrint(millis());
    oled.drawPrint();



  oled.display();
  //}


}
