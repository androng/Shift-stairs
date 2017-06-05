
    

/*
    Reads from the three dials and generates the times that the lights will turn on and off. 
    
    @param upOrDown = BOTTOM_TO_TOP or TOP_TO_BOTTOM
*/
void generateLEDTurnOnTimesAndDurations(){
    const unsigned long MAX_PROPEGATE_DELAY_MILLISEC = 3000;
    
    /* Generate propegate delay and make the small values take more of the knob than the big values 
        Speed is 1/T so exp(1/T) is more linear */
    const unsigned long PROPEGATE_DELAY = (EXPONENTIAL_DUTY_CYCLES[lastDial3Value >> 2] << 2) * MAX_PROPEGATE_DELAY_MILLISEC / MAX_ADC_VALUE;
    
    /* Increase brightness of lights. This for loop goes from -numLEDs to 0 or 
           0 to numLEDs depending on the direction of propegration. */
    char startLight = -1 * (directionTriggered - 1) * (numLEDs - 1);
    char endLight   = -1 * (directionTriggered - 2) * (numLEDs - 1);
    char loop_iter = 0;
    const unsigned long CURRENT_TIME = millis();
    for(char l = startLight; l <= endLight; l++){
        stairsTurnOnTimes[abs(l)] = CURRENT_TIME + PROPEGATE_DELAY * loop_iter;
        

        Serial.println(stairsTurnOnTimes[abs(l)]);
        
        loop_iter++;
    }    
    Serial.println();
}

