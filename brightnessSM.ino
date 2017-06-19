enum brightnessStates {
    sFullyOn,
    sOff,
    sTurningOn,
    sTurningOff,
    sOverrideSwitch
};

int brightnessState = sOff;

void brightnessSM(){
    const uint16_t FADE_DURATION = lastDial1Value * MAX_FADE_DURATION_MILLISEC / MAX_ADC_VALUE;
    const uint8_t MAX_BRIGHTNESS = EXPONENTIAL_DUTY_CYCLES[(MAX_ADC_VALUE - lastDial2Value) >> 2]; /* 1023 >> 2 = 255 */
    /* Put first and last steps at half MAX_BRIGHTNESS */
    const uint8_t DIM_BRIGHTNESS = MAX_BRIGHTNESS/16; //For the end steps when the "end steps always on" setting enabled
    const uint8_t MINIMUM_BRIGHTNESS = 1;
                
    /* Actions */
    switch(brightnessState){
    case sFullyOn:
        for(char l = 0; l < numLEDs; l++){
            ShiftPWM.SetOne(l, MAX_BRIGHTNESS);
        }
        
        
        break;
    case sOff:
        
        /* End steps logic */
        if(endStepsAlwaysOnSwitchedOn() && analogRead(PHOTORESISTOR_PIN) < LIGHT_THRESHOLD){
            
            /* End step */
            ShiftPWM.SetOne(0, max(MINIMUM_BRIGHTNESS, DIM_BRIGHTNESS));
            /* Inbetween steps */
            for(char l = 1; l < numLEDs - 1; l++){
                ShiftPWM.SetOne(l, 0);
            }
            /* End step */
            ShiftPWM.SetOne(numLEDs-1, max(MINIMUM_BRIGHTNESS, DIM_BRIGHTNESS));
            
        }  
        else {
            for(char l = 0; l < numLEDs; l++){
                ShiftPWM.SetOne(l, 0);
            }
        }
            
        
        break;
    case sTurningOn:
    case sTurningOff:{
        
        const unsigned long CURRENT_TIME = millis();
        
        unsigned long stepBrightness;
            
        for(char l = 0; l < numLEDs; l++){
            
          
            /* If fading is done */
            if(CURRENT_TIME > stairsTurnOnTimes[l] + FADE_DURATION){
                /* Set to MAX brightess */
                stepBrightness = MAX_BRIGHTNESS;
                
                if(brightnessState == sTurningOff) {
                    stepBrightness = 0;
                    
                    /* If "end steps always on" enabled AND (first or last step) */
                    if(endStepsAlwaysOnSwitchedOn() && (l == 0 || l == numLEDs - 1)){
                        stepBrightness = max(MINIMUM_BRIGHTNESS, DIM_BRIGHTNESS);
                    }
                }
            }
            /* If fading is for the future */
            else if(CURRENT_TIME < stairsTurnOnTimes[l] ){
                /* Set to zero brightess */
                stepBrightness = 0;

                /* If "end steps always on" enabled AND (first or last step) */
                if(endStepsAlwaysOnSwitchedOn() && (l == 0 || l == numLEDs - 1)){
                    stepBrightness = max(MINIMUM_BRIGHTNESS, DIM_BRIGHTNESS);
                }
                
                if(brightnessState == sTurningOff) {
                    stepBrightness = MAX_BRIGHTNESS;
                }
            }
            else {
                /* Set to an in-between brightness */
                stepBrightness = (CURRENT_TIME - stairsTurnOnTimes[l]) * MAX_BRIGHTNESS /FADE_DURATION;
            
                /* If "end steps always on" enabled AND (first or last step) */
                if(endStepsAlwaysOnSwitchedOn() && (l == 0 || l == numLEDs - 1)){
                    stepBrightness = max(stepBrightness, DIM_BRIGHTNESS);
                    stepBrightness = max(stepBrightness, MINIMUM_BRIGHTNESS);
                }

            
                if(brightnessState == sTurningOff) {
                    stepBrightness = MAX_BRIGHTNESS - stepBrightness;
                    
                    /* If "end steps always on" enabled AND (first or last step) */
                    if(endStepsAlwaysOnSwitchedOn() && (l == 0 || l == numLEDs - 1)){
                        stepBrightness = max(stepBrightness, DIM_BRIGHTNESS);
                        stepBrightness = max(stepBrightness, MINIMUM_BRIGHTNESS);
                    }

                }
            }
            ShiftPWM.SetOne(l, stepBrightness);
            
        }
        break;
    
    }
    case sOverrideSwitch:
        break;
    }
//    Serial.print("state: "); Serial.println(brightnessState);
    /* Transitions */
    switch(brightnessState){
    case sFullyOn:
        if(millis() - lastMotionTime > ON_TIME){
            generateLEDTurnOnTimesAndDurations();
            brightnessState = sTurningOff;
        }
        if(switchPressed()){
            brightnessState = sOverrideSwitch;
        }
        
        break;
    case sOff:
        if(directionTriggered != 0){
            if(analogRead(PHOTORESISTOR_PIN) < LIGHT_THRESHOLD){
                generateLEDTurnOnTimesAndDurations();
                dialsChangedSoRestartAnimation = 0;
                brightnessState = sTurningOn;
            } 
            else {
                directionTriggered = 0;
            }
        }
        if(switchPressed()){
            brightnessState = sOverrideSwitch;
        }
        break;
    case sTurningOn:{
        const unsigned long CURRENT_TIME = millis();
          
        /* If any of the lights are NOT fully on, then DON'T proceed */
        unsigned char allOn = true;
        for(unsigned char l = 0; l < numLEDs; l++){
            if(CURRENT_TIME < stairsTurnOnTimes[l] + FADE_DURATION){
                allOn = false;
                break;
            }
        }
        if(allOn){
            brightnessState = sFullyOn;
        }
        if(switchPressed()){
            brightnessState = sOverrideSwitch;
        }
        break;
    }
    case sTurningOff:{
        const unsigned long CURRENT_TIME = millis();
          
        /* If any of the lights are NOT fully off, then DON'T proceed */
        unsigned char allOff = true;
        for(unsigned char l = 0; l < numLEDs; l++){
            if(CURRENT_TIME < stairsTurnOnTimes[l] + FADE_DURATION){
                allOff = false;
                break;
            }
        }
        if(allOff){
            directionTriggered = 0;
            brightnessState = sOff;
        }
        if(switchPressed()){
            brightnessState = sOverrideSwitch;
        }
        
        
        break;
    }
    case sOverrideSwitch:
        if(switchPressed() == false){
            /* Switch all LEDs off */
            for(char l = 0; l < numLEDs; l++){
                ShiftPWM.SetOne(l, 0);
            }
            directionTriggered = 0;
            brightnessState = sOff;
        }
        else {
            
            /* Switch all LEDs on */
            for(unsigned char l = 0; l < numLEDs; l++){
                ShiftPWM.SetOne(l, MAX_BRIGHTNESS);
            }    
        }
        break;
    }
    /* Applies to ALL states*/
    checkForDialsChanged();
}

void checkForDialsChanged(){
    if(dialsChangedSoRestartAnimation){
        directionTriggered = TOP_TO_BOTTOM;
        generateLEDTurnOnTimesAndDurations();
        brightnessState = sTurningOn;
        dialsChangedSoRestartAnimation = 0;
        
        /* Make the delay before turning off slighly less than normal  */
        lastMotionTime = millis() - ON_TIME/2;
    }

        
}
