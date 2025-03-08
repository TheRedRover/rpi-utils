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
    Logger::log(LOG_DEBUG, "HDT11| Reading info from the gpio [" + 
                                               std::to_string(m_iPin) + "]");

    if (m_iPin < 0) {
        Logger::log(LOG_ERR, "HDT11| Invalid GPIO pin [" + 
                                                 std::to_string(m_iPin) + "]");
        return false;
    }

    uint64_t data = 0;

    // Send start signal
    sendRequest();

    // Switch to input mode to read data

    try {
        waitLow(420);
        waitHigh(900);
        waitLow(1000);
        for (int i = 0; i < 40; ++i) {
            data <<= 1;
            int LowTime = waitHigh(1000);
            int HighTime = waitLow(1000);
            if (LowTime < HighTime) {
                data |= 0x1;
            }
        }
        // end state
        waitHigh(1000);
    } catch (const std::exception& e) {
        gpioSetMode(m_iPin, PI_OUTPUT);
        gpioWrite(m_iPin, 1);
        Logger::log(LOG_ERR, "DHT11| Failed to get data from the sensor: [" + std::string(e.what()) + "]");
        return false;
    }

    uint8_t humHigh = (data >> 32) & 0xFF;
    uint8_t humLow = (data >> 24) & 0xFF;
    uint8_t tempHigh = (data >> 16) & 0xFF;
    uint8_t tempLow = (data >> 8) & 0xFF;
    uint8_t checksum = data & 0xFF;

    if (checksum != static_cast<uint8_t> (humHigh + humLow + tempHigh + tempLow)) {
        Logger::log(LOG_ERR, "DHT11| Failed to read data from sensor: incorrect checksum");
        return false;
    }

    fTemp = tempHigh;
    fHum = humHigh;

    return true;
}

int addons::DHT11::waitLow(uint32_t uiTimeoutMs) {
    auto StartTime = gpioTick();
    while (gpioRead(m_iPin)) {
        if (uiTimeoutMs < gpioTick() - StartTime) {
            throw std::runtime_error("Time out waiting for LOW: " + std::to_string(uiTimeoutMs));
        }
    }
    return gpioTick() - StartTime;
}

int addons::DHT11::waitHigh(uint32_t uiTimeoutMs) {
    auto StartTime = gpioTick();
    while (!gpioRead(m_iPin)) {
        if (uiTimeoutMs < gpioTick() - StartTime) {
            throw std::runtime_error("Time out waiting for HIGH: " + std::to_string(uiTimeoutMs));
        }
    }
    return gpioTick() - StartTime;
}

bool addons::DHT11::sendRequest() {
    gpioSetMode(m_iPin, PI_OUTPUT);
    gpioWrite(m_iPin,1);
    gpioDelay(50000);


    gpioWrite(m_iPin, 0);
    gpioDelay(18000);
    gpioWrite(m_iPin, 1);
    gpioSetMode(m_iPin, PI_INPUT);

    return true;
}
