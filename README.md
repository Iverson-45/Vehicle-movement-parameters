Vehicle Movement Parameters Meter 
------

Authors: Jakub Litwin, Mateusz Prądziński, Adrian Andrószowski, Błażej Szkobel 
Overview

The goal of this project is to design and build a prototype device capable of measuring, calculating, and displaying real-time vehicle movement parameters. To ensure high data throughput, low latency, and a responsive UI, the system utilizes a Dual-MCU architecture that effectively splits the workload between sensor processing and data logging/display.
Calculated & Displayed Parameters:

    Speed: [km/h] 

    Acceleration: [m/s^2] (Instantaneous acceleration) 

    Power: [Hp] 

    Torque: [Nm] 

    Inclination: [°] (Pitch / Roll) 

Hardware Architecture

The device operates on two separate microcontrollers communicating via UART.

1. Main MCU (Data Processor)

    Microcontroller: STM32F411CEU6 

    Role: Handles all sensor data acquisition, scaling, filtration, and complex sensor fusion algorithms.

    Sensors: U-blox NEO-M8N (GPS) and BMI160 (IMU + Gyroscope).

2. Secondary MCU (Display & Logger)

    Microcontroller: STM32f303k8 

    Role: Receives processed numerical data from the Main MCU and handles the user interface.

    Peripherals: Drives a 2.0-inch TFT SPI Display and handles .csv data logging to a microSD card.

Firmware & Communication

    Main MCU: Programmed in low-level C (registers) to highly optimize the code for its specific tasks. It uses interrupt-driven peripherals to minimize delays and ensure smooth data reception. It synchronizes 100 data reads per second from the BMI160 and 5 reads per second from the GPS.

    Auxiliary MCU: Written in C utilizing the HAL library. It implements heavily optimized external libraries from GitHub for the ST7789 display and SPI SD card.

    Inter-MCU Communication: The two microcontrollers communicate via UART. At startup, they perform a handshake to initiate communication. The Aux MCU sends the user-defined vehicle mass to the Main MCU, and the Main MCU continuously streams back the calculated data.

How It Works: Algorithms & Calibration

To make the device user-friendly, it does not require perfectly leveled physical mounting inside the vehicle. Instead, the data passes through a complex algorithmic pipeline divided into three distinct phases:

Phase 1: Gravity Calibration While the vehicle is completely stationary, the device measures the Earth's gravitational pull to determine the gravity vector relative to the device's physical orientation. This establishes the true Z-axis (downward force).

Phase 2: Longitudinal Axis Calibration (Direction) The user performs a short, straight-line acceleration. The device detects this forward motion vector to define the X-axis (front). Using the cross product of these established forces, the software builds an orthonormal basis. This creates a 3D rotation matrix that mathematically rotates all raw sensor vectors to perfectly align with the vehicle's actual coordinate system, automatically compensating for any "crooked" mounting.

Phase 3: Active Operation During driving, raw data from the accelerometer and gyroscope is continuously transformed using the generated rotation matrix. The system calculates the device's tilt angles from the accelerometer and applies a Complementary Filter to fuse this data with the gyroscope readings for smooth, accurate inclination and acceleration outputs.
Current State & Future Development

Semester 1 Achievements:
A fully working prototype has been built that successfully handles instrument communication, calibration algorithms, the user interface, display presentation, and SD card .csv logging.

Next Steps (Semester 2 Goals): 

    Design a dedicated power supply section.

    Create a complete device schematic and custom PCB layout.

    Design a physical 3D enclosure/case.

    Conduct extensive real-world measurement tests.

    Develop PC software to visualize the saved telemetry data.

## Preliminary Measurement Results

<img width="1400" height="1269" alt="Figure_1" src="https://github.com/user-attachments/assets/3de1d2e4-15c4-4e3c-b259-78ba988d6db6" />


During our initial field tests, the logged telemetry data was extracted and visualized using Python. We are pleased to report that the calculated power output closely aligns with the actual specifications of the vehicle used for testing.

**⚠️ Note:** Please keep in mind that this is an early prototype and not the final version of the system. The hardware architecture and calculation algorithms are still actively being refined. Further calibration, optimization, and extensive real-world validations are planned for the upcoming semester.
