# Fan Controller System

## Overview

This project implements a software control system for a variable-speed fan using an STM32-based NUCLEO-F070RB development board. The system allows users to control the fan speed through various inputs, including a rotary encoder and temperature sensor, and displays information on an LCD screen. The controller operates in standalone mode without relying on PC I/O, suitable for embedded applications.

## Features

- **Variable Fan Speed Control**: Adjust fan speed using a rotary encoder.
- **Multiple Control Modes**:
  - **Off Mode**: Fan is turned off.
  - **Open-Loop Control**: Direct control of fan speed without feedback.
  - **Closed-Loop Control (PID)**: Maintains desired RPM using a PID algorithm.
  - **Automatic Mode**: Automatically adjusts fan speed based on temperature input.
- **Real-Time RPM Measurement**: Accurate RPM readings using tachometer input with microsecond precision.
- **LCD Display**: Shows current mode, target RPM, and system status.
- **User Interface**: Simple mode switching via onboard button.
- **Debounce Logic**: Reliable input handling for rotary encoder and button presses.
- **Safety Features**: Timeout mechanisms to handle fan stoppage and prevent erroneous readings.

## Hardware Requirements

- **NUCLEO-F070RB Development Board** (STM32F070RB MCU)
- **Extension Board** (12V drive capability)
- **12V Brushless DC Fan** with tachometer output (3-wire fan)
- **Rotary Encoder** for user input
- **LCD Screen** (compatible with LCD_ST7066U driver)
- **Temperature Sensor** (for automatic mode, e.g., LM35)
- **LEDs** for status indication
- **Additional Components**:
  - Pull-up resistors
  - Push buttons
  - Wires and connectors

## Software Requirements

- **Mbed OS**: ARM Mbed operating system
- **Mbed Studio**: Development environment
- **C/C++ Compiler**: Compatible with ARM Cortex-M processors
- **Libraries**:
  - `LCD_ST7066U`: For LCD display control
  - `mRotaryEncoder`: For rotary encoder handling
  - `PID`: For PID control algorithm

## Installation and Setup

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/yourusername/fan-controller-system.git

### Import Project into Mbed Studio
1. Open Mbed Studio.
2. Import the project folder.

### Connect Hardware Components
1. Assemble the extension board with the NUCLEO-F070RB.
2. Connect the fan, rotary encoder, LCD, temperature sensor, and LEDs according to the pin assignments in the code.

### Pin Configuration (as per code)
- **LCD**: PB_15, PB_14, PB_10, PA_8, PB_2, PB_1
- **Rotary Encoder**: PA_1, PA_4
- **Fan PWM Output**: PB_0
- **Fan Tachometer Input**: PA_0
- **LEDs**: PC_0, LED1
- **Button**: BUTTON1

### Install Required Libraries
Ensure that all required libraries (`LCD_ST7066U`, `mRotaryEncoder`, `PID`) are included in your project.

### Compile the Code
Build the project in Mbed Studio.

### Flash the Microcontroller
1. Connect the Nucleo board to your PC via USB.
2. Flash the compiled binary to the microcontroller.

---

## Usage Instructions

### Power On the System
1. Connect the power supply to the extension board (ensure it provides 12V for the fan).
2. The system will initialize and display the default mode on the LCD.

### Mode Selection
1. Press the onboard button (`BUTTON1`) to cycle through the available modes:
   - **M: OFF**: Fan is turned off.
   - **M: Closed Loop**: Closed-loop control using PID.
   - **M: Open Loop**: Open-loop control without feedback.
   - **M: AUTO**: Automatic control based on temperature sensor input.

### Adjust Fan Speed
1. In **Closed Loop** and **Open Loop** modes, use the rotary encoder to set the target RPM.
2. The LCD will display the target RPM.

### Monitoring
1. The LCD displays real-time information, such as current mode and target RPM.
2. Use serial debugging (if necessary) via the USB connection at `19200` baud rate.

### Automatic Mode
1. In **AUTO** mode, the fan speed adjusts automatically based on the temperature sensor readings.
2. Ensure the temperature sensor is properly connected and calibrated.

### Shutdown
To turn off the fan, switch to **OFF** mode using the button.

---

## System Architecture

### Main Components
- **Main Loop**:
  - Handles mode updates and calls the appropriate control handler based on the current mode.
- **Control Handlers**:
  - `handle_off_ctrl()`: Stops the fan.
  - `handle_open_loop_ctrl()`: Controls fan speed without feedback.
  - `handle_closed_loop_ctrl()`: Implements PID control for precise RPM maintenance.
  - `handle_auto_ctrl()`: Adjusts fan speed based on temperature input.
- **Interrupts**:
  - **Tachometer Input**: Uses interrupts to count fan pulses for RPM calculation.
  - **Rotary Encoder**: Reads user input for setting target RPM.
- **PID Controller**:
  - Uses a PID library to compute the required duty cycle to maintain the desired RPM.
  - Parameters (`Kc`, `tauI`, `tauD`) are adjustable for tuning system performance.
- **RPM Calculation**:
  - Utilizes microsecond timing for accurate RPM measurements.
  - Implements timeout logic to reset RPM when the fan stops.

### Pin Assignments
| Component           | Pin(s)                        |
|----------------------|-------------------------------|
| **LCD**             | PB_15, PB_14, PB_10, PA_8, PB_2, PB_1 |
| **Rotary Encoder**  | PA_1, PA_4                    |
| **Fan PWM Output**  | PB_0                          |
| **Fan Tachometer**  | PA_0                          |
| **LEDs**            | PC_0, LED1                   |
| **Button**          | BUTTON1                      |
| **Serial Debugging**| USBTX, USBRX                 |

---

## Customization and Tuning

### PID Parameters
Adjust the following to fine-tune the closed-loop control response:
- **Kc**: Proportional gain.
- **tauI**: Integral time constant.
- **tauD**: Derivative time constant.

### Control Loop Timing
Modify `tSample` to change the control loop interval.

### RPM Calculation
- Adjust the number of pulses counted (`pulse_count == 4`) based on the fan's tachometer specifications.
- Modify debounce timings if necessary.

---

## Safety Precautions

### Power Supply
Ensure that the power supply voltage matches the fan's rated voltage to prevent damage.

### Connections
Double-check all wiring before powering the system to avoid short circuits.

### Heat Management
Monitor the temperature of components, especially if running the fan at high speeds for extended periods.

---

## Troubleshooting

### Fan Not Spinning
- Check the power supply and connections to the fan.
- Ensure the system is not in **OFF** mode.

### Incorrect RPM Readings
- Verify tachometer connections.
- Ensure the fan's pulses per revolution are correctly configured in the code.

### Oscillations in Fan Speed
Fine-tune the PID parameters to stabilize the control loop.

### No Display on LCD
- Check connections to the LCD.
- Ensure the `LCD_ST7066U` library is properly included and configured.

---

## Contributions
Contributions are welcome. Please follow the standard guidelines for submitting issues and pull requests.

---

## License
This project is provided under the MIT License. See the `LICENSE` file for details.

---

## Acknowledgments
- **Coursework Guidance**: Thanks to the instructional team for providing the foundational materials and hardware schematics.
- **Libraries Used**:
  - `LCD_ST7066U` by [Author/Organization]
  - `mRotaryEncoder` by [Author/Organization]
  - `PID` library adapted from Brett Beauregard's Arduino PID library.

This project was developed as part of a coursework activity to build an understanding of programming an embedded processor-based system using C.





