

void knobMode() {
  //scroll through menus and select values using only a single knob
  //keep dreamin' kid,
}

void rampUp(int ledPin, int value, int time) {
LEDFader *led = &leds[ledPin];
//scale the value parameter against a new maxBrightness global variable
//  led->fade(value, time);  
led->fade(map(value,0,255,0,maxBrightness), time); 
}

void rampDown(int ledPin, int value, int time) {     
  LEDFader *led = &leds[ledPin];
 // led->set_value(255); //turn on
  led->fade(value, time); //fade out
}

void checkLED(){
//iterate through LED array and call update  
 for (byte i = 0; i < LED_NUM; i++) {
    LEDFader *led = &leds[i];
    led->update();    
 }
}


void checkButton() {
  button.update();
//read button, debounce if possible, and set menuMode
  if(button.fell()) { //on button release
    noteLEDs = 0; //turn off normal light show for menu modes
    for(byte j=0;j<LED_NUM;j++) { leds[j].stop_fade(); leds[j].set_value(0); } //off LEDs

          switch(currMenu) {
          case 0: //this is the main sprout program
            prevValue = 0;
            currMenu = 1;
            previousMillis = currentMillis;
            break;
          case 1: //this is the main selection menu
            //value is menu selected
            switch(value) {
              case 0:
                thresholdMode(); //set change threshold multiplier
                return; 
                break;
              case 1:
                scaleMode(); //set note scale
                return;
                break;
              case 2:
                channelMode(); //set MIDI output channel
                return;
                break;
              case 3:
                brightnessMode(); //set LED max brightness
                return;
                break;
              case 4:

                return;
                break;
              case 5:

                return;
                break;
              default:
                break;
            }
            break;
          default:
            break;
        }
  }
}

void checkMenu(){
    //read the knob value, update LEDs, and wait for button press
      value = analogRead(knobPin);

      //scale knob value against number of menus
      value = map(value, knobMin, knobMax, 0, menus); //value is now menu index

      //set LEDs to flash based on value
      if(value != prevValue) { //a change in value
        //clear out prevValue LED
        leds[prevValue].stop_fade();
        leds[prevValue].set_value(0);
        prevValue = value; //store value
        previousMillis = currentMillis; //reset timeout

      } 
      else { //no value change
      }
        switch(currMenu) {
          case 0:
        //this is the main sprout program
            break;
          case 1: //this is the main selection menu
            noteLEDs = 0;
            pulse(value,maxBrightness,250); //pulse for current menu
            break;
          case 2:  //this is the submenu
            noteLEDs = 0;
            pulse(value,maxBrightness,90); //pulse for current menu        
            break;
        
          default:
            break;
        }

      if((currentMillis - previousMillis) > menuTimeout) { //menu timeout!!
        currMenu = 0; //return to main program exit all menus
        if(maxBrightness > 1) noteLEDs = 1; //normal light show, or leave off
        leds[prevValue].stop_fade();
        leds[prevValue].set_value(0);
      }

}


long readVcc() {  //https://code.google.com/p/tinkerit/wiki/SecretVoltmeter
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

void checkBattery(){
  //check battery voltage against internal 1.1v reference
  //if below the minimum value, turn off the light show to save power
  //don't check on every loop, settle delay in readVcc() slows things down a bit
 if(batteryCheck < currentMillis){
  batteryCheck = currentMillis+10000; //reset for next battery check
   
  if(readVcc() < batteryLimit) {   //if voltage > valueV
    //battery failure  
    if(checkBat) { //first battery failure
      for(byte j=0;j<LED_NUM;j++) { leds[j].stop_fade(); leds[j].set_value(0); }  //reset leds, power savings
      noteLEDs = 0;  //shut off lightshow set at noteOn event, power savings
      checkBat = 0; //update, first battery failure identified
    } else { //not first low battery cycle
      //do nothing, lights off indicates low battery
      //MIDI continues to flow, MIDI data eventually garbles at very low voltages
      //some USB-MIDI interfaces may crash due to garbled data
    } 
  } 
 }
}

void pulse(int ledPin, int maxValue, int time) {
 LEDFader *led = &leds[ledPin];
 //check on the state of the LED and force it to pulse
 if(led->is_fading() == false) {  //if not fading
   if(led->get_value() > 0) { //and is illuminated
     led->fade(0, time); //fade down
   } else led->fade(maxValue, time); //fade up
 }
}

void bootLightshow(){
 //light show to be displayed on boot 
  for (byte i = 5; i > 0; i--) {
    LEDFader *led = &leds[i-1];
//    led->set_value(200); //set to max

    led->fade(200, 150); //fade up
    while(led->is_fading()) checkLED();
   

    led->fade(0,150+i*17);  //fade down
    while(led->is_fading()) checkLED();
   //move to next LED
  }
}


//provide float map function
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//debug SRAM memory size
int freeRAM() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
} // print free RAM at any point


