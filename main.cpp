/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include <stdio.h>


DigitalOut led(LED1);
DigitalOut led_ext(PC_0);
DigitalIn button(BUTTON1);
DigitalIn inc1(PA_1);
DigitalIn inc2(PA_4);

BufferedSerial mypc (USBTX, USBRX);

int counter;
int inc1_prev;

int main()
{


    button.mode(PullUp);
    counter = 0;
    FILE* mypcFile = fdopen(&mypc, "r+");
    while (true) {

        if ( inc1 != inc1_prev )
        {
            
            //if inc1 has changed, but inc2 is different to inc1... counter clockwise. 
            if ( inc2 != inc1  )
            {
                //clockwise rotation
                led = 1;
                wait_us(5000);
                counter++;
                led = 0;
            }
            else {
                
                led_ext = 1;
                wait_us(5000);
                counter++;
                led_ext = 0;
            }

            fprintf(mypcFile, "The encoder count is %d. The previous value of encoder A is %d \n", counter, inc1_prev); 

        }
       
        inc1_prev = inc1;

    }
}
