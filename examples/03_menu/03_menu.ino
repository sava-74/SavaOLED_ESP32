/*
 * Пример 03_menu - Демонстрация меню с кнопками и вертикальной прокруткой
 *
 * Демонстрирует:
 * - Вертикальный скроллинг текста на левой стороне экрана
 * - Навигационное меню с закрашенными прямоугольниками
 * - Обработку нажатий кнопок (вверх, вниз, OK)
 * - Горизонтальный скроллинг текста выбранного пункта меню
 * - Мигание выделения при нажатии OK
 *
 * Подключение OLED дисплея:
 * SDA -> GPIO 5
 * SCL -> GPIO 4
 * VCC -> 3.3V
 * GND -> GND
 *
 * Подключение кнопок:
 * Кнопка ВВЕРХ -> GPIO 12 (pull-up)
 * Кнопка OK    -> GPIO 13 (pull-up)
 * Кнопка ВНИЗ  -> GPIO 15 (pull-up)
 *
 * Используемые шрифты:
 * - SF_Font_P8.h (8px, пропорциональный, для меню)
 * - SF_Vertical_P8.h (8px, вертикальный, для скроллинга)
 */

#include "SavaOLED_ESP32.h"                                     // Подключение библиотеки SavaOLED_ESP32 
#include "Fonts/SF_Font_P8.h"                                   // Подключение шрифта с поддержкой кириллицы CP1251
#include "Fonts/SF_Vertical_P8.h"                               // Подключение вертикального шрифта с поддержкой кириллицы CP1251

// Настройки дисплея
#define SCREEN_WIDTH 128                                        // Ширина экрана в пикселях 
#define SCREEN_HEIGHT 64                                        // Высота экрана в пикселях
#define OLED_SDA 5                                              // Пин SDA   
#define OLED_SCL 4                                              // Пин SCL

// Пины кнопок
#define BTN_UP 12                                               // Кнопка ВВЕРХ
#define BTN_OK 13                                               // Кнопка OK  
#define BTN_DOWN 15                                             // Кнопка ВНИЗ 

// Параметры меню
#define MENU_ITEMS_COUNT 5                                      // Количество пунктов меню 
#define MENU_ITEM_HEIGHT 12                                     // Высота одного пункта меню в пикселях
#define MENU_BORDER_RADIUS 2                                    // Радиус скругления углов рамки выбранного пункта
#define MENU_X_START 15                                         // Левая граница меню
#define MENU_X_END 127                                          // Правая граница меню
// Создание объекта дисплея
SavaOLED_ESP32 oled(SCREEN_WIDTH, SCREEN_HEIGHT);               // Создание объекта дисплея

// Пункты меню
const char* menuItems[MENU_ITEMS_COUNT] = {
    "Настройки",
    "Датчики",
    "Графики",
    "Информация",
    "Wi-Fi"
};

// Переменные меню
uint8_t selectedItem = 0;
bool blinkState = false;
unsigned long lastBlinkTime = 0;
bool isBlinking = false;
uint8_t blinkCount = 0;

// Переменные для антидребезга кнопок
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

void setup() {
    Serial.begin(115200);                                       // Инициализация последовательного порта

    // Инициализация кнопок
    pinMode(BTN_UP, INPUT_PULLUP);                              // Кнопка ВВЕРХ
    pinMode(BTN_OK, INPUT_PULLUP);                              // Кнопка OK     
    pinMode(BTN_DOWN, INPUT_PULLUP);                            // Кнопка ВНИЗ 

    // Инициализация OLED
    oled.init(400000, OLED_SDA, OLED_SCL);                      // Инициализация I2C и дисплея   

    // Настройка скорости скроллинга
    oled.scrollSpeed(2, true);                                 // Горизонтальный скроллинг скорость 2, зацикленный режим 
    oled.scrollSpeedVert(10);                                  // Вертикальный скроллинг скорость 10

    Serial.println("Пример 03_menu запущен");
    Serial.println("Используйте кнопки для навигации:");
    Serial.println("  GPIO 12 - Вверх");
    Serial.println("  GPIO 13 - OK (выбор)");
    Serial.println("  GPIO 15 - Вниз");
}

