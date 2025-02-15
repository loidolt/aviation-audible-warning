/*
 * Aviation Audible Warning (AAW) System for Small Airplanes
 * -----------------------------------------------------
 * Version: 0.1.0
 * Author: Chris Loidolt
 * Date: 02/15/2025
 *
 * This code implements a manifold pressure warning system using:
 *  - Arduino MKR Zero
 *    - https://store-usa.arduino.cc/collections/boards-modules/products/arduino-mkr-zero-i2s-bus-sd-for-sound-music-digital-audio-data)
 *  - Arduino MKR Relay Proto Shield (carrier board and relay support)
 *    - https://store-usa.arduino.cc/collections/mkr-family/products/arduino-mkr-relay-proto-shield
 *  - MAX98357A I2S Amplifier (for audio output)
 *    - https://www.adafruit.com/product/3006
 *  - SD Card (for storing the alert audio file)
 *    - https://www.adafruit.com/product/5249
 *
 * The system operates in three main states:
 *
 * 1. IDLE:
 *    - The system waits for either a sensor trigger (indicating a pressure warning)
 *      on analog pin A1 or a button press on analog pin A2.
 *    - In this state, no alert is active.
 *
 * 2. ALERT:
 *    - When the sensor is triggered, the system enters ALERT mode.
 *    - A relay on digital pin 1 is energized to activate a 24V indicator light.
 *    - The alert audio file ("alert.wav") is played repeatedly via I2S
 *      (using the MAX98357A amplifier) with a delay between repeats.
 *    - The alert is canceled when the pilot acknowledges by pressing the button.
 *
 * 3. CROSS_CHECK:
 *    - When the button is pressed while the system is in IDLE,
 *      the relay is briefly energized to verify that the indicator light
 *      and relay circuit are functioning correctly.
 *
 * Additional Features:
 *    - Non-blocking timing (using millis()) ensures the system remains responsive.
 *    - A watchdog timer (via WDTZero) is used to enhance reliability.
 *
 * Note: Ensure that wiring and hardware modifications (especially for I2S and relay control)
 * are thoroughly tested before deploying the system in an airplane.
 */

#include <SD.h>
#include <ArduinoSound.h>       // Provides I2S audio support via AudioOutI2S
#include <WDTZero.h>            // For watchdog timer support
// (Ensure you have the latest ArduinoSound library that supports the MAX98357A setup)

// -------------------------
// Pin Definitions
// Sensor input on A1; Button input on A2 (with internal pull‑up)
// Relay1 (for the 24V indicator light) remains on digital pin 1.
const int sensorPin = A1;
const int buttonPin = A2;
const int relayPin  = 1;

// -------------------------
// Timing Settings (in milliseconds)
const unsigned long debounceDelay    = 50;    // Button debounce time
const unsigned long crossCheckDuration = 2000; // How long to energize relay for cross-check
const unsigned long alertRepeatDelay   = 10000; // Delay between repeated audio playbacks in ALERT state
const unsigned long watchdogTimeout    = 2000; // Watchdog timeout

// -------------------------
// System State Definitions
enum SystemState { IDLE, ALERT, CROSS_CHECK };
SystemState state = IDLE;

// Timing and state management variables
unsigned long lastButtonTime    = 0;
unsigned long crossCheckStartTime = 0;
unsigned long lastAudioEndTime  = 0;   // Time when last audio finished playing
bool alertAudioStarted = false;         // Indicates if alert audio is currently playing

// -------------------------
// SD Audio File Object
// We'll use a single WAV file named "alert.wav" stored in the SD card's root folder.
SDWaveFile alertFile;

// -------------------------
// Watchdog Timer Object (for SAMD boards)
WDTZero wdt;

