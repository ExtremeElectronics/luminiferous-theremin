/*
 * Theremin
 *
 * https://github.com/ExtremeElectronics/Theremin
 *
 * theremin.c
 *
 * Copyright (c) 2024 Derek Woodroffe <tesla@extremeelectronics.co.uk>
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//pico headers
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"

#include "pico/multicore.h"
#include <VL53L0X/tof.h>

#include "sound/sound.h"
#include "display/display.h"
#include "midi.c"

#include "settings.h"

#include "sound/waveshapes.h"

#define MINDISTANCE 20
#define MAXDISTANCE 500
#define MAXDISTANCEV 300


//globals
float frequency=1000;
uint8_t volume=255;
uint8_t extern mute;
uint8_t extern vmute;
uint8_t extern fmute;
int8_t wave=0; //0=sin 1=square 2=triinc 3=tridec 4=sawtooth
struct repeating_timer ftimer;
float newfreq=1000;

char dtemp[100];

int8_t VtofState=0;
int8_t FtofState=0;

uint16_t dtimer=0;

uint16_t fdist_old=0;
uint16_t vdist_old=0;

//features
#define FEATUREMAX 5

uint8_t feature=0;
char  features[FEATUREMAX][30]={"Normal\0","Note Only\0","Wob Slow\0","Wob Med\0","Wob Fast\0"};

uint8_t wobulate=0;

//Wobulator Sine
const uint8_t msign[64] = {
    0x7f,  0x8b,  0x98,  0xa4,
    0xb0,  0xbb,  0xc6,  0xd0,
    0xd9,  0xe2,  0xe9,  0xef,
    0xf5,  0xf9,  0xfc,  0xfe,
    0xff,  0xfe,  0xfc,  0xf9,
    0xf5,  0xef,  0xe9,  0xe2,
    0xd9,  0xd0,  0xc6,  0xbb,
    0xb0,  0xa4,  0x98,  0x8b,
    0x7f,  0x73,  0x66,  0x5a,
    0x4e,  0x43,  0x38,  0x2e,
    0x25,  0x1c,  0x15,  0x0f,
    0x09,  0x05,  0x02,  0x00,
    0x00,  0x00,  0x02,  0x05,
    0x09,  0x0f,  0x15,  0x1c,
    0x25,  0x2e,  0x38,  0x43,
    0x4e,  0x5a,  0x66,  0x73
};



void gpio_conf(){
    //FLASH LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(250);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    // Vol sensor XSHUT 
    gpio_init(VOL_XSHUT);
    gpio_set_dir(VOL_XSHUT, GPIO_OUT);
    gpio_put(VOL_XSHUT,0); //disable VOL sensor
    
    //BUTTON1
    gpio_init(BUTTON1);
    gpio_pull_up(BUTTON1);
    gpio_set_dir(BUTTON1, GPIO_IN);

    //BUTTON2
    gpio_init(BUTTON2);
    gpio_pull_up(BUTTON2);
    gpio_set_dir(BUTTON2, GPIO_IN);


}

void splash(){
    printf("\n\nTheremin\n");
}

void init_I2C(void){
    //init i2c

    //SDA
    gpio_init(I2C_SDA_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    //SCL
    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);

    i2c_init(I2CInst, I2C_BAUDRATE);
}


//********************************* Core1 ************************************
//void Core1Main(){
//   Sound_Loop();
//}

//****************************** smooth out f changes ******************
bool Freq_Timer_Callback(struct repeating_timer *t){
    if (feature==0){
        if (abs(frequency-newfreq)>1){
            newfreq+=(frequency-newfreq)/4;
            set_freq(newfreq);
        }
    }
    
    if (feature==1){
        int err;
        err=nearestNote(frequency);
        if (err!=NONOTEFOUND){
            newfreq=err;
            set_freq(newfreq);
        }else{
            newfreq+=(frequency-newfreq)/4;
            set_freq(newfreq);            
        }    
    }
    
    if ( (feature>1) && (feature<=5) ){
        newfreq=frequency+msign[wobulate]-128;
        if (feature==2)wobulate+=2;
        if (feature==3)wobulate+=4;
        if (feature==4)wobulate+=8;

        if (wobulate>63) wobulate=0;
        set_freq(newfreq);
    }
    
    //display timer
    if(dtimer>0)dtimer--;

    return 1;
}

void DoDistanceF(uint16_t d){
    uint16_t f;
    if(d!=fdist_old){
      if (d < 4096){      
         if (d>MINDISTANCE && d<MAXDISTANCE){
             frequency=500-(d-MINDISTANCE);
             frequency=frequency*frequency/120;       
             //f=d-MINDISTANCE;
             //frequency=2100-f*4.2; //start at c7 down to c3'ish
             fmute=0;	 
             //selectwaveshape(wave,volume);
             if(gpio_get(BUTTON1)==0){
                 wave=((int)d/(MAXDISTANCE/WAVMAX));
                 selectwaveshape(wave,volume);
                 if (wave>=WAVMAX)wave=WAVMAX-1;
             }

         }else{
             fmute=1;
         }
      }else{
          fmute=1;
      }     
      DoMute(fmute==1 || vmute==1); 
      fdist_old=d;
    }
}

void DoDistanceV(uint16_t d){
    uint16_t v;
    if(d!=vdist_old){
        if (d < 4096){
            if (d>MINDISTANCE && d<MAXDISTANCEV){
                v=d-MINDISTANCE;
                volume=256-(v*256/(MAXDISTANCEV-MINDISTANCE));
                if(gpio_get(BUTTON2)==0){
                    feature=((int)d/(MAXDISTANCEV/FEATUREMAX));
                    if (feature>=FEATUREMAX)feature=FEATUREMAX-1;
                    printf("Fe:%i\n",feature);
                }
            }else{
                volume=0;
            }   
//         selectwaveshape(wave,volume);              
        }else{
            volume=0;
        }
        selectwaveshape(wave,volume);              
    }else{
        vdist_old=d;
    }
    DoMute(fmute==1 || vmute==1);
}


// ******************************** Main ************************************
int main() {
    set_sys_clock_khz(PICOCLOCK, false);
    stdio_init_all();
    gpio_conf();
    init_I2C();    

    sleep_ms(2000); //wait for serial usb

    init_sound();

    splash();

    printf("**************** STARTING **************** \n\n");

//    multicore_launch_core1(Core1Main);
//    printf("**************** STARTING Core 1 **************** \n\n");

    printf("Display Init\n");
    SSD1306_init(0x3C,SSD1306_W128xH64);

    sprintf(dtemp,"Luminiferous");
    drawStatusCentered(dtemp,1,1);
    sprintf(dtemp,"Theremin");
    drawStatusCentered(dtemp,10,1);


   //change default device address for freq sensor.
    printf("Change FREQ I2C address\n");
    SetDeviceAddress(I2C_DEFAULT_DEV_ADDR,I2C_FREQ_DEV_ADDR);  

    int i;
    int fDistance;
    int vDistance;
    int model, revision;
    
    //init freq sensor
    printf("init Freq Sensor \n");
    i = tofInit( I2C_FREQ_DEV_ADDR, 0); // set short range mode (up to 2m)	
    if (i != 1)	{		
      return -1; // problem - quit	
    }	
    printf("FREQUENCY VL53L0X device successfully opened on %x .\n",I2C_FREQ_DEV_ADDR);	
    i = tofGetModel(&model, &revision,I2C_FREQ_DEV_ADDR);	
    printf("Model ID - %d\n", model);	
    printf("Revision ID - %d\n", revision);

    //init Vol sensor  
    printf("init Vol Sensor \n");
    gpio_put(VOL_XSHUT,1); //enable VOL sensor
    i = tofInit(I2C_VOL_DEV_ADDR , 0); // set short range mode (up to 2m)	
    if (i != 1)	{		
      return -1; // problem - quit	
    }	
    printf("VOLUME VL53L0X device successfully opened on %x .\n",I2C_VOL_DEV_ADDR);	
    i = tofGetModel(&model, &revision,I2C_VOL_DEV_ADDR);	
    printf("Model ID - %d\n", model);	
    printf("Revision ID - %d\n", revision);

    add_repeating_timer_ms(10, Freq_Timer_Callback, NULL, &ftimer);

    while(1)
    {       
        //Volume
        if (VtofState==0){
           tofStartReadDistance(I2C_VOL_DEV_ADDR);
           VtofState++;
        }
        if (VtofState==1){
           if(is_ready(I2C_VOL_DEV_ADDR)){
             VtofState++;
           }  
        }
        if (VtofState==2){
           vDistance = readRangeContinuousMillimeters(I2C_VOL_DEV_ADDR);
           DoDistanceV(vDistance);
           VtofState=0;
        }
        
        //Frequency
        if (FtofState==0){
           tofStartReadDistance(I2C_FREQ_DEV_ADDR);
           FtofState++;
        }
        if (FtofState==1){
           if(is_ready(I2C_FREQ_DEV_ADDR)){
             FtofState++;
           }
        }
        if (FtofState==2){
           fDistance = readRangeContinuousMillimeters(I2C_FREQ_DEV_ADDR);
           DoDistanceF(fDistance);
           FtofState=0;
        }

//        printf("H:%i v:%i\n",fDistance,vDistance);

        // Update display
        if(dtimer==0){
          if (volume==0){
              sprintf(dtemp,    "Vol   Off");
          }else{
              if(mute==1){
                  sprintf(dtemp,"Mute  Off");
              }else{
                  sprintf(dtemp,"Vol   %i",volume);
            }
          }  
          drawStatusBorder(dtemp,25,2);
          if (fmute){
              sprintf(dtemp,"Freq  -   ");
          }else{    
              int mn;
              if ((feature>1) && (feature<=5)) { //no note when wobulating
                  mn=-1;
              }else{  
                  mn=getMidiNote(newfreq);
              }  
              if (mn==-1){  
                  sprintf(dtemp,"Freq  %i",(int)newfreq);
              }else{
                  sprintf(dtemp,"Freq  %i %s ",(int)newfreq,mnotes[mn]);
              }    
          }
          drawStatusBorder(dtemp,35,2);
          
          sprintf(dtemp,"Voice %s",wavenames[wave]);
          drawStatusBorder(dtemp,45,2);
          
          sprintf(dtemp,"Mode  %s",features[feature]);
          drawStatusBorder(dtemp,55,2);
          
          //refresh display from buffer
          SSD1306_sendBuffer();
          dtimer=5;
        }
        sleep_ms(1);
    } 
}


