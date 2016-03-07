
#define VERSION 65   // Version*100

/*************************************
 *       E2W RevA 
 *************************************
 * notwendige Umbaumaßnahmen:
 * Kapazität auf 3,3V 100µF
 * Kapazität auf Arduino-Reset 10µF
 * MISO und MOSI wieder richtig herum gedreht.
 * Strommessung I1 gedreht (+ u. - Eingänge vertauscht) zur Regelbereichserweiterung.
 * 
 * 
 * 65: endlich konsequent mit mA, mV und mW in den Variablen. nicht cA, cV und cW
 * 
 */
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_HX8357.h"

// These are 'flexible' lines that can be changed
#define TFT_CS 41
#define TFT_DC 42
#define TFT_RST 43 // RST can be set to -1 if you tie it to Arduino's reset

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);


// Size of the color selection boxes and the paintbrush size
#define BOXSIZE 160
#define PENRADIUS 5
int oldcolor, currentcolor;

int lastP1;  
int lastP2;  
int lastI1;  //zur Darstellung und Löschung des Stromwertes von Port1
int lastAI1;
int lastU1;  //zur Darstellung und Löschung des Spannungswertes von Port1
int lastI2;  //zur Darstellung und Löschung des Stromwertes von Port1
int lastAI2;
int lastU2;  //zur Darstellung und Löschung des Spannungswertes von Port1
int lastReportedPosE1 = 0;
int lastReportedPosE2 = 0;
int lastReportedPosE5 = 0;
int lastReportedPosE6 = 0;
int lastReportedPosE7 = 0;

//######################################################################################################
#include <Encoder.h>

// Change these pin numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability

 //der Construktor der Encoder.h Datei wurde in "void init" geändert
 //jetzt muss in der setup() funktion noch die init Methode jedes Objektes aufgerufen werden. 
 //damit das Opjekt aber global ist muss es vor der setup() schon erschaffen werden......

//ja blöd aber so gehts. Ich vermute es liegt daran, daß INPUT_PULLUP erst zu setup()-Zeiten definiert wird.
//Deshalb macht die Encoder.h Bibliothek keine Pullups obwohl sie sollte.


//jetzt werden nur mehr die Objekte erzeugt und später in setup() initialisiert.
Encoder knobLeft;
Encoder knobRight;
Encoder knobLeftMitte;
Encoder knobRightMitte;
Encoder knobUnten;


#define BUTTON_KNOP1 49
#define BUTTON_KNOP2 47
#define BUTTON_KNOP5 23
#define BUTTON_KNOP6 31
#define BUTTON_KNOP7 35
//   avoid using pins with LEDs attached
//######################################################################################################
//TAster

#define RUN_TASTER 30
#define RUN_LED 29
#define RUN_LED_GND 28

volatile unsigned long timestampRunTaste = 0;
volatile boolean runState = false;
volatile boolean lastRunTaste = HIGH;

#define TASTER4 27
#define LED4 26
#define LED4_GND 24

volatile unsigned long timestampTaste4 = 0;
volatile boolean taste4State = false;
volatile boolean lastTaste4 = HIGH;

#define ENTPRELLMILLIS 300

//######################################################################################################




//E2W-Analog IN/OUT (ADC und PWM)
//annkinntonn

#define ADC_I1  A10       //AusgangsStrom Port1
#define ADC_U1  A9        //AusgangsSpannung Port1
#define ADC_U2  A0        //AusgangsSpannung Port2
#define ADC_I2  A1        //AusgangsStrom Port2
#define ADC_2V5Ref A7       //Referenzspannungsmessung zu Autokalibrierzwecken
#define ADC_IREF A2
#define ADC_NTC1 A6
#define ADC_NTC2 A5

#define PWM_I2_SOLL 7      //Stellwert 0..3V3 mit PWM
#define GEMESSENE_2V5_REFERENZ 2499 //mV-Messung (händisch) der Refernzspannung


#define MaxPWMCount 6000
int NullpunktPWM = MaxPWMCount/2;

//''''''''''''''''''''''''''''''''''''''braucht man auch noch global:

#define DCDCENTRI 52

#define DISABLE_U2MIN 4
#define DISABLE_U1MIN 3
     
     
int ReglerErkennung = 0;
#define RE_U2max 53
#define RE_U2min 12
#define RE_U1max CANRX
#define RE_U1min CANTX

//######################################################################################################
int statemachineCounter;

//Filter
long iirI1 = 0;
long iirAI1 = 0;
long iirU1 = 0;
long iirI2 = 0;
long iirAI2 = 0;
long iirU2 = 0;
long iirIref = 0;
long iir2V5ref = 0;

int lastNTC1;
int lastNTC2;

