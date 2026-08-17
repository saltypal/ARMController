#include <Adafruit_AS5600.h>
#include <Arduino.h>
#include <Wire.h>

#include <micro_ros_arduino.h>
#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <std_msgs/msg/int32.h>
#include <stdio.h>

/**
 * ============================================================================
 *               ESP32 micro-ROS Single AS5600 Encoder Publisher
 * ============================================================================
 *
 * WHAT THIS PROGRAM DOES:
 * 1. Reads raw 12-bit magnetic angle position (0–4095) from Adafruit AS5600.
 * 2. Connects to micro-ROS agent over USB Serial transport.
 * 3. Publishes angle reading to ROS 2 topic `/Encoder_one` every 200ms (5 Hz).
 *
 * HARDWARE WIRING:
 * - ESP32 GPIO 21  <-->  AS5600 SDA
 * - ESP32 GPIO 22  <-->  AS5600 SCL
 * - 3.3V / GND     <-->  AS5600 VCC / GND
 *
 * LED FAILSAFE & STATUS:
 * - Rapid Blinking (100ms): Hardware / I2C connection error or agent pending.
 * - Solid ON: Connected to micro-ROS agent and actively publishing to
 * `/Encoder_one`.
 */

// ----------------------------------------------------------------------------
// 1. CONFIGURATION & CONSTANTS
// ----------------------------------------------------------------------------

#define ROS_TOPIC_NAME "/Encoder_one"
#define ROS_NODE_NAME "esp32_encoder_publisher"
#define PUBLISH_INTERVAL_MS 200

#define LED_PIN 2

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

#define RCCHECK(fn)                                                            \
  {                                                                            \
    rcl_ret_t temp_rc = fn;                                                    \
    if ((temp_rc != RCL_RET_OK)) {                                             \
      error_loop();                                                            \
    }                                                                          \
  }

// ----------------------------------------------------------------------------
// 2. GLOBAL STATE VARIABLES
// ----------------------------------------------------------------------------

Adafruit_AS5600 as5600; // object representing the sensor

rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

unsigned long last_publish_time = 0;


/**
 * @brief Initializes micro-ROS transport, Node, and Publisher.
 */
void setupMicroROS() {
  set_microros_transports();
  delay(2000);

  allocator = rcl_get_default_allocator();

  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_node_init_default(&node, ROS_NODE_NAME, "", &support));
  RCCHECK(rclc_publisher_init_default(
      &publisher, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
      ROS_TOPIC_NAME));
}


void error_loop() {
  while (1) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
  }
}

/**
 * @brief Initializes I2C and Adafruit AS5600 encoder with hardware check.
 */

void setupEncoder() {

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // begins the i2c communication

  // Verify AS5600 presence on address 0x36
  if (!as5600.begin(AS5600_DEFAULT_ADDR, &Wire)) { 
 
    // "Use the I²C bus represented by Wire, and communicate with the device whose I²C address is 0x36."
 
    error_loop(); // Enter failsafe blink loop if sensor hardware not found
  }
}

int32_t readEncoderValue() {
  uint16_t raw_angle = as5600.getRawAngle(); // Reads 12-bit raw angle (0-4095) from AS5600 encoder.
  return (int32_t)raw_angle;
}

void publishEncoderValue() {
  int32_t val = readEncoderValue();
  msg.data = val; // Reads AS5600 raw angle and publishes it to ROS 2 topic `/Encoder_one`.
  rcl_ret_t pub_status = rcl_publish(&publisher, &msg, NULL);
  (void)pub_status;
}


void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 1. Initialize Adafruit AS5600 hardware
  setupEncoder();
  // 2. Initialize micro-ROS communications & publisher
  setupMicroROS();
  // Turn LED solid HIGH to signal successful connection
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  unsigned long current_time = millis();

  if (current_time - last_publish_time >= PUBLISH_INTERVAL_MS) {
    last_publish_time = current_time;
    publishEncoderValue();
  }
}