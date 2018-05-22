
#include <math.h>
#include <SPI.h>
//#include <MemoryFree.h> 
#include "expoDutyCycles.h"

//Data pin is MOSI (atmega168/328: pin 11. Mega: 51) 
//Clock pin is SCK (atmega168/328: pin 13. Mega: 52)
const int ShiftPWM_latchPin=8;
const bool ShiftPWM_invertOutputs = 0; // if invertOutputs is 1, outputs will be active low. Usefull for common anode RGB led's.

#include <ShiftPWM.h>   // include ShiftPWM.h after setting the pins!

const uint8_t MAX_LEDs = 16;

const long MAX_ADC_VALUE = 1023;

const int SWITCH0_PIN = 0;
const int SWITCH1_PIN = 1;
const int END_STEPS_ALWAYS_ON_SWITCH = 11;
const int PHOTORESISTOR_PIN = 5;
const int POTENTIOMETER_PIN_FADE_SPEED = 2;
const int POTENTIOMETER_PIN_MAX_BRIGHTNESS = 4;
const int POTENTIOMETER_PIN_PROPEGATE_SPEED = 3;
const int MOTION_SENSOR_TOP_PIN = 5;
const int MOTION_SENSOR_BOTTOM_PIN = 13;
const int NUM_STAIRS_SWITCH2 = 4;
/******** NUM_STAIRS_SWITCH0 =  NOT MAPPED in digitalWrite */
const int NUM_STAIRS_SWITCH3 = 12;
const int NUM_STAIRS_SWITCH1 = 6;
const unsigned long MAX_FADE_DURATION_MILLISEC = 3000;
        
const unsigned char HIGHEST_POSSIBLE_BRIGHTNESS = 255;
const unsigned char SHIFTPWM_PWMFREQUENCY = 75;
const int numRegisters = 2;
const int MOTION_SENSOR_WARMUP_TIME = 10;
const int ON_TIME = 10000; /* The duration between turn on and turn off. */
const int LIGHT_THRESHOLD = 300; /* Anything below this sensor value will enable lights */

/* Parameters set by the PCB switches and pots on startup */
int numLEDs = 9;


/* These are used to detect rising edges in the absence of interrupts. 
   Using interrupts with ShiftPWM crashes the program. */
unsigned char lastReadTopPin = LOW;
unsigned char lastReadBotPin = LOW;

volatile unsigned char topActivated = false;
volatile unsigned char bottomActivated = false; 
unsigned long lastMotionTime = 0; 

const char TOP_TO_BOTTOM = 1;
const char BOTTOM_TO_TOP = 2;
/* For sake of the animation, stores the direction of propegation.
   Set when animation is active, cleared when animation is done.  */
char directionTriggered = 0; 

const unsigned long BRIGHTNESS_SM_PERIOD = 2000; /* in μs */
unsigned long lastBrightnessSM = 0;

const unsigned long CHECK_DIALS_PERIOD = 50; 
unsigned long lastCheckDials = 0;

const unsigned long CHECK_NUM_STEPS_PERIOD = 100; 
unsigned long lastCheckNumSteps = 0;

/* LED 0 is on the top of stairs */
unsigned long stairsTurnOnTimes[MAX_LEDs] = {0};

/* Flag for when a switch or dial is moved */
uint8_t dialsChangedSoRestartAnimation = 0;

