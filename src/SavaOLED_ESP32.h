#ifndef SavaOLED_ESP32_h
#define SavaOLED_ESP32_h

#include <Arduino.h>
// Подключаем заголовочный файл нового нативного драйвера I2C
#include "driver/i2c_master.h"
#include "SavaOLED_types.h"


struct savaFont {
    const uint8_t* data;       // Указатель на массив данных (Ширина + Байты)
    const uint16_t* offsets;   // Указатель на таблицу смещений (Где искать символ)
    uint8_t height;            // Высота символа в пикселях
    uint8_t font_index;        // Тип: 0=Numbers (Спец), 1=General (ASCII+CP1251)
};

struct TextSegment {
    const char* text;          // Буфер для фрагмента текста (не владеет памятью)
    const savaFont* fontPtr;          // Указатель на шрифт для этого фрагмента
};

struct DisplayListItem {
    const uint8_t* font_data; // Указатель на начало данных символа
    uint8_t char_width;       // Ширина этого символа в байтах/колонках
    uint8_t mode;             // Режим заливки
	const savaFont* fontPtr;		  // Указатель на шрифт для этого фрагмента
};

class SavaOLED_ESP32 {
public:

    /**
    * @brief Конструктор с параметрами I2C.
    * @param width - ширина экрана в пикселях (по умолчанию 128).
    * @param height - высота экрана в пикселях (по умолчанию 64).
    * @param segmentBufferSize - размер буфера сегмента/строки (по умолчанию 48).
    * @param port - i2c_port_t порт (I2C_NUM_0 или I2C_NUM_1).
    */
    SavaOLED_ESP32(uint8_t width = 128, uint8_t height = 64, i2c_port_t port = I2C_NUM_0);
	/**
    * @brief Деструктор. Освобождает все выделенные ресурсы.
    */
	~SavaOLED_ESP32() noexcept;

    // Запрет неявного копирования (класс владеет ресурсами)
    SavaOLED_ESP32(const SavaOLED_ESP32&) = delete;
    SavaOLED_ESP32& operator=(const SavaOLED_ESP32&) = delete;

    // --- Основные функции ---
	/**
    * @brief Установить I2C-адрес устройства.
    * @param address - адрес (по умолчанию 0x3C, возможен 0x3D).
    */
	void setAddress(uint8_t address = 0x3C);
    /**
    * @brief Инициализация I2C и дисплея.
    * @param freq - частота шины (по умолчанию 400000).
    * @param _sda - пин SDA (по умолчанию 21).
    * @param _scl - пин SCL (по умолчанию 22).
    */
    void init(uint32_t freq = 400000, int8_t _sda = 21, int8_t _scl = 22);
    
	//##############################################################################################################
	//##############################################################################################################
	// --- ПРИМИТИВЫ ---
	//##############################################################################################################
	//##############################################################################################################
	/**
    * @brief Нарисовать точку в кадровом буфере.
    * @param x - координата X (0..width-1).
    * @param y - координата Y (0..height-1).
    * @param mode - 1 = авто-инверсия/XOR, 0 = наложение/OR.
    */
    void dot(int16_t x, int16_t y, uint8_t mode = REPLACE);
	
