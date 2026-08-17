#include <Adafruit_AS5600.h>
#include <Arduino.h>
#include <Wire.h>

/**
 * ============================================================================
 *         Standalone ESP32 Adafruit AS5600 Encoder Serial Tester
 * ============================================================================
 *
 * PURPOSE:
 * Plain Arduino C++ script (without micro-ROS) to test reading the physical
 * Adafruit AS5600 magnetic encoder and printing values to the Serial Monitor.
 *
 * WIRING CONFIGURATION:
 * - ESP32 GPIO 21  <-->  AS5600 SDA
 * - ESP32 GPIO 22  <-->  AS5600 SCL
 * - 3.3V / GND     <-->  AS5600 VCC / GND
 *
 * LED FAILSAFE & STATUS GUIDE:
 * - Rapid Blinking (100ms): AS5600 sensor NOT detected on I2C address 0x36!
 *   (Failsafe traps execution until I2C connection is resolved).
 * - Solid ON: AS5600 connected properly & actively printing readings to Serial.
 *
 * SERIAL MONITOR SPEED:
 * Set Serial Monitor baud rate to 115200 baud.
 */

// Pin definitions
#define LED_PIN 2
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

// Update interval (200ms = 5 Hz update rate)
#define READ_INTERVAL_MS 200

// Adafruit AS5600 sensor instance
Adafruit_AS5600 as5600;

unsigned long last_read_time = 0;

/**
 * @brief Failsafe trap: Blinks onboard LED rapidly if hardware initialization
 * fails.
 */
void error_loop() {
  Serial.println("\n[ERROR FAILSAFE] AS5600 sensor not detected on I2C bus!");
  Serial.println("[ERROR FAILSAFE] Please check SDA (GPIO 21), SCL (GPIO 22), "
                 "3.3V, and GND wiring.");

  while (1) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
  }
}

void setup() {
  // 1. Initialize Serial communication
  Serial.begin(115200);
  delay(1000); // Allow serial monitor to open

  Serial.println("\n--- ESP32 AS5600 Encoder Serial Diagnostic ---");

  // 2. Initialize status LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 3. Initialize I2C bus on pins GPIO 21 (SDA) and GPIO 22 (SCL)
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // 4. Initialize AS5600 sensor with hardware check failsafe
  Serial.println("[INFO] Connecting to AS5600 encoder at I2C address 0x36...");
  if (!as5600.begin(AS5600_DEFAULT_ADDR, &Wire)) {
    error_loop(); // Enter failsafe rapid blink loop
  }

  Serial.println("[SUCCESS] AS5600 encoder initialized successfully!");

  // Turn LED solid ON to indicate system healthy
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  unsigned long current_time = millis();

  if (current_time - last_read_time >= READ_INTERVAL_MS) {
    last_read_time = current_time;

    // Check if magnet is detected over sensor chip
    bool magnet_present = as5600.isMagnetDetected();

    // Read raw 12-bit angle value (0 to 4095)
    uint16_t raw_angle = as5600.getRawAngle();

    // Calculate angle in degrees (0.0 to 360.0)
    float degrees = (raw_angle * 360.0) / 4096.0; 

    // Print formatted output to Serial Monitor
    Serial.print("Encoder Raw (12-bit): ");
    Serial.print(raw_angle);
    Serial.print(" | Angle (Degrees): ");
    Serial.print(degrees, 2);
    Serial.print("° | Magnet Detected: ");
    if (magnet_present) {
      Serial.println("YES [OK]");
    } else {
      Serial.println("NO [Check Magnet Placement!]");
    }
  }
}
