=========================================
               FAN CONTROLLER SYSTEM
=========================================

A versatile embedded fan control system for precise and automated speed regulation 
based on user input and environmental conditions.

-----------------------------------------
               TABLE OF CONTENTS
-----------------------------------------
1. About the Project
   - Key Features
   - Built With
2. System Architecture
   - Control Modes
   - Pin Assignments
3. Getting Started
   - Prerequisites
   - Installation & Setup
4. Usage
5. Troubleshooting
6. Future Enhancements
7. Contact
8. Acknowledgments

-----------------------------------------
            ABOUT THE PROJECT
-----------------------------------------
The Fan Controller System controls a DC fan’s speed dynamically based on user input, 
target RPM, or environmental conditions like temperature. It uses a PID control loop 
for accuracy and supports multiple modes, including open-loop and automatic (temperature-based) operation.

**Key Features:**
- Multiple control modes: OFF, Closed-Loop RPM control, Open-Loop, Auto (temperature-based), and Calibration.
- PID-based control for precise RPM maintenance.
- User interface with a rotary encoder, LCD, and button for mode selection.
- Automatic temperature-based adjustments.
- Real-time feedback via LCD and LEDs.

**Built With:**
- C++
- MBED OS
- STM32 NUCLEO-F070RB Board

**Pin Assignments:**
| Component        | Pins                                  |
|------------------|---------------------------------------|
| LCD              | PB_15, PB_14, PB_10, PA_8, PB_2, PB_1 |
| Rotary Encoder   | PA_1, PA_4                            |
| Fan PWM Output   | PB_0                                  |
| Fan Tachometer   | PA_0                                  |
| LEDs             | PC_0, LED1                            |
| Button           | BUTTON1                               |
| Serial Debug     | USBTX, USBRX                          |

-----------------------------------------
          SYSTEM ARCHITECTURE
-----------------------------------------
The system runs on an STM32 NUCLEO-F070RB, interfacing with:
- Fan (PWM-controlled).
- LCD for real-time data.
- Rotary Encoder for input.
- LEDs for system status.

**Modes of Operation:**
1. **OFF Mode**: Fan off (0% duty cycle).
2. **Closed-Loop Mode**: Maintains user-defined RPM using PID control.
3. **Open-Loop Mode**: Controls fan speed without feedback, based on a duty cycle curve.
4. **AUTO Mode**: Adjusts fan speed based on temperature to maintain a target.
5. **Calibration Mode**: Maps duty cycle to RPM for fan characterization.

-----------------------------------------
            GETTING STARTED
-----------------------------------------
**Prerequisites:**
- Mbed Studio or other MBED-enabled IDE.
- NUCLEO-F070RB Board.
- DC fan with tachometer output.
- Rotary encoder.
- LCD compatible with ST7066U.
- Temperature sensor with I2C interface.

**Installation Steps:**
1. Clone the repository: `git clone https://github.com/requiem002/ie_cw.git`
2. Open the project in Mbed Studio.
3. Compile the project and flash it onto the board.

-----------------------------------------
                USAGE
-----------------------------------------
- Use the rotary encoder to adjust RPM or temperature (depending on mode).
- Use the button to toggle modes.
- Monitor fan status on the LCD.
- LEDs indicate fan behavior:

| **Condition**            | **LED BI (Bidirectional)** |**LED2**         | **Description**                          |
|--------------------------|----------------------------|-----------------|------------------------------------------|
| High RPM (>1750)         | Green                      | OFF             | Normal high-speed operation.             |
| Low RPM (<200)           | Red                        | OFF             | Normal low-speed operation.              |
| Stalled Fan              | Red                        | ON              | Fan is stalled despite a duty cycle > 0. |
| Calibration in Progress  | Alternating Flashing       | OFF             | Indicates calibration mode is active.    |
| OFF Mode                 | OFF                        | OFF             | System is powered off.                   |

-----------------------------------------
            TROUBLESHOOTING
-----------------------------------------
**Common Issues:**
- Fan not spinning: Check the power and mode (ensure it's not OFF).
- Incorrect RPM readings: Verify tachometer wiring.
- LCD issues: Check connections and contrast settings.

-----------------------------------------
         FUTURE ENHANCEMENTS
-----------------------------------------
- EEPROM storage for settings.
- Advanced menu system for parameter adjustments.
- Improved thermal control logic with hysteresis.

-----------------------------------------
               CONTACT
-----------------------------------------
For queries or feedback, contact:
- Saad Ahmed: sa2879@bath.ac.uk
- Tom Hunter: th970@bath.ac.uk

-----------------------------------------
            ACKNOWLEDGMENTS
-----------------------------------------
Special thanks to Sanjae King and Professor Despina Moschou for guidance.

**Libraries Used:**
- LCD_ST7066U by Luis Rodriguez
- mRotaryEncoder-os by Karl Zweimüller
- PID by Brett Beauregard