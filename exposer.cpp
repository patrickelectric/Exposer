#include "exposer.h"

const uint8_t Exposer::m_header = '<';

Exposer::Exposer()
{
    Serial.begin(115200);
}

Exposer& Exposer::self()
{
    static Exposer self;
    return self;
}

void Exposer::registerVariable(String name, uint8_t typ, void* address)
{
    /*
    Must be called like this:
    assuming you want to expose the "banana" variable, and it's an uint8_t:
    registerVariable(name(banana), Exposer::_uint8_t, &banana)
    */
    m_registeredNames[m_registerCounter] = name;
    m_registeredAdresses[m_registerCounter] = address;
    m_registeredTypes[m_registerCounter] = typ;
    m_registerCounter++;
}

void Exposer::sendByte(uint8_t data, uint8_t* crc)
{
    *crc ^= data;
    sendByte(data);
}

void Exposer::sendByte(uint8_t data)
{
    Serial.write(data);
}

void Exposer::sendVariable(uint8_t index)
{
    //------------------------------------------------------------------------
    // HEADER | OPERATION | TARGET | PAYLOADSIZE | PAYLOAD | CRC |
    //------------------------------------------------------------------------

    uint8_t crc = 0;
    //header
    sendByte(m_header, &crc);
    //operation
    sendByte(READ, &crc);
    //target variable
    sendByte(index, &crc);
    const uint8_t payloadSize = m_sizes[m_registeredTypes[index]];
    //varsize + type
    sendByte(payloadSize, &crc);

    for (uint8_t j = 0, byte; j < payloadSize; j++)
    {
        byte = ((uint8_t*)(m_registeredAdresses[index]))[j];
        sendByte(byte, &crc);
    }

    sendByte(crc);
}

void Exposer::sendVariableName(uint8_t i)
{
    uint8_t crc = 0;

    //header
    sendByte(m_header, &crc);
    //operation
    sendByte(REQUEST_ALL, &crc);
    sendByte(i, &crc);
    const uint8_t size = m_registeredNames[i].length();
    // varsize + type
    sendByte(size + 1, &crc);

    for (int j = 0; j < size; j++)
    {
        sendByte(m_registeredNames[i][j], &crc);
    }

    sendByte(m_registeredTypes[i], &crc);
    sendByte(crc);
}

void Exposer::sendAllVariables()
{
    for (uint8_t i = 0; i < m_registerCounter; i++)
    {
        sendVariableName(i);
    }
}

void Exposer::processByte(uint8_t data)
{

    switch (m_currentState)
    {
        case Exposer::WAITING_HEADER:
            if (data == m_header)
            {
                m_currentState = WAITING_OPERATION;
            }
            break;


        case WAITING_OPERATION:
            m_currentOperation = data;
            switch (data)
            {
                case REQUEST_ALL:
                case WRITE:
                case READ:
                    m_currentState = WAITING_TARGET;
                    break;

                // something went wrong?
                default:
                    m_currentOperation = 0;
                    m_currentState = WAITING_HEADER;
                    break;
            }

            break;


        case WAITING_TARGET:
            m_currentTarget = data;
            m_currentState = WAITING_PAYLOAD;
            m_crc = m_header ^ m_currentOperation ^ m_currentTarget;
            break;


        case WAITING_PAYLOAD:
            m_totalPayload = data;
            m_payloadLeft = m_totalPayload;
            m_crc ^= data;
            if (m_totalPayload > 0)
            {
                m_currentState = WAITING_DATA;
            }
            else
            {
                m_currentState = WAITING_CRC;
            }

            break;


        case WAITING_DATA:
            m_databuffer[m_totalPayload-m_payloadLeft] = data;
            m_payloadLeft--;
            m_crc ^= data;
            if (m_payloadLeft == 0)
            {
                m_currentState = WAITING_CRC;
            }
            break;

        case WAITING_CRC:
            if (m_crc == data)
            {
                switch (m_currentOperation)
                {
                    case REQUEST_ALL:
                        sendAllVariables();
                        break;

                    case READ:
                        sendVariable(m_currentTarget);
                        break;

                    case WRITE:
                        writeVariable(m_currentTarget, m_totalPayload, m_databuffer);
                        break;
                }
            }
            else
            {
                Serial.println("CRC MISMATCH!");
            }
            m_currentState = WAITING_HEADER;

            break;
    }

}
void Exposer::writeVariable(uint8_t target, uint8_t totalPayload, uint8_t* databuffer)
{
    for (uint8_t i = 0; i < totalPayload; i++)
    {
        ((uint8_t*)m_registeredAdresses[target])[i] = databuffer[i];
    }
}
