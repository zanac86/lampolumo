#ifndef OLEDFONTS_H_
#define OLEDFONTS_H_

#include <stdint.h>

#define fontbyte(x) Font.font[x]

typedef const uint8_t fontdatatype;
typedef enum
{
    NORMAL = 0,
    INVERT = 1
} OLEDinverted;

typedef enum
{
    OLED_LEFT   = 253,
    OLED_RIGHT  = 254,
    OLED_CENTER = 255
} OLEDalign;

typedef struct
{
    const uint8_t* font;
    int16_t width;
    int16_t height;
    int16_t offset;
    int16_t numchars;
    OLEDinverted inverted;
} font_t;

extern font_t Font;

extern fontdatatype Font_MSX_6x8_rus1251[];

void OLED_FontSet(const uint8_t* font);

#endif /* OLEDFONTS_H_ */
