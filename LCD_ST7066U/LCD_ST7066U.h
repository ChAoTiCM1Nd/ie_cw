/*
 *     File Name: LCD_ST7066U.h
 *       Version: 0.1
 *          Date: 2016-04-1i
 *        Author: Luis F Rodriguez
 *       Company: BIOS (Biological Innovation and Optimization Services)
 *
 *   Description: This is the header file to be included in any project utilizing a
 *                16x2 LCD screen based on the ST7066U processor being controlled with
 *                an mbed-enabled LPC11U68 with a 4-bit interface. It utilizes the
 *                resources provided by the mbed platform to abstract the functioning
 *                underlying this processor.
 *
 */

#ifndef LCD_ST7066U_H
#define LCD_ST7066U_H

/*
 * MARK: Include Files
 * Any header files to be included in the project are found in this section.
 */
 
#include "mbed.h"

/*
 * MARK: Macro Definition
 *   The defitions found below were included to increase readability of the code, as it gives
 *   each command a name instead of just the value itself.
 */
 
#define wakeUp 0x30       // 
#define functionSet 0x28 // Sets the interface data type to 4 bits, 2 lines, and font size 5x8. 0x28 = 0010 1000
#define returnHome 0x02    // Sets the cursor to the beginning of the screen. 0x02 = 0000 0010
#define setCursor 0x10 // Sets cursor moving and display shift, and direction without changing DDRAM data. 0x10 = 0001 0000
#define setDisplay 0x0C   // Sets Entire Display on, Display on, Cursor on. 0x0C = 0000 1011
#define entryModeSet 0x06 // Sets Entry Mode. 0x06 = 0000 0110
#define clearDisplay 0x01 // Resets the whole display to blank

/*
 * MARK: Class Definition
 *   The class defined below abstracts the processes that control an LCD screen
 *   running on the ST7066U processor. The public functions are for writing a full
 *   line, writing a character at a specific location and clearing the full screen.
 */
 
class LCD {
public:
    LCD(PinName rs, PinName e, PinName d4, PinName d5, PinName d6, PinName d7);
    void clear();
    void write(const char* text);
    void writeLine(const char* text, int line);
    void writeCharacter(const char c, int row, int column);
    void writeCharacters(const char* text, int row, int column);
private:
    void init();
    void startUp();
    void locate(int row, int column);
    void writeCharacter(char value);
    void writeCommand(char value);
    void writeData(char value);
    void writeTime();
    void character (int row, int column, char c);
    int rows() {return 2;}
    int columns() {return 16;}
    int address(int row, int column);
    DigitalOut _rs;
    DigitalOut _e;
    DigitalOut _d4;
    DigitalOut _d5;
    DigitalOut _d6;
    DigitalOut _d7;
    int _row;
    int _column;
};

#endif