	/**
    * @brief Нарисовать линию по алгоритму Брезенхэма.
    * @param x1 - X координата начальной точки.
    * @param y1 - Y координата начальной точки.
    * @param x2 - X координата конечной точки.
    * @param y2 - Y координата конечной точки.
    * @param mode - режим отрисовки (REPLACE = очистит и поверх/INV_AUTO = авто-инверсия/ADD_UP = наложение/ERASE = ластик/ERASE_BORDER= рамка удаляет под собой).
    */
    void line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t mode = REPLACE);
	
	/**
    * @brief Нарисовать быструю горизонтальную линию.
    * @param x - X координата начальной точки.
    * @param y - Y координата линии.
    * @param w - ширина линии.
    * @param mode - режим отрисовки (REPLACE = очистит и поверх/INV_AUTO = авто-инверсия/ADD_UP = наложение/ERASE = ластик/ERASE_BORDER= рамка удаляет под собой).
    */
    void hLine(int16_t x, int16_t y, int16_t w, uint8_t mode = REPLACE);
	
	/**
    * @brief Нарисовать быструю вертикальную линию.
    * @param x - X координата линии.
    * @param y - Y координата начальной точки.
    * @param h - высота линии.
    * @param mode - режим отрисовки (REPLACE = очистит и поверх/INV_AUTO = авто-инверсия/ADD_UP = наложение/ERASE = ластик/ERASE_BORDER= рамка удаляет под собой).
    */
    void vLine(int16_t x, int16_t y, int16_t h, uint8_t mode = REPLACE);
	
    /**
    * @brief Нарисовать круг (контур или залитый).
    * @param x0 - X координата центра.
    * @param y0 - Y координата центра.
    * @param r - радиус.
    * @param mode - режим отрисовки (REPLACE = очистит и поверх/INV_AUTO = авто-инверсия/ADD_UP = наложение/ERASE = ластик/ERASE_BORDER= рамка удаляет под собой).
    * @param fill - true = залить круг, false = нарисовать контур.
    */
    void circle(int16_t x0, int16_t y0, int16_t r, uint8_t mode = REPLACE, bool fill = NO_FILL);
	
	/**
    * @brief Нарисовать прямоугольник (контур или залитый).
    * @param x - X координата верхнего левого угла.
    * @param y - Y координата верхнего левого угла.
    * @param w - ширина.
    * @param h - высота.
    * @param mode - режим отрисовки (REPLACE = очистит и поверх/INV_AUTO = авто-инверсия/ADD_UP = наложение/ERASE = ластик/ERASE_BORDER= рамка удаляет под собой).
    * @param fill - FILL = залить прямоугольник, NO_FILL = нарисовать контур.
    */
    void rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode = REPLACE, bool fill = NO_FILL);
	
	/**
    * @brief Нарисовать прямоугольник со скругленными углами.
    * @param x - X координата верхнего левого угла.
    * @param y - Y координата верхнего левого угла.
    * @param w - ширина.
    * @param h - высота.
    * @param r - радиус скругления углов.
    * @param mode - режим отрисовки (REPLACE = очистит и поверх/INV_AUTO = авто-инверсия/ADD_UP = наложение/ERASE = ластик/ERASE_BORDER= рамка удаляет под собой).
    * @param fill - FILL = залить фигуру, NO_FILL = нарисовать контур.
    */
    void rectR(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t mode = REPLACE, bool fill = NO_FILL);
	
	 /**
    * @brief Нарисовать треугольник (контур или залитый).
    * @param x0, y0 - Координаты первой вершины.
    * @param x1, y1 - Координаты второй вершины.
    * @param x2, y2 - Координаты третьей вершины.
    * @parammode - режим отрисовки (REPLACE = очистит и поверх/INV_AUTO = авто-инверсия/ADD_UP = наложение/ERASE = ластик/ERASE_BORDER= рамка удаляет под собой).
    * @param fill - FILL = залить треугольник, false = нарисовать контур.
    */
   // void triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t mode = REPLACE, bool fill = false);
   
   /**
    * @brief Нарисовать монохромный битмап.
    * @param x - X координата верхнего левого угла.
    * @param y - Y координата верхнего левого угла.
    * @param bitmap - указатель на массив с данными изображения.
    * @param w - ширина битмапа в пикселях.
    * @param h - высота битмапа в пикселях.
    * @parammode - режим отрисовки (REPLACE = очистит и поверх/INV_AUTO = авто-инверсия/ADD_UP = наложение/ERASE = ластик/ERASE_BORDER= рамка удаляет под собой).
    * @note Формат битмапа: вертикальные байты, младший бит вверху.
    *       Массив должен быть организован по колонкам.
    */
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint8_t mode = REPLACE);
	
	/**
    * @brief Быстро залить весь кадровый буфер повторяющимся узором.
    * @param pattern - байт-узор для вертикальной колонки из 8 пикселей.
    * @note 0x00 - очистка, 0xFF - полная заливка, 0xAA/0x55 - шахматка.
    */
    void fillScreen(uint8_t pattern);
	
	/**
    * @brief Нарисовать квадратичную кривую Безье.
    * @param x0, y0 - Координаты начальной точки.
    * @param x1, y1 - Координаты контрольной точки.
    * @param x2, y2 - Координаты конечной точки.
    * @parammode - режим отрисовки (REPLACE = очистит и поверх/INV_AUTO = авто-инверсия/ADD_UP = наложение/ERASE = ластик/ERASE_BORDER= рамка удаляет под собой).
    */
    void bezier(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t mode = REPLACE);
	
	/**
    * @brief Нарисовать кривую, проходящую через три заданные точки (для графиков).
    * @param x0, y0 - Координаты начальной точки.
    * @param x_peak, y_peak - Координаты вершины (пика), через которую пройдет кривая.
    * @param x2, y2 - Координаты конечной точки.
    * @param mode - режим отрисовки (REPLACE = очистит и поверх/INV_AUTO = авто-инверсия/ADD_UP = наложение/ERASE = ластик/ERASE_BORDER= рамка удаляет под собой).
    */
    void drawPeak(int16_t x0, int16_t y0, int16_t x_peak, int16_t y_peak, int16_t x2, int16_t y2, uint8_t mode = REPLACE);
	
	//##############################################################################################################
	//##############################################################################################################
	//
	//##############################################################################################################
	//##############################################################################################################
	
    /**
    * @brief Отправить текущий кадровый буфер на дисплей.
    * Обычно вызывается централизованно в loop() для регулярного обновления.
    */
    void display();
 
	/**
    * @brief Очистить кадровый буфер (установить все биты в 0).
    */
	void clear();

    /**
    * @brief Установить текущий шрифт для последующих операций print.
    * @param fontPtr - ссылка на структуру `fontPtr`.
    */
	void font(const savaFont &fontPtr);
	
    /**
    * @brief Установить режим отрисовки для текста и примитивов.
    * @param mode - режим отрисовки (REPLACE = очистит и поверх/INV_AUTO = авто-инверсия/ADD_UP = наложение).
    */
    void drawMode(uint8_t mode = REPLACE);
	
	/** // -- добавит эту строку
    * @brief Установить межсимвольный интервал для последующих print. 
    * @param spacing - количество пикселей между символами (по умолчанию 1).
    */ 
    void charSpacing(uint8_t spacing);
	
	/**
    * @brief Включить/выключить режим скроллинга для текущей строки.
    * @param enabled - true = включить, false = выключить.
    */
	void scroll(bool enabled);
	
	/**
    * @brief Установить скорость и режим скроллинга.
    * @param speed - скорость от 1 (медленно) до 10 (быстро).
    * @param loop - true = зациклить, false = проиграть один раз.
    */
    void scrollSpeed(uint8_t speed = 3, bool loop = true);

    /**
    * @brief Установить скорость вертикального скроллинга.
    * @param speed - скорость от 1 до 10.
    */
    void scrollSpeedVert(uint8_t speed = 3);

	/** // -- добавит эту строку
    * @brief Установить режим отправки кадрового буфера. 
    * @param enabled - FULL = отправлять одним блоком (быстро), PAGES = постранично (стабильно).
    */ 
    void setBuffer(bool enabled);

