#include "DHT11.h"
#include <cstdint>
#include <exception>
#include <pigpio.h>
#include <stdexcept>
#include <string>
#include <sys/syslog.h>
#include <unistd.h>

#include "logger.h"

addons::DHT11::DHT11(int iPin) : m_iPin(iPin) {}

addons::DHT11::~DHT11() {}

bool addons::DHT11::read(float& fTemp, float& fHum) {
    Logger::log(LOG_DEBUG, "DHT11| Reading info from the gpio [" + std::to_string(m_iPin) + "]");

    if (m_iPin < 0) {
        Logger::log(LOG_ERR, "DHT11| Invalid GPIO pin [" + std::to_string(m_iPin) + "]");
        return false;
    }

    uint8_t data[5] = {0, 0, 0, 0, 0};

    // Send start signal
    if (!sendRequest()) {
        return false;
    }

    try {
        // --- PHASE 1: Acknowledgment ---
        // After start pulse, sensor pulls LOW for 80us, then HIGH for 80us
        waitLow(2000);  // Wait for sensor to pull line LOW
        waitHigh(2000); // Wait for sensor to pull line HIGH
        waitLow(2000);  // Wait for sensor to pull line LOW (start of first bit)

        // --- PHASE 2: Data Transmission (40 bits) ---
        for (int i = 0; i < 40; ++i) {
            // Every bit starts with a 50us LOW pulse (which we are currently in)
            // We wait for it to go HIGH to start measuring the data pulse
            waitHigh(2000); 

            // Measure how long the line stays HIGH
            // 26-28us = "0"
            // 70us    = "1"
            uint32_t highTime = waitLow(2000);

            // Shift bits into the array (8 bits per byte)
            data[i / 8] <<= 1;
            if (highTime > 40) { // 40us is a safe threshold between 28 and 70
                data[i / 8] |= 0x1;
            }
        }

        // Final state: sensor releases line to HIGH
        // No need to throw if this fails, as data is already captured
    } catch (const std::exception& e) {
        // Reset pin to output HIGH (idle state) before returning
        gpioSetMode(m_iPin, PI_OUTPUT);
        gpioWrite(m_iPin, 1);
        Logger::log(LOG_ERR, "DHT11| Failed to get data from the sensor: [" + std::string(e.what()) + "]");
        return false;
    }

    // --- PHASE 3: Checksum and Data Extraction ---
    uint8_t humHigh  = data[0];
    uint8_t humLow   = data[1];
    uint8_t tempHigh = data[2];
    uint8_t tempLow  = data[3];
    uint8_t checksum = data[4];

    if (checksum != static_cast<uint8_t>(humHigh + humLow + tempHigh + tempLow)) {
        Logger::log(LOG_ERR, "DHT11| Failed to read data from sensor: incorrect checksum");
        return false;
    }

    fHum = static_cast<float>(humHigh);
    fTemp = static_cast<float>(tempHigh);

    // Note: If you have a DHT22 or a DHT11 that supports decimals, 
    // the logic for fHum/fTemp would need to combine high and low bytes.
    // For a standard DHT11, humHigh and tempHigh are usually sufficient.

    return true;
}

int addons::DHT11::waitLow(uint32_t uiTimeoutUs) {
    auto StartTime = gpioTick();
    while (gpioRead(m_iPin)) {
        if (uiTimeoutUs < (gpioTick() - StartTime)) {
            throw std::runtime_error("Time out waiting for LOW: " + std::to_string(uiTimeoutUs));
        }
    }
    return gpioTick() - StartTime;
}

int addons::DHT11::waitHigh(uint32_t uiTimeoutUs) {
    auto StartTime = gpioTick();
    while (!gpioRead(m_iPin)) {
        if (uiTimeoutUs < (gpioTick() - StartTime)) {
            throw std::runtime_error("Time out waiting for HIGH: " + std::to_string(uiTimeoutUs));
        }
    }
    return gpioTick() - StartTime;
}

bool addons::DHT11::sendRequest() {
    // 1. Ensure line is HIGH (idle)
    gpioSetMode(m_iPin, PI_OUTPUT);
    gpioWrite(m_iPin, 1);
    gpioDelay(50000); 

    // 2. Start signal: Pull LOW for 20ms
    gpioWrite(m_iPin, 0);
    gpioDelay(20000); 

    // 3. CRITICAL: Switch to INPUT immediately.
    // Do not manually write HIGH. Let the pull-up resistor lift the line.
    // This prevents the Pi from "fighting" the sensor if it responds fast.
    gpioSetMode(m_iPin, PI_INPUT);
    gpioSetPullUpDown(m_iPin, PI_PUD_UP);
    
    // Give the pull-up a tiny moment (microsecond) to lift the line 
    // before we start looking for the sensor's LOW pulse.
    return true;
}
