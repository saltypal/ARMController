#include <Arduino.h>
#include <micro_ros_arduino.h>

#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <stdio.h>

#include <std_msgs/msg/string.h>

/**
 * ============================================================================
 *                    ESP32 micro-ROS Random Topic Publisher
 * ============================================================================
 *
 * WHAT IS THIS PROGRAM DOING?
 * 1. Simulates 7 magnetic encoder values using ESP32 Hardware Random Generator.
 * 2. Formats these 7 readings into a single readable text String message.
 * 3. Connects to the host computer using micro-ROS over USB Serial.
 * 4. Publishes the string message periodically to ROS 2 topic `/randTest`.
 *
 * LED STATUS INDICATIONS:
 * - Blinking rapidly (100ms): Waiting for agent / initialization error.
 *   (Press RESET button on ESP32 AFTER starting micro_ros_agent on host PC).
 * - Solid HIGH: Connected and actively publishing to ROS 2!
 */

// ----------------------------------------------------------------------------
// 1. CONFIGURATION & CONSTANTS
// ----------------------------------------------------------------------------

// ROS 2 Topic Name (Requirement: "/randTest")
#define ROS_TOPIC_NAME "/randTest"

// ROS 2 Node Name (Name of this device on the ROS 2 node graph)
#define ROS_NODE_NAME "esp32_random_publisher"

// Publish interval in milliseconds (200ms = 5 Hz update rate)
#define PUBLISH_INTERVAL_MS 200

// Number of encoders being simulated
#define NUM_ENCODERS 7

// Maximum length of the character buffer for string ROS 2 message payload
#define MAX_STRING_LEN 256

// Built-in LED GPIO pin for visual status indication (GPIO 2 on standard ESP32)
#define LED_PIN 2

// Macro helper to handle micro-ROS API return codes safely
#define RCCHECK(fn)                                                            \
  {                                                                            \
    rcl_ret_t temp_rc = fn;                                                    \
    if ((temp_rc != RCL_RET_OK)) {                                             \
      error_loop();                                                            \
    }                                                                          \
  }

// ----------------------------------------------------------------------------
// 2. GLOBAL MICRO-ROS & APPLICATION STATE VARIABLES
// ----------------------------------------------------------------------------

rcl_publisher_t publisher; // ROS 2 Publisher instance handler
std_msgs__msg__String msg; // ROS 2 String Message object wrapper
rclc_support_t support;    // micro-ROS support structure (clock, context)
rcl_allocator_t allocator; // Memory allocator for micro-ROS middleware
rcl_node_t node;           // ROS 2 Node handler

char msg_buffer[MAX_STRING_LEN]; // Static array memory buffer for ROS 2 string
unsigned long last_publish_time =
    0; // Timestamp tracker for non-blocking publishing timer

// ----------------------------------------------------------------------------
// 3. MODULAR FUNCTION DECLARATIONS & IMPLEMENTATIONS
// ----------------------------------------------------------------------------

/**
 * @brief Infinite loop triggered when a micro-ROS setup error occurs.
 *
 * WHY IS THIS FUNCTION NEEDED?
 * If micro-ROS fails to connect to the agent during startup, this loop blinks
 * the ESP32 onboard LED rapidly (every 100ms).
 * FIX: Start `micro_ros_agent` on laptop, then press the ESP32 RESET button!
 */
void error_loop() {
  while (1) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
  }
}

/**
 * @brief Generates a random integer value between minVal and maxVal.
 *
 * @param minVal Minimum integer value (e.g., 0)
 * @param maxVal Maximum integer value (e.g., 4095 for 12-bit AS5600 encoder)
 * @return int A random integer in the specified range.
 */
int generateRandomEncoderValue(int minVal, int maxVal) {
  if (minVal >= maxVal)
    return minVal;
  uint32_t raw_random = esp_random();
  int range = (maxVal - minVal + 1);
  return minVal + (raw_random % range);
}

/**
 * @brief Generates 7 random encoder values and formats them into a single
 * String.
 *
 * @param buffer Output character array where formatted text will be stored.
 * @param bufferSize Maximum capacity of output character array to prevent
 * overflow.
 */
void generateEncoderString(char *buffer, size_t bufferSize) {
  int enc[NUM_ENCODERS];

  // Generate 7 random simulated 12-bit encoder readings (0 to 4095)
  for (int i = 0; i < NUM_ENCODERS; i++) {
    enc[i] = generateRandomEncoderValue(0, 4095);
  }

  // Format all 7 values into a clean, human-readable text string
  snprintf(
      buffer, bufferSize,
      "Enc1: %d, Enc2: %d, Enc3: %d, Enc4: %d, Enc5: %d, Enc6: %d, Enc7: %d",
      enc[0], enc[1], enc[2], enc[3], enc[4], enc[5], enc[6]);
}

/**
 * @brief Initializes micro-ROS communications, Node, and Publisher.
 */
void setupMicroROS() {
  // Step 1: Configure micro-ROS serial transport
  set_microros_transports();

  // Stabilization delay
  delay(2000);

  // Step 2: Get default micro-ROS memory allocator
  allocator = rcl_get_default_allocator();

  // Step 3: Initialize support structure (context & system clock)
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // Step 4: Create ROS 2 Node named "esp32_random_publisher"
  RCCHECK(rclc_node_init_default(&node, ROS_NODE_NAME, "", &support));

  // Step 5: Create ROS 2 Publisher on topic "/randTest" with
  // std_msgs/msg/String type
  RCCHECK(rclc_publisher_init_default(
      &publisher, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
      ROS_TOPIC_NAME));

  // Step 6: Bind static memory buffer to ROS message struct
  msg.data.data = msg_buffer;
  msg.data.capacity = MAX_STRING_LEN;
  msg.data.size = 0;
}

/**
 * @brief Publishes a new random string message to ROS 2 topic `/randTest`.
 */
void publishRandomMessage() {
  // Generate new random encoder readings string into buffer
  generateEncoderString(msg_buffer, MAX_STRING_LEN);

  // Set message string length field
  msg.data.size = strlen(msg_buffer);

  // Publish the message to ROS 2 topic `/randTest`
  rcl_ret_t pub_status = rcl_publish(&publisher, &msg, NULL);
  (void)pub_status; // Explicitly silence unused variable warning
}

// ----------------------------------------------------------------------------
// 4. ARDUINO MAIN ENTRY POINTS (setup & loop)
// ----------------------------------------------------------------------------

/**
 * @brief Arduino initialization routine - runs ONCE on startup.
 */
void setup() {
  // Initialize status LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize micro-ROS client, node, and topic publisher
  setupMicroROS();

  // Turn LED solid HIGH to indicate micro-ROS setup completed successfully!
  digitalWrite(LED_PIN, HIGH);
}

/**
 * @brief Arduino main loop routine - runs CONTINUOUSLY after setup.
 */
void loop() {
  unsigned long current_time = millis();

  // Check if 200 milliseconds have elapsed since last publish
  if (current_time - last_publish_time >= PUBLISH_INTERVAL_MS) {
    last_publish_time = current_time;

    // Publish random encoder string to ROS 2 topic /randTest
    publishRandomMessage();
  }
}