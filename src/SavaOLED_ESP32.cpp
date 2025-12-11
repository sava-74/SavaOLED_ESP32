#include "SavaOLED_ESP32.h"

//****************************************************************************************
//---Статические (глобальные для файла) переменные и функции---
//****************************************************************************************
// Последовательность команд для инициализации SSD1306 128x64
static const uint8_t ssd1306_init_sequence[] PROGMEM = {
OLED_DISPLAY_OFF,
OLED_SET_CLOCK_DIV, 0x80,
//OLED_SET_MUX_RATIO, _height - 1,
OLED_SET_DISPLAY_OFFSET, 0x00,
OLED_SET_DISPLAY_START_LINE | 0x0,
OLED_SET_CHARGE_PUMP, 0x14,
0x20, 0x00, // ---- Эта строка добавлена для установки Horizontal Addressing Mode
OLED_SET_SEGMENT_REMAP,
OLED_SET_COM_SCAN_DEC,
OLED_SET_COM_PINS, 0x12,
OLED_SET_CONTRAST, 0xCF,
OLED_SET_PRECHARGE, 0xF1,
OLED_SET_VCOM_DESELECT, 0x40,
OLED_SET_NORMAL_DISPLAY,
OLED_DISPLAY_ON
};

// Функция-декодер UTF-8 -> CP1251. Возвращает индекс в массиве шрифтов.
static uint16_t utf8_to_cp1251(uint8_t high, uint8_t low) {
    if (high == 0xD0) {
        if (low == 0x81) return 0xA8; // Ё   // ---- Стандартный код CP1251
        if (low >= 0x90 && low <= 0xBF) return low + 0x30; // А-п
    } else if (high == 0xD1) {
        if (low == 0x91) return 0xB8; // ё   // ---- Стандартный код CP1251
        if (low >= 0x80 && low <= 0x8F) return low + 0x70; // р-я
    }
    return '?';
}

//****************************************************************************************
//--- Конструктор и Деструктор ---
//****************************************************************************************

SavaOLED_ESP32::SavaOLED_ESP32(uint8_t width, uint8_t height, i2c_port_t port) {
    _width = width;
    _height = height;
	_port = port;
	_address = 0x3C; // <-- Инициализация адреса по умолчанию (критично)
	_bufferSize = (_width * _height) / 8;
    _buffer = new uint8_t[_bufferSize];
    _tx_buffer = new uint8_t[_bufferSize + 1];
    _bus_handle = NULL;
    _dev_handle = NULL;
	_currentFont = nullptr;
	_cursorX = 0;
	_cursorY = 0;
    _drawMode = REPLACE;
	_charSpacing = 1; 
	_cursorAlign = StrLeft;
    _cursorX2 = -1;
	_scrollEnabled = false;
    _scrollSpeed = 3;
    _scrollLoop = true;
    _scrollOffset = 0;
    _lastScrollTime = 0;
    _scrollingLineY = -1;
	_Buffer = false;
	
    // --- Инициализация бинарного буфера ---
    _lineBufferWidth = 1024; // -- изменено: увеличен буфер по ширине
    _lineBufferHeightPages = 8; // -- изменено: увеличен буфер по высоте
    _lineBuffer = new uint8_t[_lineBufferWidth * _lineBufferHeightPages];
    _vertBuffer = new uint8_t[VERT_BUF_SIZE]; 
    _vertBufferHeight = 0;
    _currentLineWidth = 0;
    _segmentCount = 0;
    _textBufferPos = 0;
    _lineChanged = false;	

    _inverted = false;
    _contrast = 0xCF; // совпадает с init sequence
}

SavaOLED_ESP32::~SavaOLED_ESP32() {
    // Корректное удаление I2C-ресурсов по реальному API (i2c_master.h)
    if (_dev_handle) {
        // i2c_master_bus_rm_device принимает дескриптор устройства
        i2c_master_bus_rm_device(_dev_handle);
        _dev_handle = NULL;
    }
    if (_bus_handle) {
        i2c_del_master_bus(_bus_handle);
        _bus_handle = NULL;
    }

    // Освобождаем память, чтобы избежать утечек
    delete[] _buffer;
    delete[] _tx_buffer;
	delete[] _lineBuffer;
    delete[] _vertBuffer;
}

//****************************************************************************************
//--- Публичные функции "Настройки" (Initialization & Configuration) ---
//****************************************************************************************

void SavaOLED_ESP32::init(uint32_t freq, int8_t _sda, int8_t _scl) {
    i2c_master_bus_config_t bus_config;
    memset(&bus_config, 0, sizeof(i2c_master_bus_config_t)); // Обнуляем структуру для безопасности
    bus_config.i2c_port = _port;
    bus_config.sda_io_num = (gpio_num_t)_sda;
    bus_config.scl_io_num = (gpio_num_t)_scl;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &_bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = _address,
        .scl_speed_hz = freq,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(_bus_handle, &dev_config, &_dev_handle));

	// Сначала отправляем статическую часть команд
    _sendCommands(ssd1306_init_sequence, sizeof(ssd1306_init_sequence));
	// Теперь отдельно формируем и отправляем команду, зависящую от высоты
    uint8_t mux_ratio_cmd[] = {OLED_SET_MUX_RATIO, (uint8_t)(_height - 1)};
    _sendCommands(mux_ratio_cmd, sizeof(mux_ratio_cmd));
    
    clearBuffer();
    display();
}

void SavaOLED_ESP32::setAddress(uint8_t address){
	_address = address;
}

void SavaOLED_ESP32::setFont(const Font &font) {
    _currentFont = &font;
}


void SavaOLED_ESP32::setDrawMode(uint8_t mode) { 
    _drawMode = mode;
}

void SavaOLED_ESP32::setCharSpacing(uint8_t spacing) { 
    _charSpacing = spacing; 
}

void SavaOLED_ESP32::setScrollSpeed(uint8_t speed, bool loop) {
    if (speed < 1) speed = 1;
    if (speed > 10) speed = 10;
    _scrollSpeed = speed;
    _scrollLoop = loop;
}

void SavaOLED_ESP32::setScroll(bool enabled) {
    _scrollEnabled = enabled;
}

void SavaOLED_ESP32::setBuffer(bool enabled) { 
    _Buffer = enabled; 
}

void SavaOLED_ESP32::setContrast(uint8_t value) {
    uint8_t cmds[2] = { OLED_SET_CONTRAST, value };
    _sendCommands(cmds, sizeof(cmds));
    _contrast = value;
}

void SavaOLED_ESP32::setPower(bool mode) {
    uint8_t cmd = mode ? OLED_DISPLAY_ON : OLED_DISPLAY_OFF;
    _sendCommands(&cmd, 1);
}

void SavaOLED_ESP32::flipH(bool mode) {
    uint8_t cmd = mode ? OLED_FLIP_H : OLED_NORMAL_H;
    _sendCommands(&cmd, 1);
}

void SavaOLED_ESP32::flipV(bool mode) {
    uint8_t cmd = mode ? OLED_FLIP_V : OLED_NORMAL_V;
    _sendCommands(&cmd, 1);

}

