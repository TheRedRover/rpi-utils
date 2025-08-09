#ifndef BOOL_READER_H_
#define BOOL_READER_H_

#include <pigpio.h>

namespace addons {

class BoolReader {
    public:
        BoolReader(int pin);
        virtual ~BoolReader();

        bool read(bool& bValue);

    private:
        int m_iPin = -1;  // by default is "detach" state

    };

}

#endif  // BOOL_READER_H_
