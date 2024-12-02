Fan Controller Mbed OS Example
The example project is a fan controller system using the NUCLEO-F070RB board and Mbed OS. It demonstrates how to implement a variable-speed fan control system using PWM, rotary encoder input, LCD display, and PID control for closed-loop feedback. The system operates in multiple modes, including off, open-loop control, closed-loop control, and automatic temperature-based control.

This project is intended for embedded systems applications and is suitable for use in standalone products without PC I/O.

Mbed OS Build Tools
Mbed CLI
Install Mbed CLI:

Follow the installation guide to install Mbed CLI on your system.

Import the Example:

From the command line, run:

bash
Copy code
mbed import https://github.com/yourusername/fan-controller.git
Replace yourusername with your GitHub username if applicable.

Change Directory:

Navigate to the imported project directory:

bash
Copy code
cd fan-controller
Application Functionality
The main() function initializes the system and enters a loop where it handles user input and controls the fan speed based on the selected mode. The application supports the following modes:

Off Mode: The fan is turned off.
Open-Loop Control Mode: The fan speed is controlled directly via PWM without feedback.
Closed-Loop Control Mode (PID): The fan speed is controlled using a PID controller to maintain a desired RPM.
Automatic Mode: The fan speed is adjusted automatically based on input from a temperature sensor.
Features
Variable Fan Speed Control using a rotary encoder.
LCD Display showing current mode and target RPM.
Accurate RPM Measurement using tachometer input with microsecond precision.
PID Control for precise closed-loop control of fan speed.
User Interface: Mode selection via button presses.
Safety Mechanisms: Timeout and debounce logic to ensure reliable operation.
Building and Running
Connect Hardware Components:

NUCLEO-F070RB Development Board.
Extension Board for 12V fan control.
12V Brushless DC Fan with tachometer output.
Rotary Encoder connected to the specified pins.
LCD Screen connected according to the pin configuration.
Temperature Sensor (for automatic mode).
Connect the Board to Your PC:

Use a USB cable to connect the Nucleo board to your PC.
Compile the Example Project:

Run the following command to build the project and program the microcontroller flash memory:

bash
Copy code
mbed compile -m NUCLEO_F070RB -t <TOOLCHAIN> --flash
Replace <TOOLCHAIN> with your preferred toolchain (GCC_ARM, ARM, or IAR). Your PC may take a few minutes to compile your code.

Flash the Binary:

If not using the --flash option, you can manually copy the binary from:

bash
Copy code
./BUILD/NUCLEO_F070RB/<TOOLCHAIN>/fan-controller.bin
Copy the binary to the Nucleo board, which appears as a USB mass storage device.

Expected Output
The LCD displays the current mode and target RPM.
The fan speed adjusts according to the selected mode and user inputs.
In closed-loop mode, the PID controller maintains the desired RPM.
In automatic mode, the fan speed changes based on the temperature sensor input.
Serial output (optional) provides debugging information at a baud rate of 19200.
Usage Instructions
Select Mode:

Press the user button on the Nucleo board (BUTTON1) to cycle through the modes:
M: OFF
M: Open Loop
M: Closed Loop
M: AUTO
Set Target RPM:

In Open Loop and Closed Loop modes, use the rotary encoder to adjust the target RPM.
The target RPM is displayed on the LCD.
Monitor Fan Speed:

The system measures the actual fan RPM and can display it via serial output for debugging.
Automatic Mode:

In AUTO mode, the fan speed adjusts automatically based on the temperature sensor reading.
Troubleshooting
Fan Not Spinning:

Ensure the fan is connected properly and the power supply is adequate.
Check that the system is not in OFF mode.
No LCD Display:

Verify connections to the LCD.
Ensure the correct pins are configured in the code.
Incorrect RPM Readings:

Check the tachometer connection.
Verify the number of pulses per revolution used in the RPM calculation matches the fan's specification.
Oscillations in Closed-Loop Mode:

Adjust the PID parameters (Kc, tauI, tauD) in the code to stabilize the control loop.
Hardware Requirements
NUCLEO-F070RB Board.
12V Brushless DC Fan (3-wire with tachometer).
Extension Board for driving the fan.
Rotary Encoder for user input.
LCD Screen for display.
Temperature Sensor (optional for automatic mode).
Push Button (onboard BUTTON1).
LEDs (optional for status indication).
Software Requirements
Mbed OS (compatible with NUCLEO-F070RB).

Mbed CLI.

Libraries:

LCD_ST7066U for LCD control.
mRotaryEncoder for rotary encoder input.
PID library for closed-loop control.
Pin Configuration
LCD:

PB_15, PB_14, PB_10, PA_8, PB_2, PB_1
Rotary Encoder:

PA_1, PA_4
Fan PWM Output:

PB_0
Fan Tachometer Input:

PA_0
Button:

BUTTON1
LEDs:

LED1, PC_0
Related Links
Mbed OS Documentation
NUCLEO-F070RB Board Information
PID Control Theory
Arm Mbed CLI
License and Contributions
The software is provided under the Apache-2.0 license. Contributions to this project are accepted under the same license. Please see CONTRIBUTING.md for more information.

This project was developed as part of a coursework activity to build an understanding of programming an embedded processor-based system using C.

Acknowledgments
Coursework Guidance: Thanks to the instructional team for providing foundational materials and hardware schematics.
Libraries Used:
LCD_ST7066U
mRotaryEncoder
PID library adapted from Brett Beauregard's Arduino PID library.
Notes
Serial Communication: The serial port is configured with a baud rate of 19200 for debugging purposes.
Safety Precautions:
Ensure all power supplies match the required voltage levels.
Double-check wiring connections before powering the system.
Contributions
Contributions are welcome. Please follow the standard guidelines for submitting issues and pull requests.

Contact Information
For any questions or support, please contact Your Name.

Disclaimer: This project is intended for educational purposes as part of coursework and is not designed for commercial deployment.