void SavaOLED_ESP32::invertDisplay(bool mode) {
    uint8_t cmd = mode ? OLED_INVERTDISPLAY : OLED_SET_NORMAL_DISPLAY;
    _sendCommands(&cmd, 1);
    _inverted = mode;
}

void SavaOLED_ESP32::setRotation(bool rotate180) {
    if (!rotate180) {
        uint8_t cmds[] = { OLED_NORMAL_H, OLED_NORMAL_V };
        _sendCommands(cmds, sizeof(cmds));
    }
    else {
        uint8_t cmds[] = { OLED_FLIP_H, OLED_FLIP_V };
        _sendCommands(cmds, sizeof(cmds));
    }
}

//****************************************************************************************
//--- Публичные функции "Управление курсором" ---
//****************************************************************************************

void SavaOLED_ESP32::setCursor(int16_t x, int16_t y, uint8_t align, int16_t x2) {
	_cursorX = x;       		
	_cursorY = y;       		
	_cursorAlign = align; 		
	_cursorX2 = x2;
	_segmentCount = 0;
    _textBufferPos = 0;
    _lineChanged = true;
}

int16_t SavaOLED_ESP32::getCursorX() const {
	//Serial.println(_currentLineWidth); 
	return _cursorX;
}

int16_t SavaOLED_ESP32::getCursorY() const {
	return _cursorY;
}

//****************************************************************************************
//--- Публичные функции "Накопления" (Data Accumulation) ---
// --- Перегруженные функции print, работающие с курсором ---
// --- Основная функция addPrint для const char* ---
//****************************************************************************************
void SavaOLED_ESP32::addPrint(const char* text) {
    if (_segmentCount >= MAX_SEGMENTS) return; // Достигнут лимит сегментов

    size_t text_len = strlen(text);
    if (_textBufferPos + text_len + 1 > TEXT_BUFFER_SIZE) return; // Недостаточно места в общем буфере

    // Копируем текст в общий буфер
    strcpy(&_textBuffer[_textBufferPos], text);

    // Создаем и заполняем новый сегмент
    _segments[_segmentCount].text = &_textBuffer[_textBufferPos];
    _segments[_segmentCount].font = _currentFont;
    //_segments[_segmentCount].drawMode = _drawMode;

    // Сдвигаем позицию в буфере и увеличиваем счетчик сегментов
    _textBufferPos += text_len; // Сдвигаем указатель на длину скопированного текста
    _textBuffer[_textBufferPos] = '\0'; // Ставим нуль-терминатор
    _textBufferPos++; // Переходим на следующую позицию
    _segmentCount++;

    _lineChanged = true;
}
void SavaOLED_ESP32::addPrint(int32_t value, uint8_t min_digits) {
    char buffer[22] = {0};
    if (min_digits > 0) {
        char format[8] = {0};
        //   'ld' теперь просто символы в конце строки
        snprintf(format, sizeof(format), "%%0%dld", min_digits); 
        snprintf(buffer, sizeof(buffer), format, value);
    } else {
        snprintf(buffer, sizeof(buffer), "%ld", value);
    }
    addPrint(buffer);
	Serial.println(buffer);
}

void SavaOLED_ESP32::addPrint(uint32_t value, uint8_t min_digits) {
    char buffer[22] = {0};
    if (min_digits > 0) {
        char format[8] = {0};
        //   'lu' теперь просто символы в конце строки
        snprintf(format, sizeof(format), "%%0%dlu", min_digits);
        snprintf(buffer, sizeof(buffer), format, value);
    } else {
        snprintf(buffer, sizeof(buffer), "%lu", value);
    }
    addPrint(buffer);
}

// Перегрузки для меньших типов просто вызывают основные реализации
void SavaOLED_ESP32::addPrint(int value, uint8_t min_digits) { addPrint((int32_t)value, min_digits); }
void SavaOLED_ESP32::addPrint(int8_t value, uint8_t min_digits) { addPrint((int32_t)value, min_digits); }
void SavaOLED_ESP32::addPrint(int16_t value, uint8_t min_digits) { addPrint((int32_t)value, min_digits); }
void SavaOLED_ESP32::addPrint(uint8_t value, uint8_t min_digits) { addPrint((uint32_t)value, min_digits); }
void SavaOLED_ESP32::addPrint(uint16_t value, uint8_t min_digits) { addPrint((uint32_t)value, min_digits); }

/*void SavaOLED_ESP32::addPrint(float value, uint8_t decimalPlaces) {char buffer[32] = { 0 }; dtostrf(value, 1, decimalPlaces, buffer); addPrint(buffer); }
void SavaOLED_ESP32::addPrint(double value, uint8_t decimalPlaces) { char buffer[32] = { 0 }; dtostrf(value, 1, decimalPlaces, buffer); addPrint(buffer); }*/

void SavaOLED_ESP32::addPrint(double value, uint8_t decimalPlaces, uint8_t min_width) {
    char buffer[32] = {0};
    char format[12] = {0};

    // Если задана минимальная ширина, создаем сложный формат
    if (min_width > 0) {
        // Создаем строку формата, например "%08.2f"
        // %0 - дополнять нулями
        // 8  - общая ширина
        // .2 - два знака после запятой
        // f  - тип float/double
        snprintf(format, sizeof(format), "%%0%d.%df", min_width, decimalPlaces);
    } 
    // Иначе создаем простой формат только с указанием точности
    else {
        snprintf(format, sizeof(format), "%%.%df", decimalPlaces);
    }
    
    // Применяем созданный формат для преобразования числа в строку
    snprintf(buffer, sizeof(buffer), format, value);
    addPrint(buffer);
}

// Перегрузка для float просто вызывает реализацию для double
void SavaOLED_ESP32::addPrint(float value, uint8_t decimalPlaces, uint8_t min_width) {
    addPrint((double)value, decimalPlaces, min_width);
}

void SavaOLED_ESP32::addPrint(const String &s) { addPrint(s.c_str()); }

//****************************************************************************************
//--- Публичные функции "Отрисовки" (Rendering) ---
//****************************************************************************************

