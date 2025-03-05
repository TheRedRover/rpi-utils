#ifndef DHT11_H_
#define DHT11_H_

#include <pigpio.h>

namespace addons {

class DHT11 {
    public:
        void attach(int iPin);
        void detach();
        bool read(float& fTemp, float& fHum);

    private:
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