void setupWatchdog() {
  wdt.begin(watchdogTimeout);
}

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for serial monitor (optional)

  // Initialize pins
  pinMode(sensorPin, INPUT);         // Sensor on A1
  pinMode(buttonPin, INPUT_PULLUP);    // Button on A2 (internal pull‑up)
  pinMode(relayPin, OUTPUT);           // Relay control (for 24V indicator light)
  digitalWrite(relayPin, LOW);         // Ensure relay is off initially

  // Initialize SD card
  if (!SD.begin()) {
    Serial.println("ERROR: SD initialization failed!");
    while (1);  // Halt – critical error.
  }
  Serial.println("SD initialization done.");

  // Open the alert WAV file from the SD card.
  alertFile = SDWaveFile("alert.wav");
  if (!alertFile) {
    Serial.println("ERROR: Could not open alert.wav");
    while (1);  // Halt – critical error.
  }
  
  // Check if the I2S output can play the file.
  if (!AudioOutI2S.canPlay(alertFile)) {
    Serial.println("ERROR: I2S cannot play alert.wav!");
    while (1);  // Halt – critical error.
  }
  Serial.println("Audio file ready for I2S playback.");

  // (Optional) Set initial volume level for I2S output (range depends on library version)
  AudioOutI2S.volume(15);  // Adjust volume as needed

  // Initialize watchdog timer
  setupWatchdog();
  
  Serial.println("Setup complete. Entering IDLE state.");
}

void loop() {
  // Feed the watchdog timer to prevent reset
  wdt.feed();

  // Read sensor and button states.
  // (For the SAMD, digitalRead() works on A1/A2 when configured as digital pins.)
  bool sensorTriggered = (digitalRead(sensorPin) == HIGH);
  bool buttonPressed   = (digitalRead(buttonPin) == LOW);
  unsigned long currentTime = millis();

  switch (state) {
    case IDLE:
      // If sensor is triggered, move to ALERT state.
      if (sensorTriggered) {
        Serial.println("Sensor triggered. Entering ALERT state.");
        state = ALERT;
        alertAudioStarted = false;  // Reset audio flag.
        lastAudioEndTime = 0;
        digitalWrite(relayPin, HIGH); // Energize relay: indicator light ON continuously.
      }
      // If button is pressed in IDLE, perform a cross-check.
      else if (buttonPressed && (currentTime - lastButtonTime > debounceDelay)) {
        lastButtonTime = currentTime;
        Serial.println("Button pressed in IDLE: Entering CROSS_CHECK state.");
        state = CROSS_CHECK;
        crossCheckStartTime = currentTime;
        digitalWrite(relayPin, HIGH); // Energize relay for cross-check.
      }
      break;

    case ALERT:
      // In ALERT state the relay remains energized (light ON).
      // If the button is pressed, acknowledge alert and exit ALERT state.
      if (buttonPressed && (currentTime - lastButtonTime > debounceDelay)) {
        lastButtonTime = currentTime;
        Serial.println("Alert acknowledged via button press. Exiting ALERT state.");
        AudioOutI2S.stop();           // Stop I2S playback.
        digitalWrite(relayPin, LOW);   // De-energize relay.
        state = IDLE;
        alertAudioStarted = false;
        lastAudioEndTime = 0;
        break;
      }

      // If not playing audio, check if it's time to play the alert sound.
      if (!alertAudioStarted) {
        if (lastAudioEndTime == 0 || (currentTime - lastAudioEndTime >= alertRepeatDelay)) {
          // (Re)open the file to reset playback from the beginning.
          alertFile = SDWaveFile("alert.wav");
          if (!AudioOutI2S.play(alertFile)) {
            Serial.println("ERROR: Could not play alert.wav via I2S!");
          } else {
            Serial.println("Playing alert.wav via I2S.");
            alertAudioStarted = true;
          }
        }
      }
      
      // Check if the audio finished playing.
      if (alertAudioStarted && !AudioOutI2S.isPlaying()) {
        alertAudioStarted = false;   // Ready for next playback.
        lastAudioEndTime = currentTime;
        Serial.println("Alert audio finished playing. Waiting before next repeat...");
      }
      
      // Optionally, if the sensor clears (manifold pressure normalizes), exit ALERT state.
      if (!sensorTriggered) {
        Serial.println("Sensor no longer triggered. Exiting ALERT state.");
        AudioOutI2S.stop();
        digitalWrite(relayPin, LOW);
        state = IDLE;
        alertAudioStarted = false;
        lastAudioEndTime = 0;
      }
      break;

    case CROSS_CHECK:
      // In CROSS_CHECK, the relay remains energized for the preset duration.
      if (currentTime - crossCheckStartTime >= crossCheckDuration) {
        digitalWrite(relayPin, LOW); // Turn off relay after cross-check period.
        Serial.println("Cross-check complete. Returning to IDLE state.");
        state = IDLE;
      }
      break;
  }

  // Short delay to reduce CPU load while keeping system responsive.
  delay(5);
}