void SavaOLED_ESP32::drawPrint() {
    if (_segmentCount == 0 && !_scrollEnabled) return;

    // --- Шаг 1: Перерисовка во временный буфер (только если текст изменился) ---
    if (_lineChanged) {
        // 1.1 Определяем высоту строки (максимальную среди всех сегментов)
        uint8_t max_line_pages = 1;
        for (uint8_t s = 0; s < _segmentCount; ++s) {
            const auto& segment = _segments[s];
            if (segment.font) {
                uint8_t pages_per_char = (segment.font->height + 7) / 8;
                if (pages_per_char > max_line_pages) {
                    max_line_pages = pages_per_char;
                }
            }
        }
        // Сохраняем высоту буфера
        _lineBufferHeightPages = max_line_pages;

        // 1.2 Очищаем буфер
        memset(_lineBuffer, 0, _lineBufferWidth * max_line_pages);
        
        int16_t current_x = 0;

        // 1.3 Рисуем сегменты
        for (uint8_t s = 0; s < _segmentCount; ++s) {
            const auto& segment = _segments[s];
            const Font* font = segment.font;
            const char* text = segment.text;

            if (!font || !text) continue;
            uint8_t pages_per_char = (font->height + 7) / 8;

            int i = 0;
            while (text[i] != '\0' && current_x < _lineBufferWidth) {
                // Декодируем UTF-8 в CP1251 или ASCII
                uint16_t char_code = (uint8_t)text[i];
                if (char_code < 128) { 
                    i++; 
                } else { 
                    char_code = utf8_to_cp1251((uint8_t)text[i], (uint8_t)text[i+1]); 
                    i += 2; 
                }
                
                // Получаем индекс символа (0..159)
                uint16_t index = _getCharIndex(font, char_code);
                
                if (index != 0xFFFF) {
                    // --- Читаем через Offsets ---
                    // 1. Находим начало данных символа
                    uint16_t glyph_data_start = font->offsets[index];
                    
                    // 2. Первый байт - это ШИРИНА символа
                    uint8_t char_width = font->data[glyph_data_start];
                    
                    // 3. Указатель на саму графику (пропускаем байт ширины)
                    const uint8_t* glyph_pixels = &font->data[glyph_data_start + 1];

                    // 4. Отрисовка столбиков
                    for (uint8_t col = 0; col < char_width; col++) {
                        if (current_x + col >= _lineBufferWidth) break;
                        
                        // Копируем байты по вертикали (страницы)
                        // Данные лежат: [Width] [Page0_Row] [Page1_Row] ...
                        // Чтобы взять байт страницы P для колонки C:
                        // Адрес = (P * char_width) + col
                        for (uint8_t p = 0; p < pages_per_char; p++) {
                            uint32_t dest_idx = (current_x + col) + (p * _lineBufferWidth);
                            
                            // Защита от выхода за пределы вертикального буфера
                            if (dest_idx < (uint32_t)(_lineBufferWidth * max_line_pages)) {
                                _lineBuffer[dest_idx] = glyph_pixels[p * char_width + col];
                            }
                        }
                    }
                    // Сдвигаем курсор на ширину символа + интервал
                    current_x += char_width + _charSpacing;
                }
            }
        }
        
        // Корректируем итоговую ширину строки (убираем последний интервал)
        _currentLineWidth = (current_x > 0) ? (current_x - _charSpacing) : 0;
        _lineChanged = false;
    }

    // --- Шаг 2: Вычисление смещений и копирование в видеобуфер ---
    int16_t startX_on_screen = _cursorX;
    int16_t endX_on_screen = (_cursorX2 == -1) ? (_width - 1) : _cursorX2;
    int16_t region_width = endX_on_screen - startX_on_screen + 1; // включительно

    uint16_t source_offset = 0; // effective non-negative offset for indexing

    const int16_t scroll_gap = 30; //интервал корусели
    uint16_t loop_width = (_currentLineWidth > 0) ? (_currentLineWidth + scroll_gap) : 0;

    bool scrolling = _scrollEnabled && (_cursorAlign == StrScroll);// || (_cursorAlign == StrLeft && _currentLineWidth > region_width));

    if (scrolling) {
        if (_cursorY != _scrollingLineY) { _scrollOffset = 0; _scrollingLineY = _cursorY; }
        unsigned long currentTime = millis();
        uint16_t scroll_delay = 1000 / (_scrollSpeed * 10);
        if (currentTime - _lastScrollTime > scroll_delay) { _lastScrollTime = currentTime; _scrollOffset++; }
        if (loop_width == 0) source_offset = 0;
        else source_offset = (uint16_t)(_scrollOffset % (uint32_t)loop_width); 
    } else {
        // no scrolling: compute startX_on_screen for alignment
        if (_cursorAlign == StrCenter) { startX_on_screen = _cursorX + (region_width / 2) - (_currentLineWidth / 2); }
        else if (_cursorAlign == StrRight) { startX_on_screen = endX_on_screen - _currentLineWidth; }
        source_offset = 0;
    }
	

    // --- Шаг 3: Копирование "окна" из _lineBuffer в _buffer ---
    const uint8_t pages = _height / 8;
    for (int16_t i = 0; i < region_width; i++) {
        int16_t screen_x = _cursorX + i;
        if (screen_x < 0 || screen_x >= _width) continue; // защита от выхода за границы
        int32_t source_x;

        if (scrolling && _scrollLoop && loop_width > 0) {
            source_x = (int32_t)((source_offset + (uint16_t)i) % loop_width);
        } else {
            // source_offset_signed is 0 for non-scrolling, for consistency convert source_offset
            int32_t signed_offset = (int32_t)source_offset;
            source_x = signed_offset + i - (startX_on_screen - _cursorX);
        }
//Serial.println(endX_on_screen);
     if (source_x >= 0 && source_x < _currentLineWidth) {
            uint8_t y_page_start = _cursorY / 8;
            uint8_t y_offset = _cursorY % 8;
            
            // ---  Простой цикл по всем страницам отрисованной строки
            for (uint8_t p = 0; p < _lineBufferHeightPages; p++) {
                uint32_t source_idx = (uint32_t)source_x + (uint32_t)p * _lineBufferWidth;
                
                uint8_t data_byte = _lineBuffer[source_idx];

                // Нельзя пропускать нули в режиме REPLACE, иначе фон не очистится!
                // Пропускаем только если это ADD_UP или INV_AUTO и байт пустой.
                if (data_byte == 0 && _drawMode != REPLACE) continue; 

                uint8_t dest_page_top = y_page_start + p;
                uint8_t dest_page_bottom = dest_page_top + 1;

                // Данные, сдвинутые на нужную позицию
                uint8_t mask_top = data_byte << y_offset;
                uint8_t mask_bottom = (y_offset > 0) ? (data_byte >> (8 - y_offset)) : 0;  

                // Защитная маска (Cover Mask). 
                // Она показывает, какие биты в байте дисплея МЫ ИМЕЕМ ПРАВО трогать.
                // 1 = это зона нашего символа (здесь мы пишем данные или стираем фон).
                // 0 = это зона выше/ниже символа в этом байте (её трогать нельзя).
                uint8_t cover_top = 0xFF << y_offset;
                uint8_t cover_bottom = (y_offset > 0) ? (0xFF >> (8 - y_offset)) : 0;

                switch (_drawMode) {  
                    case REPLACE: {  
                        // Логика: (СтарыйФон & ~ГдеМыРисуем) | (НовыеДанные & ГдеМыРисуем)
                        // Это работает даже если НовыеДанные == 0 (стирает фон)
                        if (dest_page_top < pages) {
                            uint32_t idx = screen_x + dest_page_top * _width;
                            _buffer[idx] = (_buffer[idx] & ~cover_top) | (mask_top & cover_top);
                        }
                        if (y_offset > 0 && dest_page_bottom < pages) {
                            uint32_t idx = screen_x + dest_page_bottom * _width;
                            _buffer[idx] = (_buffer[idx] & ~cover_bottom) | (mask_bottom & cover_bottom);
                        }
                        break;  
                    }  
                    case ADD_UP: {  
                        // Просто наложение (OR)
                        if (dest_page_top < pages) _buffer[screen_x + dest_page_top * _width] |= mask_top;  
                        if (y_offset > 0 && dest_page_bottom < pages) _buffer[screen_x + dest_page_bottom * _width] |= mask_bottom;  
                        break;  
                    }  
                    case INV_AUTO: {  
                        // Инверсия (XOR). Фон инвертируется только там, где есть пиксели символа.
                        if (dest_page_top < pages) _buffer[screen_x + dest_page_top * _width] ^= mask_top;  
                        if (y_offset > 0 && dest_page_bottom < pages) _buffer[screen_x + dest_page_bottom * _width] ^= mask_bottom;  
                        break;  
                    }  
                }
            }
        }
    }
}   

