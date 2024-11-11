/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include <stdio.h>
#include <cstdint>


DigitalOut led(LED1);
DigitalIn button(BUTTON1);


I2C i2c(PB_9, PB_8);

BufferedSerial mypc (USBTX, USBRX);


const int addr7bit = 0x9A;
const int addr8bit = 0x9A << 1;

//temporary register to access temperature data 
char TEMP_REG = 0x00;

int main()
{

    //temporary buffer for read temp
    char temp_data; 
    FILE* mypcFile = fdopen(&mypc, "r+");


    char data[2];
    while (true)
    {

        //Write to the address by modifying the last bit of the address - use a write command to write a "read temperature" command.
        //i2c.write(addr8bit, temp_data, 1);  //basically requesting data from sensor
        FILE* mypcFile1 = fdopen(&mypc, "r+");
        fprintf(mypcFile1,"\033[0m\033[2J\033[HI2C Searching!\n\n\n");

        int count = 0;
        fprintf(mypcFile1,"Starting....\n\n");

        for (int address=0; address<256; address+=2) {

            if (!i2c.write(address, NULL, 0)) 
            { // 0 returned is ok
                fprintf(mypcFile1,"I2C address 0x%02X\n", address);
                count++;
                   // break;
            }

        }
        fprintf(mypcFile1,"\n\n%d devices found\n", count);
        wait_us(20000); 

        i2c.write(addr8bit, &TEMP_REG, 1);

        wait_us(500000);
        i2c.read(addr7bit, &temp_data, 1);


        //fprintf(mypcFile, "Device with address 0x%x with\r\n", addr7bit);
        fprintf(mypcFile, "Temp = %d degC\n", temp_data);



         
        wait_us(1000000);
        
            

    }
    

    
}