//######################################################################################################
void setup() {
  
  
  // e1 = new DrehEncoderKlasse(50, 51, 49, 199);
knobRight.init(39, 37);
knobLeft.init(51, 50);    
knobLeftMitte.init(48, 46);
knobRightMitte.init(33, 32);
knobUnten.init(25, 22);

 pinMode(BUTTON_KNOP1, INPUT_PULLUP);
 pinMode(BUTTON_KNOP2, INPUT_PULLUP);
 pinMode(BUTTON_KNOP5, INPUT_PULLUP);
 pinMode(BUTTON_KNOP6, INPUT_PULLUP);
 pinMode(BUTTON_KNOP7, INPUT_PULLUP);
 //pinMode(34, INPUT_PULLUP);
 //pinMode(35, INPUT_PULLUP);

pinMode(RUN_TASTER, INPUT_PULLUP);
  pinMode(RUN_LED, OUTPUT);
  pinMode(RUN_LED_GND, OUTPUT);
  digitalWrite(RUN_LED_GND, LOW);
pinMode(TASTER4, INPUT_PULLUP);
  pinMode(LED4, OUTPUT);
  pinMode(LED4_GND, OUTPUT);
  digitalWrite(LED4_GND, LOW);

  pinMode(RE_U2max, INPUT_PULLUP);
  pinMode(RE_U2min, INPUT_PULLUP);
  pinMode(RE_U1max, INPUT_PULLUP);
  pinMode(RE_U1min, INPUT_PULLUP);

pinMode(DISABLE_U2MIN, OUTPUT);
pinMode(DISABLE_U1MIN, OUTPUT);

  //Analog InOut
  analogWriteResolution(12);
  analogReadResolution(12);

  lastI1 = 0;
  lastAI1 = 0;
  lastI2 = 0;
  lastAI2 = 0;
  lastU1 = 0;
  lastP1 = 0;
  lastP2 = 0;

 
  
  statemachineCounter = 0;


//Pin 7 wird echter PWM-Pin Achtung Pin 7 und 6 sind bei Arduino gegenüber dem normalen SAM3X-Namen vertauscht !!!!!!
int32_t mask_PWM_pin = digitalPinToBitMask(7);
REG_PMC_PCER1 = 1<<4;               // activate clock for PWM controller
REG_PIOC_PDR |= mask_PWM_pin;  // activate peripheral functions for pin (disables all PIO functionality)
REG_PIOC_ABSR |= mask_PWM_pin; // choose peripheral option B   
REG_PWM_CLK = 0;                     // choose clock rate, 0 -> full MCLK as reference 84MHz
REG_PWM_CMR6 = 0<<9;             // select clock and polarity for PWM channel PWML6 (pin7) -> (CPOL = 0)
REG_PWM_CPRD6 = MaxPWMCount;                // initialize PWM period -> T = value/84MHz (value: up to 16bit), value=8 -> 10.5MHz
 // note : with this 5000 setting, we have thus 1/5000 resol, with a frequency of 16khz, if we use the defaut main clock : that's good

REG_PWM_CDTY6 = 1515;                // initialize duty cycle, REG_PWM_CPRD6 / value = duty cycle, for 8/4 = 50%
REG_PWM_ENA = 1<<6;               // enable PWM on PWM channel (pin 7 = PWML6)

//Pin 6 wird echter PWM-Pin 
mask_PWM_pin = digitalPinToBitMask(6);
//REG_PMC_PCER1 = 1<<4;               // activate clock for PWM controller
REG_PIOC_PDR |= mask_PWM_pin;  // activate peripheral functions for pin (disables all PIO functionality)
REG_PIOC_ABSR |= mask_PWM_pin; // choose peripheral option B   
//REG_PWM_CLK = 0;                     // choose clock rate, 0 -> full MCLK as reference 84MHz
REG_PWM_CMR7 = 0<<9;             // select clock and polarity for PWM channel PWML7 (pin6) -> (CPOL = 0)
REG_PWM_CPRD7 = MaxPWMCount;                // initialize PWM period -> T = value/84MHz (value: up to 16bit), value=8 -> 10.5MHz
 // note : with this 5000 setting, we have thus 1/5000 resol, with a frequency of 16khz, if we use the defaut main clock : that's good

REG_PWM_CDTY7 = 1515;                // initialize duty cycle, REG_PWM_CPRD6 / value = duty cycle, for 8/4 = 50%
REG_PWM_ENA = 1<<7;               // enable PWM on PWM channel (pin 6 = PWML7)


//Pin 40 wird echter PWM-Pin für TFT-Lite
mask_PWM_pin = digitalPinToBitMask(40);
//REG_PMC_PCER1 = 1<<4;               // activate clock for PWM controller
REG_PIOC_PDR |= mask_PWM_pin;  // activate peripheral functions for pin (disables all PIO functionality)
REG_PIOC_ABSR |= mask_PWM_pin; // choose peripheral option B   
//REG_PWM_CLK = 0;                     // choose clock rate, 0 -> full MCLK as reference 84MHz
REG_PWM_CMR3 = 0<<9;             // select clock and polarity for PWM channel PWML2 (pin38) -> (CPOL = 0)
REG_PWM_CPRD3 = MaxPWMCount;                // initialize PWM period -> T = value/84MHz (value: up to 16bit), value=8 -> 10.5MHz
 // note : with this 5000 setting, we have thus 1/5000 resol, with a frequency of 16khz, if we use the defaut main clock : that's good

REG_PWM_CDTY3 = 3000;                // initialize duty cycle, REG_PWM_CPRD6 / value = duty cycle, for 8/4 = 50%
REG_PWM_ENA = 1<<3;               // enable PWM on PWM channel (pin 40 = PWML3)

//Pin 38 wird echter PWM-Pin für TFT-Lite
mask_PWM_pin = digitalPinToBitMask(38);
//REG_PMC_PCER1 = 1<<4;               // activate clock for PWM controller
REG_PIOC_PDR |= mask_PWM_pin;  // activate peripheral functions for pin (disables all PIO functionality)
REG_PIOC_ABSR |= mask_PWM_pin; // choose peripheral option B   
//REG_PWM_CLK = 0;                     // choose clock rate, 0 -> full MCLK as reference 84MHz
REG_PWM_CMR2 = 0<<9;             // select clock and polarity for PWM channel PWML2 (pin38) -> (CPOL = 0)
REG_PWM_CPRD2 = MaxPWMCount;                // initialize PWM period -> T = value/84MHz (value: up to 16bit), value=8 -> 10.5MHz
 // note : with this 5000 setting, we have thus 1/5000 resol, with a frequency of 16khz, if we use the defaut main clock : that's good

REG_PWM_CDTY2 = 3000;                // initialize duty cycle, REG_PWM_CPRD6 / value = duty cycle, for 8/4 = 50%
REG_PWM_ENA = 1<<2;               // enable PWM on PWM channel (pin 6 = PWML7)

//Pin 36 wird echter PWM-Pin für TFT-Lite
mask_PWM_pin = digitalPinToBitMask(36);
//REG_PMC_PCER1 = 1<<4;               // activate clock for PWM controller
REG_PIOC_PDR |= mask_PWM_pin;  // activate peripheral functions for pin (disables all PIO functionality)
REG_PIOC_ABSR |= mask_PWM_pin; // choose peripheral option B   
//REG_PWM_CLK = 0;                     // choose clock rate, 0 -> full MCLK as reference 84MHz
REG_PWM_CMR1 = 0<<9;             // select clock and polarity for PWM channel PWML1 (pin38) -> (CPOL = 0)
REG_PWM_CPRD1 = MaxPWMCount;                // initialize PWM period -> T = value/84MHz (value: up to 16bit), value=8 -> 10.5MHz
 // note : with this 5000 setting, we have thus 1/5000 resol, with a frequency of 16khz, if we use the defaut main clock : that's good

REG_PWM_CDTY1 = 0;                // initialize duty cycle, REG_PWM_CPRD6 / value = duty cycle, for 8/4 = 50%
REG_PWM_ENA = 1<<1;               // enable PWM on PWM channel (pin 36 = PWML1)

//Pin 9 wird echter PWM-Pin für TFT-Lite
mask_PWM_pin = digitalPinToBitMask(9);
//REG_PMC_PCER1 = 1<<4;               // activate clock for PWM controller
REG_PIOC_PDR |= mask_PWM_pin;  // activate peripheral functions for pin (disables all PIO functionality)
REG_PIOC_ABSR |= mask_PWM_pin; // choose peripheral option B   
//REG_PWM_CLK = 0;                     // choose clock rate, 0 -> full MCLK as reference 84MHz
REG_PWM_CMR4 = 0<<9;             // select clock and polarity for PWM channel PWML1 (pin38) -> (CPOL = 0)
REG_PWM_CPRD4 = MaxPWMCount;                // initialize PWM period -> T = value/84MHz (value: up to 16bit), value=8 -> 10.5MHz
 // note : with this 5000 setting, we have thus 1/5000 resol, with a frequency of 16khz, if we use the defaut main clock : that's good

REG_PWM_CDTY4 = 3000;                // initialize duty cycle, REG_PWM_CPRD6 / value = duty cycle, for 8/4 = 50%
REG_PWM_ENA = 1<<4;               // enable PWM on PWM channel (pin 36 = PWML1)

//Pin 8 wird echter PWM-Pin für TFT-Lite
mask_PWM_pin = digitalPinToBitMask(8);
//REG_PMC_PCER1 = 1<<4;               // activate clock for PWM controller
REG_PIOC_PDR |= mask_PWM_pin;  // activate peripheral functions for pin (disables all PIO functionality)
REG_PIOC_ABSR |= mask_PWM_pin; // choose peripheral option B   
//REG_PWM_CLK = 0;                     // choose clock rate, 0 -> full MCLK as reference 84MHz
REG_PWM_CMR5 = 0<<9;             // select clock and polarity for PWM channel PWML1 (pin38) -> (CPOL = 0)
REG_PWM_CPRD5 = MaxPWMCount;                // initialize PWM period -> T = value/84MHz (value: up to 16bit), value=8 -> 10.5MHz
 // note : with this 5000 setting, we have thus 1/5000 resol, with a frequency of 16khz, if we use the defaut main clock : that's good

REG_PWM_CDTY5 = 3000;                // initialize duty cycle, REG_PWM_CPRD6 / value = duty cycle, for 8/4 = 50%
REG_PWM_ENA = 1<<5;               // enable PWM on PWM channel (pin 36 = PWML1)


  tft.begin(HX8357D);
  tft.fillScreen(HX8357_BLACK);
  Serial.begin(115200);  // output
  Serial.println(INPUT_PULLUP);
  presetAnnKinnTonn();
}
//######################################################################################################

