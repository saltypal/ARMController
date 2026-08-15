#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include <Wire.h>

class EncoderManager {
public:
    EncoderManager();

    /**
     * Initializes the I2C bus and verifies connection to the encoders.
     * @return true if initialization is successful, false otherwise.
     */
    bool init();

    /**
     * Reads the current angles from all configured encoders.
     * @param angles Array to store the read angles in radians. Must be at least NUM_ENCODERS in size.
     * @return true if all readings were successful, false if there were errors.
     */
    bool readAngles(float* angles);

    /**
     * Performs a scan of the I2C bus to find connected devices.
     * Useful for debugging hardware connections.
     */
    void scanI2CBus();

private:
    /**
     * Selects the active channel on the I2C multiplexer.
     * If NUM_ENCODERS <= 1, this function does nothing.
     * @param channel The channel index (0-7).
     */
    void selectMultiplexerChannel(uint8_t channel);

    /**
     * Reads the raw angle (0-4095) from a single AS5600.
     * @return The raw angle, or -1 on error.
     */
    int16_t readRawAngle();
    
    /**
     * Converts a raw AS5600 12-bit value to radians.
     * @param rawAngle 12-bit raw value (0-4095)
     * @return Angle in radians (0 to 2*PI)
     */
    float rawToRadians(int16_t rawAngle);
};

#endif // ENCODER_MANAGER_H
