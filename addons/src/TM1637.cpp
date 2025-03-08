#include "TM1637.h"
#include "logger.h"

#include <cctype>
#include <pigpio.h>
#include <sstream>
#include <string>
#include <sys/syslog.h>
#include <bitset>

 //    - A -
 //   F     B
 //    - G -
 //   E     C
 //    - D -

std::map<char, char> addons::TM1637::CHARS_TO_SIGNAL = {
    {'\0', 0x0},
    { ' ', 0x0 },
    { '0', SegA | SegB | SegC | SegD | SegE | SegF },
    { '1', SegB | SegC },
    { '2', SegA | SegB | SegG | SegE | SegD },
    { '3', SegA | SegB | SegG | SegC | SegD },
    { '4', SegF | SegG | SegB | SegC },
    { '5', SegA | SegF | SegG | SegC | SegD },
    { '6', SegF | SegE | SegG | SegC | SegD },
    { '7', SegA | SegB | SegC },
    { '8', SegA | SegB | SegC | SegD | SegE | SegF | SegG },
    { '9', SegA | SegB | SegC | SegF | SegG },
    { 'A', SegA | SegB | SegC | SegE | SegF | SegG },
    { 'B', SegF | SegE | SegG | SegC | SegD },
    { 'C', SegA | SegF | SegE | SegD },
    { 'D', SegB | SegC | SegG | SegE | SegD },
    { 'E', SegA | SegF | SegG | SegE | SegD },
    { 'G', SegA | SegF | SegE | SegD | SegC | SegG },
    { 'F', SegA | SegF | SegG | SegE },
    { 'H', SegF | SegG | SegE | SegB | SegC },
    { 'I', SegE },
    { 'J', SegB | SegC | SegD | SegE },
    { 'L', SegF | SegE | SegD },
    { 'N', SegE | SegG | SegC },
    { 'O', SegE | SegG | SegC | SegD },
    { 'P', SegA | SegB | SegF | SegG | SegE },
    { 'R', SegG | SegE },
    { 'S', SegA | SegF | SegG | SegC | SegD },
    { 'T', SegF | SegG | SegE | SegD },
    { 'U', SegF | SegE|SegD|SegB|SegC },
    { 'Y', SegF | SegG | SegB | SegC | SegD },
    { 'Z', SegA | SegB | SegG | SegE | SegD },
    { '-', SegG },
    { '_', SegD },
    { '.', SegD },
    { ',', SegD },
    { '\'', SegB },
    { '"', SegF | SegB },
    { char(0xB0), SegF | SegA | SegB | SegG }, // Degree sign (ISO/IEC 8859-1)
    { '*', SegF | SegA | SegB | SegG },        // Degree sign replacement
    { '\\', SegF | SegG | SegC },
    { '/', SegE | SegG | SegB },
    { '^', SegF | SegA | SegB }
};

addons::TM1637::TM1637(int iIOPin, int iClkPin) : m_iIOPin(iIOPin), m_iClkPin(iClkPin) {
    gpioSetMode(m_iIOPin, PI_OUTPUT);
    gpioSetMode(m_iClkPin, PI_OUTPUT);
}

addons::TM1637::~TM1637() {}

void addons::TM1637::setBrightness(int iBr) {
    if(iBr < 0 || iBr > 7) {
        std::stringstream ss;
        ss << "TM1637| Invalid brightness level: " << iBr;
        Logger::log(LOG_ERR, ss.str());
        return;
    }

    m_iBrightness = iBr;

    display();
}

