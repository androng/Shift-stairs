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
    const uint8_t MAX_BRIGHTNESS = EXPONENTIAL_DUTY_CYCLES[lastDial2Value >> 2]; /* 1023 >> 2 = 255 */
        
    /* Actions */
    switch(brightnessState){
    case sFullyOn:
        for(char l = 0; l < numLEDs; l++){
            ShiftPWM.SetOne(l, MAX_BRIGHTNESS);
        }
        
        
        break;
    case sOff:
        /* End steps logic */
        if(digitalRead(END_STEPS_ALWAYS_ON_SWITCH) && analogRead(PHOTORESISTOR_PIN) < LIGHT_THRESHOLD){
            // TODO: Add logic 
            /* Put first and last steps at half MAX_BRIGHTNESS */
            
        } 
            
        for(char l = 0; l < numLEDs; l++){
            ShiftPWM.SetOne(l, 0);
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
                }
            }
            /* If fading is for the future */
            else if(CURRENT_TIME < stairsTurnOnTimes[l] ){
                /* Set to zero brightess */
                stepBrightness = 0;
                
                if(brightnessState == sTurningOff) {
                    stepBrightness = MAX_BRIGHTNESS;
                }
            }
            else {
                /* Set to an in-between brightness */
                stepBrightness = (CURRENT_TIME - stairsTurnOnTimes[l]) * MAX_BRIGHTNESS /FADE_DURATION;
                Serial.print("fade: ");
                Serial.println(FADE_DURATION);
            
                if(brightnessState == sTurningOff) {
                    stepBrightness = MAX_BRIGHTNESS - stepBrightness;
                }
            }
            ShiftPWM.SetOne(l, stepBrightness);
            
        }
        break;
    
    }
    case sOverrideSwitch:
        break;
    }
    Serial.print("state: "); Serial.println(brightnessState);
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
