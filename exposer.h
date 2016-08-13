#pragma once

#include <Arduino.h>

#define VARNAME(x) (#x)
#define MAX_VARS 10

class Exposer
{

private:
    Exposer& operator = (Exposer& other) = delete;
    Exposer(const Exposer& other) = delete;
    Exposer();

    void* m_registeredAdresses[MAX_VARS];
    String  m_registeredNames[MAX_VARS];
    uint8_t  m_registeredTypes[MAX_VARS];
    uint8_t  m_registerCounter = 0;
    //const is used to force compiler to add variable in .text only one time
    static const uint8_t m_header;

    enum
    {
        WAITING_HEADER,        // <
        WAITING_OPERATION,	   // request_All, read, write
        WAITING_TARGET,		   // 0-255. variable register
        WAITING_PAYLOAD,	   // 0-255. data bytes to receive
        WAITING_DATA,          // data itself
        WAITING_CRC,		   // xor crc, 1 byte.
    };

    enum
    {
        REQUEST_ALL = 33,
        WRITE,
        READ
    };


    uint8_t m_currentState = WAITING_HEADER;
    uint8_t m_currentOperation = 0;
    uint8_t m_currentTarget = 0;
    uint8_t m_payloadLeft = 0;
    uint8_t m_totalPayload = 0;
    uint8_t m_databuffer[10];
    uint8_t m_crc = 0;

    void sendAllVariables();
    void sendVariableName(uint8_t i);

    void sendVariable(uint8_t i);

    void writeVariable(uint8_t target, uint8_t totalPayload, uint8_t* databuffer);

    virtual void sendByte(uint8_t data);

    //size of each type below, perhaps should be changed to sizeof(type)
    uint8_t m_sizes[7] =          // TODO: Should not be hardcoded
    {
        1,
        2,
        4,
        1,
        2,
        4,
        4
    };

public:

    enum
    {
        _uint8_t,
        _uint16_t,
        _uint32_t,
        _int8_t,
        _int16_t,
        _int32_t,
        _float,
    };

    static Exposer& self();

    uint8_t processByte(uint8_t data);
    void registerVariable(String name, uint8_t type, void* address);
};
