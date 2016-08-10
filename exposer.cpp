#include "exposer.h"

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
    registeredNames[registerCounter] = name;
    registeredAdresses[registerCounter] = address;
    registeredTypes[registerCounter] = typ;
    registerCounter++;
}

void Exposer::sendByte(uint8_t data)
{
    Serial.write(data);
}

void Exposer::sendVariable(uint8_t index)
{
    //------------------------------------------------------------------------
    // HEADER | OPERATION | TARGET | PAYLOADSIZE | PAYLOAD |PAYLOADTYPE| CRC |
    //------------------------------------------------------------------------

    // this is the only message containing PAYLOADTIME
    Serial.print("size: ");Serial.println(sizes[registeredTypes[index]]);
    uint8_t crc = 0;
    sendByte('<');                              // header
    sendByte(READ);                             // operation
    sendByte(index);                            // target variable
    crc = '<'^ READ ^ index;
    uint8_t payloadSize = sizes[registeredTypes[index]];
    sendByte(payloadSize);                           // varsize + type
    crc = crc ^ (payloadSize);

    for (int j = 0; j < payloadSize; j++)
    {
        uint8_t byte = *(uint8_t*)(registeredAdresses[index] + j);
        sendByte(byte);
        crc ^= byte;
    }

    sendByte(crc);                                                                      // crc
}

void Exposer::sendVariableName(uint8_t i)
{

    //------------------------------------------------------------------------
    // HEADER | OPERATION | TARGET | PAYLOADSIZE | PAYLOAD |PAYLOADTYPE| CRC |
    //------------------------------------------------------------------------

    // this is the only message containing PAYLOADTIME

    uint8_t crc = 0;
    sendByte('<');                              // header
    sendByte(REQUEST_ALL);                      // operation
    sendByte(i);                                // target variable
    crc = '<'^ REQUEST_ALL ^ i;
    char buffer[10];                            //maximum of 10 chars on variable
    registeredNames[i].toCharArray(buffer,10);
    buffer[9] = '\0';
    int size = registeredNames[i].length();
    sendByte(size+1);                           // varsize + type
    crc = crc ^ (size+1);


    for (int j = 0; j < size; j++)
    {
        sendByte(buffer[j]);
        crc ^= buffer[j];
    }

    sendByte(registeredTypes[i]);

    crc ^= registeredTypes[i];
    sendByte(crc);									// crc
}

void Exposer::sendAllVariables()
{
    for (int i = 0; i < registerCounter; i++)
    {
        sendVariableName(i);
    }
}

uint8_t Exposer::processByte(uint8_t data)
{

    switch (currentState)
    {
        case Exposer::WAITING_HEADER:
            if (data == '<')
            {
                currentState = WAITING_OPERATION;
            }
            break;


        case WAITING_OPERATION:
            currentOperation = data;
            switch (data)
            {
                case REQUEST_ALL:
                    currentState = WAITING_TARGET;
                    break;

                case WRITE:
                    currentState = WAITING_TARGET;
                    break;

                case READ:
                    currentState = WAITING_TARGET;
                    break;

                default:  // something went wrong?
                    currentOperation = 0;
                    currentState = WAITING_HEADER;
                    break;
            }

            break;


        case WAITING_TARGET:
            currentTarget = data;
            currentState = WAITING_PAYLOAD;
            crc = '<' ^ currentOperation ^ currentTarget;
            break;


        case WAITING_PAYLOAD:
            totalPayload = data;
            payloadLeft = totalPayload;
            crc ^= data;
            if (totalPayload > 0)
            {
                currentState = WAITING_DATA;
            }
            else
            {
                currentState = WAITING_CRC;
            }

            break;


        case WAITING_DATA:
            databuffer[totalPayload-payloadLeft] = data;
            payloadLeft--;
            crc ^= data;
            if (payloadLeft == 0)
            {
                currentState = WAITING_CRC;
            }
            break;

        case WAITING_CRC:
            if (crc == data)
            {
                switch (currentOperation)
                {
                    case REQUEST_ALL:
                        sendAllVariables();
                        break;

                    case READ:
                        sendVariable(currentTarget);
                        break;

                    case WRITE:
                        writeVariable(currentTarget, totalPayload, databuffer);
                        break;
                }
            }
            else
            {
                Serial.println("CRC MISMATCH!");
            }
            currentState = WAITING_HEADER;

            break;
    }

}
void Exposer::writeVariable(uint8_t target, uint8_t totalPayload, uint8_t* databuffer)
{
    for (int i = 0; i < totalPayload; i++)
    {
        * (uint8_t*)registeredAdresses[target+i] = databuffer[i];
    }
}
