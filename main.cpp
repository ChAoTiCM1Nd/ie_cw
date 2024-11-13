/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */



//      Note: The code below is to control the PWM fan in a basic fashion. 

#include "mbed.h"
#include <stdio.h>
#include <cstdint>


PwmOut fan(PB_0);
InterruptIn fan_taco(PA_0);

// Bufferial serial
BufferedSerial mypc (USBTX, USBRX);

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

    fprintf(mypcFile,"\033[0m\033[2J\033[HI2C Searching!\n\n\n"); 

    fan.period(0.1f);
    fan.write(0.9f);

    //fan_taco.rise(&count_pulse());

    while (true)
    {

            

    }
    

    
}
