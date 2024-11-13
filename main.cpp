/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */



//      Note: The code below is to control the PWM fan in a basic fashion. 

#include "mbed.h"
#include <stdio.h>
#include <cstdint>


PwmOut led(LED1);
PwmOut fan(PB_0);
InterruptIn fan_taco(PA_0);





BufferedSerial mypc (USBTX, USBRX);


//temporary register to access temperature data 
char TEMP_REG = 0x00;

volatile int pulse_count;


void count_pulse()
{

    pulse_count++;



}

int main()
{

    //temporary buffer for read temp
    char temp_data; 
    FILE* mypcFile = fdopen(&mypc, "r+");

    fprintf(mypcFile,"Starting....\n\n");


    char data[2];

    led.period(2.0f);
    led.write(0.25f);

    fan.period(0.0f);
    fan.write(0.0f);

    //fan_taco.rise(&count_pulse());

    while (true)
    {

        
       
       //fprintf(mypcFile,"Taco Signal is: %d \n",fan_taco );
        
            

    }
    

    
}
