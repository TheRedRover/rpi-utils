#ifndef DHT11_H_
#define DHT11_H_

#include <pigpio.h>

namespace addons {

class DHT11 {
    public:
        void attach(int iPin);
        void detach();
        bool read(float& fTemp, float& fHum);
        DHT11() {};
        virtual ~DHT11() {};

    private:
        int waitLow(uint32_t uiTimeoutUs);
        int waitHigh(uint32_t uiTimeoutUs);
        bool sendRequest();

        int m_iPin = -1;  // by default is "detach" state

    };

}

inline void addons::DHT11::attach(int iPin) {
    m_iPin = iPin;
}

inline void addons::DHT11::detach() {
    m_iPin = -1;
}

#endif  // DHT11_H_