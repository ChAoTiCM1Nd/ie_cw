/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

//      Note: The code below is to control the PWM fan in a basic fashion. 

#include "mbed.h"
#include <stdio.h>
#include <cstdint>
#include "LCD_ST7066U.h"

LCD lcd(PB_15, PB_14, PB_10, PA_8, PB_2, PB_1); // Adjust pin names as per your hardware



// Bufferial serial
BufferedSerial mypc (USBTX, USBRX);



int main()
{

    lcd.clear();                  // Clear the screen
    lcd.writeLine("RPM Control", 0); // Write text to the first line
    lcd.writeLine("Initializing...", 1); // Write text to the second line

    //temporary buffer for read temp
    char temp_data; 
    FILE* mypcFile = fdopen(&mypc, "r+");

    fprintf(mypcFile,"Starting....\n\n");

    fprintf(mypcFile,"\033[0m\033[2J\033[HI2C Searching!\n\n\n"); 



    while (true)
    {
        lcd.writeLine("Initializing...", 0); // Write text to the second line
            

    }
    

    
}
