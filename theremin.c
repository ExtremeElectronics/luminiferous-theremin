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
#include "mapping.c"

#include "flash.c"

#include "settings.h"
#include "display/assets.h"
#include "sound/waveshapes.h"

#define MINDISTANCE 30
#define MAXDISTANCE 350
#define MAXDISTANCEV 300

#define SMOOTHING 2.7 //was 4 larger smoother/slower

//globals
float frequency=1000;
uint8_t volume=255;
uint8_t extern mute;
uint8_t extern vmute;
uint8_t extern fmute;
int8_t wave=0; //from table in waveshapes.c
struct repeating_timer ftimer;
float newfreq=1000;

char dtemp[100];
uint8_t firstdisplay=0;

int8_t VtofState=0;
int8_t FtofState=0;

uint16_t dtimer=10000; //stop display updates

uint16_t fdist_old=0;
uint16_t vdist_old=0;

uint8_t sinister=0;

//features
#define FEATUREMAX 5

uint8_t feature=0;
char  features[FEATUREMAX][30]={"Normal\0","Note Only\0","Wob Slow\0","Wob Med\0","Wob Fast\0"};

uint8_t wobulate=0;

uint16_t vcnt=0;
uint16_t fcnt=0;
uint16_t vres=0;
uint16_t fres=0;

char timecompiled[20];

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
    
    //LEFT_BUTTON
    gpio_init(LEFT_BUTTON);
    gpio_pull_up(LEFT_BUTTON);
    gpio_set_dir(LEFT_BUTTON, GPIO_IN);

    //RIGHT_BUTTON
    gpio_init(RIGHT_BUTTON);
    gpio_pull_up(RIGHT_BUTTON);
    gpio_set_dir(RIGHT_BUTTON, GPIO_IN);


}