void addons::TM1637::display() {
    std::stringstream ss;
    ss << "TM1637| Displaying: [" << m_data << "]";
    Logger::log(LOG_DEBUG, ss.str());

    int iAtt = 3;
    bool bRes = true;

    do {
        ss.str("");
        ss << "TM1637| Display attempt: [" << iAtt << "]";
        Logger::log(LOG_DEBUG, ss.str());
        startTransmission();
        writeByte(AUTO_ADDRESS_MODE);
        stopTransmission();

        startTransmission();
        writeByte(ADDRESS_OF_FIRST);
        for (int i = 0; i < 4; i++) {
            bRes &= writeByte(charToSignal(i, m_data[i]));
            gpioDelay(20);
        }
        stopTransmission();
    
        startTransmission();
        writeByte(DISPLAY_ON + m_iBrightness);
        stopTransmission();
        --iAtt;

    } while(!bRes && iAtt > 0);

}

void addons::TM1637::display(char cData, int iPos) {
    if (iPos < 0 || iPos > 3) {
        std::stringstream ss;
        ss << "TM1637| Display error. Invalid position: " << iPos;
        Logger::log(LOG_ERR, ss.str());
    }

    m_data[iPos] = cData;

    display();
}

void addons::TM1637::display(int iData, bool bDots) {
    display(std::to_string(iData), bDots);
}
void addons::TM1637::display(const std::string& sData, bool bDots) {
    if (sData.size() > 4) {
        std::stringstream ss;
        ss << "TM1637| Invalid data to display: [" << sData << "], length: [" << sData.size() << "]";
        Logger::log(LOG_ERR, ss.str());
        return;
    }

    for (auto i = 0; i < 4; i++ ) {
        m_data[i] = 0;
    }

    std::copy(sData.begin(), sData.end(), m_data);

    m_bPoints = bDots;

    display();
}

void addons::TM1637::switchPoints(bool bPoints) {
    m_bPoints = bPoints;
    display();
}

void addons::TM1637::startTransmission() {
    gpioSetMode(m_iIOPin, PI_OUTPUT);
    gpioWrite(m_iIOPin, 1);
    gpioWrite(m_iClkPin, 1);
    gpioDelay(10);
    gpioWrite(m_iIOPin, 0);
    gpioDelay(10);
    gpioWrite(m_iClkPin, 0);
}

void addons::TM1637::stopTransmission() {
    gpioSetMode(m_iIOPin, PI_OUTPUT);
    gpioWrite(m_iClkPin, 0);
    gpioWrite(m_iIOPin, 0);
    gpioDelay(10);
    gpioWrite(m_iClkPin, 1);
    gpioDelay(10);
    gpioWrite(m_iIOPin, 1);
}

bool addons::TM1637::writeByte(char cByte) {
    char cMask = 0x01;
    for (int i = 0; i < 8; i++) {
        gpioWrite(m_iClkPin, 0);
        gpioDelay(50);

        gpioWrite(m_iIOPin, (cByte & cMask)? 1: 0);

        cMask <<= 1;
        gpioDelay(50);
        gpioWrite(m_iClkPin, 1);
        gpioDelay(50);
    }

    bool bAck = false;
    gpioSetMode(m_iIOPin, PI_INPUT);
    gpioWrite(m_iClkPin, 0);
    gpioWrite(m_iIOPin, 0);
    gpioDelay(50);
    for (int i=1; i<=50; i++) {
        if (0==gpioRead(m_iIOPin)) {
            bAck = true;
            break;
        }
        gpioDelay(20);
    }

    gpioWrite(m_iClkPin, 1);
    gpioSetMode(m_iIOPin, PI_OUTPUT);
    gpioDelay(50);
    gpioWrite(m_iClkPin, 0);
    gpioDelay(50);
    return bAck;
}

char addons::TM1637::charToSignal(int iPos, char ch) {
    char cSig = 0x0;
    if (CHARS_TO_SIGNAL.count(std::toupper(ch))) {
        cSig = CHARS_TO_SIGNAL[std::toupper(ch)];
    } else {
        std::stringstream ss;
        ss << "Char is not in list: " << ch << ".";
        Logger::log(LOG_WARNING, ss.str());
    }

    if (1==iPos && m_bPoints) {
        // DP ony supported at center
        cSig |= SegDP;
    }

    return cSig;
}