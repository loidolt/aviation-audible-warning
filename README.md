# Aviation Audible Warning

Aviation Audible Warning is an open-source, flexible audible warning system for aviation applications. Originally designed to monitor manifold pressure in small aircraft, this project is built on the Arduino MKR Zero and can be easily adapted to monitor other critical metrics. The system provides both audible and visual alerts using an I2S audio amplifier and a relay-controlled 24V indicator light.

## Features

- **Flexible Monitoring:**  
  While initially targeting manifold pressure anomalies, the system is designed to be easily extended for other monitored metrics.
  
- **Multi-State Operation:**  
  - **IDLE:** Monitors sensor inputs and awaits user interaction.  
  - **ALERT:** Activates when a monitored condition is triggered. Plays an audible alert (from an SD card file) via I2S and energizes an indicator light continuously until acknowledged.  
  - **CROSS_CHECK:** Allows for a system self-test by briefly energizing the indicator light upon a button press.

- **I2S Audio Output:**  
  Uses the MAX98357A I2S amplifier for high-quality audio output.

- **Robust Design:**  
  Incorporates non-blocking timing (using `millis()`) and a watchdog timer (via the WDTZero library) for enhanced reliability and fault tolerance.

## Hardware Requirements

- **Arduino MKR Zero**  
- **MAX98357A I2S 3W Class D Amplifier Breakout**  
  - Wiring:  
    - **LRCLK** → MKR Zero **-3**  
    - **BCLK** → MKR Zero **-2**  
    - **DIN**   → MKR Zero **A6**  
    - **VIN**   → VCC  
    - **GND**   → GND
- **SD Card Module/Shield** (with the alert audio file `alert.wav` in the root directory)
- **Relay Shield/Board**  
  - **Relay1:** Connected to digital pin **1** (switches the 24V indicator light)
- **Sensor:** Connected to analog pin **A1** (for the monitored metric, e.g., manifold pressure)
- **Button:** Connected to analog pin **A2** (with internal pull‑up enabled)

## Software Requirements

- Arduino IDE (version 1.8.x or later) or PlatformIO
- [ArduinoSound](https://www.arduino.cc/reference/en/libraries/arduinosound/) library (with I2S support)
- [WDTZero](https://github.com/kentaylor/WDTZero) library for watchdog timer support
- SD library (bundled with the Arduino IDE)

## Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/your-username/aviation-audible-warning.git

2. Open the project in the Arduino IDE or your preferred development environment.
3. Install the required libraries via the Library Manager:
  - ArduinoSound
  - WDTZero
4. Copy your alert audio file (alert.wav) to the root of your SD card.
5. Upload the code to your Arduino MKR Zero.


## Wiring Diagram

- **Sensor:** Connect to A1.
- **Button:** Connect to A2 (ensure the button is configured with an internal pull-up).
- **Relay (Indicator Light):**
  - Relay1 connected to digital pin 1 (controls a 24V indicator light).
- **MAX98357A I2S Amplifier:**
  - LRCLK → MKR Zero -3
  - BCLK → MKR Zero -2
  - DIN → MKR Zero A6
  - VIN → VCC
  - GND → GND

## Operation

- **IDLE State:** The system waits for either:
  - A sensor trigger indicating a monitored condition (e.g., manifold pressure anomaly) to enter ALERT mode.
  - A button press to perform a CROSS_CHECK (briefly energizes the indicator light to verify functionality).

- **ALERT State:** When a warning is detected, the system:
  - Energizes the relay (indicator light ON).
  - Plays the alert.wav audio file repeatedly via I2S, with a delay between repeats.
  - Exits ALERT state when the button is pressed (acknowledging the warning) or when the sensor returns to normal.

- **CROSS_CHECK State:**
  - When initiated from IDLE via button press, the system briefly energizes the relay for a self-test, then returns to IDLE.

## Contributing

Contributions, issues, and feature requests are welcome!
Feel free to check the issues page for ideas and updates.

## License

This project is licensed under the GNU 3.0 License. See the LICENSE file for details.

## Disclaimer

Important: This system is intended for experimental and educational purposes. Thoroughly test and validate the system in a controlled environment before any actual in-flight use. The authors assume no responsibility for any damage or injury resulting from the use of this system.