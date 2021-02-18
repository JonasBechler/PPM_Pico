/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "ppm.pio.h"

PIO pio = pio0;
uint sm;
const uint PPM_PIN = 15;


//---------------------------------------------------------
//Helpers--------------------------------------------------
//---------------------------------------------------------

static uint8_t map(int64_t x, int64_t fromZero, int64_t fromFull, uint8_t toZero, uint8_t toFull){
    int64_t fromSize =  fromFull-fromZero;
    uint8_t toSize =  toFull-toZero;
    
    return (uint8_t)((x-fromZero)*toSize/fromSize+toZero);
};



bool available = false;  
int8_t valuesIndex = 0;  

uint8_t values[4];
static uint32_t valuesTempBounds[2] ={7,12}; //Number of borders of T_on in integer 0.7ms-1.7ms
uint32_t valuesTemp[4];
static void mapValues(){
    available = false;
    uint64_t buffer;
    //conv to uint8 0-255
    for (int i = 0; i < 4; i++){
        buffer = valuesTemp[i];
        buffer = (0xffff - buffer);
        buffer = buffer - valuesTempBounds[0];
        buffer = buffer * 256 / (valuesTempBounds[1] - valuesTempBounds[0]);
        values[i] = buffer;
        buffer = 0;
    }
    //assign values of 0 correct
    buffer = values[0];
    values[0] = 0;
    for (int i = 8; i > 0; i--){
        buffer = buffer % (1<<i);
        values[0] = (buffer > (1<<(i-1))) << i;
    }
}
absolute_time_t lastInterruptTime;
absolute_time_t thisInterruptTime;
void __isr interruptValuesRX(){
    thisInterruptTime = get_absolute_time();
    if (absolute_time_diff_us (lastInterruptTime, thisInterruptTime) > 3000){
        valuesIndex = 0;
    }
    uint32_t newValue = pio_sm_get_blocking(pio, sm);
    
    //write first 4 chanels
    if (valuesIndex < 4){
        valuesTemp[valuesIndex] = newValue;
    }
    
    //finish one frame
    if (valuesIndex >= 7){
        //remap and publish data
        mapValues();
        valuesIndex = 0;
        available = true;
    }
    else{
        valuesIndex++;
    }
    
    lastInterruptTime = thisInterruptTime;
    pio_sm_put_blocking(pio, sm, 0xffff);
    irq_clear(PIO0_IRQ_0);
    
    printf("ISR\n");
}


void core1_main() { 
    //Init outputs
    
    
    //Init PIO
    uint offset = pio_add_program(pio, &ppm_program);
    sm = pio_claim_unused_sm(pio, true);
    ppm_program_init(pio, sm, offset, PPM_PIN);
    
    //Init Interrupts
    irq_add_shared_handler(PIO0_IRQ_0, interruptValuesRX, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(PIO0_IRQ_0, true);
    
    //Init
    pio_sm_put_blocking(pio, sm, 0xffff);
    printf("start sampling\n");
    uint32_t dataBuff;
    while (true) {
        if(available){
            available = false;
            dataBuff = values[0]<<24 + values[1]<<16 + values[2]<<8 + values[3];
            multicore_fifo_push_blocking(dataBuff);
            printf("%i, %i, %i, %i\n", values[0], values[1],values[2],values[3]);
        }
        else{
            sleep_ms(500);
            printf("nothing new\n");
            
        }
  
    }
}


int main() {
    stdio_init_all();
    sleep_ms(5000);
    printf("start\n");
    
    
    
    multicore_launch_core1(core1_main);
    
    
    gpio_set_function(0, GPIO_FUNC_PWM);
    gpio_set_function(1, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(0);
    
    pwm_set_clkdiv(slice_num, clock_get_hz(clk_sys) * 50); //Adjust PWM Freq
    pwm_set_wrap(slice_num, 255);
    pwm_set_enabled(slice_num, true);
    
    
  
    while (true) {
        uint32_t data = multicore_fifo_pop_blocking ();
        uint8_t status = data>>24;
        uint8_t speed = data>>16;
        uint8_t steer = data>>8;
        
        pwm_set_gpio_level(0, speed);
        pwm_set_gpio_level(1, steer);
        
        sleep_ms(2000);
        


    }
    return 0;
}

