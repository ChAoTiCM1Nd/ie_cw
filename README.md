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