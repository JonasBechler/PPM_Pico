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

bool available = false;
int8_t valuesIndex = 0;

void interruptValuesRX()
{
    available = true;
    valuesIndex++;
    irq_clear(PIO0_IRQ_0);
}

void core1_main()
{

    //Init PIO
    uint offset = pio_add_program(pio, &ppm_program);
    sm = pio_claim_unused_sm(pio, true);
    ppm_program_init(pio, sm, offset, PPM_PIN);

    //Init Interrupts
    irq_add_shared_handler(PIO0_IRQ_0, interruptValuesRX, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(PIO0_IRQ_0, true);

    //Init

    while (true)
    {
        if (available)
        {
            printf("ISR %i \n", valuesIndex);
            available = false;
        }
        else
        {
            sleep_ms(5000);
            printf("nothing new\n");
        }
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(5000);
    printf("start\n");

    multicore_launch_core1(core1_main);

    return 0;
}