/*void SavaOLED_ESP32::drawPrintVert() { /// это рабочий код ! я его вернул 
    if (_segmentCount == 0 && !_scrollEnabled) return;

    // 1. Очистка вертикального буфера
    memset(_vertBuffer, 0, VERT_BUF_SIZE);
    uint16_t current_buf_y = 0;
    
    // Максимальная ширина столбца (в страницах), которую мы поддерживаем в буфере (4 страницы = 32px)
    const uint8_t max_buf_pages = 4; 

    // --- ЭТАП 1: РЕНДЕРИНГ В БУФЕР (Сверху вниз, start Y = 0) ---
    for (uint8_t s = 0; s < _segmentCount; ++s) {
        const auto& segment = _segments[s];
        const Font* font = segment.font;
        const char* text = segment.text;
        if (!font || !text) continue;

        uint8_t pages_per_char = (font->height + 7) / 8;
        if (pages_per_char > max_buf_pages) pages_per_char = max_buf_pages;

        int i = 0;
        while (text[i] != '\0') {
            uint16_t char_code = (uint8_t)text[i];
            if (char_code < 128) { i++; } 
            else { char_code = utf8_to_cp1251((uint8_t)text[i], (uint8_t)text[i+1]); i += 2; }

            uint16_t index = _getCharIndex(font, char_code);
            if (index == 0xFFFF) continue;

            const uint8_t* char_ptr = &font->data[font->offsets[index]];
            uint8_t raw_size = *char_ptr;
            const uint8_t* glyph_pixels = char_ptr + 1;

            // Autocrop (Scan Forward)
            uint8_t start_col = 0;
            bool found = false;
            for (; start_col < raw_size; start_col++) {
                for (uint8_t p = 0; p < pages_per_char; p++) {
                    if (glyph_pixels[p * raw_size + start_col] != 0) { found = true; break; }
                }
                if (found) break;
            }

            // Autocrop (Scan Backward)
            uint8_t end_col = raw_size - 1;
            if (found) {
                found = false;
                for (; end_col > start_col; end_col--) {
                    for (uint8_t p = 0; p < pages_per_char; p++) {
                        if (glyph_pixels[p * raw_size + end_col] != 0) { found = true; break; }
                    }
                    if (found) break;
                }
            } else {
                // Пробел
                current_buf_y += (raw_size > 4 ? raw_size / 3 : 4) + _charSpacing;
                continue;
            }

            // Копируем полезные байты в _vertBuffer
            // В буфере данные лежат линейно: [Y0_Page0] [Y0_Page1] .. [Y1_Page0] ..
            for (uint8_t col = start_col; col <= end_col; col++) {
                // Проверка переполнения буфера по высоте
                if ((current_buf_y * max_buf_pages) >= VERT_BUF_SIZE) break;

                for (uint8_t p = 0; p < pages_per_char; p++) {
                    uint8_t val = glyph_pixels[p * raw_size + col];
                    // Пишем в буфер. Адрес = (Y * Ширина_буфера_в_страницах) + Текущая_страница
                    _vertBuffer[current_buf_y * max_buf_pages + p] = val;
                }
                current_buf_y++;
            }
            current_buf_y += _charSpacing;
        }
    }
    
    // Удаляем последний интервал
    if (current_buf_y > 0) current_buf_y -= _charSpacing;
    _vertBufferHeight = current_buf_y;

    // --- ЭТАП 2: РАСЧЕТ КООРДИНАТ НА ЭКРАНЕ ---
    int32_t screen_y_start = _cursorY;

    // Логика скроллинга
    if (_scrollEnabled && _cursorAlign == StrScroll) {
        // Тайминг
        unsigned long currentTime = millis();
        uint16_t scroll_delay = 1000 / (_scrollSpeed * 10);
        if (currentTime - _lastScrollTime > scroll_delay) { 
            _lastScrollTime = currentTime; 
            _scrollOffset++; 
        }
        
        uint32_t loop_height = _vertBufferHeight + 30; // 30px пауза между повторами
        uint32_t effective_offset = _scrollOffset % loop_height;
        
        // Смещаем текст вверх
        screen_y_start -= effective_offset;
    } 
    // Логика выравнивания (если не скролл)
    else {
        if (_cursorAlign == StrCenter) {
            screen_y_start = _cursorY - (_vertBufferHeight / 2);
        } else if (_cursorAlign == StrRight) { // Используем как Bottom align
            screen_y_start = _cursorY - _vertBufferHeight;
        }
    }

    // --- ЭТАП 3: БЛИТТИНГ (Копирование из VertBuffer на Экран) ---
    // Нам нужно скопировать весь _vertBuffer, накладывая его на экран начиная с screen_y_start
    
    // Цикл по Y буфера (от 0 до _vertBufferHeight)
    // Для скроллинга (зацикливания) нужно рисовать дважды, если текст уехал вверх
    
    for (int pass = 0; pass < 2; pass++) { // Проход 2 нужен для "хвоста" при скроллинге
        int32_t current_render_y = screen_y_start;
        if (pass == 1) {
            if (!_scrollEnabled) break;
            current_render_y += (_vertBufferHeight + 30); // Рисуем копию снизу
        }

        for (uint16_t buf_y = 0; buf_y < _vertBufferHeight; buf_y++) {
            int32_t abs_y = current_render_y + buf_y;

            // Оптимизация: если строка за пределами экрана
            if (abs_y >= _height) break; 
            if (abs_y < -8) continue; // -8 потому что байт может торчать снизу

            // Вычисляем координаты на экране
            // abs_y может быть отрицательным (уехал вверх), но маски это учтут
            
            // X координата (столбец)
            int16_t draw_x = _cursorX; // Тут можно добавить логику StrRight для X, если нужно

            // Y привязка к страницам экрана
            // Важно: abs_y может быть не кратен 8.
            // Нам нужно взять байты из буфера и сдвинуть их.
            
            int16_t page_y = (abs_y >= 0) ? (abs_y / 8) : ((abs_y - 7) / 8);
            uint8_t bit_shift = (abs_y >= 0) ? (abs_y % 8) : (8 + (abs_y % 8));
            if (bit_shift == 8) bit_shift = 0;

            for (uint8_t p = 0; p < max_buf_pages; p++) {
                // Данные из вертикального буфера
                uint8_t data = _vertBuffer[buf_y * max_buf_pages + p];
                if (data == 0 && _drawMode != REPLACE) continue;

                // Сдвиг и маски
                uint8_t mask_top = data << bit_shift;
                uint8_t mask_bottom = (bit_shift > 0) ? (data >> (8 - bit_shift)) : 0;
                
                uint8_t cover_top = 0xFF << bit_shift;
                uint8_t cover_bottom = (bit_shift > 0) ? (0xFF >> (8 - bit_shift)) : 0;

                // Запись Top Page
                if (page_y >= 0 && page_y < (_height/8)) {
                    uint32_t idx = (draw_x + p) + page_y * _width; // +p сдвигает колонки в ширину (толщина букв)
                    // Проверка ширины экрана
                    if ((draw_x + p) < _width) {
                        if (_drawMode == REPLACE) _buffer[idx] = (_buffer[idx] & ~cover_top) | mask_top;
                        else if (_drawMode == ADD_UP) _buffer[idx] |= mask_top;
                        else if (_drawMode == INV_AUTO) _buffer[idx] ^= mask_top;
                    }
                }

                // Запись Bottom Page
                if (bit_shift > 0) {
                    int16_t next_page = page_y + 1;
                    if (next_page >= 0 && next_page < (_height/8)) {
                        uint32_t idx = (draw_x + p) + next_page * _width;
                        if ((draw_x + p) < _width) {
                            if (_drawMode == REPLACE) _buffer[idx] = (_buffer[idx] & ~cover_bottom) | mask_bottom;
                            else if (_drawMode == ADD_UP) _buffer[idx] |= mask_bottom;
                            else if (_drawMode == INV_AUTO) _buffer[idx] ^= mask_bottom;
                        }
                    }
                }
            }
        }
    }
}*/