void setup()   {                
    pinMode(ShiftPWM_latchPin, OUTPUT);  
    SPI.setBitOrder(LSBFIRST);
    // SPI_CLOCK_DIV2 is only a tiny bit faster in sending out the last byte. 
    // SPI transfer and calculations overlap for the other bytes.
    SPI.setClockDivider(SPI_CLOCK_DIV4); 
    SPI.begin(); 
  
    Serial.begin(9600);
    
   
    /* Turn on pullup resistors */
    digitalWrite(MOTION_SENSOR_TOP_PIN, HIGH);
    digitalWrite(MOTION_SENSOR_BOTTOM_PIN, HIGH);
    digitalWrite(SWITCH0_PIN, HIGH);
    digitalWrite(SWITCH1_PIN, HIGH);
    PORTD |= (1<<5); /* stupid digitalWrite doesn't have this pin mapped */
    digitalWrite(NUM_STAIRS_SWITCH1, HIGH);
    digitalWrite(NUM_STAIRS_SWITCH2, HIGH);
    digitalWrite(NUM_STAIRS_SWITCH3, HIGH);
    digitalWrite(END_STEPS_ALWAYS_ON_SWITCH, HIGH);
    
    delay(1);
    /* Read the PCB hardware and configure parameters */
    numLEDs = readHexSwitch();
    if(numLEDs == 0){
        numLEDs = MAX_LEDs;  
    }
    speedDialsChanged();
    
    ShiftPWM.SetAmountOfRegisters(numRegisters);
    ShiftPWM.Start(SHIFTPWM_PWMFREQUENCY,255);  
    // Print information about the interrupt frequency, duration and load on your program
    ShiftPWM.SetAll(0);
    ShiftPWM.PrintInterruptLoad();
    
    
    // Fade in all outputs
    for(int j=0;j<HIGHEST_POSSIBLE_BRIGHTNESS;j++){
        ShiftPWM.SetAll(j);  
        delay(3);
    }
    // Fade out all outputs
    for(int j=HIGHEST_POSSIBLE_BRIGHTNESS;j>=0;j--){
        ShiftPWM.SetAll(j);  
        delay(3);
    }
}
void loop()
{    
//    Serial.print("port ");
//    Serial.print((uint16_t)port_to_output_PGM_ct[digital_pin_to_port_PGM_ct[NUM_STAIRS_SWITCH1]]);
//    Serial.print(" pin ");
//    Serial.println(digital_pin_to_bit_PGM_ct[NUM_STAIRS_SWITCH1]);
//
//    Serial.print("port ");
//    Serial.print((uint16_t)&PORTD);
//    Serial.println(" pin 5 Expected");
    
    
     /* Detect falling edge with polling. Interrupts crash the program. */
    unsigned char pinRead = digitalRead(MOTION_SENSOR_TOP_PIN);
    if(lastReadTopPin == HIGH && pinRead == LOW){
        topActivated = true;
    }
    lastReadTopPin = pinRead;
    /* Detect rising edge with polling. Interrupts crash the program. */
    pinRead = digitalRead(MOTION_SENSOR_BOTTOM_PIN);
    if(lastReadBotPin == HIGH && pinRead == LOW){
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
    
    if(millis() - lastCheckDials > CHECK_DIALS_PERIOD){
        if(speedDialsChanged() != 0){
            dialsChangedSoRestartAnimation = 1;
        }
        
        lastCheckDials = millis();        
    }

    if(millis() - lastCheckNumSteps > CHECK_NUM_STEPS_PERIOD){
        numLEDs = readHexSwitch();
        if(numLEDs == 0){
            numLEDs = MAX_LEDs;  
        }
        
        lastCheckNumSteps = millis();        
    }    
}
/** 
    Returns true if switchs are in different positions. 
*/
boolean switchPressed(){
    return (digitalRead(SWITCH0_PIN) ^ digitalRead(SWITCH1_PIN));
}
/** Reads the 17-position switch */
uint8_t readHexSwitch(){
    uint8_t switchPosition = 0;
    switchPosition |= (!digitalRead(NUM_STAIRS_SWITCH3)) << 3;
    switchPosition |= (!digitalRead(NUM_STAIRS_SWITCH2)) << 2;
    switchPosition |= (!digitalRead(NUM_STAIRS_SWITCH1)) << 1;
    switchPosition |= (!(PIND & (1 << 5)));
    
    return switchPosition;
}

int lastDial1Value = analogRead(POTENTIOMETER_PIN_FADE_SPEED);
int lastDial2Value = analogRead(POTENTIOMETER_PIN_MAX_BRIGHTNESS);
int lastDial3Value = analogRead(POTENTIOMETER_PIN_PROPEGATE_SPEED);

/* Returns true if the positions of the SPEED dials are different from what they were
   at the last call of this function. 
   Also saves the values of all three dials. 
 */
uint8_t speedDialsChanged(){
    const int CHANGE_BY_AT_LEAST_THIS_MANY_ADC_UNITS = 15;
    
    int newDial1Value = analogRead(POTENTIOMETER_PIN_FADE_SPEED);
    int newDial2Value = analogRead(POTENTIOMETER_PIN_MAX_BRIGHTNESS);
    int newDial3Value = analogRead(POTENTIOMETER_PIN_PROPEGATE_SPEED);

    boolean dialsHaveChanged = 0; 
    
    if(newDial1Value - lastDial1Value >= CHANGE_BY_AT_LEAST_THIS_MANY_ADC_UNITS ||
        newDial1Value - lastDial1Value <= -CHANGE_BY_AT_LEAST_THIS_MANY_ADC_UNITS ||
//        newDial2Value - lastDial2Value >= CHANGE_BY_AT_LEAST_THIS_MANY_ADC_UNITS ||      //This is commented because I did not want
//        newDial2Value - lastDial2Value <= -CHANGE_BY_AT_LEAST_THIS_MANY_ADC_UNITS ||     //the brightness dial to restart the stairs animation
        newDial3Value - lastDial3Value >= CHANGE_BY_AT_LEAST_THIS_MANY_ADC_UNITS ||
        newDial3Value - lastDial3Value <= -CHANGE_BY_AT_LEAST_THIS_MANY_ADC_UNITS){
        
        dialsHaveChanged = 1;      
    }
    
    
    lastDial1Value = newDial1Value;
    lastDial2Value = newDial2Value;
    lastDial3Value = newDial3Value;

    return dialsHaveChanged;
}

uint8_t endStepsAlwaysOnSwitchedOn(){
    return !digitalRead(END_STEPS_ALWAYS_ON_SWITCH);
}