//#############################################################################################################################
// --- Преобразование типов ---
//#############################################################################################################################

	/**
    * @brief Добавить C-строку в текущий буфер печати.
    * @param text - нуль-терминированная строка.
    */
	void print(const char* text); 
    
	/**
    * @brief Добавить целое число (int) в буфер печати с опциональным форматированием.
    * @param value - значение для вывода.
    * @param min_digits - (опционально) минимальное количество знаков (дополняется ведущими нулями).
    */
    void print(int value, uint8_t min_digits = 0);
	
	/**
    * @brief Добавить целое число (int8_t -128 | 127) в буфер печати.
    *@param min_digits - (опционально) минимальное количество знаков (дополняется ведущими нулями).
	*/
    void print(int8_t value, uint8_t min_digits = 0);
	
	/**
    * @brief Добавить целое число (uint8_t(byte) 0 | 255) в буфер печати.
    *@param min_digits - (опционально) минимальное количество знаков (дополняется ведущими нулями).
	*/
	void print(uint8_t value, uint8_t min_digits = 0);
	
	/**
    * @brief Добавить целое число (int16_t(short) -32 768 | 32 767) в буфер печати.
    *@param min_digits - (опционально) минимальное количество знаков (дополняется ведущими нулями).
	*/
	void print(int16_t value, uint8_t min_digits = 0);
	
	/**
    * @brief Добавить целое число (uint16_t(unsigned short) 0 | 65 535) ) в буфер печати.
    *@param min_digits - (опционально) минимальное количество знаков (дополняется ведущими нулями).
	*/
	void print(uint16_t value, uint8_t min_digits = 0);
	
	/**
    * @brief Добавить целое число (uint32_t(unsigned long) 0 | 4 294 967 295) в буфер печати.
    *@param min_digits - (опционально) минимальное количество знаков (дополняется ведущими нулями).
	*/
	void print(uint32_t value, uint8_t min_digits = 0);
    
	/**
    * @brief Добавить целое число (int32_t(long) -2 147 483 648 | 2 147 483 647) в буфер печати.
    *@param min_digits - (опционально) минимальное количество знаков (дополняется ведущими нулями).
	*/
    void print(int32_t value, uint8_t min_digits = 0); 
    
	/**
    * @brief Добавить число с плавающей точкой (float) в буфер печати.
    * @param value - значение.
    * @param decimalPlaces - количество знаков после запятой.
	*@param min_width - (опционально) минимальная общая ширина вывода (дополняется пробелами слева).
    */
    void print(float value, uint8_t decimalPlaces = 2, uint8_t min_width = 0); 
    
	/**
    * @brief Добавить double в буфер печати.
    *@param min_width - (опционально) минимальная общая ширина вывода (дополняется пробелами слева).
	*/
    void print(double value, uint8_t decimalPlaces = 2, uint8_t min_width = 0); 
    
	/**
    * @brief Добавить объект String в буфер печати.
    */
    void print(const String &s); 
