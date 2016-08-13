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
    sendByte(m_header);                              // header
    sendByte(READ);                             // operation
    sendByte(index);                            // target variable
    crc = m_header ^ READ ^ index;
    uint8_t payloadSize = m_sizes[m_registeredTypes[index]];
    sendByte(payloadSize);                           // varsize + type
    crc = crc ^ (payloadSize);

    for (int j = 0; j < payloadSize; j++)
    {
        uint8_t byte = ((uint8_t*)(m_registeredAdresses[index]))[j];
        sendByte(byte);
        crc ^= byte;
    }

    sendByte(crc);                                                                      // crc
}

void Exposer::sendVariableName(uint8_t i)
{
    uint8_t crc = 0;
    sendByte(m_header);                              // header
    sendByte(REQUEST_ALL);                      // operation
    sendByte(i);                                // target variable
    crc = m_header ^ REQUEST_ALL ^ i;
    char buffer[10];                            //maximum of 10 chars on variable
    m_registeredNames[i].toCharArray(buffer,10);
    buffer[9] = '\0';
    int size = m_registeredNames[i].length();
    sendByte(size+1);                           // varsize + type
    crc = crc ^ (size+1);


    for (int j = 0; j < size; j++)
    {
        sendByte(buffer[j]);
        crc ^= buffer[j];
    }

    sendByte(m_registeredTypes[i]);

    crc ^= m_registeredTypes[i];
    sendByte(crc);									// crc
}

void Exposer::sendAllVariables()
{
    for (int i = 0; i < m_registerCounter; i++)
    {
        sendVariableName(i);
    }
}

uint8_t Exposer::processByte(uint8_t data)
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

                default:  // something went wrong?
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
    for (int i = 0; i < totalPayload; i++)
    {
        ((uint8_t*)m_registeredAdresses[target])[i] = databuffer[i];
    }
}
