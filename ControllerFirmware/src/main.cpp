#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AS5600.h>

#include <micro_ros_arduino.h>
#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>

/**
 * ============================================================================
 *               ESP32 micro-ROS Single AS5600 Encoder Publisher
 * ============================================================================
 * 
 * OVERVIEW:
 * 1. Non-blocking initialization allows micro-ROS to connect EVEN IF the 
 *    encoder sensor is not physically plugged in yet.
 * 2. If AS5600 is connected: Publishes raw 12-bit position (0–4095).
 * 3. If AS5600 is missing: Publishes `-1` to ROS 2 topic `/Encoder_one` and 
 *    blinks LED to warn of missing sensor.
 * 
 * WIRING:
 * - SDA: ESP32 GPIO 21
 * - SCL: ESP32 GPIO 22
 * - VCC: 3.3V / GND
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

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){ error_loop(); }}

// ----------------------------------------------------------------------------
// 2. GLOBAL HARDWARE & MICRO-ROS STATE
// ----------------------------------------------------------------------------

Adafruit_AS5600 as5600;
rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

bool encoder_initialized = false;
unsigned long last_publish_time = 0;

// ----------------------------------------------------------------------------
// 3. HELPER FUNCTIONS
// ----------------------------------------------------------------------------

void error_loop() {
    while (1) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(100);
    }
}

/**
 * @brief Tries to initialize AS5600 on I2C bus (Non-blocking).
 */
void setupEncoder() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Attempt AS5600 initialization without halting micro-ROS on failure
    if (as5600.begin(AS5600_DEFAULT_ADDR, &Wire)) {
        encoder_initialized = true;
    } else {
        encoder_initialized = false;
    }
}

/**
 * @brief Reads 12-bit angle (0-4095) or returns -1 if sensor is missing/disconnected.
 */
int32_t readEncoderValue() {
    if (!encoder_initialized) {
        // Try re-initializing once if sensor was plugged in late
        if (as5600.begin(AS5600_DEFAULT_ADDR, &Wire)) {
            encoder_initialized = true;
        } else {
            return -1; // Sensor offline / not connected
        }
    }
    
    uint16_t raw_angle = as5600.getRawAngle();
    return (int32_t)raw_angle;
}

/**
 * @brief Initializes micro-ROS transports, Node, and Publisher.
 */
void setupMicroROS() {
    set_microros_transports();
    delay(2000);

    allocator = rcl_get_default_allocator();

    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
    RCCHECK(rclc_node_init_default(&node, ROS_NODE_NAME, "", &support));
    RCCHECK(rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        ROS_TOPIC_NAME
    ));
}

/**
 * @brief Publishes encoder reading (or -1 if missing) to `/Encoder_one`.
 */
void publishEncoderValue() {
    int32_t val = readEncoderValue();
    msg.data = val;

    rcl_ret_t pub_status = rcl_publish(&publisher, &msg, NULL);
    (void)pub_status;

    // Visual status update:
    if (!encoder_initialized) {
        // Blink LED briefly if sensor is missing
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    } else {
        // Solid ON when sensor is active and publishing valid data
        digitalWrite(LED_PIN, HIGH);
    }
}

// ----------------------------------------------------------------------------
// 4. MAIN SETUP AND LOOP
// ----------------------------------------------------------------------------

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // 1. Setup encoder (non-blocking)
    setupEncoder();

    // 2. Setup micro-ROS node & publisher
    setupMicroROS();

    digitalWrite(LED_PIN, HIGH);
}

void loop() {
    unsigned long current_time = millis();

    if (current_time - last_publish_time >= PUBLISH_INTERVAL_MS) {
        last_publish_time = current_time;
        publishEncoderValue();
    }
}