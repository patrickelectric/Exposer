#include "exposer.h"

Exposer* exposer = &Exposer::self();

uint8_t led = 0;

uint8_t testuint8 = 0;
uint16_t testuint16 = 0;
uint32_t testuint32 = 0;
int8_t testint8 = 0;
int16_t testint16 = 0;
int32_t testint32 = 0;
float testfloat = 0;
String teststring = "Batata";

unsigned long next;

void setup()
{
    Serial.begin(115200);
    pinMode(13, OUTPUT);
    exposer->registerVariable(VARNAME(led), Exposer::_uint8_t, &led);
    exposer->registerVariable(VARNAME(testuint8), Exposer::_uint8_t, &testuint8);
    exposer->registerVariable(VARNAME(testuint16), Exposer::_uint16_t, &testuint16);
    exposer->registerVariable(VARNAME(testuint32), Exposer::_uint32_t, &testuint32);
    exposer->registerVariable(VARNAME(testint8), Exposer::_int8_t, &testint8);
    exposer->registerVariable(VARNAME(testint16), Exposer::_int16_t, &testint16);
    exposer->registerVariable(VARNAME(testint32), Exposer::_int32_t, &testint32);
    exposer->registerVariable(VARNAME(testfloat), Exposer::_float, &testfloat);
    exposer->registerVariable(VARNAME(teststring), Exposer::_string, &teststring);

    next = millis()+1000;
}

void loop()
{

    digitalWrite(13,(testint8==-50));

    exposer->update();

    if (millis()>next){
        Serial.print("uint8:");Serial.println(testuint8);
        Serial.print("uint16:");Serial.println(testuint16);
        Serial.print("uint32:");Serial.println(testuint32);
        Serial.print("int8:");Serial.println(testint8);
        Serial.print("int16:");Serial.println(testint16);
        Serial.print("int32:");Serial.println(testint32);
        Serial.print("float:");Serial.println(testfloat);
        Serial.print("string:");Serial.println(teststring);
        next = millis()+1000;
    }

}

