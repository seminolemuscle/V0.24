/*
  Interrup Based Encoder LED Blinking
 
 Turns on and off a light emitting diode(LED) connected to digital  
 pin 13, when an encoder sends a rising edge to digital input pin 2.
 
 
 The circuit:
 * LED attached from pin 13 to ground 
 * encoder attached to pin 2
 * 10K resistor attached to pin 2 from 5v+
 
 * Note: on most Arduinos there is already an LED on the board
 attached to pin 13.
 
 
 created 2015
 Jordan Berke
 */

// constants won't change. They're used here to 
// set pin numbers:
#include <SPI.h>
#include <Wire.h>
//#include <LiquidCrystal_I2C.h>  //Nate comments
#include <Adafruit_GFX.h>         //Nate addition
#include <Adafruit_SSD1306.h>     //Nate addition

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

const int ledPin =  13;      // the number of the LED pin
const int buttonPinRight =  14;      // the number of the LED pin
const int buttonPinLeft = 15;
long ticLength = 2950; //in micrometers
volatile int state = LOW;
volatile int goingUpward = HIGH;
int isGoingUpwardLast = 0;
int currentStateTemp = 0;
long startheight = 0;
long sumVelocities = 0;
long avgVelocity = 0;
unsigned long starttime = 0;
int tics = 0;
int rep = 0;
int repDone = 0;
int repDoneLast = 0;
int repDisplay = 0;
int repDisplayLast = 0;
long displacement = 0;
long lastDisplacement = 0;
unsigned long tic_time = 0;
unsigned long tic_time2 = 0;
unsigned long displayTime = 0;
const unsigned long backlightTime = 10000000;
int i = 0;
int myVelocities[150] = {0};
int initialized = 0;
float repArray[20] = {0};
int buttonStateRight = 0; // variable for reading the pushbutton status
int buttonStateLeft = 0; // variable for reading the pushbutton status
int buttonstateRtemp = 0;
int buttonstateLtemp = 0;
int replast = 0;
boolean backlightFlag = 1;
float testVelocity[20] = {0};
  // Pin 13: Arduino has an LED connected on pin 13
  // Pin 11: Teensy 2.0 has the LED on pin 11
  // Pin 6: Teensy++ 2.0 has the LED on pin 6

int buttonstate = 0;        
static unsigned long last_interrupt_time = 0;
static unsigned long last_interrupt_time2 = 0;
static unsigned long last_tic_time = 0;
long instvel = 0;
//LiquidCrystal_I2C lcd(0x27,20,4); //Addr: 0x3F, 20 chars & 4 lines    Nate comment

void setup() {
  // initialize the LED pin as an output:
  
  pinMode(ledPin, OUTPUT);     
  pinMode(buttonPinRight, INPUT); 
  pinMode(buttonPinLeft, INPUT); 
  pinMode(21, INPUT); 
  pinMode(20, INPUT); 
  delay(200); //display needs about 100ms to initialize the IC
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 128x64)  //Nate addition
  display.clearDisplay();
  // text display tests //nate add
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Open");
  display.setCursor(0,25);
  display.println("Barbell");
  display.display();           //end nate add
//  lcd.init();
//  lcd.backlight();
//  lcd.setCursor(0, 0);
  // initialize the pushbutton pin as an input:
  attachInterrupt(21, encoderState, RISING);
  attachInterrupt(20, grabDirection, CHANGE);
  //attachInterrupt(7, grabDirection2, FALLING);
  //Serial.begin(57600);
}

void loop() {
  // read the state of the pushbutton value:
  //chstatemachine();
  buttonStateRight = digitalRead(buttonPinRight);
  buttonStateLeft = digitalRead(buttonPinLeft);
  initializeDisplay();    //
  calcRep(goingUpward, state);
  buttonStateCalc(buttonStateRight, buttonStateLeft);
  setDepth();

  //digitalWrite(ledPin, state);
}

void initializeDisplay(){
  if(!initialized){
    delay(2000);  
    display.clearDisplay(); //nate add
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0,15);
      display.println("Begin Set!");
      display.display();         //end nate add
//    lcd.clear();
//    lcd.setCursor(0, 0);
//    lcd.print("Begin Set!");
    initialized = 1;
  }
}

void calcRep(int isGoingUpward, int currentState){
  if (currentState != currentStateTemp)
  {
    long denom = 0;
    //Since you just found a rising edge, take down the time
    tic_time = micros();
    //increment or decrement the distance by one tic length, depending on direction
    if (isGoingUpward){
      displacement += ticLength;
      // If you're going upward but you were just going downward, clear your array so you can start a fresh rep
      if (!isGoingUpwardLast){
          memset(myVelocities,0,sizeof(myVelocities));
          startheight = displacement;
          starttime = micros();
          rep += 1;
          sumVelocities = 0;
          lastDisplacement = startheight;
          tic_time2 = 0;
          i = 0;
      }
      // This records a value every 20ms
      if (tic_time - tic_time2 > 20000){
        denom = (long)(tic_time - tic_time2);
        //displacement is in micrometers, denom is in microseconds, so instvel is in m/s.
        instvel = ((displacement - lastDisplacement)*1000)/denom;
        myVelocities[i] = (int)instvel;
        tic_time2 = tic_time;
        lastDisplacement = displacement;
        i += 1;
      } 
    } else {
      // If you're going downward, and you were just going upward, you potentially just finished a rep. 
      // Do your math, check if it fits the rep criteria, and store it in an array.
      if (isGoingUpwardLast){
        if ((displacement - startheight) > 150000){
          for (int count = 0; count <= i; count++){
           sumVelocities = sumVelocities + (long)myVelocities[count];
           //Serial.println(sumVelocities);
          }
          avgVelocity = sumVelocities/(long)i;
          testVelocity[rep] = (float)(displacement - startheight)/(float)(micros() - starttime);
          repArray[rep] = (float)avgVelocity/1000;
          repDone = rep;
          display.clearDisplay(); //nate add
          display.setTextSize(3);
          display.setTextColor(WHITE);
          display.setCursor(0,12);
          display.print(repArray[rep]);
          display.print(" m/s");
          display.display();         //end nate add
          //lcd.clear();
          //lcd.print(repArray[rep]);
          //lcd.print(" m/s");
          //Serial.println(repArray[rep]);
        } else { 
          rep -= 1;
          }
      }
      displacement -= ticLength;
      }
      
    isGoingUpwardLast = isGoingUpward;
   //Serial.println(instvel);
   currentStateTemp = currentState;
  }
}

