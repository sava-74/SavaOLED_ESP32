/*
 * Пример 02_text - Демонстрация вывода текста и температуры
 *
 * Демонстрирует:
 * - Вывод текста на русском и английском языках
 * - Горизонтальный скроллинг с автоматическим переключением
 * - Вывод температуры семисегментным шрифтом
 * - Настройку шрифтов, режимов отрисовки и межсимвольных интервалов
 * - Анимацию значений температуры
 *
 * Подключение OLED дисплея:
 * SDA -> GPIO 5
 * SCL -> GPIO 4
 * VCC -> 3.3V
 * GND -> GND
 *
 * Используемые шрифты:
 * - SF_Font_P8.h (8px, пропорциональный, кириллица CP1251)
 * - SF_7Seg_Temper_NM10x14.h (14px, семисегментный для температуры)
 *   Специальные символы в шрифте температуры:
 *   ":" = точка + символ градуса (°)
 *   "+" = буква C (Цельсий)
 */

#include "SavaOLED_ESP32.h"
#include "Fonts/SF_Font_P8.h"
#include "Fonts/SF_7Seg_Temper_NM10x14.h"

// Настройки дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 5
#define OLED_SCL 4

// Создание объекта дисплея
SavaOLED_ESP32 oled(SCREEN_WIDTH, SCREEN_HEIGHT);

// Переменные для анимации температуры
float temperature = 5.0;
bool tempIncreasing = true;
unsigned long lastTempUpdate = 0;

// Переменные для переключения скроллинга
uint8_t scrollMode = 0;  // 0=русская скроллится, 1=английская скроллится, 2=обе статичные
unsigned long lastScrollSwitch = 0;

void setup() {
    Serial.begin(115200);

    // Инициализация OLED
    oled.init(400000, OLED_SDA, OLED_SCL);

    // Настройка скорости скроллинга (скорость 5, зацикленный режим)
    oled.scrollSpeed(5, true);

    Serial.println("Пример 02_text запущен");
    Serial.println("Демонстрация текста и температуры на OLED");
}

void loop() {
    // Переключение режима скроллинга каждые 5 секунд
    if (millis() - lastScrollSwitch > 10000) {
        lastScrollSwitch = millis();
        scrollMode++;
        if (scrollMode > 2) scrollMode = 0;

        Serial.print("Режим скроллинга: ");
        Serial.println(scrollMode == 0 ? "Русская" : (scrollMode == 1 ? "Английская" : "Обе статичные"));
    }

    // Обновление температуры каждые 500 мс
    if (millis() - lastTempUpdate > 500) {
        lastTempUpdate = millis();

        if (tempIncreasing) {
            temperature += 0.5;
            if (temperature >= 25.0) {
                temperature = 25.0;
                tempIncreasing = false;
            }
        } else {
            temperature -= 0.5;
            if (temperature <= 5.0) {
                temperature = 5.0;
                tempIncreasing = true;
            }
        }
    }

    // Очистка экрана
    oled.clear();

    // ========================================
    // Строка 1: Русский текст
    // ========================================
    oled.cursor(0, 0, StrScroll, 127);
    oled.font(SF_Font_P8);
    oled.drawMode(REPLACE);
    oled.charSpacing(1);
    oled.scroll(scrollMode == 0);  // Скролл включен только в режиме 0
    oled.print("Русский текст с прокруткой - демонстрация горизонтального скроллинга библиотеки SavaOLED");
    oled.drawPrint();

    // ========================================
    // Строка 2: Английский текст
    // ========================================
    oled.cursor(0, 10, StrScroll, 127);
    oled.font(SF_Font_P8);
    oled.drawMode(REPLACE);
    oled.charSpacing(1);
    oled.scroll(scrollMode == 1);  // Скролл включен только в режиме 1
    oled.print("English text scrolling example - SavaOLED library demonstration");
    oled.drawPrint();

    // ========================================
    // Строка 3: Температура (по центру)
    // ========================================
    oled.cursor(0, 30, StrCenter);
    oled.font(SF_7Seg_Temper_NM10x14);
    oled.drawMode(REPLACE);
    oled.charSpacing(1);
    oled.scroll(false);

    // Формат вывода: C05.5°
    // "C" - буква C
    oled.print("+");

    // Число с дробной частью (формат XX.X, всего 4 символа включая точку)
    // Параметры print(float, decimals, min_width): decimals=1, min_width=4
    oled.print(temperature, 1, 4);

    // "°" - символ градуса (в шрифте это ":")
    oled.print(":");

    oled.drawPrint();

    // Отправка буфера на дисплей
    oled.display();
}
