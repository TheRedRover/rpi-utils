#ifndef TM1637_H_
#define TM1637_H_

#include <map>
#include <string>

namespace addons {

class TM1637 {

public:
    TM1637(int iIOPin, int iClkPin);
    virtual ~TM1637();

    void setBrightness(int iBr);

    void display();
    void display(char cData, int iPos);
    void display(int iData, bool bDots);
    void display(const std::string& sData, bool bDots);
    void switchPoints(bool bPoints);

    void clear();
private:
    enum Segment {
        SegA  = 0x01, //0b00000001
        SegB  = 0x02, //0b00000010
        SegC  = 0x04, //0b00000100
        SegD  = 0x08, //0b00001000
        SegE  = 0x10, //0b00010000
        SegF  = 0x20, //0b00100000
        SegG  = 0x40, //0b01000000
        SegDP = 0x80, //0b10000000
    };

    enum Mode {
        FIXED_ADDRESS_MODE  = 0x44,
        AUTO_ADDRESS_MODE   = 0x40,
    };

    const char ADDRESS_OF_FIRST = 0xC0;
    const char DISPLAY_ON = 0x88;

    static std::map<char, char> CHARS_TO_SIGNAL;

    void startTransmission();
    void stopTransmission();
    bool writeByte(char cByte);
    char charToSignal(int pos, char ch);

    int m_iIOPin;
    int m_iClkPin;
    int m_iBrightness=7;
    bool m_bPoints = false;

    char m_data[5] {0,0,0,0, '\0'};

};

}

#endif // TM1637_H_