void SavaOLED_ESP32::drawPrintVert() {
    if (_segmentCount == 0) return;

    // Вспомогательная структура для хранения вычислений первого прохода
    struct CharLayout {
        uint16_t index;
        uint8_t raw_width;      // Ширина в байтах из файла
        uint8_t skip_top;       // Сколько пустых строк пикселей сверху
        uint8_t real_height;    // Реальная высота символа в пикселях
        bool is_space;
    };
    
    CharLayout layouts[128]; // Буфер метрик
    uint16_t layout_count = 0;
    uint16_t total_pixel_height = 0;

    // --- ПРОХОД 1: АНАЛИЗ И РАСЧЕТ ВЫСОТЫ (SCAN ROWS) ---
    for (uint8_t s = 0; s < _segmentCount; ++s) {
        const auto& segment = _segments[s];
        if (!segment.font || !segment.text) continue;

        const Font* font = segment.font;
        uint8_t font_h_pixels = font->height; 
        //uint8_t pages_per_char = (font_h_pixels + 7) / 8;

        int i = 0;
        while (segment.text[i] != '\0') {
            if (layout_count >= 128) break;

            uint16_t char_code = (uint8_t)segment.text[i];
            if (char_code < 128) { i++; } 
            else { char_code = utf8_to_cp1251((uint8_t)segment.text[i], (uint8_t)segment.text[i+1]); i += 2; }

            uint16_t index = _getCharIndex(font, char_code);
            if (index == 0xFFFF) continue;

            const uint8_t* char_ptr = &font->data[font->offsets[index]];
            uint8_t raw_width = *char_ptr;
            const uint8_t* pixels = char_ptr + 1;

            // 1.1 СКАНИРОВАНИЕ СВЕРХУ (Поиск пустых строк пикселей)
            uint8_t empty_top = 0;
            for (int row = 0; row < font_h_pixels; row++) {
                bool row_is_empty = true;
                for (int col = 0; col < raw_width; col++) {
                    uint8_t p = row / 8;
                    uint8_t b = row % 8;
                    // Проверяем бит 'b' во всех колонках на странице 'p'
                    if (pixels[p * raw_width + col] & (1 << b)) {
                        row_is_empty = false;
                        break;
                    }
                }
                if (!row_is_empty) break;
                empty_top++;
            }

            CharLayout l;
            l.index = index;
            l.raw_width = raw_width;
            l.skip_top = empty_top;
            
            // Если весь символ пустой (пробел)
            if (empty_top == font_h_pixels) {
                l.is_space = true;
                l.real_height = (font_h_pixels > 4) ? (font_h_pixels / 3) : 4;
            } else {
                l.is_space = false;
                // 1.2 СКАНИРОВАНИЕ СНИЗУ
                uint8_t empty_bottom = 0;
                for (int row = font_h_pixels - 1; row >= empty_top; row--) {
                    bool row_is_empty = true;
                    for (int col = 0; col < raw_width; col++) {
                        uint8_t p = row / 8;
                        uint8_t b = row % 8;
                        if (pixels[p * raw_width + col] & (1 << b)) {
                            row_is_empty = false;
                            break;
                        }
                    }
                    if (!row_is_empty) break;
                    empty_bottom++;
                }
                l.real_height = font_h_pixels - empty_top - empty_bottom;
            }

            total_pixel_height += l.real_height + _charSpacing;
            layouts[layout_count++] = l;
        }
    }
    if (total_pixel_height > 0) total_pixel_height -= _charSpacing;


    // --- ПРОХОД 2: ОТРИСОВКА ---
    int32_t screen_y = _cursorY;
    
    // Применяем выравнивание
    // Рассчитываем доступное пространство от курсора до низа экрана
    // Рассчитываем доступную высоту зоны
    int16_t available_height;
    
    // Если задан 4-й параметр в setCursor (x2 > 0), используем его как ВЫСОТУ зоны
    if (_cursorX2 > 0) {
        available_height = _cursorX2;
    } else {
        // Иначе используем всё пространство до низа экрана
        available_height = _height - _cursorY;
    }
    
    // Применяем выравнивание внутри этой зоны
    if (_cursorAlign == StrCenter) {
        screen_y = _cursorY + (available_height - total_pixel_height) / 2;
    } 
    else if (_cursorAlign == StrRight) {
        screen_y = _cursorY + available_height - total_pixel_height;
    }

    // Рисуем каждый символ
    for (uint16_t k = 0; k < layout_count; k++) {
        CharLayout l = layouts[k];
        
        // *Упрощение: берем шрифт из 0-го сегмента, т.к. обычно шрифт один
        const Font* font = _segments[0].font; 
        const uint8_t* pixels = &font->data[font->offsets[l.index]] + 1;
        uint8_t pages_per_char = (font->height + 7) / 8;

        if (l.is_space) {
            screen_y += l.real_height + _charSpacing;
            continue;
        }

        // Рисуем колонки (которые визуально становятся строками символа)
        for (uint8_t col = 0; col < l.raw_width; col++) {
            // X координата не меняется для символа (он часть столбца)
            // Но у нас есть ширина символа (толщина штриха), она идет по X
            int16_t draw_x = _cursorX + col;
            if (draw_x >= _width) continue;

            // Собираем вертикальный столбец данных из всех страниц шрифта
            uint32_t col_data = 0;
            for (uint8_t p = 0; p < pages_per_char; p++) {
                col_data |= ((uint32_t)pixels[p * l.raw_width + col] << (p * 8));
            }

            // А. СДВИГАЕМ ДАННЫЕ ВВЕРХ (Удаляем пустоту empty_top)
            col_data >>= l.skip_top;
            
            // Б. Маскируем лишнее снизу (оставляем только real_height бит)
            uint32_t height_mask = (1UL << l.real_height) - 1;
            col_data &= height_mask;

            // В. Рендерим на экран по координате screen_y
            // Нам нужно разбить col_data обратно на страницы экрана
            
            int16_t start_page = (screen_y >= 0) ? (screen_y / 8) : -1;
            uint8_t bit_off = (screen_y >= 0) ? (screen_y % 8) : (8 + (screen_y % 8));

            // Сдвигаем данные на фазу экрана
            uint64_t render_data = (uint64_t)col_data << bit_off;
            uint64_t render_mask = (uint64_t)height_mask << bit_off;

            // Записываем в буфер постранично (до 4-5 страниц макс)
            for (int p_off = 0; p_off < 5; p_off++) {
                int16_t dest_page = start_page + p_off;
                if (dest_page >= 0 && dest_page < (_height / 8)) {
                    uint32_t idx = draw_x + dest_page * _width;
                    uint8_t byte_data = (uint8_t)(render_data & 0xFF);
                    uint8_t byte_mask = (uint8_t)(render_mask & 0xFF);

                    if (byte_mask) {
                        if (_drawMode == REPLACE) 
                            _buffer[idx] = (_buffer[idx] & ~byte_mask) | (byte_data & byte_mask);
                        else if (_drawMode == ADD_UP) 
                            _buffer[idx] |= (byte_data & byte_mask);
                        else if (_drawMode == INV_AUTO) 
                            _buffer[idx] ^= (byte_data & byte_mask);
                    }
                }
                render_data >>= 8;
                render_mask >>= 8;
                if (render_mask == 0) break;
            }
        }
        screen_y += l.real_height + _charSpacing;
    }
}



