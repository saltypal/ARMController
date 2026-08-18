#include <Adafruit_AS5600.h>
#include <Arduino.h>
#include <Wire.h>

/**
 * ============================================================================
 *      ESP32 Multi-Encoder Test: TCA9548A MUX (Channels 0 - 7)
 * ============================================================================
 *
 * WHAT THIS CODE DOES:
 * 1. Connects to TCA9548A 8-Channel I2C MUX at address 0x70.
 * 2. Iterates through all 8 channels (CH0 to CH7) every 200 ms.
 * 3. Switches MUX channel, reads the AS5600 encoder (if present), and prints
 *    the raw 12-bit angle (0–4095), calculated degrees (0–360°), and magnet
 * status.
 * 4. Gracefully prints "NOT CONNECTED" for any channel without an encoder.
 *
 * WIRING:
 * - ESP32 GPIO 21 (SDA)  <-->  TCA9548A SDA
 * - ESP32 GPIO 22 (SCL)  <-->  TCA9548A SCL
 * - TCA9548A SDx / SCx   <-->  Each AS5600's SDA / SCL (Channels 0..7)
 * - TCA9548A A0, A1, A2  <-->  GND (I2C address 0x70)
 * - Power                <-->  3.3V & GND to all boards
 *
 * SERIAL MONITOR: 115200 baud
 */

// ----------------------------------------------------------------------------
// CONFIGURATION & CONSTANTS
// ----------------------------------------------------------------------------

#define LED_PIN 2
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define TCA9548A_ADDR 0x70

#define NUM_CHANNELS 8       // Total channels on TCA9548A (0 to 7)
#define READ_INTERVAL_MS 500 // Read all channels every 200 ms (5 Hz)

// ----------------------------------------------------------------------------
// GLOBAL OBJECTS & STATE
// ----------------------------------------------------------------------------

Adafruit_AS5600 as5600;
unsigned long last_read_time = 0;

// Data structure to hold reading for each channel
struct EncoderData {
  bool connected;
  uint16_t raw_angle;
  float degrees;
  bool magnet_ok;
};

EncoderData channel_data[NUM_CHANNELS];

// ----------------------------------------------------------------------------
// HELPER FUNCTIONS
// ----------------------------------------------------------------------------

/**
 * @brief Infinite loop for fatal errors (e.g. MUX missing). Blinks LED rapidly.
 */
void error_loop(const char *message) {
  Serial.println(message);
  while (1) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
  }
}

/**
 * @brief Selects active channel on TCA9548A MUX.
 *        Writing (1 << channel) enables channel `channel`.
 *        Passing 255 disables all channels.
 */
void selectMuxChannel(uint8_t channel) {
  Wire.beginTransmission(TCA9548A_ADDR);
  if (channel < NUM_CHANNELS) {
    Wire.write(1 << channel);
  } else {
    Wire.write(0x00); // Disable all
  }
  Wire.endTransmission();
}

/**
 * @brief Reads the AS5600 encoder on the currently active MUX channel.
 */
void readActiveChannel(uint8_t ch) {
  // Attempt AS5600 initialization on active channel
  if (as5600.begin(AS5600_DEFAULT_ADDR, &Wire)) {
    channel_data[ch].connected = true;
    channel_data[ch].raw_angle = as5600.getRawAngle();
    channel_data[ch].degrees = (channel_data[ch].raw_angle * 360.0) / 4096.0;
    channel_data[ch].magnet_ok = as5600.isMagnetDetected();
  } else {
    channel_data[ch].connected = false;
    channel_data[ch].raw_angle = 0;
    channel_data[ch].degrees = 0.0;
    channel_data[ch].magnet_ok = false;
  }
}

// ----------------------------------------------------------------------------
// MAIN SETUP AND LOOP
// ----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==================================================");
  Serial.println("  ESP32 TCA9548A All-Channel Encoder Monitor   ");
  Serial.println("==================================================\n");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize I2C bus
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  delay(100);

  // Verify TCA9548A MUX presence
  Serial.print("[INFO] Checking TCA9548A MUX at address 0x70... ");
  Wire.beginTransmission(TCA9548A_ADDR);
  if (Wire.endTransmission() != 0) {
    error_loop("[FAIL] TCA9548A MUX not found! Check SDA (GPIO 21), SCL (GPIO "
               "22), 3.3V, GND.");
  }
  Serial.println("FOUND [OK]\n");

  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  unsigned long current_time = millis();

  if (current_time - last_read_time >= READ_INTERVAL_MS) {
    last_read_time = current_time;

    int connected_count = 0;

    // 1. Read all 8 MUX channels
    for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
      selectMuxChannel(ch);
      delay(2); // Short settling delay for I2C line switching
      readActiveChannel(ch);

      if (channel_data[ch].connected) {
        connected_count++;
      }
    }

    // Disable channels after reading
    selectMuxChannel(255);

    // 2. Print formatted summary to Serial Monitor
    Serial.println("-----------------------------------------------------------"
                   "---------------------");
    Serial.print("TIMESTAMP: ");
    Serial.print(millis());
    Serial.print(" ms | Connected Encoders: ");
    Serial.print(connected_count);
    Serial.print("/");
    Serial.println(NUM_CHANNELS);
    Serial.println("-----------------------------------------------------------"
                   "---------------------");

    for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
      Serial.print("  CH");
      Serial.print(ch);
      Serial.print(" (SD");
      Serial.print(ch);
      Serial.print("/SC");
      Serial.print(ch);
      Serial.print("): ");

      if (channel_data[ch].connected) {
        Serial.print("Raw = ");
        if (channel_data[ch].raw_angle < 1000)
          Serial.print(" ");
        if (channel_data[ch].raw_angle < 100)
          Serial.print(" ");
        if (channel_data[ch].raw_angle < 10)
          Serial.print(" ");
        Serial.print(channel_data[ch].raw_angle);

        Serial.print(" | Angle = ");
        if (channel_data[ch].degrees < 100.0)
          Serial.print(" ");
        if (channel_data[ch].degrees < 10.0)
          Serial.print(" ");
        Serial.print(channel_data[ch].degrees, 1);
        Serial.print("° | Magnet = ");
        Serial.println(channel_data[ch].magnet_ok ? "OK" : "NO MAGNET!");
      } else {
        Serial.println("--- NOT CONNECTED ---");
      }
    }
    Serial.println();
  }
}
