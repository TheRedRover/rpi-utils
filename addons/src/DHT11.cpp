#include "DHT11.h"
#include <pigpio.h>
#include <unistd.h>

#include "logger.h"

bool addons::DHT11::read(float& fTemp, float& fHum) {
    uint8_t data[5] = {0};
    int bit_count = 0, byte_index = 0;
    
    // Ensure pigpio is initialized
    if (gpioInitialise() < 0) {
        
        return false;
    }

    // Send start signal
    gpioSetMode(m_iPin, PI_OUTPUT);
    gpioWrite(m_iPin, 0);
    usleep(18000);  // Wait at least 18ms
    gpioWrite(m_iPin, 1);
    usleep(40);
    
    // Switch to input mode to read data
    gpioSetMode(m_iPin, PI_INPUT);
    
    // Wait for response signal
    uint32_t start_time = gpioTick();
    while (gpioRead(m_iPin) == 1) {
        if (gpioTick() - start_time > 100) {
            Logger::log(LOG_ERR, "DHT11 | response timeout");
            return false;
        }
    }

    start_time = gpioTick();
    while (gpioRead(m_iPin) == 0); // Wait for sensor to pull up
    while (gpioRead(m_iPin) == 1); // Wait for sensor to pull down

    // Read 40 bits (5 bytes) from sensor
    for (int i = 0; i < 40; i++) {
        while (gpioRead(m_iPin) == 0); // Wait for m_iPin to go high
        start_time = gpioTick();

        while (gpioRead(m_iPin) == 1); // Measure the width of the high pulse
        if (gpioTick() - start_time > 50) {
            data[byte_index] |= (1 << (7 - bit_count)); // Set bit to 1
        }

        bit_count++;
        if (bit_count == 8) {
            bit_count = 0;
            byte_index++;
        }
    }

    // Verify checksum
    if (data[4] != (data[0] + data[1] + data[2] + data[3])) {
        Logger::log(LOG_ERR, "DHT11 | failed to verify checksum");
        return false;
    }

    fHum = data[0];
    fTemp = data[2];

    gpioTerminate();

    return true;
}