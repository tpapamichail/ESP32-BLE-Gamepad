# BLE Gamepad with ESP32

Διαθέσιμο και στα [Ελληνικά](README.el.md).

A comprehensive example of implementing a wireless gamepad using the ESP32 and the `BleGamepad` library. This project allows you to create a custom gamepad that connects via Bluetooth to devices such as computers, smartphones, and tablets.

## Table of Contents

- [Description](#description)
- [Features](#features)
- [Requirements](#requirements)
- [Hardware Setup](#hardware-setup)
- [Installation](#installation)
- [Usage](#usage)
- [Customization](#customization)
- [License](#license)
- [Contributing](#contributing)
- [Acknowledgments](#acknowledgments)

## Description

This project demonstrates how to create a BLE gamepad using an ESP32. The ESP32 functions as a Bluetooth Low Energy (BLE) device recognized as a gamepad by other devices. You can connect buttons to specific GPIO pins of the ESP32 and use them to send commands to connected devices.

## Features

- **Supports 16 buttons**: Including standard buttons like A, B, X, Y, L1, R1, L2, R2, Start, Select, D-Pad, and more.
- **Button debouncing**: Ensures reliable button press detection.
- **Sleep mode**: Enters deep sleep after a specified period of inactivity to save power.
- **Wake-up from button**: Can wake up from deep sleep by pressing a specific button.
- **Indicator LED**: Shows the connection status of the device.

## Requirements

- **Hardware**:
  - ESP32 development board
  - Push buttons for each command you want to support
  - Wires to connect the buttons to the ESP32
- **Software**:
  - Arduino IDE (version 1.8.13 or later)
  - ESP32 support installed in the Arduino IDE
  - `BleGamepad` library installed in the Arduino IDE

## Hardware Setup

Connect the buttons to the corresponding GPIO pins of the ESP32 as described below. Each button should be connected between the GPIO pin and ground (GND).

| Button                 | GPIO Pin |
|------------------------|----------|
| BUTTON_A_PIN           | 32       |
| BUTTON_B_PIN           | 33       |
| BUTTON_X_PIN           | 25       |
| BUTTON_Y_PIN           | 26       |
| BUTTON_L1_PIN          | 27       |
| BUTTON_R1_PIN          | 14       |
| BUTTON_L2_PIN          | 12       |
| BUTTON_R2_PIN          | 13       |
| BUTTON_START_PIN       | 23       |
| BUTTON_SELECT_PIN      | 22       |
| BUTTON_THUMB_LEFT_PIN  | 4        |
| BUTTON_THUMB_RIGHT_PIN | 5        |
| BUTTON_DPAD_UP_PIN     | 21       |
| BUTTON_DPAD_DOWN_PIN   | 19       |
| BUTTON_DPAD_LEFT_PIN   | 18       |
| BUTTON_DPAD_RIGHT_PIN  | 17       |

**Note**: The built-in LED of the ESP32 is usually on GPIO 2.

## Installation

1. **Install the Arduino IDE**:

   Download and install the Arduino IDE from the [official website](https://www.arduino.cc/en/software).

2. **Install ESP32 support in the Arduino IDE**:

   - Open the Arduino IDE.
   - Go to `File` > `Preferences`.
   - In the `Additional Boards Manager URLs` field, add:

     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```

   - Click `OK`.
   - Navigate to `Tools` > `Board` > `Boards Manager`.
   - Search for `esp32` and install the `esp32` package by Espressif Systems.

3. **Install the BleGamepad library**:

   - Go to `Sketch` > `Include Library` > `Manage Libraries`.
   - Search for `BleGamepad`.
   - Install the `ESP32-BLE-Gamepad` library by `lemmingDev`.

4. **Download the code**:

   - Download or clone this repository.
   - Open the `.ino` file in the Arduino IDE.

## Usage

1. **Program the ESP32**:

   - Connect the ESP32 to your computer via USB.
   - Select the appropriate board from `Tools` > `Board` (e.g., `ESP32 Dev Module`).
   - Select the correct port from `Tools` > `Port`.
   - Click the `Upload` button to upload the program to the ESP32.

2. **Pair with a device**:

   - After programming, the ESP32 will start functioning as a BLE gamepad.
   - On the device you want to connect to (computer, smartphone, etc.), go to the Bluetooth settings.
   - Search for new devices.
   - You should see a device named `ESP32 Gamepad` or similar.
   - Select the device to pair.

3. **Use the gamepad**:

   - Once pairing is complete, the ESP32 will be recognized as a gamepad.
   - Press the buttons you've connected to send commands to your device.
   - The built-in LED will be lit when there is an active connection.

4. **Sleep mode**:

   - If there is no activity for 5 minutes (default value), the ESP32 will enter deep sleep to save power.
   - Press the `A` button (or the button defined as `WAKE_BUTTON_PIN`) to wake up the device.

## Customization

- **Changing GPIO pins**:

  If you want to use different GPIO pins, you can modify the corresponding lines in the code:

  ```cpp
  constexpr int BUTTON_A_PIN = <new_pin>;
  ```

- **Setting the sleep timeout**:

  You can change the idle time before the device enters sleep by modifying the `SLEEP_TIMEOUT` constant (in milliseconds):

  ```cpp
  constexpr unsigned long SLEEP_TIMEOUT = 300000; // e.g., for 5 minutes
  ```

- **Adjusting the polling interval**:

  To change how frequently the buttons are checked, adjust the `POLLING_INTERVAL` constant:

  ```cpp
  constexpr unsigned long POLLING_INTERVAL = 10; // in milliseconds
  ```

- **Adding additional functionality**:

  You can extend the code to support analog axes, vibration, or other features supported by the `BleGamepad` library.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for more information.

## Contributing

Contributions are welcome! You can open an issue to report bugs or suggest improvements. Also, feel free to submit pull requests with enhancements or new features.

## Acknowledgments

- **[lemmingDev](https://github.com/lemmingDev)** for the [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad) library.
- **ESP32 Community** for the support and tools they provide.

---

**Note**: This project was created to help other developers implement a BLE gamepad with the ESP32. If you have questions or need assistance, feel free to reach out.