// main loop, work is done by interrupt service routines, this one only prints stuff
void loop() { 
  int p1,p2,i1,u1,u2,i2,iref,u2V5Ref;
  long uADC;
  int x; //Offset für tft.print zum rechts zentrieren
  int encoder1Val, encoder2Val, encoder5Val, encoder6Val, encoder7Val;
  int color = 0x4111; //0x0111 dunkelblau
  
  encoder1Val = knobLeft.read()>>2;     //Begrenzung der Einstellung auf 60V und 0V
  if (encoder1Val > 600) {          
    encoder1Val = 600;
    knobLeft.write(600<<2);
  }
  if (encoder1Val < 0) {
    encoder1Val = 0;
    knobLeft.write(0);
  }
  if (lastReportedPosE1 != encoder1Val) {

    color = 0x4111;
    if (digitalRead(RE_U1max) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(25+x, BOXSIZE + 10, 3, color, lastReportedPosE1*100, encoder1Val*100, 4, "V");
    //tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(134, BOXSIZE + 24);
    tft.print("max");  
  
    Serial.print("Encoder 1: ");
    Serial.println(encoder1Val, DEC);
    lastReportedPosE1 = encoder1Val;
  }

  encoder2Val = knobLeftMitte.read()>>2;
  if (encoder2Val > encoder1Val) {
    encoder2Val = encoder1Val;
    knobLeftMitte.write(encoder1Val<<2);
  }
  if (encoder2Val < 0) {
    encoder2Val = 0;
    knobLeftMitte.write(0);
  }
  if (encoder2Val == 0) {
    digitalWrite(DISABLE_U1MIN, HIGH);
  }
  else {
    digitalWrite(DISABLE_U1MIN, LOW);
  }
  if (lastReportedPosE2 != encoder2Val) {
    color = 0x4111;
    if (digitalRead(RE_U1min) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(25+x, BOXSIZE + BOXSIZE-31, 3, color, lastReportedPosE2*100, encoder2Val*100, 4, "V");
   // tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(134, BOXSIZE + BOXSIZE-31+14);
    tft.print("min");
    
    Serial.print("Encoder 2: ");
    Serial.println(encoder2Val, DEC);
    lastReportedPosE2 = encoder2Val;
  }

  encoder7Val = knobRight.read()>>2;
  if (encoder7Val > 600) {          
    encoder7Val = 600;
    knobRight.write(600<<2);
  }
  if (encoder7Val < 0) {
    encoder7Val = 0;
    knobRight.write(0);
  }
  if (lastReportedPosE7 != encoder7Val) {
    color = 0x4111;
    if (digitalRead(RE_U2max) == 0) color = HX8357_YELLOW; 
    tftTextWertUpdate(BOXSIZE+25+x, BOXSIZE + 10, 3, color, lastReportedPosE7*100, encoder7Val*100, 4, "V");
    //tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(BOXSIZE+134, BOXSIZE + 24);
    tft.print("max");
    
    Serial.print("Encoder 7: ");
    Serial.println(encoder7Val, DEC);
    lastReportedPosE7 = encoder7Val;
  }

  encoder6Val = knobRightMitte.read()>>2;
  if (encoder6Val > encoder7Val) {
    encoder6Val = encoder7Val;
    knobRightMitte.write(encoder6Val<<2);
  }
  if (encoder6Val < 0) {
    encoder6Val = 0;
    knobRightMitte.write(0);
  }
  if (encoder6Val == 0) {
    digitalWrite(DISABLE_U2MIN, HIGH);
  }
  else {
    digitalWrite(DISABLE_U2MIN, LOW);
  }
  
  if (lastReportedPosE6 != encoder6Val) {
    color = 0x4111;
    if (digitalRead(RE_U2min) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(BOXSIZE+25+x, BOXSIZE + BOXSIZE-31, 3, color, lastReportedPosE6*100, encoder6Val*100, 4, "V");
   // tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(BOXSIZE+134, BOXSIZE + BOXSIZE-31+14);
    tft.print("min");

    
    Serial.print("Encoder 6: ");
    Serial.println(encoder6Val, DEC);
    lastReportedPosE6 = encoder6Val;
  }

  encoder5Val = knobUnten.read()>>2;
  if (encoder5Val > 1500) {          
    encoder5Val = 1500;
    knobUnten.write(1500<<2);
  }
  if (encoder5Val < -1500) {
    encoder5Val = -1500;
    knobUnten.write(-1500*4);
  }
  if (lastReportedPosE5 != encoder5Val) {
    color = 0x4111;
    if (digitalRead(RE_U2min) == 1 && digitalRead(RE_U2max) == 1 && digitalRead(RE_U1min) == 1 && digitalRead(RE_U1max) == 1) color = HX8357_YELLOW;
    tftTextWertUpdate(BOXSIZE+25+x,  3*BOXSIZE-31, 3, color, lastReportedPosE5*10, encoder5Val*10, 4, "A");
   // tft.print("A");
    tft.setTextSize(1);
    tft.setCursor(BOXSIZE+134, BOXSIZE*3-31+14);
    tft.print("soll");
    
    Serial.print("Encoder 5: ");
    Serial.println(encoder5Val, DEC);
    lastReportedPosE5 = encoder5Val;
  }
  


  
  if ((digitalRead(BUTTON_KNOP1) == LOW) && (runState == false)){
    u1 = analogRead(ADC_U1);
    u1 = map(u1,0, 4095, 0, 693); 
    knobLeft.write((u1+10)<<2);    
    Serial.println("Taste1");
  }
  if ((digitalRead(BUTTON_KNOP2) == LOW ) && (runState == false)) {
    knobLeftMitte.write(0);    
    Serial.println("Taste2");
  }
  if ((digitalRead(BUTTON_KNOP5) == LOW ) && (runState == false)) {
    knobUnten.write(0);    
    Serial.println("Taste5");
  }
  if ((digitalRead(BUTTON_KNOP6) == LOW ) && (runState == false)) {
    knobRightMitte.write(0);    
    Serial.println("Taste6");
  }
  if ((digitalRead(BUTTON_KNOP7) == LOW ) && (runState == false)) {
    u2 = analogRead(ADC_U2);
    u2 = map(u2,0, 4095, 0, 693); 
    knobRight.write((u2+10)<<2);    
    Serial.println("Taste7");
  }
  if ((digitalRead(BUTTON_KNOP1) == LOW) && (digitalRead(BUTTON_KNOP7) == LOW))  {
    tft.fillScreen(HX8357_BLACK); //TFT
    Serial.println("ClearScreen");
    presetAnnKinnTonn();
  }



  if ((digitalRead(BUTTON_KNOP2) == LOW ) && (runState == true)) {
    Serial.print("UAREF: ");Serial.print((long) GEMESSENE_2V5_REFERENZ * 65536 / iir2V5ref);
    Serial.print(" Version: ");Serial.print((float)VERSION/100);
   Serial.print(" U1min: ");Serial.print(digitalRead(RE_U1min));
   Serial.print("  U1max: ");Serial.print(digitalRead(RE_U1max));
   Serial.print("  U2min: ");Serial.print(digitalRead(RE_U2min));
   Serial.print("  U2max: ");Serial.println(digitalRead(RE_U2max));
  }
int regErkNew = digitalRead(RE_U1min)*8 + digitalRead(RE_U1max)*4 + digitalRead(RE_U2min)*2 + digitalRead(RE_U2max);
if (regErkNew != ReglerErkennung){
    color = 0x4111;
    if (digitalRead(RE_U1max) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(25+x, BOXSIZE + 10, 3, color, lastReportedPosE1*100, encoder1Val*100, 4, "V");
    //tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(134, BOXSIZE + 24);
    tft.print("max");  

    color = 0x4111;
    if (digitalRead(RE_U1min) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(25+x, BOXSIZE + BOXSIZE-31, 3, color, lastReportedPosE2*100, encoder2Val*100, 4, "V");
  //  tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(134, BOXSIZE + BOXSIZE-31+14);
    tft.print("min");
    

    color = 0x4111;
    if (digitalRead(RE_U2max) == 0) color = HX8357_YELLOW; 
    tftTextWertUpdate(BOXSIZE+25+x, BOXSIZE + 10, 3, color, lastReportedPosE7*100, encoder7Val*100, 4, "V");
   // tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(BOXSIZE+134, BOXSIZE + 24);
    tft.print("max");

    color = 0x4111;
    if (digitalRead(RE_U2min) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(BOXSIZE+25+x, BOXSIZE + BOXSIZE-31, 3, color, lastReportedPosE6*100, encoder6Val*100, 4, "V");
   // tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(BOXSIZE+134, BOXSIZE + BOXSIZE-31+14);
    tft.print("min");

    color = 0x4111;
    if (digitalRead(RE_U2min) == 1 && digitalRead(RE_U2max) == 1 && digitalRead(RE_U1min) == 1 && digitalRead(RE_U1max) == 1) color = HX8357_YELLOW;
    tftTextWertUpdate(BOXSIZE+25+x, 3*BOXSIZE-31, 3, color, lastReportedPosE5*10, encoder5Val*10, 4, "A");
   // tft.print("A");
    tft.setTextSize(1);
    tft.setCursor(BOXSIZE+134, BOXSIZE*3-31+14);
    tft.print("soll");

    /////
    
}
ReglerErkennung = regErkNew;
  
  //--------------------------- ADC ---------------------------------------------------------- 

  u2V5Ref = analogRead(ADC_2V5Ref);
  //iir2V5ref = u2V5Ref;
  iir2V5ref = iir2V5ref - iir2V5ref/16;
  iir2V5ref = u2V5Ref + iir2V5ref;
  
  uADC = GEMESSENE_2V5_REFERENZ * 65536 / iir2V5ref;  //ADC-Spannung in mV

  u1 = analogRead(ADC_U1);
  u2 = analogRead(ADC_U2);
  
  iref = analogRead(ADC_IREF);
  i1 = analogRead(ADC_I1);
  i2 = analogRead(ADC_I2);

//Alternative Strom-Berechnung....hoffentlich genauer **************** ja e ganz gut...
/*
int aI1, aI2;
  aI1 = i1-iref;
  aI2 = i2-iref;

  iirAI1 = iirAI1 - iirAI1/16;
  iirAI1 = aI1 + iirAI1;

  iirAI2 = iirAI2 - iirAI2/16;
  iirAI2 = aI2 + iirAI2;
  

*/
//********************************************************************
  
  iirU1 = iirU1 - iirU1/16;
  iirU1 = u1 + iirU1;

  iirU2 = iirU2 - iirU2/16;
  iirU2 = u2 + iirU2;

  iirI1 = iirI1 - iirI1/16;
  iirI1 = i1 + iirI1;

  iirI2 = iirI2 - iirI2/16;
  iirI2 = i2 + iirI2;

  iirIref = iirIref - iirIref/16;
  iirIref = iref + iirIref;
  
  if (statemachineCounter >= 3000) {
    
    u1 = map(iirU1,0, 65535, 0, (long) uADC*21*1001/1000)+ 50;   //guteStellezumKalibrieren !!!
    u2 = map(iirU2,0, 65535, 0, (long) uADC*21*996/1000) +146; //Faktor 0,996 Offset 146mV sollte lieber alles in mV gehalten sein
    i1 = map(iirI1,0, 65535, 0, uADC*10);
    i2 = map(iirI2,0, 65535, 0, uADC*10);
    //aI1 = map(iirAI1,0, 65535, 0, uADC*100/18);
    //aI2 = map(iirAI2,0, 65535, 0, uADC*100/18);
    iref = map(iirIref,0, 65535, 0, uADC*10);

    i1 = (iref - i1)*10/18; //nur mit mit Stromrichtungsumbau i1!
    i2 = (i2 - iref)*10/18;
    
    lastU1 = tftTextWertUpdate(10, BOXSIZE + BOXSIZE/2-14, 4, HX8357_BLUE, lastU1, u1,4, "V");
   // tft.print("V");
    // jetzt mit Leistungsanzeige!************************
    p1 = u1 * i1 / 1000;
    lastP1 = tftTextWertUpdate(20, BOXSIZE/2-14-40, 3, HX8357_MAGENTA, lastP1, p1,5, "W");
   // tft.print("W");           
    //***************************************************-
    lastI1 = tftTextWertUpdate(10, BOXSIZE + BOXSIZE/2-14+BOXSIZE, 4, HX8357_RED, lastI1, i1,4, "A");
    //tft.print("A");
    //lastAI1 = tftTextWertUpdate(10, BOXSIZE + 14+BOXSIZE, 2, HX8357_RED, lastAI1*100, aI1*100,5);
    //tft.print("mA");
      
    
    //U2 
    lastU2 = tftTextWertUpdate(10+BOXSIZE, BOXSIZE + BOXSIZE/2-14, 4, HX8357_BLUE, lastU2, u2,4, "V");
    //tft.print("V");
                      
    // jetzt mit Leistungsanzeige!*********************
    p2 = u2 * i2 / 1000;
    lastP2 = tftTextWertUpdate(20+BOXSIZE,  BOXSIZE/2-14-40, 3, HX8357_MAGENTA, lastP2, p2,5, "W");
    //tft.print("W");                    
    //***************************************************-
    lastI2 = tftTextWertUpdate(10+BOXSIZE, BOXSIZE + BOXSIZE/2-14+BOXSIZE, 4, HX8357_RED, lastI2, i2,4, "A");
    //tft.print("A");
    //lastAI2 = tftTextWertUpdate(10+BOXSIZE, BOXSIZE + 14+BOXSIZE, 2, HX8357_RED, lastAI2*100, aI2*100,5);
    //tft.print("mA");

int newNTC1 = analogRead(ADC_NTC1);
int newNTC2 = analogRead(ADC_NTC2);

      tftTextWertUpdate(40,BOXSIZE/3+5, 1, 0xAAAA,(3520-lastNTC1)*1000/24,(3520-newNTC1)*1000/24, 4, " GC NTC1");
     // tft.print(" GC NTC1");
      tftTextWertUpdate(BOXSIZE + 45,BOXSIZE/3+5, 1, 0xAAAA,(3520-lastNTC2)*1000/24,(3520-newNTC2)*1000/24, 4, " GC NTC2");
      //tft.print(" GC NTC2");

      lastNTC1 = newNTC1;
      lastNTC2 = newNTC2;
    
 }
 

//******************************************PWM Settings**********************************************  
 // REG_PWM_CDTY2 = 2000;    //TFT-Backlight
 // if (REG_PWM_CDTY2 > 6000) REG_PWM_CDTY2 = 0;

  int kalibrierterPWMU1Max= map(encoder1Val, 0, 2*uADC, 0 , MaxPWMCount*10+ 50);
  if (kalibrierterPWMU1Max > 12) kalibrierterPWMU1Max -=12; //Offset einrechnen und verhindern daß Wert negativ wird 
  REG_PWM_CDTY3 = kalibrierterPWMU1Max;

  int kalibrierterPWMU1Min= map(encoder2Val, 0, 2*uADC, 0 , MaxPWMCount*10- 100);
  if (kalibrierterPWMU1Min > 10) kalibrierterPWMU1Min -=10;
  REG_PWM_CDTY2 = kalibrierterPWMU1Min;
  // nix mehr analogWrite(PWM_bla_bla, ...
  //                                                         Faktor v    v Offset
  
  int kalibrierterPWMU2Max= map(encoder7Val, 0, 2*uADC, 0 , MaxPWMCount*10- 300);
  if (kalibrierterPWMU2Max > 15) kalibrierterPWMU2Max -=15; //Offset einrechnen und verhindern daß Wert negativ wird
  REG_PWM_CDTY6 = kalibrierterPWMU2Max;  // Faktor und Offset wurden durch probieren bestimmt
  
  int kalibrierterPWMU2Min= map(encoder6Val, 0, 2*uADC, 0 , MaxPWMCount*10- 300);
  if (kalibrierterPWMU2Min > 15) kalibrierterPWMU2Min -=15;
  REG_PWM_CDTY7 = kalibrierterPWMU2Min;

  

  if (statemachineCounter%200 == 0) {
    if (abs(lastI2) <= 400) NullpunktPWM = MaxPWMCount/2; //Normalzustand, in beide Richtungen gleich
    if (lastI2 > 400) {
      NullpunktPWM = NullpunktPWM - 30;
      if (NullpunktPWM <= MaxPWMCount / 8) NullpunktPWM = MaxPWMCount / 8;
      //Serial.println("Jetzt fliessen über 4A");
    }
    if (lastI2 < -400){
      NullpunktPWM =NullpunktPWM + 30;
      if (NullpunktPWM >= MaxPWMCount *7 / 8) NullpunktPWM = MaxPWMCount *7 / 8;
      //Serial.println("Jetzt fliessen unter -4A");
    }
  //Serial.print("Nullpunkt: ");Serial.print(NullpunktPWM);Serial.print("   i2: ");Serial.println(lastI2);
  }
  
  REG_PWM_CDTY4 = NullpunktPWM;  //nur mit Umbau, sonst sollte immer //genau auf die Hälfte
  REG_PWM_CDTY5 = map(encoder5Val, -916, 916, NullpunktPWM - MaxPWMCount/2 , NullpunktPWM + MaxPWMCount/2); //Stromstellwert 
  if (REG_PWM_CDTY5 <= 0) REG_PWM_CDTY5 = 0;


  if (statemachineCounter >=3000) statemachineCounter = 0;
  statemachineCounter++;

  //RUN/Stop-Taste***************************************************************************************************************************************
  int runTaste = digitalRead(RUN_TASTER);
  if (!runTaste && lastRunTaste && millis()-timestampRunTaste > ENTPRELLMILLIS){
    if (!runState) {
      pinMode(DCDCENTRI, OUTPUT);
      //PORTB |= _BV(PIN1); //auf HIGH
      digitalWrite(DCDCENTRI,HIGH);
      delayMicroseconds(10);
      pinMode(DCDCENTRI, INPUT);
      runState = true;
      tft.drawRect(10, BOXSIZE/3+20, 2*BOXSIZE-20, BOXSIZE*2/3-25, 0xD1CC);
      tft.setCursor(BOXSIZE/2,BOXSIZE/2+30);
      tft.setTextSize(2);
      tft.setTextColor(HX8357_RED);
      tft.print("...running!"); 
    }
    else {
      runState = false;
      tft.drawRect(10, BOXSIZE/3+20, 2*BOXSIZE-20, BOXSIZE*2/3-25, HX8357_BLACK);
      tft.setCursor(BOXSIZE/2,BOXSIZE/2+30);
      tft.setTextSize(2);
      tft.setTextColor(HX8357_BLACK);
      tft.print("...running!"); 
    }
    timestampRunTaste = millis();
  }
  lastRunTaste = runTaste;
  if (!digitalRead(DCDCENTRI)) {runState = false;};  //Wenn der Wandler wegen Überspannung abgeworfen wird

  if(runState) {
    digitalWrite(RUN_LED,HIGH);
    if (statemachineCounter >= 3000) {
      //tft.fillRect(15, BOXSIZE*2+15, BOXSIZE-30, BOXSIZE-55, HX8357_BLACK);

    //etwas das jeden Durchlauf im Run State passieren soll.
    }
  }else {
    digitalWrite(RUN_LED,LOW);  
    pinMode(DCDCENTRI, OUTPUT);
    digitalWrite(DCDCENTRI,LOW);
  }
  //Taste4***************************************************************************************************************************************
  int taste4 = digitalRead(TASTER4);
  if (!taste4 && lastTaste4 && millis()-timestampTaste4 > ENTPRELLMILLIS){
    if (!taste4State) {
      //do something einmal bei Tastendruck
      taste4State = true;
    }
    else {
      //do something einmal bei Tastendruck
      taste4State = false;
    }
    timestampTaste4 = millis();
  }
  lastTaste4 = taste4;
  //if (!digitalRead(??)) {taste4State = false;};  //Wenn der On-Zustand verlassen werden soll

  if(taste4State) {
    //do something dauerhaft bis zum nächsten Tastendruck
     REG_PWM_CDTY1 = 800;    //TFT-Backlight
     
    //digitalWrite(LED4,HIGH);
  }else {
    //do something dauerhaft bis zum nächsten Tastendruck
    //digitalWrite(LED4,LOW); 
     REG_PWM_CDTY1 = 3000;    //TFT-Backlight 
     //pinMode(3, INPUT);
     //pinMode(4, INPUT);
    
  }
digitalWrite(LED4,LOW); //Weil sonst niemand das Licht ausmacht
  //-------------------------------------------------------------------------------------

}
//######################################################################################################
void tftPrintFloatMilli(int x, int y, int Textsize, int Color,int WertInMilliEinheiten, char* Einheit, int Offset){
    if ((WertInMilliEinheiten == abs(WertInMilliEinheiten)) && (WertInMilliEinheiten < 10000)){
      Offset += 3;
    }else if ((WertInMilliEinheiten > -10000) && (WertInMilliEinheiten < 100000)){
      Offset += 2;
    }else if ((WertInMilliEinheiten > -100000) && (WertInMilliEinheiten < 1000000)){
      Offset += 1;
    }
    Offset = Offset * Textsize * 6; //Pixelabstand 6 Pixel pro Zeichen
    tft.setCursor(x + Offset,y);
    tft.setTextSize(Textsize);
    tft.setTextColor(Color);
    tft.print((float) WertInMilliEinheiten/1000);
    tft.print(Einheit);
}

int tftTextWertUpdate(int x, int y, int Textsize, int Color,int lastWert,int newWert, int digits, char* Einheit){
  int aktuellerText, xOff;

    if (digits == 4) xOff = -2;
    if (digits == 5) xOff = -1;
    
    tftPrintFloatMilli(x, y, Textsize, HX8357_BLACK, lastWert, Einheit, xOff);
    tftPrintFloatMilli(x, y, Textsize, Color, newWert, Einheit, xOff);
    
    aktuellerText = newWert;
    return aktuellerText;
}

//preset
void presetAnnKinnTonn(){
  int u1,u2;
    u1 = analogRead(ADC_U1);
    u1 = map(u1,0, 4095, 0, 693); 
    knobLeft.write((u1+10)<<2);  
    u2 = analogRead(ADC_U2);
    u2 = map(u2,0, 4095, 0, 693); 
    knobRight.write((u2+10)<<2);  
    tft.drawRect(1, BOXSIZE + 1, BOXSIZE-2, BOXSIZE-2, 0x51C0);
    tft.drawRect(BOXSIZE + 1, BOXSIZE + 1, BOXSIZE-2, BOXSIZE-2, 0x51C0);
    tft.drawRect(1, 2*BOXSIZE + 1, BOXSIZE-2, BOXSIZE-2, 0x51C1);
    tft.drawRect(BOXSIZE + 1, 2*BOXSIZE + 1, BOXSIZE-2, BOXSIZE-2, 0x51C1);
    tft.drawRect(1, 1, BOXSIZE-2, BOXSIZE/3+15, 0x51C1);
    tft.drawRect(BOXSIZE + 1, 1, BOXSIZE-2, BOXSIZE/3+15, 0x51C1);

    tft.setCursor(BOXSIZE/2-10,BOXSIZE/2);
    tft.setTextSize(2);
    tft.setTextColor(0x7542);
    tft.print("Energy2Work "); tft.print((float) VERSION/100);
    
}


