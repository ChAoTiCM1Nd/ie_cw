<!-- Improved compatibility of back to top link: See: https://github.com/othneildrew/Best-README-Template/pull/73 -->
<a id="readme-top"></a>
<!--
*** Thanks for checking out the Best-README-Template. If you have a suggestion
*** that would make this better, please fork the repo and create a pull request
*** or simply open an issue with the tag "enhancement".
*** Don't forget to give the project a star!
*** Thanks again! Now go create something AMAZING! :D
-->



<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![project_license][license-shield]][license-url]
[![LinkedIn][linkedin-shield]][linkedin-url]



<!-- PROJECT LOGO -->
<br />
<div align="center">
  <a href="https://github.com/requiem002/ie_cw">
    <img src="fan.gif" alt="Logo" width="80" height="80">
  </a>

<h3 align="center">Fan Controller</h3>

  <p align="center">
    project_description
    <br />
    <a href="https://github.com/requiem002/ie_cw"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://github.com/requiem002/ie_cw">View Demo</a>
    ·
    <a href="https://github.com/requiem002/ie_cw/issues/new?labels=bug&template=bug-report---.md">Report Bug</a>
    ·
    <a href="https://github.com/requiem002/ie_cw/issues/new?labels=enhancement&template=feature-request---.md">Request Feature</a>
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project


[![Product Name Screen Shot][product-screenshot]](https://example.com)

Here's a blank template to get started. To avoid retyping too much info, do a search and replace with your text editor for the following: `github_username`, `repo_name`, `twitter_handle`, `linkedin_username`, `email_client`, `email`, `project_title`, `project_description`, `project_license`
This project implements a software control system for a variable-speed fan using an STM32-based NUCLEO-F070RB development board. The system allows users to control the fan speed through various inputs, including a rotary encoder and temperature sensor, and displays information on an LCD screen. The controller operates in standalone mode without relying on PC I/O, suitable for embedded applications.

<p align="right">(<a href="#readme-top">back to top</a>)</p>


<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

This is an example of how you may give instructions on setting up your project locally.
To get a local copy up and running follow these simple example steps.

### Prerequisites

This is an example of how to list things you need to use the software and how to install them.
* npm
  ```sh
  npm install npm@latest -g
  ```

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

## Installation and Setup
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
<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Usage

Use this space to show useful examples of how a project can be used. Additional screenshots, code examples and demos work well in this space. You may also link to more resources.

_For more examples, please refer to the [Documentation](https://example.com)_

<p align="right">(<a href="#readme-top">back to top</a>)</p>

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
<!-- ROADMAP -->
## Roadmap

- [ ] Feature 1
- [ ] Feature 2
- [ ] Feature 3
    - [ ] Nested Feature

See the [open issues](https://github.com/requiem002/ie_cw/issues) for a full list of proposed features (and known issues).

<p align="right">(<a href="#readme-top">back to top</a>)</p>



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
  <img src="https://contrib.rocks/image?repo=github_username/repo_name" alt="contrib.rocks image" />
</a>



<!-- LICENSE -->
## License

Distributed under the project_license. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

Saad Ahmed sa2879@bath.ac.uk
Tom Hunter th____@bath.ac.uk

Project Link: [https://github.com/requiem002/ie_cw](https://github.com/requiem002/ie_cw)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

* []()
* []()
* []()

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/github_username/repo_name.svg?style=for-the-badge
[contributors-url]: https://github.com/requiem002/ie_cw/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/github_username/repo_name.svg?style=for-the-badge
[forks-url]: https://github.com/requiem002/ie_cw/network/members
[stars-shield]: https://img.shields.io/github/stars/github_username/repo_name.svg?style=for-the-badge
[stars-url]: https://github.com/requiem002/ie_cw/stargazers
[issues-shield]: https://img.shields.io/github/issues/github_username/repo_name.svg?style=for-the-badge
[issues-url]: https://github.com/requiem002/ie_cw/issues
[license-shield]: https://img.shields.io/github/license/github_username/repo_name.svg?style=for-the-badge
[license-url]: https://github.com/requiem002/ie_cw/blob/master/LICENSE.txt
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url]: https://linkedin.com/in/linkedin_username
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
