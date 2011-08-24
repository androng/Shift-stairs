
#include <math.h>
#include <SPI.h>

//Data pin is MOSI (atmega168/328: pin 11. Mega: 51) 
//Clock pin is SCK (atmega168/328: pin 13. Mega: 52)
const int ShiftPWM_latchPin=8;
const bool ShiftPWM_invertOutputs = 0; // if invertOutputs is 1, outputs will be active low. Usefull for common anode RGB led's.

#include <ShiftPWM.h>   // include ShiftPWM.h after setting the pins!

const int SWITCH_PIN = A0;
const int MOTION_SENSOR_TOP_PIN = 2;
const int MOTION_SENSOR_BOTTOM_PIN = 3;

const unsigned char maxBrightness = 255;
const unsigned char pwmFrequency = 75;
const int numRegisters = 2;
const int numLEDs = 12;
const int DELAYMICROS_STEP_LIGHT = 400;
const int MOTION_SENSOR_WARMUP_TIME = 0;
//const int MIN_TIME_BETWEEN_INTERRUPTS = 1000;
const int ON_TIME = 30000; /* The duration between turn on and turn off. */

volatile boolean topActivated = false;
volatile boolean bottomActivated = false; 
//volatile unsigned long topActivatedTime = 0;
//volatile unsigned long bottomActivatedTime = 0; 

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
    
    /* Attach interrupt to top and bottom motion sensors. */
    attachInterrupt(0, topISR, RISING);
    attachInterrupt(1, bottomISR, RISING);
    
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
    if(switchPressed() == false){
        if(topActivated){
            increment(0, 0);
            delayWithOverride();
            increment(0, 1);
        } else if(bottomActivated){
            increment(1, 0);
            delayWithOverride();
            increment(1, 1);
        }
        
        topActivated = false;
        bottomActivated = false;
    }
    
    if(switchPressed()){
        override();
    }
}
/**
    Turns on/off the LEDs by fading them in/out.
    @param boolean descending: 0 for ascending and 1 for descending.
    @param boolean onOff: 0 for on and 1 for off.
*/
void increment(boolean descending, boolean onOff){
    
    /* Cycle through each LED. This very messed up for loop is just a clever way of 
       generating the start and end LEDs for two modes. */
    for(int i = -1 * descending * (numLEDs - 1) ; i <= abs((descending - 1) * (numLEDs - 1)) ; i++){
        int brightness;
        
        if(onOff == 0){
            brightness = 0;
            while(brightness < maxBrightness){
                ShiftPWM.SetOne(abs(i), brightness);
                brightness++;
                delayMicroseconds(DELAYMICROS_STEP_LIGHT);
                
                if(switchPressed()){
                    return;
                }
    	    }
        } 
        else if(onOff == 1){
            brightness = maxBrightness;
            while(brightness >= 0){
                ShiftPWM.SetOne(abs(i), brightness);
                brightness--;
                delayMicroseconds(DELAYMICROS_STEP_LIGHT);
                
                if(switchPressed()){
                    return;
                }
    	    }
        }
    }
}
/** 
    Returns true if switch is in "1" position. 
*/
boolean switchPressed(){
    return !digitalRead(SWITCH_PIN);
}
/**
    Keep lights on until switch is not pressed anymore. This cannot be an ISR because
    ShiftPWM relies on interrupts. Also tried using "shiftout" method but it did not work. 
*/
void override(){
    ShiftPWM.SetAll(maxBrightness);
    while(switchPressed()){
    }
    ShiftPWM.SetAll(0);
    topActivated = false;
    bottomActivated = false;
}
/**
    This is just a delay that stops if the switch is pressed. 
*/
void delayWithOverride(){
    for(int n = 0; n < ON_TIME / 10; n++){
        delay(10);
        
        if(switchPressed()){
            break;
        }
    }
}
void topISR(){
    if(millis() > MOTION_SENSOR_WARMUP_TIME && topActivated == false){
        Serial.println("Top activated");
        topActivated = true;
//        topActivatedTime = millis();
    }
}
void bottomISR(){
    if(millis() > MOTION_SENSOR_WARMUP_TIME && bottomActivated == false){
        bottomActivated = true;
//        bottomActivatedTime = millis();
    }
}