//#############################################################################################################################
//#############################################################################################################################
    /**
    * @brief Завершить накопление строки и подготовить её для отображения.
    * Запускает отрисовку содержимого внутреннего текстового буфера в кадровый буфер.
    */
    void drawPrint();
	
    /**
    * @brief Вывод вертикального текста (сверху вниз).
    * Использует текущие настройки шрифта. Шрифт должен быть повернут на 90 градусов.
    */
    void drawPrintVert();

	/**
    * @brief Установить позицию курсора и режим выравнивания для следующих print().
    * @param x - координата X (точка привязки или левая граница).
    * @param y - координата Y.
    * @param align - режим выравнивания (StrLeft, StrCenter, StrRight).
    * @param x2 - правая граница диапазона X (по умолчанию -1 -> ширина экрана).
    */
	void cursor(int16_t x, int16_t y, uint8_t align = StrLeft, int16_t x2 = -1);
    
	/**
    * @brief Получить текущую X позицию курсора.
    */
	int16_t getCursorX() const;
    
	/**
    * @brief Получить текущую Y позицию курсора.
    */
	int16_t getCursorY() const;

    /**< @brief Получить ширину текущей строки в пикселях */
    uint16_t getTextWidth() const;
    /**< @brief Получить высоту текущей строки в пикселях */
    uint16_t getTextHeight() const;
    //****************************************************************************************
    //--- Публичные функции управления дисплеем ---
    /**
    * @brief Установить контраст (яркость) дисплея.
    * @param value - 0..255.
    */
    void contrast(uint8_t value);      // 0..255
    /**
    * @brief Включить или выключить питание/вывод дисплея.
    * @param mode - true = включить, false = выключить.
    */
    void power(bool mode);             // включить/выключить
    /**
    * @brief Отразить экран по горизонтали аппаратно (SEG remap).
    * @param mode - true = отразить, false = нормальная ориентация.
    */
    void flipH(bool mode);                // горизонтальное отражение (аппаратно)
    /**
    * @brief Отразить экран по вертикали аппаратно (COM scan direction).
    * @param mode - true = отразить, false = нормальная ориентация.
    */
    void flipV(bool mode);                // вертикальное отражение (аппаратно)
    /**
    * @brief Включить или выключить аппаратную инверсию экрана.
    * @param mode - true = инвертировать, false = нормальный режим.
    */
    void invertDisplay(bool mode);        // аппаратная инверсия (A7 / A6)
    /**
    * @brief Установить аппаратный поворот экрана на 0 или 180 градусов.
    * @param rotate180 - true или false.
    */
    void rotation(bool rotate180); 

