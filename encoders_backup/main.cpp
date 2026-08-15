#include <Arduino.h>
#include "config.h"
#include "EncoderManager.h"
#include "MicroRosNode.h"

// Global instances
EncoderManager encoderManager;
MicroRosNode rosNode;

// Array to hold encoder angles
float currentAngles[NUM_ENCODERS];

// Timer variables for non-blocking execution
unsigned long lastPublishTime = 0;
const unsigned long publishIntervalMs = 1000 / ROS_PUBLISH_FREQUENCY_HZ;

void setup() {
#ifdef DEBUG_ENABLE
    Serial.begin(115200);
    // Wait a moment for serial to connect
    delay(2000);
    DEBUG_PRINTLN("--- ESP32 AS5600 Encoder Node Starting ---");
#endif

    // 1. Initialize Encoders
    if (!encoderManager.init()) {
        DEBUG_PRINTLN("Warning: Not all encoders initialized successfully.");
    } else {
        DEBUG_PRINTLN("Encoders initialized successfully.");
    }

    // 2. Initialize Micro-ROS (this will block until connected to agent)
    // NOTE: Micro-ROS over serial uses the main Serial port by default!
    // If DEBUG_ENABLE is defined, ensure it doesn't conflict or use a different serial port for debug.
    rosNode.init();
    DEBUG_PRINTLN("Micro-ROS node initialized.");
}

void loop() {
    unsigned long currentMillis = millis();

    // Check if it's time to read encoders and publish
    if (currentMillis - lastPublishTime >= publishIntervalMs) {
        lastPublishTime = currentMillis;

        // Read all configured encoders
        if (encoderManager.readAngles(currentAngles)) {
            // Successfully read, publish to ROS 2
            rosNode.publishAngles(currentAngles);
        } else {
            DEBUG_PRINTLN("Error reading from encoders.");
        }
    }

    // Spin the micro-ROS executor to handle potential incoming callbacks
    rosNode.spinSome();
}