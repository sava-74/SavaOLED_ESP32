#ifndef SAVAOLED_TYPES_H
#define SAVAOLED_TYPES_H


// Команды для SSD1306
#define OLED_DISPLAY_OFF        0xAE
#define OLED_DISPLAY_ON         0xAF
#define OLED_SET_CHARGE_PUMP    0x8D
#define OLED_SET_MUX_RATIO      0xA8
#define OLED_SET_DISPLAY_OFFSET 0xD3
#define OLED_SET_DISPLAY_START_LINE 0x40
#define OLED_SET_SEGMENT_REMAP  0xA1
#define OLED_SET_COM_SCAN_DEC   0xC8
#define OLED_SET_COM_PINS       0xDA
#define OLED_SET_CONTRAST       0x81
#define OLED_SET_NORMAL_DISPLAY 0xA6
#define OLED_SET_PRECHARGE      0xD9
#define OLED_SET_VCOM_DESELECT  0xDB
#define OLED_SET_CLOCK_DIV      0xD5
#define OLED_COLUMN_ADDR        0x21
#define OLED_PAGE_ADDR          0x22
#define OLED_FLIP_V     0xC0
#define OLED_NORMAL_V   0xC8
#define OLED_FLIP_H     0xA0
#define OLED_NORMAL_H   0xA1
#define OLED_INVERTDISPLAY 0xA7

#define NO_FILL false
#define FILL true

#define ADD_UP 0 
#define INV_AUTO 1
#define REPLACE 2
#define ERASE 3
#define ERASE_BORDER 4

#define FULL_FRAME true
#define PAGES_FRAME false

// Уникальные имена для режимов выравнивания, как вы и предложили
#define StrLeft   0
#define StrCenter 1
#define StrRight  2
#define StrScroll 3
#define StrUp     0
#define StrDown   2

#endif // SAVAOLED_TYPES_H