#include <Arduino.h>
#include <Wire.h>

/**
 * ============================================================================
 *                 ESP32 Full I2C & MUX Channel Bus Scanner
 * ============================================================================
 *
 * WHAT THIS DIAGNOSTIC DOES:
 * 1. Scans main I2C bus (GPIO 21 SDA, GPIO 22 SCL) for all devices (0x01 to
 * 0x7F). (Expect: 0x70 if TCA9548A MUX is present, 0x36 if direct AS5600
 * present).
 * 2. If TCA9548A is found at 0x70, it opens each channel (0 through 7) one by
 * one and scans for devices behind each channel!
 */

#define LED_PIN 2
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define TCA9548A_ADDR 0x70

void selectMuxChannel(uint8_t channel) {
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write((channel > 7) ? 0x00 : (1 << channel));
  Wire.endTransmission();
}

void scanBus(const char *label) {
  byte error, address;
  int nDevices = 0;

  Serial.print("--- Scanning ");
  Serial.print(label);
  Serial.println(" ---");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("  [FOUND] Device at I2C address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);

      // Print common device identification hints
      if (address == 0x70)
        Serial.print(" (TCA9548A I2C MUX)");
      else if (address == 0x36)
        Serial.print(" (AS5600 Encoder)");
      Serial.println();

      nDevices++;
    }
  }

  if (nDevices == 0) {
    Serial.println("  [NONE] No I2C devices found on this bus/channel.");
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  delay(100);
}

void loop() {
  Serial.println("\n==================================================");
  Serial.println("         STARTING FULL I2C & MUX SCANNER          ");
  Serial.println("==================================================");

  // Step 1: Scan main ESP32 bus
  selectMuxChannel(255); // Disable all MUX channels first
  scanBus("Main ESP32 I2C Bus (GPIO 21 / 22)");

  // Step 2: Check if MUX is present at 0x70
  Wire.beginTransmission(TCA9548A_ADDR);
  if (Wire.endTransmission() == 0) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println(
        ">>> TCA9548A MUX DETECTED AT 0x70! Scanning channels 0-7...\n");

    // Step 3: Scan each MUX channel individually
    for (uint8_t ch = 0; ch < 8; ch++) {
      selectMuxChannel(ch);
      delay(10);

      char chLabel[32];
      snprintf(chLabel, sizeof(chLabel), "MUX Channel %d (SD%d/SC%d)", ch, ch,
               ch);
      scanBus(chLabel);
    }

    // Disable channels after scanning
    selectMuxChannel(255);
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println(">>> [WARNING] TCA9548A MUX NOT detected at address 0x70!");
    Serial.println("    Check MUX power (VCC/GND), SDA (GPIO 21), SCL (GPIO "
                   "22), & A0/A1/A2 pins.");
  }

  Serial.println("==================================================");
  Serial.println("Scan complete. Rescanning in 5 seconds...\n");
  delay(5000);
}