void SavaOLED_ESP32::display() { 
    if (_Buffer) { 
        _displayFullBuffer(); 
    } else { 
        _displayPaged(); 
    } 
}


void SavaOLED_ESP32::clearBuffer() {
    // Возвращаем на очистку нулями, чтобы видеть результат, а не белый экран
    memset(_buffer, 0x00, _bufferSize);
}

//****************************************************************************************
//--- Публичные функции "Примитивы"  ---
//****************************************************************************************

void SavaOLED_ESP32::dot(int16_t x, int16_t y, uint8_t mode) {
    _drawPixel(x, y, mode);
}

void SavaOLED_ESP32::line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t mode) {
    int16_t dx = abs(x2 - x1);
    int16_t dy = -abs(y2 - y1);
    int16_t sx = x1 < x2 ? 1 : -1;
    int16_t sy = y1 < y2 ? 1 : -1;
    int16_t err = dx + dy;
    int16_t e2;

    for (;;) {
        _drawPixel(x1, y1, mode);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void SavaOLED_ESP32::hLine(int16_t x, int16_t y, int16_t w, uint8_t mode) {
    for (int16_t i = x; i < x + w; i++) {
        _drawPixel(i, y, mode);
    }
}

void SavaOLED_ESP32::vLine(int16_t x, int16_t y, int16_t h, uint8_t mode) {
    for (int16_t i = y; i < y + h; i++) {
        _drawPixel(x, i, mode);
    }
}

void SavaOLED_ESP32::circle(int16_t x0, int16_t y0, int16_t r, uint8_t mode, bool fill) {
    if (r < 0) return;
	// --- СПЕЦ-РЕЖИМ: Очистка фона + Белая рамка ---
	if (mode == ERASE_BORDER && fill) {
        circle(x0, y0, r, ERASE, true);    // Шаг 1: Стираем круг (черный блин)
        circle(x0, y0, r, ADD_UP, false);  // Шаг 2: Рисуем белый контур
        return;
    }
	
	
    if (fill) {
        // --- НОВЫЙ ДВУХПРОХОДНЫЙ АЛГОРИТМ ЗАЛИВКИ ---
        if (r == 0) {
            _drawPixel(x0, y0, mode);
            return;
        }

        // Создаем временный массив на стеке для хранения половинной ширины для каждой Y-координаты
        uint8_t half_widths[r + 1];
        memset(half_widths, 0, sizeof(half_widths));

        // --- Проход 1: Вычисление ---
        // Запускаем алгоритм Брезенхэма, но не рисуем, а только заполняем массив максимальных ширин.
        int16_t f = 1 - r;
        int16_t ddF_x = 1;
        int16_t ddF_y = -2 * r;
        int16_t x = 0;
        int16_t y = r;

        while (y >= x) {
            // Для высоты 'y' половина ширины равна 'x'
            // Для высоты 'x' половина ширины равна 'y'
            // Мы всегда берем максимальное из найденных значений
            if (half_widths[y] < x) half_widths[y] = x;
            if (half_widths[x] < y) half_widths[x] = y;

            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;
        }

        // --- Проход 2: Отрисовка ---
        // Теперь рисуем горизонтальные линии, используя вычисленные идеальные ширины.
        for (int16_t i = 0; i <= r; i++) {
            uint8_t hw = half_widths[i];
            // Рисуем симметричные линии сверху и снизу от центра
            hLine(x0 - hw, y0 + i, 2 * hw + 1, mode);
            if (i != 0) {
                hLine(x0 - hw, y0 - i, 2 * hw + 1, mode);
            }
        }

    } else {
        // --- ВАШ 100% РАБОЧИЙ КОД ДЛЯ КОНТУРА. СКОПИРОВАН БЕЗ ИЗМЕНЕНИЙ. ---
        int16_t f = 1 - r;
        int16_t ddF_x = 1;
        int16_t ddF_y = -2 * r;
        int16_t x = 0;
        int16_t y = r;
        
        _drawPixel(x0, y0 + r, mode);
        _drawPixel(x0, y0 - r, mode);
        _drawPixel(x0 + r, y0, mode);
        _drawPixel(x0 - r, y0, mode);

        while (y >= x) {
            _drawPixel(x0 + x, y0 + y, mode);
            _drawPixel(x0 - x, y0 + y, mode);
            _drawPixel(x0 + x, y0 - y, mode);
            _drawPixel(x0 - x, y0 - y, mode);

            if (x != y) {
                _drawPixel(x0 + y, y0 + x, mode);
                _drawPixel(x0 - y, y0 + x, mode);
                _drawPixel(x0 + y, y0 - x, mode);
                _drawPixel(x0 - y, y0 - x, mode);
            }
            
            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;
        }
    }
}

void SavaOLED_ESP32::rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode, bool fill) {
    if (w <= 0 || h <= 0) return;
	if (mode == ERASE_BORDER && fill) {
        rect(x, y, w, h, ERASE, true);    // Шаг 1: Стираем всё внутри (черный прямоугольник)
        rect(x, y, w, h, ADD_UP, false);  // Шаг 2: Рисуем белую рамку поверх
        return;
    }
    if (fill) {
        // Заливка: просто рисуем стопку горизонтальных линий
        for (int16_t i = y; i < y + h; i++) {
            hLine(x, i, w, mode);
        }
    } else {
        // Контур: 4 линии
        hLine(x+1, y, w - 2, mode);          // Верхняя
        hLine(x+1, y + h - 1, w - 2 , mode);  // Нижняя
        vLine(x, y, h, mode);          // Левая
        vLine(x + w - 1 , y, h, mode);  // Правая
    }
}

void SavaOLED_ESP32::rectR(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t mode, bool fill) {
    if (w <= 0 || h <= 0) return;
    if (r < 0) r = 0;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;

	// --- СПЕЦ-РЕЖИМ: Очистка фона + Белая рамка ---
    if (mode == ERASE_BORDER && fill) {
        rectR(x, y, w, h, r, ERASE, true);   // Шаг 1: Стираем залитую фигуру
        rectR(x, y, w, h, r, ADD_UP, false); // Шаг 2: Рисуем контур
        return;
    }

    if (fill) {
        // --- НОВЫЙ АЛГОРИТМ ЗАЛИВКИ ИЗ ТРЕХ ЧАСТЕЙ ---

        // Часть 1: Заливаем центральный прямоугольник
        rect(x, y + r, w, h - 2 * r, mode, true);

        // Часть 2 и 3: Заливаем верхнюю и нижнюю "шапки"
        // Для этого используем надежный двухпроходный алгоритм из fillCircle

        // --- Проход 1: Вычисление идеальных ширин для дуг ---
        uint8_t half_widths[r + 1];
        memset(half_widths, 0, sizeof(half_widths));
        
        int16_t f = 1 - r;
        int16_t ddF_x = 1;
        int16_t ddF_y = -2 * r;
        int16_t x_calc = 0;
        int16_t y_calc = r;

        while (y_calc >= x_calc) {
            if (half_widths[y_calc] < x_calc) half_widths[y_calc] = x_calc;
            if (half_widths[x_calc] < y_calc) half_widths[x_calc] = y_calc;
            if (f >= 0) { y_calc--; ddF_y += 2; f += ddF_y; }
            x_calc++; ddF_x += 2; f += ddF_x;
        }

        // --- Проход 2: Отрисовка "шапок" ---
        for (int16_t i = 0; i < r; i++) {
            uint8_t hw = half_widths[r - i];
            int16_t line_w = w - 2 * (r - hw);
            int16_t line_x = x + (r - hw);
            
            // Рисуем линию для верхней "шапки"
            hLine(line_x, y + i, line_w, mode);
            // Рисуем линию для нижней "шапки"
            hLine(line_x, y + h - 1 - i, line_w, mode);
        }
    } else {
        // --- КОНТУР ---
        // Рисуем 4 прямых отрезка
        hLine(x + r, y, w - 2 * r, mode);          // Верхняя
        hLine(x + r, y + h - 1, w - 2 * r, mode);  // Нижняя
        vLine(x, y + r, h - 2 * r, mode);          // Левая
        vLine(x + w - 1, y + r, h - 2 * r, mode);  // Правая

        // Рисуем 4 угловые дуги
        _drawQuarterCircle(x + r, y + r, r, 1, mode);             // Верхний левый
        _drawQuarterCircle(x + w - r - 1, y + r, r, 0, mode);     // Верхний правый
        _drawQuarterCircle(x + w - r - 1, y + h - r - 1, r, 3, mode); // Нижний правый
        _drawQuarterCircle(x + r, y + h - r - 1, r, 2, mode);     // Нижний левый
    }
}


void SavaOLED_ESP32::drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint8_t mode) {
    // Проверка, находится ли битмап полностью за пределами экрана
    if ((x >= _width) || (y >= _height) || ((x + w) <= 0) || ((y + h) <= 0)) {
        return;
    }

    for (int16_t j = 0; j < w; j++) {
        for (int16_t i = 0; i < h; i++) {
            // Вычисляем абсолютные координаты пикселя на экране
            int16_t screen_x = x + j;
            int16_t screen_y = y + i;

            // ОБЯЗАТЕЛЬНО: Проверка границ перед ручной записью в буфер
            if (screen_x < 0 || screen_x >= _width || screen_y < 0 || screen_y >= _height) continue;

            // Получаем значение пикселя из битмапа (1 или 0)
            bool pixel_on = bitmap[ (i / 8) * w + j ] & (1 << (i % 8));

            if (pixel_on) {
                // Если пиксель активен (1) — рисуем его стандартной функцией (она отработает согласно mode)
                _drawPixel(screen_x, screen_y, mode);
            } else {
                // Если пиксель НЕ активен (0) — это важно только для режима REPLACE!
                // В режимах ADD_UP и INV_AUTO ноль — это прозрачность, ничего делать не надо.
                // А в REPLACE ноль должен СТИРАТЬ фон.
                if (mode == REPLACE) {
                    // Ручное стирание пикселя (установка в 0)
                    _buffer[screen_x + (screen_y / 8) * _width] &= ~(1 << (screen_y % 8));
                }
            }
        }
    }

}

