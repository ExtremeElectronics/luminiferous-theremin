#include <string.h>
#include "hardware/i2c.h"

//position of lines in display
#define STATUSLINE3 52
#define STATUSLINE2 41
#define STATUSLINE1 30
#define STATUSLINEP 43
//display width in chars (allowing for border)
#define DISPLAYWIDTH 14


/// Register addresses from datasheet
enum SSD1306_REG_ADDRESSES {
    SSD1306_CONTRAST = 0x81,
    SSD1306_DISPLAYALL_ON_RESUME = 0xA4,
    SSD1306_DISPLAYALL_ON = 0xA5,
    SSD1306_INVERTED_OFF = 0xA6,
    SSD1306_INVERTED_ON = 0xA7,
    SSD1306_DISPLAY_OFF = 0xAE,
    SSD1306_DISPLAY_ON = 0xAF,
    SSD1306_DISPLAYOFFSET = 0xD3,
    SSD1306_COMPINS = 0xDA,
    SSD1306_VCOMDETECT = 0xDB,
    SSD1306_DISPLAYCLOCKDIV = 0xD5,
    SSD1306_PRECHARGE = 0xD9,
    SSD1306_MULTIPLEX = 0xA8,
    SSD1306_LOWCOLUMN = 0x00,
    SSD1306_HIGHCOLUMN = 0x10,
    SSD1306_STARTLINE = 0x40,
    SSD1306_MEMORYMODE = 0x20,
    SSD1306_MEMORYMODE_HORZONTAL = 0x00,
    SSD1306_MEMORYMODE_VERTICAL = 0x01,
    SSD1306_MEMORYMODE_PAGE = 0x10,
    SSD1306_COLUMNADDR = 0x21,
    SSD1306_PAGEADDR = 0x22,
    SSD1306_COM_REMAP_OFF = 0xC0,
    SSD1306_COM_REMAP_ON = 0xC8,
    SSD1306_CLUMN_REMAP_OFF = 0xA0,
    SSD1306_CLUMN_REMAP_ON = 0xA1,
    SSD1306_CHARGEPUMP = 0x8D,
    SSD1306_EXTERNALVCC = 0x1,
    SSD1306_SWITCHCAPVCC = 0x2,
};

/// \enum pico_ssd1306::Size
enum SSD1306_Size {
    /// Display size W128xH64
    SSD1306_W128xH64,
    /// Display size W128xH32
    SSD1306_W128xH32
};

/// \enum pico_ssd1306::WriteMode
enum SSD1306_WriteMode{
    /// sets pixel on regardless of its state
    SSD1306_ADD = 0,
    /// sets pixel off regardless of its state
    SSD1306_SUBTRACT = 1,
    /// inverts pixel, so 1->0 or 0->1
    SSD1306_INVERT = 2,
    /// overwrites (must be on 8 bit boundries
    SSD1306_OVERWRITE = 3
    
};

enum SSD1306_Rotation{
    SSD1306_DEG0,
    SSD1306_DEG90
};



void SSD1306_cmd(unsigned char command);

void SSD1306_init(uint16_t SSD1306_Address, enum SSD1306_Size size);

void SSD1306_setPixel(int16_t x, int16_t y, enum SSD1306_WriteMode mode);

void SSD1306_sendBuffer();

void SSD1306_addBitmapImage(int16_t anchorX, int16_t anchorY, uint8_t image_width, uint8_t image_height, uint8_t *image,
                    enum SSD1306_WriteMode mode);

void SSD1306_setBuffer(unsigned char *buffer);

void SSD1306_setOrientation(bool orientation);


void SSD1306_clear();

void SSD1306_invertDisplay();

void SSD1306_setContrast(unsigned char contrast);

void SSD1306_turnOff();

void SSD1306_turnOn();
    
void drawText( const char *text, uint8_t anchor_x,uint8_t anchor_y);

void SSD1306_drawText(const unsigned char *font, const char *text, uint8_t anchor_x,
                  uint8_t anchor_y, enum SSD1306_WriteMode mode, enum SSD1306_Rotation rotation);

void SSD1306_drawChar(const unsigned char *font, char c, uint8_t anchor_x, uint8_t anchor_y,
                  enum SSD1306_WriteMode mode, enum SSD1306_Rotation rotation) ;
                  
void SSD1306_background_image(unsigned char *bitmap);                  

void drawStatus( const char *text,uint8_t anchor_y);

void SSD1306_fillRect( uint8_t x_start, uint8_t y_start, uint8_t x_end, uint8_t y_end,enum SSD1306_WriteMode mode);

void DispConn(char * text);

void drawStatusBorder(char * text,uint8_t anchor_y,uint8_t border);

void drawStatusCentered(char * text,uint8_t anchor_y, uint8_t border);

void setpixel(uint16_t x,uint16_t y,uint8_t onoff);

