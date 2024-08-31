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

#include "settings.h"

#include "sound/sound.h"
#include "display/display.h"
#include "midi.c"
#include "mapping.c"

#include "flash.c"
#include "neopixel.c"

#include "display/assets.h"
#include "sound/waveshapes.h"

#define MINDISTANCE 30
#define MAXDISTANCE 450 //was 350
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

int8_t LtofState=0;
int8_t RtofState=0;

uint16_t dtimer=10000; //stop display updates
uint16_t ltimer=10; //led timer

uint16_t fdist_old=0;
uint16_t vdist_old=0;

uint8_t sinister=0;

//Modes
#define MODEMAX 5 //Vibrato needs more work.
uint8_t mode=0;
char  modes[MODEMAX][30]={"Normal\0","Note Only\0","Wob Slow\0","Wob Med\0","Wob Fast\0","Vibrato"};

uint8_t wobulate=0;
uint8_t vibrato=0;

uint16_t vcnt=0;
uint16_t fcnt=0;
uint16_t vres=0;
uint16_t fres=0;

uint8_t debug=0;

char timecompiled[20];

void gpio_conf(){
    //FLASH LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(250);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    // Vol sensor XSHUT 
    gpio_init(LEFT_XSHUT);
    gpio_set_dir(LEFT_XSHUT, GPIO_OUT);
    gpio_put(LEFT_XSHUT,0); //disable LEFT sensor
    
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
        drawStatusCentered("P-Right",30,1);
    }else{
        drawStatusCentered("P-Left",30,1);
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
    
    //needs this for reliability of starting tof sensors.    
    sleep_ms(250);
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
        mode=fdata[4];
    }
}

void save_flash_settings(void){
    fdata[2]=sinister;
    fdata[3]=wave;
    fdata[4]=mode;;
    write_flash();
}



//********************************* Core1 ************************************
//void Core1Main(){
//   Sound_Loop();
//}


void selectwaveshapeV(uint8_t wave,uint8_t volume){
    if (mode==5){
        if (vibrato>32){
            selectwaveshape(wave,volume);
        }else{
            selectwaveshape(wave,volume>>1);
        }
    }else{
        selectwaveshape(wave,volume);    
    }
}


//****************************** smooth out Frequency changes ******************
bool Freq_Timer_Callback(struct repeating_timer *t){
    if ((mode==0) || (mode==5) ){
        if (abs(frequency-newfreq)>1){
            newfreq+=(frequency-newfreq)/SMOOTHING;
            set_freq(newfreq);
        }
    }
    
    if (mode==1){
        int nf;
        nf=nearestNoteR(frequency);
//        printf("%i,%f\n",nf,frequency);
        if (nf!=NONOTEFOUND){
            newfreq=nf;
            set_freq(newfreq);
        }        
    }
    
    if ( (mode>1) && (mode<=4) ){
        newfreq=frequency+msign[wobulate]-128;
        if (mode==2)wobulate+=2;
        if (mode==3)wobulate+=4;
        if (mode==4)wobulate+=8;

        if (wobulate>63) wobulate=0;
        set_freq(newfreq);
    }
    
    if (mode==5){
       vibrato+=4;
       if (vibrato>64){ 
           vibrato=0;
           selectwaveshapeV(wave,volume);
       }
       if (vibrato==33){
           selectwaveshapeV(wave,volume);
       }
    }
    
    //display timer
    if(dtimer>0)dtimer--;
    // led timer
    if(ltimer>0)ltimer--;

    return 1;
}

