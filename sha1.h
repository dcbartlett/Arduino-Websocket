#ifndef Sha1_h
#define Sha1_h

#include <inttypes.h>
#include <WString.h>

class Sha1
{
public:

    static uint8_t const HashLength = 20;
    static uint8_t const BlockLength = 64;

    Sha1();
    void update(uint8_t const* data, uint16_t len)
    {
        for (uint16_t i = 0; i < len; ++i)
        {
            write(data[i]);
        }
    }

    void update(uint8_t data)
    {
        write(data);
    }

    void update(String const& s)
    {
        for (uint16_t i = 0; i < s.length(); ++i)
        {
            write(s[i]);
        }
    }

    void finish(uint8_t* out);

private:
    union _buffer
    {
      uint8_t b[BlockLength];
      uint32_t w[BlockLength/4];
    };
    union _state
    {
      uint8_t b[BlockLength];
      uint32_t w[BlockLength/4];
    };

    size_t write(uint8_t data);
    void pad();
    void addUncounted(uint8_t data);
    void hashBlock();
    _buffer buffer;
    uint8_t bufferOffset;
    _state state;
    uint32_t byteCount;
    uint8_t keyBuffer[BlockLength];
    uint8_t innerHash[HashLength];
};

#endif
