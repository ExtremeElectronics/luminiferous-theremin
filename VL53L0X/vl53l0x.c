#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//pico headers
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"

#include "vl53l0x.h"
#include "tof.h"

#include "../settings.h"

void init_vl53l0x(uint8_t iA){
    int model, revision;
    
    if (tofInit(iA,0)){
        printf("laser init %x OK\n",iA);
        int i = tofGetModel(&model, &revision,iA);	
        printf("Model ID - %d\n", model);	
        printf("Revision ID - %d\n", revision);
    }        


}


