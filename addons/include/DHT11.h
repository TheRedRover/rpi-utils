#ifndef DHT11_H_
#define DHT11_H_

#include <pigpio.h>

namespace addons {

class DHT11 {
    public:
        DHT11(int pin);
        virtual ~DHT11();

        bool read(float& fTemp, float& fHum);

    private:
        int waitLow(uint32_t uiTimeoutUs);
        int waitHigh(uint32_t uiTimeoutUs);
        bool sendRequest();

        int m_iPin = -1;  // by default is "detach" state

    };

}

#endif  // DHT11_H_
