#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#include "ledline.c"


//const int PIXEL = 0;

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) |
           ((uint32_t)(g) << 16) |
           (uint32_t)(b);
}


void init_NeoPixel(void){
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    char str[12];

    ws2812_program_init(pio, sm, offset, PIXEL, 800000, true);

}

//value 0-LEDMAX intencity 0-270
void neopixel_fromline(uint16_t value,uint16_t intencity){
   if ((intencity<1) || (intencity>270) || (value<1) || (value>LEDMAX) ) {
       put_pixel(0);   
   }else{
       uint8_t r=intencity*colline[value][0]/256;   
       uint8_t g=intencity*colline[value][1]/256;
       uint8_t b=intencity*colline[value][2]/256;
       put_pixel(urgb_u32(r,g,b));
   }
}
