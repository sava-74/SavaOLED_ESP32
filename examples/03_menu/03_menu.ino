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

#include "SavaOLED_ESP32.h"
#include "Fonts/SF_Font_P8.h"
#include "Fonts/SF_Vertical_P8.h"

// Настройки дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 5
#define OLED_SCL 4

// Пины кнопок
#define BTN_UP 12
#define BTN_OK 13
#define BTN_DOWN 15

// Параметры меню
#define MENU_ITEMS_COUNT 5
#define MENU_ITEM_HEIGHT 12
#define MENU_BORDER_RADIUS 2
#define MENU_X_START 15
#define MENU_X_END 127

// Создание объекта дисплея
SavaOLED_ESP32 oled(SCREEN_WIDTH, SCREEN_HEIGHT);

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
    Serial.begin(115200);

    // Инициализация кнопок
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_OK, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);

    // Инициализация OLED
    oled.init(400000, OLED_SDA, OLED_SCL);

    // Настройка скорости скроллинга
    oled.scrollSpeed(10, true);
    oled.scrollSpeedVert(10);

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
    oled.cursor(0, 0, StrScroll);
    oled.font(SF_Vertical_P8);
    oled.drawMode(REPLACE);
    oled.charSpacing(1);
    oled.scroll(true);
    oled.print("Тут могла быть ваша реклама");
    oled.drawPrintVert();

    // ========================================
    // Правая сторона: Меню
    // ========================================
    for (uint8_t i = 0; i < MENU_ITEMS_COUNT; i++) {
        uint8_t itemY = i * MENU_ITEM_HEIGHT;

        if (i == selectedItem) {
            // Выбранный пункт меню
            if (!isBlinking || (isBlinking && blinkState)) {
                // Закрашенный прямоугольник
                oled.rectR(MENU_X_START, itemY, MENU_X_END - MENU_X_START, MENU_ITEM_HEIGHT,
                          MENU_BORDER_RADIUS, REPLACE, FILL);
            } else {
                // При мигании - пустой прямоугольник
                oled.rectR(MENU_X_START, itemY, MENU_X_END - MENU_X_START, MENU_ITEM_HEIGHT,
                          MENU_BORDER_RADIUS, REPLACE, NO_FILL);
            }

            // Текст с автоинверсией и скроллингом
            oled.cursor(MENU_X_START + 2, itemY + 2, StrScroll, MENU_X_END);
            oled.font(SF_Font_P8);
            oled.drawMode(INV_AUTO);
            oled.charSpacing(1);
            oled.scroll(true);
            oled.print(menuItems[i]);
            oled.drawPrint();
        } else {
            // Обычный пункт меню без рамки
            oled.cursor(MENU_X_START + 2, itemY + 2, StrLeft);
            oled.font(SF_Font_P8);
            oled.drawMode(REPLACE);
            oled.charSpacing(1);
            oled.scroll(false);
            oled.print(menuItems[i]);
            oled.drawPrint();
        }
    }

    // Отправка буфера на дисплей
    oled.display();
}