void loop() {
    // Обработка кнопок с антидребезгом
    if (millis() - lastButtonPress > debounceDelay) {
        if (digitalRead(BTN_UP) == LOW && !isBlinking) {
            lastButtonPress = millis();
            if (selectedItem > 0) {
                selectedItem--;
            } else {
                selectedItem = MENU_ITEMS_COUNT - 1;
            }
            Serial.print("Выбран пункт: ");
            Serial.println(menuItems[selectedItem]);
        }

        if (digitalRead(BTN_DOWN) == LOW && !isBlinking) {
            lastButtonPress = millis();
            if (selectedItem < MENU_ITEMS_COUNT - 1) {
                selectedItem++;
            } else {
                selectedItem = 0;
            }
            Serial.print("Выбран пункт: ");
            Serial.println(menuItems[selectedItem]);
        }

        if (digitalRead(BTN_OK) == LOW && !isBlinking) {
            lastButtonPress = millis();
            isBlinking = true;
            blinkCount = 0;
            blinkState = false;
            Serial.print("Нажата кнопка OK: ");
            Serial.println(menuItems[selectedItem]);
        }
    }

    // Обработка мигания при нажатии OK
    if (isBlinking) {
        if (millis() - lastBlinkTime > 200) {
            lastBlinkTime = millis();
            blinkState = !blinkState;
            blinkCount++;
            if (blinkCount >= 6) {  // 3 полных цикла мигания
                isBlinking = false;
                blinkState = false;
            }
        }
    }

    // Очистка экрана
    oled.clear();

    // ========================================
    // Левая сторона: Вертикальный скроллинг
    // ========================================
    oled.cursor(0, 0, StrScroll);                               // Скроллинг по всей ширине
    oled.font(SF_Vertical_P8);                                  // Вертикальный шрифт с поддержкой кириллицы CP1251
    oled.drawMode(REPLACE);                                     // Режим отрисовки: очистить и поверх
    oled.charSpacing(1);                                        // Межсимвольный интервал 1 пиксель
    oled.scroll(true);                                          // Включение вертикального скроллинга
    oled.print("Тут могла быть ваша реклама");                  // Текст для вертикального скроллинга
    oled.drawPrintVert();                                       // Отрисовка накопленного вертикального текста

    // ========================================
    // Правая сторона: Меню
    // ========================================
    for (uint8_t i = 0; i < MENU_ITEMS_COUNT; i++) {            // Цикл по всем пунктам меню
        uint8_t itemY = i * MENU_ITEM_HEIGHT;                   // Вычисление Y позиции пункта меню

        if (i == selectedItem) {                                // Если выбран этот пункт меню
            // Выбранный пункт меню
            if (!isBlinking || (isBlinking && blinkState)) {    // Если не мигает или мигает в видимом состоянии
                // Закрашенный прямоугольник
                oled.rectR(MENU_X_START, itemY, MENU_X_END - MENU_X_START, MENU_ITEM_HEIGHT,
                          MENU_BORDER_RADIUS, REPLACE, FILL);
            } else {                                            // Если мигает и не в видимом состоянии
                // При мигании - пустой прямоугольник
                oled.rectR(MENU_X_START, itemY, MENU_X_END - MENU_X_START, MENU_ITEM_HEIGHT,
                          MENU_BORDER_RADIUS, REPLACE, NO_FILL);
            }

            // Текст с автоинверсией и скроллингом
            oled.cursor(MENU_X_START + 2, itemY + 2, StrScroll, MENU_X_END);    // Скроллинг в пределах пункта меню
            oled.font(SF_Font_P8);                              // Шрифт с поддержкой кириллицы CP1251
            oled.drawMode(INV_AUTO);                            // Режим отрисовки: авто-инверсия
            oled.charSpacing(1);                                // Межсимвольный интервал 1 пиксель
            oled.scroll(true);                                  // Включение горизонтального скроллинга
            oled.print(menuItems[i]);                           // Текст пункта меню
            oled.drawPrint();                                   // Отрисовка накопленного текста
        } else {
            // Обычный пункт меню без рамки
            oled.cursor(MENU_X_START + 2, itemY + 2, StrLeft);  // Выравнивание по левому краю
            oled.font(SF_Font_P8);                              // Шрифт с поддержкой кириллицы CP1251    
            oled.drawMode(REPLACE);                             // Режим отрисовки: очистить и поверх  
            oled.charSpacing(1);                                // Межсимвольный интервал 1 пиксель 
            oled.scroll(false);                                 // Отключение горизонтального скроллинга
            oled.print(menuItems[i]);                           // Текст пункта меню
            oled.drawPrint();                                   // Отрисовка накопленного текста
        }
    }

    // Отправка буфера на дисплей
    oled.display();
}
