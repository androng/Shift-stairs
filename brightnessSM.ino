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
        /* Increase brightness of lights. This for loop goes from -numLEDs to 0 or 
           0 to numLEDs depending on the direction of propegration. */
        char startLight = -1 * (directionTriggered - 1) * (numLEDs - 1);
        char endLight   = -1 * (directionTriggered - 2) * (numLEDs - 1);
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
        /* Decrease brightness of lights. This for loop goes from -numLEDs to 0 or 
           0 to numLEDs depending on the direction of propegration. */
        char startLight = -1 * (directionTriggered - 1) * (numLEDs - 1);
        char endLight   = -1 * (directionTriggered - 2) * (numLEDs - 1);
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
//    Serial.print("state: "); Serial.println(brightnessState);
    /* Transitions */
    switch(brightnessState){
    case sFullyOn:
        if(millis() - lastMotionTime > ON_TIME){
            brightnessState = sTurningOff;
        }
        if(switchPressed()){
            transitionToOverrideSwitch();
        }
        break;
    case sOff:
        if(directionTriggered != 0){
            if(analogRead(PHOTORESISTOR_PIN) < LIGHT_THRESHOLD){
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
        for(unsigned char l = 0; l < numLEDs; l++){
            if(brightnesses[l] != maxBrightness){
                allOn = false;
                break;
            }
        }
        if(allOn){
            brightnessState = sFullyOn;
        }
        if(switchPressed()){
            transitionToOverrideSwitch();
        }
        break;
    }
    case sTurningOff:{
        /* If all the lights are off then proceed */
        unsigned char allOff = true;
        for(unsigned char l = 0; l < numLEDs; l++){
            if(brightnesses[l] != 0){
                allOff = false;
                break;
            }
        }
        if(allOff){
            directionTriggered = 0;
            brightnessState = sOff;
        }
        if(switchPressed()){
            transitionToOverrideSwitch();
        }
        break;
    }
    case sOverrideSwitch:
        if(switchPressed() == false){
            /* Switch all LEDs off */
            for(char l = 0; l < numLEDs; l++){
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
    for(unsigned char l = 0; l < numLEDs; l++){
        ShiftPWM.SetOne(l, maxBrightness);
        brightnesses[l] = maxBrightness;
    }        
}

