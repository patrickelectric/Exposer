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

void Exposer::sendVariable(uint8_t i)
{

	//------------------------------------------------------------
	// HEADER | OPERATION | TARGET | PAYLOADSIZE | PAYLOAD | CRC |
	//------------------------------------------------------------

	uint8_t crc = 0;
	sendByte('<');								// header
	sendByte(REQUEST_ALL);						// operation
	sendByte(i);								// target variable
	crc = '<'^ REQUEST_ALL ^ i;	
	char buffer[10];
	registeredNames[i].toCharArray(buffer,10);
	buffer[9] = '\0';
	int size = registeredNames[i].length();
	sendByte(size);								// payloadSize
	crc = crc ^ size;
	

	for(int j = 0; j < size; j++)
	{
		sendByte(buffer[j]);
		crc ^= buffer[j];
	}
	sendByte(crc);									// crc
}	

void Exposer::sendAllVariables()
{
	Serial.print("Sending all variables! count: "); Serial.println(registerCounter);
	for(int i = 0; i < registerCounter; i++)
	{
		Serial.println(registeredNames[i]);
		sendVariable(i);
	}
}

uint8_t Exposer::processByte(uint8_t data)
{

	switch(currentState){


		case Exposer::WAITING_HEADER:
			if (data == '<')
			{
				currentState = WAITING_OPERATION;
				Serial.println("got header");
			}
		break;


		case WAITING_OPERATION:
			Serial.print("got operation ");Serial.println(data);
			currentOperation = data;
			switch(data)
			{
				case REQUEST_ALL:
					crc = '<' ^ REQUEST_ALL;
					currentState = WAITING_CRC;
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
					Serial.println("bad operation!");
					Serial.println(data);
				break;
			}
			
		break;


		case WAITING_TARGET:
			currentTarget = data;
			currentState = WAITING_PAYLOAD;
			crc = data ^ currentOperation;
			Serial.print("got target");Serial.println(data);
		break;


		case WAITING_PAYLOAD:
			Serial.print("got payload size ");Serial.println(data);
			totalPayload = data;
			payloadLeft = totalPayload;
			currentState = WAITING_DATA;
			crc ^= data;
		break;


		case WAITING_DATA:
			Serial.println("got data!");
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
				Serial.println("CRC CHECKS!");
			
				switch(currentOperation)
				{
					case REQUEST_ALL:
						sendAllVariables();
					break;

					case READ:

					break;

					case WRITE:

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