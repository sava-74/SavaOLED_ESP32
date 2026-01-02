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

#include "SavaOLED_ESP32.h"                     // Подключение библиотеки SavaOLED_ESP32
#include "Fonts/SF_Font_P8.h"                   // Подключение шрифта с поддержкой кириллицы CP1251
#include "Fonts/SF_7Seg_Temper_NM10x14.h"       // Подключение семисегментного шрифта для температуры

// Настройки дисплея
#define SCREEN_WIDTH 128                        // Ширина экрана в пикселях
#define SCREEN_HEIGHT 64                        // Высота экрана в пикселях
#define OLED_SDA 5                              // Пин SDA
#define OLED_SCL 4                              // Пин SCL

// Создание объекта дисплея
SavaOLED_ESP32 oled(SCREEN_WIDTH, SCREEN_HEIGHT);

// Переменные для анимации температуры
float temperature = 5.0;                        // Начальная температура
bool tempIncreasing = true;                     // Флаг направления изменения температуры
unsigned long lastTempUpdate = 0;               // Время последнего обновления температуры

// Переменные для переключения скроллинга
uint8_t scrollMode = 0;                         // 0=русская скроллится, 1=английская скроллится, 2=обе статичные
unsigned long lastScrollSwitch = 0;             // Время последнего переключения режима скроллинга

void setup() {
    Serial.begin(115200);       

    // Инициализация OLED
    oled.init(400000, OLED_SDA, OLED_SCL);       // Инициализация I2C и дисплея
    oled.setAddress(0x3C);                       // Установка I2C-адреса
    oled.clear();                                // Очистка экрана
    oled.display();                              // Обновление экрана

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
    oled.cursor(0, 0, StrScroll, 127);          // Скроллинг в пределах 0..127 пикселей
    oled.font(SF_Font_P8);                      // Шрифт с поддержкой кириллицы CP1251
    oled.drawMode(REPLACE);                     // Режим отрисовки: очистить и поверх
    oled.charSpacing(1);                        // Межсимвольный интервал 1 пиксель
    oled.scroll(scrollMode == 0);               // Скролл включен только в режиме 0
    oled.print("Русский текст с прокруткой - демонстрация горизонтального скроллинга библиотеки SavaOLED");
    oled.drawPrint();

    // ========================================
    // Строка 2: Английский текст
    // ========================================
    oled.cursor(0, 10, StrScroll, 127);         // Скроллинг в пределах 0..127 пикселей
    oled.font(SF_Font_P8);                      // Шрифт с поддержкой кириллицы CP1251    
    oled.drawMode(REPLACE);                     // Режим отрисовки: очистить и поверх
    oled.charSpacing(1);                        // Межсимвольный интервал 1 пиксель
    oled.scroll(scrollMode == 1);               // Скролл включен только в режиме 1
    oled.print("English text scrolling example - SavaOLED library demonstration");
    oled.drawPrint();                           // Отрисовка накопленного текста

    // ========================================
    // Строка 3: Температура (по центру)
    // ========================================
    oled.cursor(0, 30, StrCenter);              // Выравнивание по центру
    oled.font(SF_7Seg_Temper_NM10x14);          // Шрифт для температуры
    oled.drawMode(REPLACE);                     // Режим отрисовки: очистить и поверх
    oled.charSpacing(1);                        // Межсимвольный интервал 1 пиксель
    oled.scroll(false);                         // Скролл отключен
    // Формат вывода: C05.5°
    // "C" - буква C
    oled.print("+");                            // Плюс символ в шрифте - это буква C   
    // Число с дробной частью (формат XX.X, всего 4 символа включая точку)
    // Параметры print(float, decimals, min_width): decimals=1, min_width=4
    oled.print(temperature, 1, 4);              // Температура с 1 знаком после запятой, минимум 4 символа
    // "°" - символ градуса (в шрифте это ":")
    oled.print(":");                            // Двоеточие в шрифте - это символ градуса  
    oled.drawPrint();                           // Отрисовка температуры

    // Отправка буфера на дисплей
    oled.display();                             //  Обновление экрана
}