private:
    // --- Внутренние функции ---
	/**
    * @brief Отправить массив команд контроллеру через I2C.
    * @param cmds - указатель на команды.
    * @param len - количество байт команд.
    */
	void _sendCommands(const uint8_t* cmds, uint8_t len);
	/**
    * @brief Получить индекс символа в шрифте по коду символа.
    * @param fontPtr - указатель на используемый шрифт.
    * @param char_code - код символа (может быть CP1251 для кириллицы).
    * @return индекс в таблице шрифта или 0xFFFF если символ не найден.
    */
	uint16_t _getCharIndex(const savaFont* fontPtr, uint16_t char_code);
	
	/**
    * @brief Внутренняя функция для отрисовки пикселя с разными режимами.
    * @param x - координата X.
    * @param y - координата Y.
    * @param mode - режим отрисовки (1 = XOR, 0 = OR).
    */
	void _drawPixel(int16_t x, int16_t y, uint8_t mode);
	
	/**
    * @brief Внутренняя функция для отрисовки четверти круга (дуги).
    * @param x0 - X координата центра дуги.
    * @param y0 - Y координата центра дуги.
    * @param r - радиус.
    * @param corner - номер угла (0-3, против часовой стрелки, 0=верхний правый).
    * @param mode - режим отрисовки.
    */
	void _drawQuarterCircle(int16_t x0, int16_t y0, int16_t r, uint8_t corner, uint8_t mode);
    
	void _displayPaged();       						/**< @brief Отправка буфера по страницам (стабильный метод) */ 
    void _displayFullBuffer();  						/**< @brief Отправка буфера целиком (быстрый метод) */

	
    // --- Переменные ---
    i2c_port_t _port;            						/**< @brief Номер I2C-порта (I2C_NUM_0 / I2C_NUM_1) */
    int8_t _scl;                 						/**< @brief Номер SCL-пина, установлен через init() */
    int8_t _sda;                 						/**< @brief Номер SDA-пина, установлен через init() */
    uint8_t _address;            						/**< @brief I2C-адрес устройства (0x3C / 0x3D) */
    i2c_master_bus_handle_t _bus_handle; 				/**< @brief Дескриптор шины I2C */
    i2c_master_dev_handle_t _dev_handle; 				/**< @brief Дескриптор устройства на шине */

	const savaFont* _currentFont; 							/**< @brief Указатель на текущий выбранный шрифт */
		
	int16_t _cursorX; 									/**< @brief Текущая X позиция курсора (визуальная) */	
	int16_t _cursorY; 									/**< @brief Текущая Y позиция курсора (визуальная) */		
	uint8_t _cursorAlign; 								/**< @brief Режим выравнивания курсора (StrLeft/StrCenter/StrRight/StrScroll) */	
	int16_t _cursorX2;    								/**< @brief Правая граница диапазона X для выравнивания (или -1) */
    bool _scrollEnabled;        						/**< @brief Флаг включенного скроллинга для текущей строки */
	uint8_t _scrollSpeed;       						/**< @brief Скорость скроллинга (1..10) */
    bool _scrollLoop;           						/**< @brief Флаг зацикливания скролла */
    uint32_t _scrollOffset;     						/**< @brief Текущее смещение скролла (натуральное число, инкрементируется со временем) */
    unsigned long _lastScrollTime; 						/**< @brief Время (millis) последнего шага скролла */
	int16_t _scrollingLineY;    						/**< @brief Y координата строки, для которой ведётся скроллинг */
    uint32_t _vertScrollOffset;     					/**< @brief Текущее смещение вертикального скролла (натуральное число, инкрементируется со временем) */
    unsigned long _vertLastScrollTime; 					/**< @brief Время (millis) последнего шага вертикального скролла */
    uint8_t _vertScrollSpeed;       					/**< @brief Скорость вертикального скролла (1..10) */
	
	uint8_t* _lineBuffer;           					/**< @brief Временный бинарный буфер для рендеринга строки (по колонкам) */
    uint16_t _lineBufferWidth;      					/**< @brief Ширина _lineBuffer в колонках */
    uint8_t  _lineBufferHeightPages;					/**< @brief Высота _lineBuffer в страницах (8-строчных блоков) */
    uint16_t _currentLineWidth;     					/**< @brief Фактическая ширина отрисованной строки в _lineBuffer */

	static const uint8_t MAX_SEGMENTS = 8; 				/**< @brief Максимум 8 фрагментов с разными шрифтами на одну строку*/
    static const size_t TEXT_BUFFER_SIZE = 256; 		/**< @brief Общий размер буфера для текста всех фрагментов*/        //было 128
    TextSegment _segments[MAX_SEGMENTS];      			/**< @brief Массив сегментов для текущей строки */
    uint8_t _segmentCount;                    			/**< @brief Количество используемых сегментов в массиве */
    char _textBuffer[TEXT_BUFFER_SIZE];       			/**< @brief Общий буфер для хранения текста всех сегментов */
    size_t _textBufferPos;                    			/**< @brief Текущая позиция в общем текстовом буфере */
    bool _lineChanged;              					/**< @brief Флаг, что текст строки изменился и требует повторного рендера */
	
	uint8_t _charSpacing; 								/**< @brief Межсимвольный интервал в пикселях */
	uint8_t _drawMode; 									/**< @brief Текущий режим отрисовки (REPLACE, INV_AUTO, ADD_UP) */
	
	uint8_t _width;   									/**< @brief Ширина дисплея в пикселях */
    uint8_t _height;   									/**< @brief Высота дисплея в пикселях */
    uint16_t _bufferSize; 								/**< @brief Размер кадрового буфера в байтах (_width * _height / 8) */
	
    uint8_t* _buffer;									/**< @brief Кадровый буфер (формат страниц SSD1306) */
    uint8_t* _tx_buffer; 								/**< @brief Вспомогательный буфер передачи (включая управляющий байт) */

    bool _inverted;      								/**< @brief Состояние аппаратной инверсии экрана (true = inverted) */
    uint8_t _contrast;   								/**< @brief Текущее значение контраста (0..255) */
	bool _Buffer;      									/**< @brief Флаг режима отправки буфера (true = целиком, false = постранично) */ 

    uint8_t* _vertBuffer;                               /**< @brief Вертикальный буфер (лента) */        
    uint16_t _vertBufferHeight;                         /**< @brief Текущая высота текста в буфере */
    static const uint16_t VERT_BUF_SIZE = 2048;         /**< @brief Размер (хватит на ~500 пикселей высоты при ширине 32px) */
    
};

#endif // SavaOLED_ESP32_h