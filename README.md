<!-- Improved compatibility of back to top link: See: https://github.com/othneildrew/Best-README-Template/pull/73 -->
<a id="readme-top"></a>
<!--
*** Thanks for checking out the Best-README-Template. If you have a suggestion
*** that would make this better, please fork the repo and create a pull request
*** or simply open an issue with the tag "enhancement".
*** Don't forget to give the project a star!
*** Thanks again! Now go create something AMAZING! :D
-->

<!-- Improved Fan Controller README -->

<a id="readme-top"></a>

[![Contributors][contributors-shield]][contributors-url]
[![Issues][issues-shield]][issues-url]
[![License][license-shield]][license-url]
[![Saad's LinkedIn][linkedin-shield]][saad-linkedin-url]
[![Tom's LinkedIn][linkedin-shield]][tom-linkedin-url]

<br />
<div align="center">
  <a href="https://github.com/requiem002/ie_cw">
    <img src="resources/fan.gif" alt="Fan Controller Logo" width="120" height="120">
  </a>

  <h1 align="center">Fan Controller System</h1>

  <p align="center">
    A versatile embedded fan control system for precise and automated speed regulation based on user input and environmental conditions.
    <br />
    <a href="https://github.com/requiem002/ie_cw"><strong>Explore the Docs »</strong></a>
    <br />
    ·
    <a href="https://github.com/requiem002/ie_cw/issues/new?labels=bug&template=bug-report---.md">Report Bug</a>
    ·
    <a href="https://github.com/requiem002/ie_cw/issues/new?labels=enhancement&template=feature-request---.md">Request Feature</a>
  </p>
</div>

---

## Table of Contents

1. [About the Project](#about-the-project)
   - [Key Features](#key-features)
   - [Built With](#built-with)
2. [System Architecture](#system-architecture)
   - [Control Modes](#control-modes)
   - [PID Control Overview](#pid-control-overview)
   - [Pin Assignments](#pin-assignments)
3. [Getting Started](#getting-started)
   - [Prerequisites](#prerequisites)
   - [Installation & Setup](#installation--setup)
   - [Hardware Connections](#hardware-connections)
4. [Usage](#usage)
   - [Interacting with the System](#interacting-with-the-system)
   - [Tuning the PID Parameters](#tuning-the-pid-parameters)
5. [Troubleshooting](#troubleshooting)
6. [Roadmap](#roadmap)
7. [Contributing](#contributing)
8. [License](#license)
9. [Contact](#contact)
10. [Acknowledgments](#acknowledgments)

---

## About the Project

The Fan Controller System is designed to control a DC fan’s speed dynamically based on user input, target RPM, or environmental conditions such as temperature. It leverages a PID control loop for closed-loop accuracy and offers multiple modes, including open-loop and automatic (temperature-based) operation.

Whether you need a stable RPM under varying loads, a fan speed that responds to temperature changes, or a manual tuning mode, this controller provides a flexible and extensible solution for embedded applications. It runs on an STM32 NUCLEO-F070RB board, interfacing with an LCD, rotary encoder, temperature sensor, LEDs, and a push button for full standalone operation—no PC required after deployment.

### Key Features
- **Multiple Control Modes**: Choose between OFF, Closed-Loop RPM control, Open-Loop control, Auto Temperature-based control, and Calibration mode.
- **PID-based Closed-Loop**: Precisely maintain target RPM under changing conditions.
- **User-Friendly Interface**: Adjust setpoints with a rotary encoder, view status on an LCD, and use a button to cycle through modes.
- **Temperature Feedback**: Automatically increase or decrease fan speed to maintain desired temperature.
- **Dynamic Display & Indicators**: LCD output for RPM, temperature, and mode status, plus LED indicators for quick status checks.

### Built With
- **C/C++** with Mbed OS
- **STM32 NUCLEO-F070RB Development Board**
- **LCD_ST7066U Library**
- **mRotaryEncoder Library**
- **PID Controller Library**

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## System Architecture

<img src="https://github.com/requiem002/ie_cw/blob/master/resources/system_arch.jpg" width=60%>
The firmware continuously reads sensors, updates control parameters, and drives the fan accordingly. It runs in a loop, calling different handlers based on the current mode.


### Control Modes
- **OFF**: Fan is turned off (0% duty cycle).
- **Closed-Loop (ENCDR_C_LOOP)**: Uses PID to maintain a user-defined RPM set via the rotary encoder.
- **Open-Loop (ENCDR_O_LOOP)**: Sets fan speed based on a predetermined duty cycle curve and target RPM input, without feedback correction.
- **AUTO**: Adjusts fan speed automatically based on the measured temperature, employing a PID-like approach to reach the target temperature.
- **CALIB**: Attempts to map duty cycle to RPM by stepping down from full speed, useful for characterizing the fan.

### PID Control Overview
In closed-loop and auto modes, a PID controller adjusts the PWM duty cycle. The parameters `Kc` (Proportional), `tauI` (Integral), and `tauD` (Derivative) can be tuned to optimize system responsiveness and stability.

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

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Getting Started

To run this project locally, follow these steps to set up your environment and hardware.

### Prerequisites
- Mbed Studio or another Mbed-enabled IDE
- NUCLEO-F070RB Board
- External DC fan with tachometer output
- Rotary encoder
- LCD compatible with ST7066U
- Temperature sensor compatible with I2C interface
- Basic soldering and wiring tools

### Installation & Setup
1. **Clone the Repository**:
   ```bash
   git clone https://github.com/requiem002/ie_cw.git
2. **Open the Project in Mbed Studio**
   - Launch MBED Studio
   - Import the cloned folder as an MBED project.

3. **Install Libraries**:
   - Ensure all required libraries are included in `mbed_app.json` or imported as libraries.

4. **Compile & Flash**
    - Connect the NUCLEO board via USB.
    - Build the project in Mbed Studio.
    - Drag and drop the compiled `.bin` file onto the NUCLEO drive.

---

## Hardware Connections

Refer to the **pin assignment table** above. Connect the fan, tachometer, encoder, LCD, and temperature sensor as indicated. Double-check all wiring before powering on.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Usage

Once powered, the LCD will show the current mode. Use the push button to cycle through modes:

- **OFF Mode**: Fan is off.
- **Closed-Loop Mode**: Turn the rotary encoder to set a target RPM. The PID controller maintains this RPM.
- **Open-Loop Mode**: Turn the encoder to set a target RPM, but the duty cycle is determined by a predefined curve, not PID feedback.
- **AUTO Mode**: Turn the encoder to set a target temperature. The fan will speed up or slow down to maintain this temperature.
- **CALIB Mode**: The system automatically reduces duty cycle from 100% downwards, mapping RPM vs. duty cycle for future reference.

---

## Interacting with the System

- **Rotary Encoder**: Adjust target RPM or temperature depending on the mode.
- **Button**: Cycle through the five modes.
- **LCD & LEDs**: Monitor current RPM, duty cycle, setpoint, and temperature. LED colors and LCD lines help visualize the system state.

---

## Troubleshooting

- **Fan Not Spinning**:
  - Check power supply and ensure you’re not in OFF mode.
- **Incorrect RPM Readings**:
  - Verify tachometer wiring. Make sure the pulse-per-revolution assumption matches your fan’s specifications.
- **LCD Not Displaying**:
  - Confirm LCD pin connections and contrast settings.
- **Inaccurate performance with new fan **:
  - Run the calibration mode to obtain and automatically set correct parameters for a new fan.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---

## Roadmap

- Add EEPROM storage for PID parameters and user preferences.
- Implement a menu system for switching modes and setting parameters via the encoder.
- Integrate a more sophisticated temperature sensor for improved accuracy.
- Add fan fault detection and notifications.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

---


<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Top contributors:

<a href="https://github.com/requiem002/ie_cw/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=requiem002/ie_cw" alt="contrib.rocks image" />
</a>



<!-- LICENSE -->
## License

Distributed under the project_license. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

Saad Ahmed sa2879@bath.ac.uk
Tom Hunter th970@bath.ac.uk

Project Link: [https://github.com/requiem002/ie_cw](https://github.com/requiem002/ie_cw)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

* **Coursework Guidance**: Thanks to Sanjae King and Professor Despina Moschou for providing the foundational materials and hardware schematics.
* **Generative AI**: Acknowledgments to ChatGPT by OpenAI, along with Claude Opus by Anthropic for code checking and guidance.
* **Libraries Used**:
  - `LCD_ST7066U` library by Luis Rodriguez [https://os.mbed.com/users/luisfrdr/code/LCD_ST7066U/]
  - `mRotaryEncoder-os` library by Karl Zweimüller [https://os.mbed.com/users/charly/code/mRotaryEncoder-os/]
  - `PID` library adapted from Brett Beauregard's Arduino PID library.
This project was developed as part of a coursework activity to build an understanding of programming an embedded processor-based system using C.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/requiem002/ie_cw.svg?style=for-the-badge
[contributors-url]: https://github.com/requiem002/ie_cw/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/requiem002/ie_cw.svg?style=for-the-badge
[forks-url]: https://github.com/requiem002/ie_cw/network/members
[stars-shield]: https://img.shields.io/github/stars/requiem002/ie_cw.svg?style=for-the-badge
[stars-url]: https://github.com/requiem002/ie_cw/stargazers
[issues-shield]: https://img.shields.io/github/issues/requiem002/ie_cw.svg?style=for-the-badge
[issues-url]: https://github.com/requiem002/ie_cw/issues
[license-shield]: https://img.shields.io/github/license/requiem002/ie_cw.svg?style=for-the-badge
[license-url]: https://github.com/requiem002/ie_cw/blob/master/LICENSE.txt
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[saad-linkedin-url]: https://www.linkedin.com/in/saadahmed02/
[tom-linkedin-url]: https://www.linkedin.com/in/tomehunter/
[product-screenshot]: images/screenshot.png
[Next.js]: https://img.shields.io/badge/next.js-000000?style=for-the-badge&logo=nextdotjs&logoColor=white
[Next-url]: https://nextjs.org/
[React.js]: https://img.shields.io/badge/React-20232A?style=for-the-badge&logo=react&logoColor=61DAFB
[React-url]: https://reactjs.org/
[Vue.js]: https://img.shields.io/badge/Vue.js-35495E?style=for-the-badge&logo=vuedotjs&logoColor=4FC08D
[Vue-url]: https://vuejs.org/
[Angular.io]: https://img.shields.io/badge/Angular-DD0031?style=for-the-badge&logo=angular&logoColor=white
[Angular-url]: https://angular.io/
[Svelte.dev]: https://img.shields.io/badge/Svelte-4A4A55?style=for-the-badge&logo=svelte&logoColor=FF3E00
[Svelte-url]: https://svelte.dev/
[Laravel.com]: https://img.shields.io/badge/Laravel-FF2D20?style=for-the-badge&logo=laravel&logoColor=white
[Laravel-url]: https://laravel.com
[Bootstrap.com]: https://img.shields.io/badge/Bootstrap-563D7C?style=for-the-badge&logo=bootstrap&logoColor=white
[Bootstrap-url]: https://getbootstrap.com
[JQuery.com]: https://img.shields.io/badge/jQuery-0769AD?style=for-the-badge&logo=jquery&logoColor=white
[JQuery-url]: https://jquery.com 