void splash(){
    sprintf(timecompiled,"%s",__DATE__);
    printf("\n                                                  ");
    printf("\n                                                  ");
    printf("\n                                                  ");
    printf("\n                                                  ");
    printf("\n      -        Extreme Kits           -           ");
    printf("\n       -       extkits.uk/lt         -            ");
    printf("\n        -                           -             ");
    printf("\n         -                         -              ");
    printf("\n           _______________________                ");
    printf("\n          /     Luniniferous      \\               ");
    printf("\n         /        Theremin         \\              ");
    printf("\n         ---------------------------              ");
    printf("\n                                                  ");
    printf("\n      Copyright (c) 2024 Derek Woodroffe          ");
    printf("\n                %s  ",timecompiled);
    printf("\n\n");  
        
    SSD1306_background_image(extkits);
    SSD1306_sendBuffer();
    sleep_ms(2000);
    
    SSD1306_background_image(theremin);
    SSD1306_sendBuffer();
  
    sprintf(dtemp,"Luminiferous");
    drawStatusCentered(dtemp,1,1);
    sprintf(dtemp,"Theremin");
    drawStatusCentered(dtemp,10,1);
    drawStatusCentered(timecompiled,20,1);
    if(sinister==0){
        drawStatusCentered("F-Right",30,1);
    }else{
        drawStatusCentered("F-Left",30,1);
    }      
    //refresh display from buffer
    SSD1306_sendBuffer();
    
    dtimer=200; //wait 2000ms

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

// flash settings
void get_flash_settings(void){
  if (read_flash()){
     //if bad magic format flash page
     format_flash();
  }else{
     //usable data starts at 2
     sinister=fdata[2];
     wave=fdata[3];
     feature=fdata[4];
  }

}

void save_flash_settings(void){
    fdata[2]=sinister;
    fdata[3]=wave;
    fdata[4]=feature;;
    write_flash();
}



//********************************* Core1 ************************************
//void Core1Main(){
//   Sound_Loop();
//}

//****************************** smooth out f changes ******************
bool Freq_Timer_Callback(struct repeating_timer *t){
    if (feature==0){
        if (abs(frequency-newfreq)>1){
            newfreq+=(frequency-newfreq)/SMOOTHING;
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


//Distance to frequency and voice selection
void DoFrequency(uint16_t d){
    uint16_t f;
    if(d!=fdist_old){
        if (d < 4096){      
            if (d>MINDISTANCE && d<MAXDISTANCE){
                frequency=farray[(MAXDISTANCE-MINDISTANCE)-(d-MINDISTANCE)-1];             
                fmute=0;	 
            }else{fmute=1;}
        }else{fmute=1;}     
        DoMute(fmute==1 || vmute==1); 
        fdist_old=d;
    }
}

//Distance to Volume and Mode selection
void DoVolume(uint16_t d){
    uint16_t v;
    if(d!=vdist_old){
        if (d < 4096){
            if (d>MINDISTANCE && d<MAXDISTANCEV){
                v=d-MINDISTANCE;
                volume=256-(v*256/(MAXDISTANCEV-MINDISTANCE));
            }else{volume=0;}   
        }else{volume=0;}
        selectwaveshape(wave,volume);              
    }else{vdist_old=d;}
    DoMute(fmute==1 || vmute==1);
}


//voice selection
void DoVoice(uint16_t d){
    uint16_t f;
    if (d < 4096){      
        if (d>MINDISTANCE && d<MAXDISTANCE){
            printf("fb %i,%i,%i\n",d-MINDISTANCE,(MAXDISTANCE-MINDISTANCE),WAVMAX);
            wave=((d-MINDISTANCE )/(float)((MAXDISTANCE-MINDISTANCE)/WAVMAX));
            if (wave>=WAVMAX)wave=WAVMAX-1;
            selectwaveshape(wave,volume);
            save_flash_settings();
        }
    }
}

//Mode selection
void DoMode(uint16_t d){
    uint16_t v;
    if (d < 4096){
        if (d>MINDISTANCE && d<MAXDISTANCEV){
            feature=((int)d/(MAXDISTANCEV/FEATUREMAX));
            if (feature>=FEATUREMAX)feature=FEATUREMAX-1;
            printf("Fe:%i\n",feature);
            save_flash_settings();
        }   
    }
}



// ******************************** Main ************************************
int main() {
    set_sys_clock_khz(PICOCLOCK, false);
    stdio_init_all();
    gpio_conf();
    init_I2C();    

    sleep_ms(2000); //wait for serial usb

    init_sound();

    printf("Display Init\n");
    SSD1306_init(0x3C,SSD1306_W128xH64);

/*
    extern char __flash_binary_end;
    uintptr_t end = (uintptr_t) &__flash_binary_end;
    printf("Binary ends at %08x\n", end);

    print_flash();
*/

    //saved settings and POR buttons
    if( (gpio_get(LEFT_BUTTON)==0) && (gpio_get(RIGHT_BUTTON)==0) ){
        printf("Settings Reset\n");
        save_flash_settings();
    }else{  
        //getsaved settings
        get_flash_settings();

        //handedness
        if(gpio_get(RIGHT_BUTTON)==0){
           sinister=0;
           save_flash_settings();
        }
        if(gpio_get(LEFT_BUTTON)==0){
           sinister=1;
           save_flash_settings();
        }
    }


    splash();

    printf("**************** STARTING **************** \n\n");


//    multicore_launch_core1(Core1Main);
//    printf("**************** STARTING Core 1 **************** \n\n");


   //change default device address for freq sensor.
    printf("Change FREQ I2C address\n");
    SetDeviceAddress(I2C_DEFAULT_DEV_ADDR,I2C_FREQ_DEV_ADDR);  

    int i;
    int rDistance;
    int lDistance;
    int model, revision;
    
    //init freq sensor
    printf("\nInit Freq Sensor \n");
    i = tofInit( I2C_FREQ_DEV_ADDR, 0); // set short range mode (up to 2m)	
    if (i != 1)	{		
        return -1; // problem - quit	
    }	
    printf("FREQUENCY VL53L0X device successfully opened on %x .\n",I2C_FREQ_DEV_ADDR);	
    i = tofGetModel(&model, &revision,I2C_FREQ_DEV_ADDR);	
    printf("Model ID - %d\n", model);	
    printf("Revision ID - %d\n", revision);

    //init Vol sensor  
    printf("\nInit Vol Sensor \n");
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
            lDistance = readRangeContinuousMillimeters(I2C_VOL_DEV_ADDR);
            //DoDistanceV(vDistance);
            if(gpio_get(RIGHT_BUTTON)==1){
                if (!sinister){
                    DoVolume(lDistance);
                }else{
                    DoFrequency(lDistance);
                }
            }else{    
                DoVoice(lDistance);
            }
            VtofState=0;
            vcnt++;
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
            rDistance = readRangeContinuousMillimeters(I2C_FREQ_DEV_ADDR);
            //DoDistanceF(fDistance);
            if(gpio_get(LEFT_BUTTON)==1){
                if(!sinister){
                    DoFrequency(rDistance);
                }else{
                    DoVolume(rDistance);
                }
            }else{    
                DoMode(rDistance);
            }    
            FtofState=0;
            fcnt++;
        }

//        printf("H:%i v:%i\n",fDistance,vDistance);

        // Update display
        if(dtimer==0){
            if (firstdisplay==0){
               firstdisplay=1;
               SSD1306_clear();
               sprintf(dtemp,"Luminiferous");
               drawStatusCentered(dtemp,1,1);
               sprintf(dtemp,"Theremin");
               drawStatusCentered(dtemp,10,1);
            }
        
            if (volume==0){
                sprintf(dtemp,    "Vol   Off");
            }else{
                if(mute==1){
                    sprintf(dtemp,"Mute  Off");
                }else{
                    sprintf(dtemp,"Vol   %i%%",(int)(volume/2.5));
                }
            }  
            drawStatusBorder(dtemp,25,0);
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
            drawStatusBorder(dtemp,35,0);
          
            sprintf(dtemp,"Voice %s",wavenames[wave]);
            drawStatusBorder(dtemp,45,0);
          
            sprintf(dtemp,"Mode  %s",features[feature]);
            drawStatusBorder(dtemp,55,0);
          
            //refresh display from buffer
            SSD1306_sendBuffer();
          
            //then every 100mS
            dtimer=10;

            fres=fcnt;
            vres=vcnt;
            fcnt=0;
            vcnt=0;
//          printf(" vc %i, vf %i \n",vres,fres);
          }
          sleep_ms(1);
     } 
}