void SavaOLED_ESP32::bezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t mode) {
    // Определяем количество шагов для отрисовки.
    // Хорошая аппроксимация - половина периметра "огибающего" полигона.
    int16_t steps = (abs(x1 - x0) + abs(y1 - y0) + abs(x2 - x1) + abs(y2 - y1));
    if (steps < 4) steps = 4; // Минимальное количество шагов для гладкости

    int16_t last_x = -1, last_y = -1;

    for (int16_t i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        float u = 1.0 - t;
        float tt = t * t;
        float uu = u * u;
        
        // Формула квадратичной кривой Безье
        float x = uu * x0 + 2.0 * u * t * x1 + tt * x2;
        float y = uu * y0 + 2.0 * u * t * y1 + tt * y2;

        int16_t ix = (int16_t)(x + 0.5); // Округляем до ближайшего целого
        int16_t iy = (int16_t)(y + 0.5);

        // Рисуем пиксель, только если он не совпадает с предыдущим,
        // чтобы избежать лишних XOR-операций на одной и той же точке.
        if (ix != last_x || iy != last_y) {
            _drawPixel(ix, iy, mode);
            last_x = ix;
            last_y = iy;
        }
    }
}

void SavaOLED_ESP32::drawPeak(int16_t x0, int16_t y0, int16_t x_peak, int16_t y_peak, int16_t x2, int16_t y2, uint8_t mode) {
    // Вычисляем "виртуальную" контрольную точку P1, чтобы кривая прошла через P_peak.
    // Формула: P1 = 2 * P_peak - (P0 + P2) / 2
    
    // Вычисляем середину отрезка P0-P2
    float mid_x = (float)(x0 + x2) / 2.0;
    float mid_y = (float)(y0 + y2) / 2.0;

    // Вычисляем контрольную точку
    int16_t control_x = (int16_t)(2.0 * x_peak - mid_x);
    int16_t control_y = (int16_t)(2.0 * y_peak - mid_y);

    // Вызываем нашу рабочую функцию bezier с вычисленной контрольной точкой
    bezier(x0, y0, control_x, control_y, x2, y2, mode);
}


//****************************************************************************************
//****************************************************************************************
//****************************************************************************************
//****************************************************************************************
//--- Приватные функции  ---
//****************************************************************************************