//Distance to frequency and voice selection
void DoFrequency(uint16_t d){
    uint16_t f;
    if(d!=fdist_old){
        if (d < 4096){      
            if (d<MINDISTANCE)d=MINDISTANCE;
            if (d>=MINDISTANCE && d<MAXDISTANCE){
//                printf(" %i,%i\n",(MAXDISTANCE-MINDISTANCE)-(d-MINDISTANCE)-1,d);
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
            if (d<MINDISTANCE)d=MINDISTANCE;
            if (d>=MINDISTANCE && d<MAXDISTANCEV){
                v=d-MINDISTANCE+1;
                volume=255-(v*255/(MAXDISTANCEV-MINDISTANCE));
//        printf("Vol:d:%i MD:%i v:%i volume:%i\n",d,MINDISTANCE,v,volume);
            }else{volume=0;}   
        }else{volume=0;}
        selectwaveshapeV(wave,volume);              
    } //else{vdist_old=d;}
    vdist_old=d;
    DoMute(fmute==1 || vmute==1);
    
}

//voice selection
void DoVoice(uint16_t d){
    uint16_t f;
    if (d < 4096){      
        if (d<MINDISTANCE)d=MINDISTANCE;
        if (d>=MINDISTANCE && d<MAXDISTANCE){
            printf("fb %i,%i,%i\n",d-MINDISTANCE,(MAXDISTANCE-MINDISTANCE),WAVMAX);
            wave=((d-MINDISTANCE )/(float)((MAXDISTANCE-MINDISTANCE)/WAVMAX));
            if (wave>=WAVMAX)wave=WAVMAX-1;
            selectwaveshapeV(wave,volume);
            save_flash_settings();
        }
    }
}

//Mode selection
void DoMode(uint16_t d){
    uint16_t v;
    if (d < 4096){
        if (d<MINDISTANCE)d=MINDISTANCE;
        if (d>=MINDISTANCE && d<MAXDISTANCEV){
            mode=((int)d/(MAXDISTANCEV/MODEMAX));
            if (mode>=MODEMAX)mode=MODEMAX-1;
            printf("Fe:%i\n",modes);
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

    //init neopixel
    init_NeoPixel();

    //saved settings and POR buttons
    if( (gpio_get(LEFT_BUTTON)==0) && (gpio_get(RIGHT_BUTTON)==0) ){
        printf("Settings Reset\n");
        save_flash_settings();
        debug=1;
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

   //change default device address for freq sensor.
    printf("Change RIGHT I2C address\n");
    SetDeviceAddress(I2C_DEFAULT_DEV_ADDR,I2C_RIGHT_DEV_ADDR);  

    int i;
    uint16_t rDistance;
    uint16_t lDistance;
    int model, revision;
    
    //init freq sensor
    printf("\nInit Right Sensor \n");
    i = tofInit( I2C_RIGHT_DEV_ADDR, 0); // set short range mode (up to 0.5m)	
    if (i != 1)	{		
//        return -1; // problem - quit	
        printf("Cant INIT RIGHT sensor\n");
    }	
    printf("Right VL53L0X device successfully opened on %x .\n",I2C_RIGHT_DEV_ADDR);	
    i = tofGetModel(&model, &revision,I2C_RIGHT_DEV_ADDR);	
    printf("Model ID - %d\n", model);	
    printf("Revision ID - %d\n", revision);

    //init Vol sensor  
    printf("\nInit Left Sensor \n");
    gpio_put(LEFT_XSHUT,1); //enable LEFT sensor
    i = tofInit(I2C_LEFT_DEV_ADDR , 0); // set short range mode (up to 0.5m)	
    if (i != 1)	{		
//        return -1; // problem - quit	
        printf("Cant INIT LEFT sensor\n");
    }	
    printf("Left VL53L0X device successfully opened on %x .\n",I2C_LEFT_DEV_ADDR);	
    i = tofGetModel(&model, &revision,I2C_LEFT_DEV_ADDR);	
    printf("Model ID - %d\n", model);	
    printf("Revision ID - %d\n", revision);

    add_repeating_timer_ms(10, Freq_Timer_Callback, NULL, &ftimer);

    while(1)
    {       
        //Left
        if (LtofState==0){
            tofStartReadDistance(I2C_LEFT_DEV_ADDR);
            LtofState++;
        }
        if (LtofState==1){
            if(is_ready(I2C_LEFT_DEV_ADDR)){
                 LtofState++;
           }  
        }
        if (LtofState==2){
            lDistance = readRangeContinuousMillimeters(I2C_LEFT_DEV_ADDR);
            if (debug) printf("Left %i\n",lDistance);
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
            LtofState=0;
            vcnt++;
        }
        
        //Frequency
        if (RtofState==0){
            tofStartReadDistance(I2C_RIGHT_DEV_ADDR);
            RtofState++;
        }
        if (RtofState==1){
            if(is_ready(I2C_RIGHT_DEV_ADDR)){
                 RtofState++;
           }
        }
        if (RtofState==2){
            rDistance = readRangeContinuousMillimeters(I2C_RIGHT_DEV_ADDR);
            if (debug) printf("Right %i\n",rDistance);
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
            RtofState=0;
            fcnt++;
        }

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
                sprintf(dtemp,"Pitch -   ");
            }else{    
                int mn;
                if ((mode>1) && (mode<=5)) { //no note when wobulating
                    mn=-1;
                }else{  
                    mn=getMidiNote(newfreq);
                }  
                if (mn==-1){  
                    sprintf(dtemp,"Pitch %i",(int)newfreq);
                }else{
                    sprintf(dtemp,"Pitch %i %s ",(int)newfreq,mnotes[mn]);
                }    
            }
            drawStatusBorder(dtemp,35,0);
          
            sprintf(dtemp,"Voice %s",wavenames[wave]);
            drawStatusBorder(dtemp,45,0);
          
            sprintf(dtemp,"Mode  %s",modes[mode]);
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

        if(ltimer==0){
            //update neopixel
            if((mute==1) || (volume==0)){
                put_pixel(0);
            }else{
                if(!sinister){
                     neopixel_fromline(rDistance,volume);
    //                  printf("%i,%i\n",rDistance,volume);
                }else{
                     neopixel_fromline(lDistance,volume);
    //                  printf("%i,%i\n",lDistance,volume);
                }
            } 
            ltimer=5;
        }    
        sleep_ms(0.1);
     } 
}



