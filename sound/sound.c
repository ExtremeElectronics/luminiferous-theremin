/**
 * https://github.com/ExtremeElectronics/theremin
 *
 * sound.c
 *
 * Code to implement sound features for theremin - Copyright (c) 2024 Derek Woodroffe <tesla@extremeelectronics.co.uk>
 * on a Pi PicoW
 * DMA and PWM code from https://parthssharma.github.io/Pico/DMASineWave.html
 *
 * SPDX-License-Identifier: BSD-3-Clause
**/

#include <string.h>
#include "malloc.h"
#include "stdarg.h"
#include <stdio.h>
#include <stdlib.h>

//pico stuff
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"

#include "sound.h"
#include "../settings.h"

#include "waveshapes.c"
#define WAVTABLESIZE 256


union pwm32{
    uint32_t x;
    struct{
        uint8_t al;
        uint8_t ah;
        uint8_t bl;
        uint8_t bh;
    };
};

//sound buffer
union pwm32 sbuffer[WAVTABLESIZE];
union pwm32 * bufferptr = &sbuffer[0];

uint32_t dmafreq=64000; //dma timer frequency

uint8_t old_mute=0;
uint8_t old_freq=0;

uint8_t mute=0;

uint8_t vmute=0;
uint8_t fmute=0;

extern float volume;
extern uint8_t wave; //wave shape selection ??? remove

uint PWMslice;

const uint32_t trigg = 1; //The transaction count

//dma channels
int pwm_dma_chan;
int ctl_dma_chan; 

uint8_t old_ws,old_vol;

int ptimer ; //dma pacing timer

//select wavetable from MAXWAVETABLES and set volume 0-255
void selectwaveshape(uint8_t ws,uint8_t vol){
   if((old_ws==ws) && (old_vol==vol)){
       return;
   }else{   
       if(vol==0){
           vmute=1;     
       }else{
           vmute=0;
           uint16_t a;
           int x ;
           uint8_t y;

           for(a=0;a<WAVTABLESIZE;a++){       
               x=waveshapes[ws][a];
               x=(x-128)*vol;
               y=(x>>8)+128;
               sbuffer[a].al=y;
               sbuffer[a].bl=y;
           }  

       }
       DoMute(vmute==1 || fmute==1);
   }
   old_ws==ws;
   old_vol=vol;
}

void SetPWM(void){
    gpio_init(soundIO1);
    gpio_set_dir(soundIO1,GPIO_OUT);
    gpio_set_function(soundIO1, GPIO_FUNC_PWM);

    gpio_init(soundIO2);
    gpio_set_dir(soundIO2,GPIO_OUT);
    gpio_set_function(soundIO2, GPIO_FUNC_PWM);

    //get slice from gpio pin
    PWMslice=pwm_gpio_to_slice_num(soundIO1);
 
    //setup pwm
//    pwm_set_clkdiv(PWMslice,4);//31mhz
    pwm_set_clkdiv(PWMslice,1);//loads of mhz
    pwm_set_both_levels(PWMslice,128,128);
    pwm_set_output_polarity(PWMslice,true,false);
    pwm_set_wrap (PWMslice, 256);
    pwm_set_enabled(PWMslice,true);

    //setup DMA
    // Setup DMA channel to drive the PWM
    pwm_dma_chan = dma_claim_unused_channel(true);
    // Setup DMA channel to control the DMA
    ctl_dma_chan = dma_claim_unused_channel(true);

    //ctl chan config
    dma_channel_config ctl_dma_chan_config = dma_channel_get_default_config(ctl_dma_chan);
    channel_config_set_transfer_data_size(&ctl_dma_chan_config, DMA_SIZE_32);
    channel_config_set_read_increment(&ctl_dma_chan_config, false);
    channel_config_set_write_increment(&ctl_dma_chan_config, false);

    // Setup the ctl channel 
    dma_channel_configure(
        ctl_dma_chan, &ctl_dma_chan_config,
        &dma_hw->ch[pwm_dma_chan].al3_read_addr_trig , //trigger reload the channel counter and start pwm_dma_chan
        &bufferptr, // pointer to pointer start of buffer
        1,
        false // Dont Start.
    );    

    //pwm channel config
    dma_channel_config pwm_dma_chan_config = dma_channel_get_default_config(pwm_dma_chan);
    channel_config_set_transfer_data_size(&pwm_dma_chan_config, DMA_SIZE_32);
    channel_config_set_read_increment(&pwm_dma_chan_config, true);
    channel_config_set_write_increment(&pwm_dma_chan_config, false);

    //dma dreq by timer  
    ptimer = dma_claim_unused_timer(true /* required */);
    dma_timer_set_fraction(ptimer, 1, 1000);  // divide system clock by num/denom
    int treq = dma_get_timer_dreq(ptimer);

    channel_config_set_dreq(&pwm_dma_chan_config, treq); //Select a transfer request signal. timer0??
    channel_config_set_chain_to(&pwm_dma_chan_config, ctl_dma_chan); //When this channel completes, it will trigger ctl_dma_chan

    // Setup the PWM channel 
    dma_channel_configure(
        pwm_dma_chan, &pwm_dma_chan_config,
        &pwm_hw->slice[PWMslice].cc, // Write to PWM counter compare
        sbuffer, // Read values from waveshapes
        WAVTABLESIZE, // 256 values to copy
        false // Dont Start.
    );
    //dma_start_channel_mask(1u << ctl_dma_chan); //Start control channel
}

void init_sound(void){
    selectwaveshape(0,255);
    SetPWM();
    int a;
    printf("Sound INIT\n");    
}

void set_freq(uint16_t f){
//    printf("f %i\n",f);
    if(f!=old_freq){
        //printf("Set Freq\n");
        dmafreq=PICOCLOCK*1000/256/f;
        dma_timer_set_fraction(ptimer, 1, dmafreq);  // divide system clock by num/denomdma_timer_set_fraction(ptimer, 1, 400);  // divide system clock by num/denom 
    }
    old_freq=f;
}

void DoMute(uint8_t m){
  // printf("m %i\n",mute);

    mute=m;
    if(mute!=old_mute){

//mute by stopping DMA
        if(mute==1){
//         printf("######################### Mute\n");
            hw_clear_bits(&dma_hw->ch[ctl_dma_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);
//            hw_clear_bits(&dma_hw->ch[pwm_dma_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);
        }
        if(mute==0){
//        printf("########################### Unmute\n");
            hw_set_bits(&dma_hw->ch[ctl_dma_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);
            hw_set_bits(&dma_hw->ch[pwm_dma_chan].al1_ctrl, DMA_CH0_CTRL_TRIG_EN_BITS);

            dma_start_channel_mask(1u << ctl_dma_chan); //Start control channel
        }
    }  
    old_mute=mute;
}


// DFA in core1
void Sound_Loop(void){
    while(1){
        sleep_us(100);
    }
}
