#include "BoolReader.h"

#include <pigpio.h>
#include <string>
#include <sys/syslog.h>

#include "logger.h"

using namespace addons;

BoolReader::BoolReader(int iPin) : m_iPin(iPin) {}

BoolReader::~BoolReader() {}

bool BoolReader::read(bool& bValue) {
    try {
        gpioSetMode(m_iPin, PI_OUTPUT);
        gpioDelay(10);
        int iData =  gpioRead(m_iPin);
        Logger::log(LOG_DEBUG, "BoolReader| Get data [" + std::to_string(iData) +"]");
        bValue = iData;
    } catch (const std::exception& e) {
        Logger::log(LOG_ERR, "BoolReader| Failed to get data from the sensor: [" + std::string(e.what()) + "]");
        return false;
    }
    return true;
}