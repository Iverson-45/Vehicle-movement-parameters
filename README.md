# Vehicle-movement-parameters

The system calcualates and displays:
---------------------------------

Speed [km/h]

Acceleration [m/s^2]

Power [Hp]

Torque [Nm]

Inclination[°]


Hardware Architecture
-----

The system uses a Dual-MCU architecture to effectively split the processing and logging workload, ensuring high data throughput and display responsiveness.

1. Main MCU (Data Processor)

    Microcontroller: STM32F411CEU6

    Role: Handles all sensor data acquisition, complex sensor fusion algorithms (e.g., Complementary Filter for Inclination), and parameter calculation.

    Sensors: U-blox NEO-M8N (GPS) and BMI160 (IMU + Gyro).

2. Secondary MCU (Display & Logger)

    Microcontroller: STM32f303k8

    Role: Receives processed data from the Main MCU.

    Output: Drives the 2-inch TFT SPI Display and handles SD Card data logging.


Communication
------------
The two MCU's communicate witch each other by UART protocol exchanging information & data.
