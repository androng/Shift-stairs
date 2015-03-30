
#include <math.h>
#include <SPI.h>
//#include <MemoryFree.h> 
//#include "expoDutyCycles.h"

//Data pin is MOSI (atmega168/328: pin 11. Mega: 51) 
//Clock pin is SCK (atmega168/328: pin 13. Mega: 52)
const int ShiftPWM_latchPin=10;
const bool ShiftPWM_invertOutputs = 0; // if invertOutputs is 1, outputs will be active low. Usefull for common anode RGB led's.

#include <ShiftPWM.h>   // include ShiftPWM.h after setting the pins!
#include "expoDutyCycles.h"

const int SWITCH_PIN = A0;
const int PHOTORESISTOR_PIN = A2;
const int MOTION_SENSOR_TOP_PIN = 2;
const int MOTION_SENSOR_BOTTOM_PIN = 3;

const unsigned char maxBrightness = 255;
const unsigned short MAX_BANISTER_BRIGHTNESS = 765;
const unsigned char pwmFrequency = 75;
const int numRegisters = 2;
const int NUMLEDs = 9;
const int MOTION_SENSOR_WARMUP_TIME = 10;
const int ON_TIME = 10000; /* The duration between turn on and turn off. */
const int LIGHT_THRESHOLD = 300; /* Anything below this sensor value will disable lights except override switch. */
const unsigned char BANISTER_PIN_1 = 14;
const unsigned char BANISTER_PIN_2 = 15;

/* These are used to detect rising edges in the absence of interrupts. 
   Using interrupts with ShiftPWM crashes the program. */
unsigned char lastReadTopPin = LOW;
unsigned char lastReadBotPin = LOW;

volatile unsigned char topActivated = false;
volatile unsigned char bottomActivated = false; 
unsigned long lastMotionTime = 0; 

const char BOTTOM_TO_TOP = 1;
const char TOP_TO_BOTTOM = 2;
/* For sake of the animation, stores the direction of propegation.
   Set when animation is active, cleared when animation is done.  */
char directionTriggered = 0; 

const unsigned long BRIGHTNESS_SM_PERIOD = 2000; /* in μs */
unsigned long lastBrightnessSM = 0;

/* LED 0 is on the top of stairs */
unsigned char brightnesses[NUMLEDs] = {0};
short banisterBrightness = 0;

void setup()   {                
    pinMode(ShiftPWM_latchPin, OUTPUT);  
    SPI.setBitOrder(LSBFIRST);
    // SPI_CLOCK_DIV2 is only a tiny bit faster in sending out the last byte. 
    // SPI transfer and calculations overlap for the other bytes.
    SPI.setClockDivider(SPI_CLOCK_DIV4); 
    SPI.begin(); 
  
    Serial.begin(9600);
    
    /* Turn on pullup resistor for switch */
    digitalWrite(SWITCH_PIN, HIGH);
    
    ShiftPWM.SetAmountOfRegisters(numRegisters);
    ShiftPWM.Start(pwmFrequency,maxBrightness);  
    // Print information about the interrupt frequency, duration and load on your program
    ShiftPWM.SetAll(0);
    ShiftPWM.PrintInterruptLoad();
    // Fade in all outputs
    for(int j=0;j<maxBrightness;j++){
        ShiftPWM.SetAll(j);  
        delay(3);
    }
    // Fade out all outputs
    for(int j=maxBrightness;j>=0;j--){
        ShiftPWM.SetAll(j);  
        delay(3);
    }
}
void loop()
{    
    /* Detect rising edge with polling. Interrupts crash the program. */
    unsigned char pinRead = digitalRead(MOTION_SENSOR_TOP_PIN);
    if(pinRead == HIGH && lastReadTopPin == LOW){
        topActivated = true;
    }
    lastReadTopPin = pinRead;
    /* Detect rising edge with polling. Interrupts crash the program. */
    pinRead = digitalRead(MOTION_SENSOR_BOTTOM_PIN);
    if(pinRead == HIGH && lastReadBotPin == LOW){
        bottomActivated = true;
    }
    lastReadBotPin = pinRead;
    
    /* Resets flags */
    if(topActivated){
        if(directionTriggered == 0){
            directionTriggered = TOP_TO_BOTTOM;
        }
        lastMotionTime = millis();
        topActivated = false;
    }
    if(bottomActivated){
        if(directionTriggered == 0){
            directionTriggered = BOTTOM_TO_TOP;
        }
        lastMotionTime = millis();
        bottomActivated = false;
    }
    
    /* State machine */
    if(micros() - lastBrightnessSM > BRIGHTNESS_SM_PERIOD){
        brightnessSM();
        lastBrightnessSM = micros();
    }
}
/** 
    Returns true if switch is in "1" position. 
*/
boolean switchPressed(){
    return !digitalRead(SWITCH_PIN);
}