void thresholdMode() {
  int runMode = 1;
  noteLEDs = 2; //turn on special Note visualization for feedback on threshold effect
  while(runMode) {
    //float knobValue 
    threshold = analogRead(knobPin);  
    //set threshold to knobValue mapping
    threshold = mapfloat(threshold, knobMin, knobMax, threshMin, threshMax);
    pulse(value,maxBrightness,90); //pulse for current menu    
    
    checkLED();
    if(index >= samplesize)  { analyzeSample(); }  //keep samples running
    checkNote();  //turn off expired notes 
    checkControl();  //update control value

    button.update();
    if(button.fell()) runMode = 0;
    
    currentMillis = millis();
  }  //after button press retain threshold setting
  currMenu = 0; //return to main program
  noteLEDs = 1; //normal light show
  leds[prevValue].stop_fade();
  leds[prevValue].set_value(0);
}


void scaleMode() {
  int runMode = 1;
  int prevScale = 0;
  while(runMode) {
    currScale = analogRead(knobPin);  
    //set current Scale choice
    currScale = map(currScale, knobMin, knobMax, 0, scaleCount);
    
    pulse(value,maxBrightness,150); //pulse for current menu    
    pulse(currScale,maxBrightness,90); //display selected scale if scaleCount <= 5

    if(currScale != prevScale) { //clear last value if change
      leds[prevScale].stop_fade();
      leds[prevScale].set_value(0);
    }
    prevScale = currScale;
    
    checkLED();
    if(index >= samplesize)  { analyzeSample(); }  //keep samples running
    checkNote();  //turn off expired notes 
    checkControl();  //update control value

    button.update();
    if(button.fell()) runMode = 0;
    
    currentMillis = millis();
  }  //after button press retain threshold setting
  currMenu = 0; //return to main program
  noteLEDs = 1; //normal light show
  leds[prevValue].stop_fade();
  leds[prevValue].set_value(0);
  leds[currScale].stop_fade();
  leds[currScale].set_value(0);
  
}

void channelMode() {
  int runMode = 1;
  while(runMode) {
    channel = analogRead(knobPin);  
    //set current MIDI Channel between 1 and 16
    channel = map(channel, knobMin, knobMax, 1, 17);
    
    pulse(value,maxBrightness,90); //pulse for current menu    
    
    checkLED();
    if(index >= samplesize)  { analyzeSample(); }  //keep samples running
    checkNote();  //turn off expired notes 
    checkControl();  //update control value

    button.update();
    if(button.fell()) runMode = 0;
    
    currentMillis = millis();
  }  //after button press retain threshold setting
  currMenu = 0; //return to main program
  noteLEDs = 1; //normal light show
  leds[prevValue].stop_fade();
  leds[prevValue].set_value(0);
}

void brightnessMode() {
  int runMode = 1;
  while(runMode) {
    maxBrightness = analogRead(knobPin);  
    //set led maxBrightness
    maxBrightness = map(maxBrightness, knobMin, knobMax, 1, 255);

    if(maxBrightness>1) pulse(value,maxBrightness,90); //pulse for current menu    
    else pulse(value,1,50); //fast dim pulse for 0 note lightshow
    
    checkLED();
    if(index >= samplesize)  { analyzeSample(); }  //keep samples running
    checkNote();  //turn off expired notes 
    checkControl();  //update control value

    button.update();
    if(button.fell()) runMode = 0;
    
    currentMillis = millis();
  }  //after button press retain threshold setting
  currMenu = 0; //return to main program
  if(maxBrightness > 1) noteLEDs = 1; //normal light show, unles lowest value
  leds[prevValue].stop_fade();
  leds[prevValue].set_value(0);
}

