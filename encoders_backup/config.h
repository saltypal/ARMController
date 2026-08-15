#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// Hardware Configuration
// ==========================================

// Fallback if NUM_ENCODERS is not defined in platformio.ini
#ifndef NUM_ENCODERS
#define NUM_ENCODERS 1
#endif

// I2C Pins
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_CLOCK_SPEED 400000 // 400 kHz Fast Mode

// AS5600 Constants
#define AS5600_I2C_ADDRESS 0x36
#define AS5600_RAW_ANGLE_REG_HI 0x0C
#define AS5600_RAW_ANGLE_REG_LO 0x0D

// I2C Multiplexer Constants (TCA9548A)
#define TCA9548A_I2C_ADDRESS 0x70

// ==========================================
// Software Configuration
// ==========================================

// ROS 2 Configuration
#define ROS_PUBLISH_FREQUENCY_HZ 50
#define ROS_NODE_NAME "arm_encoder_node"
#define ROS_TOPIC_NAME "/arm/joint_states"

// Debugging
// Uncomment the following line to enable debug printing to Serial.
// Note: if micro-ROS uses Serial as transport, do NOT enable this unless using a separate hardware serial port!
// #define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#endif // CONFIG_H
