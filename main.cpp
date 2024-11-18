
//Basic LCD control!!!

#include "mbed.h"
#include <stdio.h>
#include <cstdint>
#include "LCD_ST7066U.h"

LCD lcd(PB_15, PB_14, PB_10, PA_8, PB_2, PB_1); // Adjust pin names as per your hardware

int main()
{
    lcd.writeLine("RPM Control :)", 0); // Write text to the first line
    while (true)
    {
        lcd.writeLine("Initializing...", 1); // Write text to the second line
        wait_us(500000);
        lcd.clear();
        
    }

}
