enum brightnessStates {
    sFullyOn,
    sOff,
    sTurningOn,
    sTurningOff,
    sOverrideSwitch
};

int brightnessState = sOff;

void brightnessSM(){
    /* Actions */
    switch(brightnessState){
    case sFullyOn:
        break;
    case sOff:
        break;
    case sTurningOn:{
        /* Increase brightness of lights. This for loop goes from -NUMLEDs to 0 or 
           0 to NUMLEDs depending on the direction of propegration. */
        char startLight = -1 * (directionTriggered - 1) * (NUMLEDs - 1);
        char endLight   = -1 * (directionTriggered - 2) * (NUMLEDs - 1);
        for(char l = startLight; l <= endLight; l++){
            
            /* Turn on the next LED only if the ones before it 
               are on*/
            if(brightnesses[abs(l)] != maxBrightness){
                brightnesses[abs(l)] += 5;
                ShiftPWM.SetOne(abs(l), brightnesses[abs(l)]);
                break;
            }

            /* Turn on the next LED only if the one before it is partially on */
//            if(brightnesses[abs(l)] != maxBrightness){ 
//                if(l == startLight || (brightnesses[abs(l - 1)] > maxBrightness*3/10)){
//                    brightnesses[abs(l)] += 1;
//                    ShiftPWM.SetOne(abs(l), expoDutyCycles[brightnesses[abs(l)]]);
//                }
//            }
        }
        break;
    }
    case sTurningOff:{
        /* Decrease brightness of lights. This for loop goes from -NUMLEDs to 0 or 
           0 to NUMLEDs depending on the direction of propegration. */
        char startLight = -1 * (directionTriggered - 1) * (NUMLEDs - 1);
        char endLight   = -1 * (directionTriggered - 2) * (NUMLEDs - 1);
        for(char l = startLight; l <= endLight; l++){
            /* Turn on the next LED only if the ones before it 
               are on*/
            if(brightnesses[abs(l)] != 0){
                brightnesses[abs(l)] -= 5;
                ShiftPWM.SetOne(abs(l), brightnesses[abs(l)]);
                break;
            }
            
            /* Turn on the next LED only if the one before it is partially on */
//            if(brightnesses[abs(l)] != 0){ 
//                if(l == startLight || (brightnesses[abs(l - 1)] < maxBrightness*9/10)){
//                    brightnesses[abs(l)] -= 1;
//                    ShiftPWM.SetOne(abs(l), expoDutyCycles[brightnesses[abs(l)]]);
//                }
//            }
        }
        break;
    }
    case sOverrideSwitch:
        break;
    }
    
    /* Transitions */
    switch(brightnessState){
    case sFullyOn:
        if(millis() - lastMotionTime > ON_TIME){
            brightnessState = sTurningOff;
//            Serial.println(brightnessState);
        }
        if(switchPressed()){
            transitionToOverrideSwitch();
        }
        break;
    case sOff:
        if(directionTriggered != 0){
            if(analogRead(PHOTORESISTOR_PIN) > LIGHT_THRESHOLD){
                brightnessState = sTurningOn;
            } 
            else {
                directionTriggered = 0;
            }
        }
        if(switchPressed()){
            transitionToOverrideSwitch();
        }
        break;
    case sTurningOn:{
        /* If all the lights are on then proceed */
        unsigned char allOn = true;
        for(unsigned char l = 0; l < NUMLEDs; l++){
            if(brightnesses[l] != maxBrightness){
                allOn = false;
                break;
            }
        }
        if(allOn){
            brightnessState = sFullyOn;
//            Serial.println(brightnessState);
        }
        if(switchPressed()){
            transitionToOverrideSwitch();
        }
        break;
    }
    case sTurningOff:{
        /* If all the lights are off then proceed */
        unsigned char allOff = true;
        for(unsigned char l = 0; l < NUMLEDs; l++){
            if(brightnesses[l] != 0){
                allOff = false;
                break;
            }
        }
        if(allOff){
            directionTriggered = 0;
            brightnessState = sOff;
//            Serial.println(brightnessState);
        }
        if(switchPressed()){
            transitionToOverrideSwitch();
        }
        break;
    }
    case sOverrideSwitch:
        if(switchPressed() == false){
            /* Switch all LEDs off */
            for(char l = 0; l < NUMLEDs; l++){
                ShiftPWM.SetOne(l, 0);
                brightnesses[l] = 0;
            }
            directionTriggered = 0;
            brightnessState = sOff;
        }
        break;
    }
}
void transitionToOverrideSwitch(){
    brightnessState = sOverrideSwitch;
    /* Switch all LEDs on */
    for(unsigned char l = 0; l < NUMLEDs; l++){
        ShiftPWM.SetOne(l, maxBrightness);
        brightnesses[l] = maxBrightness;
    }        
}

