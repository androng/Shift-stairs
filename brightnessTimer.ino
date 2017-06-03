
long stairsTurnOnTimes[MAX_LEDs] = {0};

/*
    Reads from the three dials and generates the times that the lights will turn on and off. 
    
    @param upOrDown = BOTTOM_TO_TOP or TOP_TO_BOTTOM
*/
void generateLEDTurnOnTimesAndDurations(boolean upOrDown){
    const long MAX_ADC_VALUE = 1023;
    const long MAX_PROPEGATE_DELAY_MILLISEC = 3000;
    
    const int PROPEGATE_DELAY = lastDial3Value * MAX_PROPEGATE_DELAY_MILLISEC / MAX_ADC_VALUE;
    
    /* Increase brightness of lights. This for loop goes from -numLEDs to 0 or 
           0 to numLEDs depending on the direction of propegration. */
    char startLight = -1 * (upOrDown - 1) * (numLEDs - 1);
    char endLight   = -1 * (upOrDown - 2) * (numLEDs - 1);
    char loop_iter = 0;
    const long CURRENT_TIME = millis();
    for(char l = startLight; l <= endLight; l++){
        stairsTurnOnTimes[abs(l)] = CURRENT_TIME + PROPEGATE_DELAY * loop_iter;
        
        Serial.print(stairsTurnOnTimes[abs(l)]);
        Serial.print(", ");
        Serial.println(stairsTurnOffTimes[abs(l)]);
        
        loop_iter++;
    }    
    Serial.println();
}

void generateLEDBrightnessesBasedOnCurrentTime(){
   
    const long MAX_FADE_DURATION_MILLISEC = 3000;
        
    const int FADE_DURATION = lastDial1Value * MAX_FADE_DURATION_MILLISEC / MAX_ADC_VALUE;
    const int MAX_BRIGHTNESS = lastDial2Value >> 2; /* 1024/4 = 256 */
    
    const long CURRENT_TIME = millis();
    
    
}
