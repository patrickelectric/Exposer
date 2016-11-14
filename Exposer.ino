#include "exposer.h"
#include <math.h>

// Declare exposer
Exposer* exposer = &Exposer::self();

const float ppi = 2*M_PI;

// Create some variables
uint8_t led = 0;
uint8_t testuint8 = 0;
uint16_t testuint16 = 0;
uint32_t testuint32 = 0;
int8_t testint8 = 0;
int16_t testint16 = 0;
int32_t testint32 = 0;
static float testfloat = 1.11;
String teststring = "Batata";

unsigned long next;

void setup()
{
    // Hardware initialization
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
    // Get actual time
    unsigned int now = millis();

    // Create omega
    float w0 = ppi*now/100;
    float w1 = ppi*now/1000;
    float w2 = ppi*now/2000;
    float w3 = ppi*now/3000;
    float w4 = ppi*now/4000;

    // Update variables
    led = now/100;
    testuint8 = 100*(sin(w1) + 1);
    testuint16 = 100*(sin(w2 + ppi/4) + 1);
    testuint32 = 100*(sin(w3 - ppi/4) + 1);
    testint8 = 100*sin(w1);
    testint16 = 100*sin(w2 + ppi/4);
    testint32 = 100*sin(w3 - ppi/4);
    testfloat = atan(testint8)*30;

    digitalWrite(13, (testint8 > 124));

    // Run exposer
    exposer->update();

    // Write something in serial monitor
    if (now>next){
        Serial.print("uint8:");Serial.println(testuint8);
        Serial.print("uint16:");Serial.println(testuint16);
        Serial.print("uint32:");Serial.println(testuint32);
        Serial.print("int8:");Serial.println(testint8);
        Serial.print("int16:");Serial.println(testint16);
        Serial.print("int32:");Serial.println(testint32);
        Serial.print("float:");Serial.println(testfloat);
        Serial.print("string:");Serial.println(teststring);

        // Write something in 1s
        next = millis()+1000;
    }

}
