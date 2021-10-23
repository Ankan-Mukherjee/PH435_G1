
/*------------
MIDI_PsychoGalvanometer v021
Accepts pulse inputs from a Galvanic Conductance sensor 
consisting of a 555 timer set as an astablemultivibrator and two electrodes. 
Through sampling pulse widths and identifying fluctuations, MIDI note and control messages 
are generated.  Features include Threshold, Scaling, Control Number, and Control Voltage 
using PWM through an RC Low Pass filter.
MIDIsprout.com
-------------*/

#include <EEPROMex.h> //store and read variables to nonvolitle memory
#include <Bounce2.h> //https://github.com/thomasfredericks/Bounce-Arduino-Wiring
#include <LEDFader.h> //manage LEDs without delay() jgillick/arduino-LEDFader https://github.com/jgillick/arduino-LEDFader.git
int maxBrightness = 190;

//******************************
//set scaled values, sorted array, first element scale length
const int scaleCount = 5;
const int scaleLen = 13; //maximum scale length plus 1 for 'used length'
int currScale = 0; //current scale, default Chrom
int scale[scaleCount][scaleLen] = {
  {12,1,2,3,4,5,6,7,8,9,10,11,12}, //Chromatic
  {7,1, 3, 5, 6, 8, 10, 12}, //Major
  {7,1, 3, 4, 6, 8, 9, 11}, //DiaMinor
  {7,1, 2, 2, 5, 6, 9, 11}, //Indian
  {7,1, 3, 4, 6, 8, 9, 11} //Minor
};

int root = 0; //initialize for root, pitch shifting
//*******************************

const byte interruptPin = INT0; //galvanometer input
const byte knobPin = A0; //knob analog input
Bounce button = Bounce(); //debounce button using Bounce2
const byte buttonPin = A1; //tact button input
int menus = 5; //number of main menus
int mode = 0; //0 = Threshold, 1 = Scale, 2 = Brightness
int currMenu = 0;


const byte samplesize = 10; //set sample array size
const byte analysize = samplesize - 1;  //trim for analysis array

const byte polyphony = 5; //above 8 notes may run out of ram
int channel = 1;  //setting channel to 11 or 12 often helps simply computer midi routing setups
int noteMin = 36; //C2  - keyboard note minimum
int noteMax = 96; //C7  - keyboard note maximum
byte QY8= 0;  //sends each note out chan 1-4, for use with General MIDI like Yamaha QY8 sequencer
byte controlNumber = 80; //set to mappable control, low values may interfere with other soft synth controls!!
byte controlVoltage = 1; //output PWM CV on controlLED, pin 17, PB3, digital 11 *lowpass filter
long batteryLimit = 3000; //voltage check minimum, 3.0~2.7V under load; causes lightshow to turn off (save power)
byte checkBat = 1;

byte timeout = 0;
int value = 0;
int prevValue = 0;

volatile unsigned long microseconds; //sampling timer
volatile byte index = 0;
volatile unsigned long samples[samplesize];

float threshold = 1.7;   //2.3;  //change threshold multiplier
float threshMin = 1.61; //scaling threshold min
float threshMax = 3.71; //scaling threshold max
float knobMin = 1;
float knobMax = 1024;

unsigned long previousMillis = 0;
unsigned long currentMillis = 1;
unsigned long batteryCheck = 0; //battery check delay timer
unsigned long menuTimeout = 5000; //5 seconds timeout in menu mode
 
#define LED_NUM 6
LEDFader leds[LED_NUM] = { // 6 LEDs (perhaps 2 RGB LEDs)
  LEDFader(3),
  LEDFader(5),
  LEDFader(6),
  LEDFader(9),
  LEDFader(10),
  LEDFader(11)  //Control Voltage output or controlLED
};
int ledNums[LED_NUM] = {3,5,6,9,10,11};
byte controlLED = 5; //array index of control LED (CV out)
byte noteLEDs = 1;  //performs lightshow set at noteOn event

typedef struct _MIDImessage { //build structure for Note and Control MIDImessages
  unsigned int type;
  int value;
  int velocity;
  long duration;
  long period;
  int channel;
} 
MIDImessage;
MIDImessage noteArray[polyphony]; //manage MIDImessage data as an array with size polyphony
int noteIndex = 0;
MIDImessage controlMessage; //manage MIDImessage data for Control Message (CV out)


void setup()
{
  pinMode(knobPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  button.attach(buttonPin);
  button.interval(5);
  
  randomSeed(analogRead(0)); //seed for QY8 4 channel mode
  Serial.begin(31250);  //initialize at MIDI rate
  //Serial.begin(9600); //for debugging 
  
  controlMessage.value = 0;  //begin CV at 0
  //MIDIpanic(); //dont panic, unless you are sure it is nessisary
  checkBattery(); // shut off lightshow if power is too low
  if(noteLEDs) bootLightshow(); //a light show to display on system boot
  attachInterrupt(interruptPin, sample, RISING);  //begin sampling from interrupt
  
}

void loop()
{
  currentMillis = millis();   //manage time
  //checkBattery(); //on low power, shutoff lightShow, continue MIDI operation
  
  checkButton();  //its about to get really funky in here

  if(index >= samplesize)  { analyzeSample(); }  //if samples array full, also checked in analyzeSample(), call sample analysis   
  checkNote();  //turn off expired notes 
  checkControl();  //update control value
  checkLED();  //LED management without delay()

  if(currMenu>0) checkMenu(); //allow main loop by checking current menu mode, and updating millis

}





