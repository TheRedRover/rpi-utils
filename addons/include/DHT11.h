#ifndef DHT11_H_
#define DHT11_H_

#include <optional>
#include <pigpio.h>

namespace addons {

class DHT11 {
    public:
        DHT11(int pin);
        virtual ~DHT11();

        std::optional<float> getTemp();
        std::optional<float> getHum();

    private:
        bool read();
        int waitLow(uint32_t uiTimeoutUs);
        int waitHigh(uint32_t uiTimeoutUs);
        bool sendRequest();

        int m_iPin = -1;  // by default is "detach" state

        std::optional<float> m_optTemp;
        std::optional<float> m_optHum;
        uint32_t m_lastReadTick = 0;
    };

}

#endif  // DHT11_H_
