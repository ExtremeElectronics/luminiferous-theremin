/**
 * mostly taken from
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * Modified by Derek Woodroffe Extremeelectronics 2024
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/flash.h"

// We're going to erase and reprogram a region 1.5M from the start of flash.
// Once done, we can access this at XIP_BASE + 1.5Mk.
#define FLASH_TARGET_OFFSET (1536 * 1024) 

#define FLASH_MAGIC0 0xAA
#define FLASH_MAGIC1 0x55

//FLASH_PAG_SIZE SIZE defined in spi_flash.c

uint8_t fdata[FLASH_PAGE_SIZE];

const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);



int read_flash(void){
    //read returns 1 if bad magic
    printf("Read Flash\n");
    for (int i = 0; i < FLASH_PAGE_SIZE; ++i) {
       fdata[i] = flash_target_contents[i];
    }
    uint8_t r=0;
    if(fdata[0]!=FLASH_MAGIC0) r=1;
    if(fdata[1]!=FLASH_MAGIC1) r=1;
    if(fdata[FLASH_PAGE_SIZE-2]!=FLASH_MAGIC0) r=1;
    if(fdata[FLASH_PAGE_SIZE-1]!=FLASH_MAGIC1) r=1;

    if(r==1) printf("BAD FLASH MAGIC\n");

    return r;
    
}

void print_flash(void) {
    read_flash();
    for (size_t i = 0; i < FLASH_PAGE_SIZE; ++i) {
        printf("%02x", fdata[i]);
        if (i % 16 == 15)
            printf("\n");
        else
            printf(" ");
    }
}

void write_flash(void){
    printf("Write Flash");
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, fdata, FLASH_PAGE_SIZE);
    restore_interrupts (ints);
}

void format_flash(void){
    printf("Format Flash");
    uint8_t x;
    //FLASHSECTOR SIZE defined in spi_flash.c
    fdata[0]=FLASH_MAGIC0;
    fdata[1]=FLASH_MAGIC1;
    for(x=2;x<254;x++)fdata[x]=0;
    fdata[FLASH_PAGE_SIZE-2]=FLASH_MAGIC0;
    fdata[FLASH_PAGE_SIZE-1]=FLASH_MAGIC1;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, fdata, FLASH_PAGE_SIZE);
    restore_interrupts (ints);
}



