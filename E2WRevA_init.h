#ifndef E2WRevA_init_h
#define E2WRevA_init_h
#include "Arduino.h"
//class Rechteck
//{
//  private:
//  int breite;
//  int laenge;
//  public:
//  Rechteck(int b, int l);
//  int get_Breite()
//  void set_Breite(int b);
//  int get_Laenge()
//  void set_Laenge(int b);
//};



//######################################################################################################
//################################## PIN DEFINITIONEN ##################################################

// These are 'flexible' lines that can be changed
#define TFT_CS 41
#define TFT_DC 42
#define TFT_RST 43 // RST can be set to -1 if you tie it to Arduino's reset
//TAster
#define BUTTON_KNOP1 49
#define BUTTON_KNOP2 47
#define BUTTON_KNOP5 23
#define BUTTON_KNOP6 31
#define BUTTON_KNOP7 35

#define RUN_TASTER 30
#define RUN_LED 29
#define RUN_LED_GND 28

#define TASTER4 27
#define LED4 26
#define LED4_GND 24

#define ADC_I1  A10       //AusgangsStrom Port1
#define ADC_U1  A9        //AusgangsSpannung Port1
#define ADC_U2  A0        //AusgangsSpannung Port2
#define ADC_I2  A1        //AusgangsStrom Port2
#define ADC_2V5Ref A7       //Referenzspannungsmessung zu Autokalibrierzwecken
#define ADC_IREF A2
#define ADC_NTC1 A6
#define ADC_NTC2 A5

#define DCDCENTRI 52

#define DISABLE_U2MIN 4
#define DISABLE_U1MIN 3

#define RE_U2max 53
#define RE_U2min 12
#define RE_U1max CANRX
#define RE_U1min CANTX
//######################################################################################################
//################################## Konstanten ########################################################
// Size of the boxes for TFT-Layout
#define BOXSIZE 160
#define MaxPWMCount 6000
#define TFTadcUpdateTime 500
#define SumUpIntervalTime 5


//TFT Farben
//#define VIOLETTBLAU 0x4111
#define VIOLETTBLAU 0x4210
#define DUNKELBLAU 0x0111

//######################################################################################################
//################################## GLOBALE VARIABLEN #################################################

long lastP1;  
long lastP2;  
long lastI1;  //zur Darstellung und Löschung des Stromwertes von Port1
long lastAI1;
unsigned long lastU1;  //zur Darstellung und Löschung des Spannungswertes von Port1
long lastI2;  //zur Darstellung und Löschung des Stromwertes von Port1
long lastAI2;
unsigned long lastU2;  //zur Darstellung und Löschung des Spannungswertes von Port1
int lastReportedPosE1 = 0;
int lastReportedPosE2 = 0;
int lastReportedPosE5 = 0;
int lastReportedPosE6 = 0;
int lastReportedPosE7 = 0;

volatile unsigned long timestampTaste4 = 0;
volatile boolean taste4State = false;
volatile boolean lastTaste4 = HIGH;

volatile unsigned long timestampRunTaste = 0;
volatile boolean runState = false;
volatile boolean lastRunTaste = HIGH;

int ReglerErkennung = 0;
unsigned long TFTadcUpdateTimeStamp = 0;
unsigned long sumUpStartTime = 0;


//Filter
long iirI1 = 0;
long iirAI1 = 0;
unsigned long iirU1 = 0;
long iirI2 = 0;
long iirAI2 = 0;
unsigned long iirU2 = 0;
long iirIref = 0;
long iir2V5ref = 0;

int lastNTC1;
int lastNTC2;

//für das Fernsteuern über die serielle
String inputString = "";         // a string to hold incoming data
String valueString = "";         // zum Einlesen eines Zahlenwertes
boolean stringComplete = false;  // whether the string is complete
boolean nachTrennzeichen = false; //gibt an ob schon ein Leerzeichen im String vorkommt

int NullpunktPWM = MaxPWMCount/2;

void initE2W(void){
//######################################################################################################
//################################## PIN INIT ##########################################################
  pinMode(BUTTON_KNOP1, INPUT_PULLUP);
  pinMode(BUTTON_KNOP2, INPUT_PULLUP);
  pinMode(BUTTON_KNOP5, INPUT_PULLUP);
  pinMode(BUTTON_KNOP6, INPUT_PULLUP);
  pinMode(BUTTON_KNOP7, INPUT_PULLUP);

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

  
//######################################################################################################
//################################## GLOBALE VARIABLEN INIT ############################################
  
  lastI1 = 0;
  lastAI1 = 0;
  lastI2 = 0;
  lastAI2 = 0;
  lastU1 = 0;
  lastP1 = 0;
  lastP2 = 0;
  
//######################################################################################################
//################################## PWM INIT ##########################################################

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

};
#endif
