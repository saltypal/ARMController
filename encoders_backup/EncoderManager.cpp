#include "EncoderManager.h"

EncoderManager::EncoderManager() {
}

bool EncoderManager::init() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_SPEED);
    
    DEBUG_PRINTLN("EncoderManager: I2C Initialized.");
    
    // Optional: scan bus for debug
    scanI2CBus();

    // Verify connections to all configured encoders
    bool allOk = true;
    for (int i = 0; i < NUM_ENCODERS; i++) {
        selectMultiplexerChannel(i);
        
        Wire.beginTransmission(AS5600_I2C_ADDRESS);
        if (Wire.endTransmission() != 0) {
            DEBUG_PRINT("EncoderManager: Failed to find AS5600 on channel ");
            DEBUG_PRINTLN(i);
            allOk = false;
        } else {
            DEBUG_PRINT("EncoderManager: Found AS5600 on channel ");
            DEBUG_PRINTLN(i);
        }
    }
    
    return allOk;
}

bool EncoderManager::readAngles(float* angles) {
    bool success = true;
    for (int i = 0; i < NUM_ENCODERS; i++) {
        selectMultiplexerChannel(i);
        
        int16_t raw = readRawAngle();
        if (raw >= 0) {
            angles[i] = rawToRadians(raw);
        } else {
            angles[i] = 0.0f; // Default on error, could also retain last known value
            success = false;
        }
    }
    return success;
}

void EncoderManager::selectMultiplexerChannel(uint8_t channel) {
#if NUM_ENCODERS > 1
    if (channel > 7) return; // TCA9548A supports channels 0-7
    
    Wire.beginTransmission(TCA9548A_I2C_ADDRESS);
    Wire.write(1 << channel); // Enable only the specified channel
    Wire.endTransmission();
#else
    // If we only have 1 encoder, we assume it's connected directly to the I2C bus
    // and no multiplexer is used. Do nothing.
    (void)channel; // Prevent unused parameter warning
#endif
}

int16_t EncoderManager::readRawAngle() {
    Wire.beginTransmission(AS5600_I2C_ADDRESS);
    Wire.write(AS5600_RAW_ANGLE_REG_HI);
    if (Wire.endTransmission(false) != 0) {
        return -1; // Communication error
    }
    
    Wire.requestFrom(AS5600_I2C_ADDRESS, 2);
    if (Wire.available() == 2) {
        uint8_t highByte = Wire.read();
        uint8_t lowByte = Wire.read();
        // AS5600 raw angle is a 12-bit value
        return ((highByte << 8) | lowByte) & 0x0FFF;
    }
    return -1;
}

float EncoderManager::rawToRadians(int16_t rawAngle) {
    // 12-bit resolution means values 0 to 4095 map to 0 to 2*PI
    return ((float)rawAngle / 4096.0f) * 2.0f * PI;
}

void EncoderManager::scanI2CBus() {
    DEBUG_PRINTLN("EncoderManager: Scanning I2C bus...");
    byte error, address;
    int nDevices = 0;

    for(address = 1; address < 127; address++ ) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            DEBUG_PRINT("I2C device found at address 0x");
            if (address < 16) DEBUG_PRINT("0");
            DEBUG_PRINTLN(address, HEX);
            nDevices++;
        }
        else if (error == 4) {
            DEBUG_PRINT("Unknown error at address 0x");
            if (address < 16) DEBUG_PRINT("0");
            DEBUG_PRINTLN(address, HEX);
        }    
    }
    if (nDevices == 0) {
        DEBUG_PRINTLN("No I2C devices found\n");
    } else {
        DEBUG_PRINTLN("done\n");
    }
}
