=========================================
               FAN CONTROLLER SYSTEM
=========================================

A versatile embedded fan control system for precise and automated speed regulation based on user input and environmental conditions.

-----------------------------------------
               TABLE OF CONTENTS
-----------------------------------------
1. About the Project
2. Features
3. Usage
   - LED Guide
   - LCD Screen Format
4. Control Modes
5. System Architecture
6. Getting Started
   - Prerequisites
   - Installation Steps
7. Troubleshooting
8. Future Enhancements
9. Contact
10. Acknowledgments

-----------------------------------------
            ABOUT THE PROJECT
-----------------------------------------
The Fan Controller System dynamically controls a DC fan’s speed based on user input, target RPM, or temperature. It uses PID control for precise regulation and supports multiple operating modes, including open-loop and temperature-based automatic control.

-----------------------------------------
               FEATURES
-----------------------------------------
- Multiple control modes: OFF, Closed-Loop RPM control, Open-Loop, Auto (temperature-based), Calibration.
- PID-based control for accurate RPM maintenance.
- Real-time user interface with a rotary encoder, LCD, and LEDs.
- Automatic temperature-based adjustments.
- Clear visual feedback through LEDs.

-----------------------------------------
                USAGE
-----------------------------------------
The Fan Controller System is designed for intuitive use. Follow the steps below to operate the system:

1. **Powering On**  
   Connect the system to a power source. The LCD will display `M: OFF`, indicating that the system is in **OFF Mode**.

2. **Navigating Modes**  
   - Press the **button** to cycle through the following modes:  
     `OFF → Closed-Loop → Open-Loop → Auto → Calibration → OFF`.
   - The current mode is displayed on the **LCD** in the format:  
     `M: XX. T/TT= XXXX`, where `XX` indicates the mode.

3. **Adjusting Parameters**  
   - Use the **rotary encoder** to adjust the target **RPM** (Closed-Loop or Open-Loop modes) or target **temperature** (Auto mode).  
   - The adjusted target is reflected on the **LCD**:  
     - `T= XXXX` for RPM-based modes.  
     - `TT= XXXX` for temperature-based Auto mode.

4. **Reading LCD Output**  
   - The LCD has two lines:  
     - **Line 1:** Displays mode (`M`), target RPM/temperature (`T/TT`).  
     - **Line 2:** Displays actual temperature (`AT`) and actual RPM (`RPM`).  
   - Example:  
     ```
     M: CL. T= 1500
     AT=25. RPM= 1450
     ```
     This indicates Closed-Loop mode with a target RPM of 1500, current temperature of 25°C, and actual RPM of 1450.

5. **LED Indicators**  
   The **LEDs** provide additional system feedback:  
   - **LED BI (Bidirectional):**  
     - **Green:** High-speed operation (>1750 RPM).  
     - **Red:** Low-speed operation (<200 RPM).  
     - **Flashing:** Calibration in progress.  
   - **LED2:**  
     - **ON:** Indicates a stalled fan (duty cycle > 0, RPM = 0).  
     - **OFF:** Normal operation or OFF mode.

6. **Calibration Mode**  
   - When in **Calibration Mode**, the fan will automatically run at various duty cycles to map RPM-to-duty cycle characteristics.  
   - Progress is displayed on the LCD in the format:  
     ```
     Calibrating...
     Max RPM: XXXX
     Min RPM: XXXX
     ```
   - Wait for the calibration to complete. The system will indicate completion by stopping the fan and showing the calibration results on the LCD.

7. **Stopping the Fan**  
   - To stop the fan, switch to **OFF Mode** by pressing the button until `M: OFF` is displayed on the LCD.  
   - In this mode, the fan will remain off (0% duty cycle).

### Tips for Optimal Usage:
- **Temperature Control in Auto Mode:**  
  Ensure the temperature sensor is properly connected and functional. The system adjusts the fan speed to maintain the set temperature (`TT`).
  
- **Closed-Loop and Open-Loop Modes:**  
  In Closed-Loop mode, the fan speed dynamically adjusts to match the target RPM using PID control. Open-Loop mode directly sets the fan speed based on a pre-defined duty cycle curve.

-----------------------------------------
         CONTROL MODES
-----------------------------------------
1. **OFF Mode:** Fan is off (0% duty cycle).
2. **Closed-Loop Mode:** Maintains user-defined RPM using PID control.
3. **Open-Loop Mode:** Duty cycle-based fan control without feedback.
4. **Auto Mode:** Adjusts fan speed based on ambient temperature.
5. **Calibration Mode:** Maps duty cycle to RPM for fan characterization.

-----------------------------------------
          SYSTEM ARCHITECTURE
-----------------------------------------
The system runs on an STM32 NUCLEO-F070RB, interfacing with:
- Fan (PWM-controlled).
- LCD for real-time data.
- Rotary Encoder for user input.
- LEDs for status indication.

### Pin Assignments
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
            GETTING STARTED
-----------------------------------------
### Prerequisites
- STM32 NUCLEO-F070RB board.
- DC fan with tachometer.
- Rotary encoder.
- LCD compatible with ST7066U.
- I2C-based temperature sensor.

### Installation Steps
1. Clone the repository: 
'git clone https://github.com/requiem002/ie_cw.git
2. Open the project in Mbed Studio.
3. Compile the code and flash it to the STM32 board.
4. Connect hardware as per the **Pin Assignments**.


-----------------------------------------
         TROUBLESHOOTING
-----------------------------------------
**Common Issues and Solutions:**

- **Fan not spinning:**  
- Ensure the system is not in OFF mode.  
- Verify fan wiring and ensure both LED BI and LED2 are not red simultaneously.

- **Incorrect RPM readings:**  
- Check the tachometer connections for proper wiring.  
- Ensure there is no noise or signal interruption.

- **LCD not responding:**  
- Verify connections and contrast settings.  
- Ensure the system is powered on and in an active mode.  

- **System unresponsive to button or encoder input:**  
- Check hardware connections.  
- Ensure no physical damage to the components.  

-----------------------------------------
      FUTURE ENHANCEMENTS
-----------------------------------------
- EEPROM storage for user settings.
- Advanced menu for parameter tuning.
- Hysteresis in temperature control logic.

-----------------------------------------
            CONTACT
-----------------------------------------
For queries or feedback, contact:
- **Saad Ahmed:** sa2879@bath.ac.uk  
- **Tom Hunter:** th970@bath.ac.uk

-----------------------------------------
         ACKNOWLEDGMENTS
-----------------------------------------
Special thanks to Sanjae King and Professor Despina Moschou for their guidance.

**Libraries Used:**
- LCD_ST7066U by Luis Rodriguez
- mRotaryEncoder-os by Karl Zweimüller
- PID by Brett Beauregard