// Отправляем кадр по страницам (стабильнее чем одна большая транзакция)
void SavaOLED_ESP32::_displayPaged() {
    const uint8_t display_cmds[] = {
        OLED_COLUMN_ADDR, 0, (uint8_t)(_width - 1),
        OLED_PAGE_ADDR, 0, (uint8_t)((_height / 8) - 1)
    };
    _sendCommands(display_cmds, sizeof(display_cmds));
    if (!_dev_handle) {
        Serial.println("display(): device not initialized");
        return;
    }
    const uint8_t pages = _height / 8;
    for (uint8_t p = 0; p < pages; ++p) {
        _tx_buffer[0] = 0x40; // Управляющий байт для данных
        memcpy(&_tx_buffer[1], &_buffer[p * _width], _width);
        esp_err_t ret = i2c_master_transmit(_dev_handle, _tx_buffer, _width + 1, 500);
        if (ret != ESP_OK) {
            Serial.printf("Error in display(): sending page %u. Code: %s\n", (unsigned)p, esp_err_to_name(ret));
            // Продолжаем попытки отправки остальных страниц
        }
    }
}

void SavaOLED_ESP32::_displayFullBuffer() { 
    const uint8_t display_cmds[] = {  
        OLED_COLUMN_ADDR, 0, (uint8_t)(_width - 1),  
        OLED_PAGE_ADDR, 0, (uint8_t)((_height / 8) - 1)  
    };  
    _sendCommands(display_cmds, sizeof(display_cmds));  
    if (!_dev_handle) {  
        Serial.println("_displayFullBuffer(): device not initialized");  
        return;  
    }  
    _tx_buffer[0] = 0x40; // Управляющий байт для данных  
    memcpy(&_tx_buffer[1], _buffer, _bufferSize);  
    esp_err_t ret = i2c_master_transmit(_dev_handle, _tx_buffer, _bufferSize + 1, 1000);  
    if (ret != ESP_OK) {  
        Serial.printf("Error in _displayFullBuffer(): %s\n", esp_err_to_name(ret));  
    }  
}

void SavaOLED_ESP32::_sendCommands(const uint8_t* cmds, uint8_t len) {
    if (!_dev_handle) {
        Serial.println("_sendCommands(): device not initialized");
        return;
    }
    if (len == 0) return;

    constexpr size_t MAX_CMD_LEN = 32;
    if (len > MAX_CMD_LEN) {
        Serial.println("_sendCommands(): command length too large");
        return;
    }

    uint8_t cmd_buffer[MAX_CMD_LEN + 1] = { 0 };
    // Первый байт - управляющий, говорит что дальше идут команды
    cmd_buffer[0] = 0x00; 
    // Копируем все команды из Flash-памяти в наш буфер
    memcpy_P(&cmd_buffer[1], cmds, len);
    // Отправляем весь буфер (управляющий байт + все команды) за одну транзакцию
    esp_err_t ret = i2c_master_transmit(_dev_handle, cmd_buffer, len + 1, 100);
    if (ret != ESP_OK) {
        Serial.printf("Error sending commands. Code: %s\n", esp_err_to_name(ret));
    }
}

uint16_t SavaOLED_ESP32::_getCharIndex(const Font* font, uint16_t char_code) {
    if (!font) return 0xFFFF;

    // --- Тип 0: Numbers (Цифры и спецсимволы) ---
    if (font->font_index == 0) {
        if (char_code >= '0' && char_code <= '9') return char_code - '0' + 1; // '0' -> 1
        if (char_code == '.') return 0;
        if (char_code == ':') return 11;
        if (char_code == '-') return 12;
        // Тут можно добавить иконку батарейки или другие спецзнаки
        return 0xFFFF;
    }

    // --- Тип 1: General Hybrid (Сквозной: ASCII + Cyrillic) ---
    if (font->font_index == 1) {
        // ASCII (0x20..0x7E) -> Индексы 0..94
        if (char_code >= 0x20 && char_code <= 0x7E) {
            return char_code - 0x20;
        }
        // Кириллица CP1251 (0xC0..0xFF) -> Индексы 95..158
        if (char_code >= 0xC0 && char_code <= 0xFF) {
            return char_code - 0xC0 + 95;
        }
        // Спецсимволы Ё/ё
        if (char_code == 0xA8) return 159; // Ё (по нашей таблице)
        if (char_code == 0xB8) return 160; // ё (по нашей таблице)
        
        return 0xFFFF;
    }

    return 0xFFFF;
}



void SavaOLED_ESP32::_drawPixel(int16_t x, int16_t y, uint8_t mode) { // -- эта строку изменить
    if (x < 0 || x >= _width || y < 0 || y >= _height) {
        return;
    }
    uint16_t byte_index = x + (y / 8) * _width;
    uint8_t bit_pos = y % 8;

	switch (mode) {
        case ERASE_BORDER:
		case ADD_UP:
        case REPLACE: // Для одиночного белого пикселя REPLACE и ADD_UP делают одно и то же (ставят 1)
            _buffer[byte_index] |= (1 << bit_pos);
            break;
        case INV_AUTO:
            _buffer[byte_index] ^= (1 << bit_pos);
            break;
        case ERASE: // Новый режим: принудительная очистка бита (рисуем черным)
            _buffer[byte_index] &= ~(1 << bit_pos);
            break;
    }
}

void SavaOLED_ESP32::_drawQuarterCircle(int16_t x0, int16_t y0, int16_t r, uint8_t corner, uint8_t mode) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    // --- ФИЛЬТРУЕМ ВЫВОД НАЧАЛЬНЫХ ТОЧЕК ---
    if (corner == 0) _drawPixel(x0 + r, y0, mode); // Верхний правый
    if (corner == 1) _drawPixel(x0 - r, y0, mode); // Верхний левый
    if (corner == 2) _drawPixel(x0 - r, y0, mode); // Нижний левый
    if (corner == 3) _drawPixel(x0 + r, y0, mode); // Нижний правый

    if (corner == 0) _drawPixel(x0, y0 - r, mode); // Верхний правый
    if (corner == 1) _drawPixel(x0, y0 - r, mode); // Верхний левый
    if (corner == 2) _drawPixel(x0, y0 + r, mode); // Нижний левый
    if (corner == 3) _drawPixel(x0, y0 + r, mode); // Нижний правый

    // --- ФИЛЬТРУЕМ ВЫВОД ТОЧЕК ИЗ ЦИКЛА ---
    while (y >= x) {
        // Фильтруем первую группу отражений
        if (corner == 3) _drawPixel(x0 + x, y0 + y, mode); // Нижний правый
        if (corner == 2) _drawPixel(x0 - x, y0 + y, mode); // Нижний левый
        if (corner == 0) _drawPixel(x0 + x, y0 - y, mode); // Верхний правый
        if (corner == 1) _drawPixel(x0 - x, y0 - y, mode); // Верхний левый

        // Фильтруем вторую группу отражений
        if (x != y) {
            if (corner == 3) _drawPixel(x0 + y, y0 + x, mode); // Нижний правый
            if (corner == 2) _drawPixel(x0 - y, y0 + x, mode); // Нижний левый
            if (corner == 0) _drawPixel(x0 + y, y0 - x, mode); // Верхний правый
            if (corner == 1) _drawPixel(x0 - y, y0 - x, mode); // Верхний левый
        }
        
        // Внутренняя логика "черного ящика" остается НЕИЗМЕННОЙ
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
    }
}