//momentary button hooked up to hardware interrupt 2 will print out my array when pressed, after i'm done collecting velocities
void buttonStateCalc(int buttonstateR, int buttonstateL){
  
  // since hooking up to battery power, we want to turn off backlight after a certain time.
  // if the displayed rep hasn't changed in a while, we don't need the backlight
  if ((micros() - displayTime) > backlightTime){
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    //lcd.setBacklight(0);
    backlightFlag = 0; // might not be necessary anymore -- Nate
  }
  
  if (repDone != repDoneLast){
    repDisplay = repDone; 
    repDoneLast = repDone;
  }
  

  if (!buttonstateRtemp && buttonstateR){
    if (backlightFlag){
      repDisplay += 1;
    }else {
    display.ssd1306_command(SSD1306_DISPLAYON); 
    //lcd.setBacklight(1);
    backlightFlag = 1;
    displayTime = micros();
    }
  }
    
  if ((!buttonstateLtemp && buttonstateL) && (repDisplay > 1)){
    if (backlightFlag){
      repDisplay -= 1;
    }else {
    display.ssd1306_command(SSD1306_DISPLAYON);  
    //lcd.setBacklight(1);
    backlightFlag = 1;
    displayTime = micros();
    }
  }
  
  if (repDisplay != repDisplayLast){
    // if the displayed rep changes, keep the time so we know when to dim the backlight
    displayTime = micros();
    // make sure we can see the new rep
    display.ssd1306_command(SSD1306_DISPLAYON);
    //lcd.setBacklight(1);
    backlightFlag = 1;
    if (repDisplay == (repDone + 1)){
        display.clearDisplay(); //nate add
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(0,0);
        display.print("Ready?");
        display.setTextSize(1);
        display.setCursor(0,25);
        display.print("(Delete past set?)");
        display.display();         //end nate add
//      lcd.clear();
//      lcd.setCursor(0, 0);
//      lcd.print("Ready?");
//      lcd.setCursor(0, 1);
//      lcd.print("(Delete past set?)");
    } else if (repDisplay > (repDone + 1)){
        display.clearDisplay(); //nate add
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(0,15);
        display.print("Begin Set!");
        display.display();         //end nate add
//      lcd.clear();
//      lcd.setCursor(0, 0);
//      lcd.print("Begin Set!");
      startheight = displacement;
      rep = 1;
      repDone = 0;
      repDoneLast = 0;
      sumVelocities = 0;
      memset(repArray,0,sizeof(repArray));
      memset(testVelocity,0,sizeof(testVelocity));
      memset(myVelocities,0,sizeof(myVelocities));
      i = 0;
    } else {
        display.clearDisplay(); //nate add
        display.setTextSize(3);
        display.setTextColor(WHITE);
        display.setCursor(0,12);
        display.print(repArray[repDisplay]);
        display.setTextSize(2);
        display.setCursor(80,6);
        display.print(" m/s");
        display.setTextSize(1);
        display.setCursor(0,0);
        display.print("Rep#: ");
        display.print(repDisplay);
        display.display();         //end nate add
//      lcd.clear();
//      lcd.setCursor(0, 0);
//      lcd.print(repArray[repDisplay]);
//      lcd.print(" m/s");
//      lcd.setCursor(0, 1);
//      lcd.print("Rep #: ");
//      lcd.print(repDisplay);
//      lcd.setCursor(0,2);
//      lcd.print("Test Vel: ");
//      lcd.print(testVelocity[repDisplay]);
      //Serial.println(buttonstateR);
    }
    repDisplayLast = repDisplay; 
  }
  //Serial.println(buttonstateR);
  //Serial.println(buttonstateRtemp);
  //Serial.println("break");
  buttonstateRtemp = buttonstateR;
  buttonstateLtemp = buttonstateL;
  
 //delay(100);
}

void setDepth(){

  

}

/*************************************************************/
/******************** Interrupt Handlers *********************/
/*************************************************************/


void encoderState()
{
  //unsigned long interrupt_time = micros();
  //max tach pulse width is 20 micros. Double that to be safe. Min is 3. Info starts to deteriorate at less than 12 micros
  //We could just use "RISING" for interrupt handler, since a tach pulse happens every time the encoder state changes.
  //if (interrupt_time - last_interrupt_time >40)
    //{
      state = !state;
    //}
  //last_interrupt_time = interrupt_time;
}

void grabDirection()
{
  goingUpward = digitalRead(20);
}

//void grabDirection2()
//{
  //goingUpward = 0;
//}

