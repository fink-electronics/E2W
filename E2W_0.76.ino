
#define VERSION 76   // centi-Version

/*************************************
 *       E2W RevA 
 *************************************
 * notwendige Umbaumaßnahmen:
 * Kapazität auf 3,3V 100µF
 * Kapazität auf Arduino-Reset 10µF
 * MISO und MOSI wieder richtig herum gedreht.
 * Strommessung I1 gedreht (+ u. - Eingänge vertauscht) zur Regelbereichserweiterung.
 * 2 zusätzliche MosFETs zum deaktivieren der Umin-Regler (an Port D3 und D4)
 * 10nF Kapazitäten auf Dreh-Encoder-Pins
 * 
 * 65: endlich konsequent mit mA, mV und mW in den Variablen. nicht cA, cV und cW, nur in den knobs nicht ganz
 * 66: 2016-03-08_Die steuerung über die Serielle Schnittstelle ist drin
 * 67: Struktur mit E2WRevA_init.h verbessert, Statemachine-Counter rausgeschmissen!
 * 68: MPPTracker (Taste4 blinkt im MPPT-Betrieb
 * 69: Hintergrund-Licht einstellung während MPPT mit U2Min-Knob
 * 70: Einschaltstromimpuls minimiert, noch in Arbeit!, ADC_VC A8, war leider nix!
 * 71: Einschaltstrombegrenzung durch VC-Kontrolle funktioniert.
 * 72: am besten mit PWM 0 für ISoll beginnen, bester STartwert für beide Richtungen (70 und 71 waren sinnlos)
 * 73: Kalibrierung soll vereinfacht werden. 
 * ADC mit fixen Abtastraten durch TimerInterrupt.
 * im runState werden die Sollwerte der Spannungsgrenzen nachgeregelt. In der Stromgrenze auch :-)
 * MaxPWMCount auf 7000 geändert 
 * PWM für TFT-LED auf 84kHz und 1000Counts verändert, damit es nicht pfeift.
 * 74: jetzt mit Personalisierung (so wie Seriennummer)
 * 75: der Integrator für die Nachregelung wurde verbessert. Zeile 544 bis ??? gehört aber noch überarbeiet!
 * Stellwerte sollen besser den Zielwert treffen (schon vor der Nachregelung). Die Nachregelung kann beim ersten einschalten 
 * ein anlaufen den Wandlers verhindern!
 * 
 * 
 */


//######################################################################################################
#define ENTPRELLMILLIS 300
#define GEMESSENE_2V5_REFERENZ 2500 //mV-Messung (händisch) der Refernzspannung
String thingName = "   EndzeitWandler ";//+Version                // Wie heißt das Ding
String runText =   "..........aktiv!.........";         // Anzeige wenn Wandler eingeschalten ist
String hiUser  =   "   *AKT2*";         // Persönliche Message am Hauptscreen

#define OFFSET_U1 0      //Offset für Spannungsmessung U1 in mV
#define FAKTOR_U1 1000      //Faktor für Spannungsmessung U1 in Promille
#define OFFSET_U2 0      //Offset für Spannungsmessung U1 in mV
#define FAKTOR_U2 1000      //Faktor für Spannungsmessung U1 in Promille
#define OFFSET_I1 0       //Offset für Strommessung I1 in mA
#define FAKTOR_I1 1000      //Faktor für Strommessung I1 in Promille
#define OFFSET_I2 0       //Offset für Strommessung I2 in mA
#define FAKTOR_I2 1000      //Faktor für Strommessung I2 in Promille
#include "E2WRevA_init.h"

#include <DueTimer.h>  //für einheitliche Abtastraten
#define ABTASTPERIODE 5000  //in Mikro-Sekunden
//######################################################################################################
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_HX8357.h"

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
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

//######################################################################################################
void setup() {
  initE2W();
  
  knobRight.init(39, 37);
  knobLeft.init(51, 50);    
  knobLeftMitte.init(48, 46);
  knobRightMitte.init(33, 32);
  knobUnten.init(25, 22);

  analogWriteResolution(12);
  analogReadResolution(12);

  tft.begin(HX8357D);
  tft.fillScreen(HX8357_BLACK);
  Serial.begin(115200);  // output
  //Serial.println(INPUT_PULLUP);
  inputString.reserve(200);
  valueString.reserve(20);
  Serial.println("*** Energy 2 Work ***");
  Serial.println(); promt();
  presetAnnKinnTonn();
  digitalWrite(LED4,LOW); //Weil sonst niemand das Licht ausmacht
  pinMode(14, OUTPUT);
 digitalWrite(14,LOW);
 //analogWrite(14, 2000);
  //pinMode(36, OUTPUT);
  //digitalWrite(36,HIGH);
  //analogWrite(2, 200);
  dcdcSTOP();
  
  Timer.getAvailable().attachInterrupt(timerInterruptProcedure).start(ABTASTPERIODE);
}
//######################################################################################################

void timerInterruptProcedure (){
 // digitalWrite(14,HIGH);

  adc_U1 = analogRead(ADC_U1);
  adc_U2 = analogRead(ADC_U2);
  adc_I1 = analogRead(ADC_I1);
  adc_I2 = analogRead(ADC_I2);
  adc_Iref = analogRead(ADC_IREF);
  adc_2V5ref = analogRead(ADC_2V5Ref);
  adc_NTC1 = analogRead(ADC_NTC1);
  adc_NTC2 = analogRead(ADC_NTC2);


  iirADC_U1 -= iirADC_U1/16;
  iirADC_U1 += adc_U1;
  iirADC_U2 -= iirADC_U2/16;
  iirADC_U2 += adc_U2;
  iirADC_I1 -= iirADC_I1/16;
  iirADC_I1 += adc_I1;
  iirADC_I2 -= iirADC_I2/16;
  iirADC_I2 += adc_I2;
  iirADC_Iref -= iirADC_Iref/16;
  iirADC_Iref += adc_Iref;
  iirADC_2V5ref -= iirADC_2V5ref/16;
  iirADC_2V5ref += adc_2V5ref;
  iirADC_NTC1 -= iirADC_NTC1/16;
  iirADC_NTC1 += adc_NTC1;
  iirADC_NTC2 -= iirADC_NTC2/16;
  iirADC_NTC2 += adc_NTC2;
  
  
  //uref_mV = GEMESSENE_2V5_REFERENZ * 65536 / iirADC_2V5ref;  //ADC-Spannung in mV
  
  //digitalWrite(14,LOW);
}

void loop() { 
  long p1,p2,i1,i2,iref,u2V5Ref;
  unsigned long u1, u2, uADC;

  int x; //Offset für tft.print zum rechts zentrieren
  int encoder1Val, encoder2Val, encoder5Val, encoder6Val, encoder7Val;
  int color = VIOLETTBLAU; //0x0111 dunkelblau

  //digitalWrite(14,HIGH);
  
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
    color = VIOLETTBLAU;
    if (digitalRead(RE_U1max) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(25+x, BOXSIZE + 10, 3, color, lastReportedPosE1*100, encoder1Val*100, 4, "V");
    tft.setTextSize(1); tft.setCursor(134, BOXSIZE + 24); tft.print("max");  
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
    color = VIOLETTBLAU;
    if (digitalRead(RE_U1min) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(25+x, BOXSIZE + BOXSIZE-31, 3, color, lastReportedPosE2*100, encoder2Val*100, 4, "V");
    tft.setTextSize(1); tft.setCursor(134, BOXSIZE + BOXSIZE-31+14); tft.print("min");
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
    color = VIOLETTBLAU;
    if (digitalRead(RE_U2max) == 0) color = HX8357_YELLOW; 
    tftTextWertUpdate(BOXSIZE+25+x, BOXSIZE + 10, 3, color, lastReportedPosE7*100, encoder7Val*100, 4, "V");
    tft.setTextSize(1); tft.setCursor(BOXSIZE+134, BOXSIZE + 24); tft.print("max");
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
    color = VIOLETTBLAU;
    if (digitalRead(RE_U2min) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(BOXSIZE+25+x, BOXSIZE + BOXSIZE-31, 3, color, lastReportedPosE6*100, encoder6Val*100, 4, "V");
    tft.setTextSize(1); tft.setCursor(BOXSIZE+134, BOXSIZE + BOXSIZE-31+14); tft.print("min");
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
    color = VIOLETTBLAU;
    if (digitalRead(RE_U2min) == 1 && digitalRead(RE_U2max) == 1 && digitalRead(RE_U1min) == 1 && digitalRead(RE_U1max) == 1) color = HX8357_YELLOW;
    tftTextWertUpdate(BOXSIZE+25+x,  3*BOXSIZE-31, 3, color, lastReportedPosE5*10, encoder5Val*10, 4, "A");
    tft.setTextSize(1); tft.setCursor(BOXSIZE+134, BOXSIZE*3-31+14); tft.print("soll");
    lastReportedPosE5 = encoder5Val;
  }

//Die Tasten der Encoder werden im Inaktiven Zustand dazu benutzt sinnvolle Standardwerte zu setzen
// die maximalSpannungsgrenzen auf 1V über die anliegende Spannung und die min-Grenzen auf 0V. Strom auch auf 0A
  if ((digitalRead(BUTTON_KNOP1) == LOW) && (runState == false)){
    u1 = analogRead(ADC_U1);
    u1 = map(u1,0, 4095, 0, 693); 
    knobLeft.write((u1+10)<<2);    
    //Serial.println("Taste1");
  }
  if ((digitalRead(BUTTON_KNOP2) == LOW ) && (runState == false)) {
    knobLeftMitte.write(0);    
    //Serial.println("Taste2");
  }
  if ((digitalRead(BUTTON_KNOP5) == LOW ) && (runState == false)) {
    knobUnten.write(0);    
    //Serial.println("Taste5");
  }
  if ((digitalRead(BUTTON_KNOP6) == LOW ) && (runState == false)) {
    knobRightMitte.write(0);    
    //Serial.println("Taste6");
  }
  if ((digitalRead(BUTTON_KNOP7) == LOW ) && (runState == false)) {
    u2 = analogRead(ADC_U2);
    u2 = map(u2,0, 4095, 0, 693); 
    knobRight.write((u2+10)<<2);    
    //Serial.println("Taste7");
  }
  if ((digitalRead(BUTTON_KNOP1) == LOW) && (digitalRead(BUTTON_KNOP7) == LOW))  {
    tft.fillScreen(HX8357_BLACK); //TFT
    Serial.println("ClearScreen");
    presetAnnKinnTonn();
  }


  //RUN/Stop-Taste***************************************************************************************************************************************
  int runTaste = digitalRead(RUN_TASTER);
  if (!runTaste && lastRunTaste && millis()-timestampRunTaste > ENTPRELLMILLIS){
    if (!runState) {
      dcdcRUN();
    }
    else {
      dcdcSTOP();
    }
    timestampRunTaste = millis();
  }
  lastRunTaste = runTaste;
  if (!digitalRead(DCDCENTRI) && runState) {
    runState = false;
    dcdcSTOP();
    };  //Wenn der Wandler wegen Überspannung abgeworfen wird

  if(runState) {
    digitalWrite(RUN_LED,HIGH);
    REG_PWM_CDTY5 = i2SollPWM;
 
  }else {
    digitalWrite(RUN_LED,LOW); 
    REG_PWM_CDTY5 = 0;

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
    int tmppt = millis() - mpptTimeStamp;
    if ((tmppt >= MpptTime) && (digitalRead(RE_U2min) == 0)) {
      mpptTimeStamp = millis();
      mppt(); //MPPT aktivieren
    }
    if ((digitalRead(BUTTON_KNOP5) == LOW ) && (runState == true)) {
      REG_PWM_CDTY1 = 1000;    //TFT-Backlight
    } else {
      REG_PWM_CDTY1 = 870;    //TFT-Backlight
    }
  
    //Taste4LED soll blinken wenn taste4State aktiv ist
    blink(Taster4BlinONkTime, Taster4BlinOFFkTime, LED4);
        
  }else {
    //do something dauerhaft bis zum nächsten Tastendruck
    digitalWrite(LED4,LOW); 
    REG_PWM_CDTY1 = 950;    //TFT-Backlight 
     //pinMode(3, INPUT);
     //pinMode(4, INPUT);
    
  }

//**************************************** Reglererkennung ************************************************************************************  
int regErkNew = digitalRead(RE_U1min)*8 + digitalRead(RE_U1max)*4 + digitalRead(RE_U2min)*2 + digitalRead(RE_U2max);
if (regErkNew != ReglerErkennung){
    color = VIOLETTBLAU;
    if (digitalRead(RE_U1max) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(25+x, BOXSIZE + 10, 3, color, lastReportedPosE1*100, encoder1Val*100, 4, "V");
    //tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(134, BOXSIZE + 24);
    tft.print("max");  

    color = VIOLETTBLAU;
    if (digitalRead(RE_U1min) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(25+x, BOXSIZE + BOXSIZE-31, 3, color, lastReportedPosE2*100, encoder2Val*100, 4, "V");
  //  tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(134, BOXSIZE + BOXSIZE-31+14);
    tft.print("min");
    

    color = VIOLETTBLAU;
    if (digitalRead(RE_U2max) == 0) color = HX8357_YELLOW; 
    tftTextWertUpdate(BOXSIZE+25+x, BOXSIZE + 10, 3, color, lastReportedPosE7*100, encoder7Val*100, 4, "V");
   // tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(BOXSIZE+134, BOXSIZE + 24);
    tft.print("max");

    color = VIOLETTBLAU;
    if (digitalRead(RE_U2min) == 0) color = HX8357_YELLOW;
    tftTextWertUpdate(BOXSIZE+25+x, BOXSIZE + BOXSIZE-31, 3, color, lastReportedPosE6*100, encoder6Val*100, 4, "V");
   // tft.print("V");
    tft.setTextSize(1);
    tft.setCursor(BOXSIZE+134, BOXSIZE + BOXSIZE-31+14);
    tft.print("min");

    color = VIOLETTBLAU;
    if (digitalRead(RE_U2min) == 1 && digitalRead(RE_U2max) == 1 && digitalRead(RE_U1min) == 1 && digitalRead(RE_U1max) == 1) color = HX8357_YELLOW;
    tftTextWertUpdate(BOXSIZE+25+x, 3*BOXSIZE-31, 3, color, lastReportedPosE5*10, encoder5Val*10, 4, "A");
   // tft.print("A");
    tft.setTextSize(1);
    tft.setCursor(BOXSIZE+134, BOXSIZE*3-31+14);
    tft.print("soll");

    /////
    
}
ReglerErkennung = regErkNew;

noInterrupts();
 uADC = GEMESSENE_2V5_REFERENZ * 65536 / iirADC_2V5ref;  //ADC-Spannung in mV   
interrupts();
//*********************ADC Anzeige ***********************************************
  int t = millis() - TFTadcUpdateTimeStamp;
  if (t >= TFTadcUpdateTime) {
    TFTadcUpdateTimeStamp = millis();

   noInterrupts();                                  //ADC-Werte abrufen, diese werden im Timer-Interrupt beschrieben
    u1 = map(iirADC_U1,0, 65535, 0,  uADC*21/10);   
    u2 = map(iirADC_U2,0, 65535, 0, uADC *21/10); 
    i1 = map(iirADC_I1,0, 65535, 0, uADC*50/9);
    i2 = map(iirADC_I2,0, 65535, 0, uADC*50/9);
    iref = map(iirADC_Iref,0, 65535, 0, uADC*50/9);
    int newNTC1 = adc_NTC1;
    int newNTC2 = adc_NTC2;
   interrupts();
   
    u1 = u1 * FAKTOR_U1/100 + OFFSET_U1;            //U1 Spannungsmessung-Kalibrierung
    u2 = u2 * FAKTOR_U2/100 + OFFSET_U2;            //U2 Spannungsmessung-Kalibrierung
    i1 = (iref - i1) * FAKTOR_I1/1000 + OFFSET_I1;  //I1 Stromsmessung-Kalibrierung      
    i2 = (i2 - iref) * FAKTOR_I2/1000 + OFFSET_I2;  //I2 Stromsmessung-Kalibrierung 
    p1 = (long) u1 * i1 / 1000;
    p2 = (long) u2 * i2 / 1000;
    newNTC1 = (3520-newNTC1)*1000/24;
    newNTC2 = (3520-newNTC2)*1000/24;

    lastP1 = tftTextWertUpdate(20, BOXSIZE/2-14-40, 3, HX8357_MAGENTA, lastP1, p1,5, "W");
    lastU1 = tftTextWertUpdate(10, BOXSIZE + BOXSIZE/2-14, 4, HX8357_BLUE, lastU1, u1,4, "V");
    lastI1 = tftTextWertUpdate(10, BOXSIZE + BOXSIZE/2-14+BOXSIZE, 4, HX8357_RED, lastI1, i1,4, "A");
   
    lastP2 = tftTextWertUpdate(20+BOXSIZE,  BOXSIZE/2-14-40, 3, HX8357_MAGENTA, lastP2, p2,5, "W");
    lastU2 = tftTextWertUpdate(10+BOXSIZE, BOXSIZE + BOXSIZE/2-14, 4, HX8357_BLUE, lastU2, u2,4, "V");                         
    lastI2 = tftTextWertUpdate(10+BOXSIZE, BOXSIZE + BOXSIZE/2-14+BOXSIZE, 4, HX8357_RED, lastI2, i2,4, "A");

    lastNTC1 = tftTextWertUpdate(40,BOXSIZE/3+5, 1, 0xAAAA,lastNTC1,newNTC1, 4, " GC NTC1");
    lastNTC2 = tftTextWertUpdate(BOXSIZE + 45,BOXSIZE/3+5, 1, 0xAAAA,lastNTC2,newNTC2, 4, " GC NTC2");
  
     if ((digitalRead(RE_U2max) == 0) && runState)  {
        nachRegelungU2Max -= u2 - encoder7Val*100;
//          Serial.print(encoder7Val*100);Serial.print(" U2:");Serial.print(u2);
//          Serial.print("  Nachregelung U2max um:");Serial.println(nachRegelungU2Max);
//          if(encoder7Val*10 == round((float) u2/10)) Serial.println("supi");
        if(nachRegelungU2Max > 4000) nachRegelungU2Max=4000;
        if(nachRegelungU2Max < -4000) nachRegelungU2Max = -4000;
        
      }
      if ((digitalRead(RE_U1max) == 0) && runState)  {
        
        nachRegelungU1Max -= u1 - encoder1Val*100;
//          Serial.print(encoder1Val*100);Serial.print(" U1:");Serial.print(u1);
//          Serial.print("  Nachregelung U1max um:");Serial.println(nachRegelungU1Max);
//          if(encoder1Val*10 == round((float) u1/10)) Serial.println("supi");
        if(nachRegelungU1Max > 4000) nachRegelungU1Max=4000;
        if(nachRegelungU1Max < -4000) nachRegelungU1Max = -4000;
      }
      if ((digitalRead(RE_U2min) == 0) && runState)  {    
        nachRegelungU2Min -= u2 - encoder6Val*100;
//        Serial.print(encoder6Val*100);Serial.print(" U2:");Serial.print(u2);    
//        Serial.print("  Nachregelung U2min um:");Serial.println(nachRegelungU2Min);
//        if(encoder7Val*10 == round((float) u2/10)) Serial.println("supi");
        if(nachRegelungU2Min > 4000) nachRegelungU2Min=4000;
        if(nachRegelungU2Min < -4000) nachRegelungU2Min = -4000;        
      }
      if ((digitalRead(RE_U1min) == 0) && runState)  {
       // Serial.print(encoder1Val*100);Serial.print(" U1:");Serial.print(u1);        
        nachRegelungU1Min -= u1 - encoder2Val*100;
        //Serial.print("  Nachregelung U1min um:");Serial.println(nachRegelungU1Min);       
        //if(encoder2Val*10 == round((float) u1/10)) Serial.println("supi");
        if(nachRegelungU1Min > 4000) nachRegelungU1Min=4000;
        if(nachRegelungU1Min < -4000) nachRegelungU1Min = -4000;       
      }
      if (digitalRead(RE_U2min) == 1 && digitalRead(RE_U2max) == 1 && digitalRead(RE_U1min) == 1 && digitalRead(RE_U1max) == 1 && runState){
        //Serial.print(encoder5Val*10);Serial.print(" I2:");Serial.print(i2);        
        nachRegelungI2Soll -= i2 - encoder5Val*10;
        //Serial.print("  Nachregelung I2soll um:");Serial.println(nachRegelungI2Soll);       
        //if(encoder5Val*10 == round((float) i2)) Serial.println("supi");
        if(nachRegelungI2Soll > 1000) nachRegelungI2Soll = 1000;
        if(nachRegelungI2Soll < -1000) nachRegelungI2Soll = -1000;
      }    
 }    
 

//******************************************PWM Settings**********************************************  
 // REG_PWM_CDTY2 = 2000;    //TFT-Backlight
 // if (REG_PWM_CDTY2 > 6000) REG_PWM_CDTY2 = 0;

  int kalibrierterPWMU1Max= map(encoder1Val, 0, 2*uADC, 0 , MaxPWMCount*10) + nachRegelungU1Max/20;
  if (kalibrierterPWMU1Max < 0) kalibrierterPWMU1Max =0;  
  REG_PWM_CDTY3 = kalibrierterPWMU1Max;

  int kalibrierterPWMU1Min= map(encoder2Val, 0, 2*uADC, 0 , MaxPWMCount*10) + nachRegelungU1Min/20;
  if (kalibrierterPWMU1Min < 0) kalibrierterPWMU1Min =0;  
  REG_PWM_CDTY2 = kalibrierterPWMU1Min;
    
  int kalibrierterPWMU2Max= map(encoder7Val, 0, 2*uADC, 0 , MaxPWMCount*10) + nachRegelungU2Max/20;
  if (kalibrierterPWMU2Max < 0) kalibrierterPWMU2Max =0;   //verhindern daß Wert negativ wird
  REG_PWM_CDTY6 = kalibrierterPWMU2Max;  
  
  int kalibrierterPWMU2Min= map(encoder6Val, 0, 2*uADC, 0 , MaxPWMCount*10) + nachRegelungU2Min/20;
  if (kalibrierterPWMU2Min < 0) kalibrierterPWMU2Min =0;  
  REG_PWM_CDTY7 = kalibrierterPWMU2Min;

  //Der Nullpunkt wird verschoben um einen größeren Strombereich messen zu können. Auf die laufende Wandlerregelung hat das keinen Einfluss, da 
  //in Hardware die Differenz aus Strommesswert und Nullpunktwert gebildet wird. 
  t = millis() - sumUpStartTime;
  if (t >= SumUpIntervalTime) {
    sumUpStartTime = millis();

    
    if (abs(lastI2) <= 3000)pwmZielwert = MaxPWMCount/2;      
    if (lastI2 > 4000) pwmZielwert = MaxPWMCount/4;     //Anpassung für "foo" damit kaputter Sensor noch geht!
    if (lastI2 < -4000) pwmZielwert = MaxPWMCount*7/8; 
    
    NullpunktPWM = NullpunktPWM - (NullpunktPWM-pwmZielwert)/16;
    if (NullpunktPWM <= MaxPWMCount / 8) NullpunktPWM = MaxPWMCount / 4;   //Anpassung für "foo" damit kaputter Sensor noch geht!
    if (NullpunktPWM >= MaxPWMCount *7 / 8) NullpunktPWM = MaxPWMCount *7 / 8;
  //Serial.print("Nullpunkt: ");Serial.print(NullpunktPWM);Serial.print("   i2: ");Serial.println(lastI2);
  }
  
  REG_PWM_CDTY4 = NullpunktPWM;  //nur mit Umbau, sonst sollte immer genau auf die Hälfte
  i2SollPWM = map(encoder5Val, -916, 916, NullpunktPWM - MaxPWMCount/2 , NullpunktPWM + MaxPWMCount/2) + nachRegelungI2Soll/5; //Stromstellwert 
  if (i2SollPWM <= 0) i2SollPWM = 0;
  if (i2SollPWM >= MaxPWMCount) i2SollPWM = MaxPWMCount;
//  if(runState) {                                     //steht weiter oben bei RUN-Taste
//    REG_PWM_CDTY5 = i2SollPWM;   //nur im Runstate
//  }



  //--------------------------------------serial control -----------------------------------------------
  serialEvent(); //call the function
  // print the string when a newline arrives:
  if (stringComplete) {
    parseString();
    // clear the string:
    inputString = "";
    valueString = "";
    stringComplete = false;
    nachTrennzeichen = false;
  }


//digitalWrite(14,LOW);
delay(1);
  
}
//######################################################################################################



void mppt(void){
  long powerPoint = (long) lastU2 * lastI2;
  int mpptU2min = (knobRightMitte.read()>>2)*100;
  if (powerPoint > lastPowerPoint) mpptUpDown = !mpptUpDown; //Leistung soll ja negativ maximal werden (aufgenommene Leistung)
  if (mpptUpDown){
    mpptU2min += MPPT_STEP; 
  }else{
    mpptU2min -= MPPT_STEP; 
  }
  knobRightMitte.write(mpptU2min*4/100);
  lastPowerPoint = powerPoint;
}
/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    char inChar = Serial.read();
    Serial.print(inChar);
    inputString += inChar;
    if (inChar == ' ') nachTrennzeichen = true;
    if (nachTrennzeichen && ((isDigit(inChar) || (inChar == '-')))) {
      valueString+= (char)inChar;
    }
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void parseString(void){
  //Serial.print(inputString);
  //Serial.println(valueString);

  if (inputString.length() == 2) {
   // Serial.print("******************************************************");
    promt();
  }
  else if (inputString.startsWith("?") || inputString.startsWith("help")) {
    Serial.println("******************** help ****************************");
    Serial.println("Befehle zum einstellen oder auslesen:");
    Serial.println("******************************************************");
    Serial.println("i2soll [+/-Wert in mA]..................Strom-Sollwert");
    Serial.println("u1max [Wert in mV]......................U1-Maximalwert");
    Serial.println("u1min [Wert in mV]......................U1-Minimalwert");
    Serial.println("u2max [Wert in mV]......................U2-Maximalwert");
    Serial.println("u2min [Wert in mV]......................U2-Minimalwert");
    Serial.println("endcdc [0 oder 1]..................OFF/ON DCDC-Wandler");
    Serial.println("tftpwm [Wert 0..6000]..........PWM der TFT Beleuchtung");
    Serial.println("statusLED [0 oder 1]..........OFF/ON Status-Taster-LED");
    Serial.println("******************************************************");
    Serial.println("Befehle ohne Wertangabe antworten den aktuellen Wert");
    Serial.println("folgende Parameter können nur ausgelesen werden:");
    Serial.println("******************************************************");
    Serial.println("i1...............................Messwert des Strom I1");
    Serial.println("i2...............................Messwert des Strom I2");
    Serial.println("u1............................Messwert der Spannung U1");
    Serial.println("u2............................Messwert der Spannung U2");
    Serial.println("uiref...........Messwert der Nullpunkt-Vorgabespannung");
    Serial.println("vref.......Messwert der Externen Referenzspannung 2,5V");
    Serial.println("t1...................Temperatur in GC des Sensors NTC1");
    Serial.println("t2...................Temperatur in GC des Sensors NTC2");
    
    Serial.println("******************************************************");
    Serial.println("Kleinschreibung beachten!(z.B. nicht Endcdc 0)");
      Serial.print("******************************************************");
    Serial.println(); promt();
  }
//  else if (inputString.startsWith("ver")) {
//    Serial.print("Hardware: E2W RevA  Software: "); Serial.print(valueString.toInt()); Serial.print("mA gesetzt");
//    promt();
//  }
  else if (inputString.startsWith("i2soll")) {
    if(inputString.length() <= 8) {//mit NewLine & CR oder eben nur NL
      Serial.print((knobUnten.read()>>2)*10); 
    }
    else {
      Serial.print("i2soll wird auf "); Serial.print(valueString.toInt()); Serial.print("mA gesetzt");
      knobUnten.write(valueString.toInt()*4/10);
    }
    Serial.println(); promt();
  }
  else if (inputString.startsWith("u1max")) {
    if(inputString.length() <= 7) {//mit NewLine & CR oder eben nur NL
      Serial.print((knobLeft.read()>>2)*100); 
    }
    else{
      Serial.print("u1max wird auf "); Serial.print(valueString.toInt()); Serial.print("mV gesetzt");
      knobLeft.write(valueString.toInt()*4/100);
    }
    Serial.println(); promt();
  }
  else if (inputString.startsWith("u1min")) {
    if(inputString.length() <= 7) {//mit NewLine & CR oder eben nur NL
      Serial.print((knobLeftMitte.read()>>2)*100); 
    }
    else{
      Serial.print("u1min wird auf "); Serial.print(valueString.toInt()); Serial.print("mV gesetzt");
      knobLeftMitte.write(valueString.toInt()*4/100);
    }
    Serial.println(); promt();
  }
  else if (inputString.startsWith("u2max")) {
    if(inputString.length() <= 7) {//mit NewLine & CR oder eben nur NL
      Serial.print((knobRight.read()>>2)*100); 
    }
    else{
      Serial.print("u2max wird auf "); Serial.print(valueString.toInt()); Serial.print("mV gesetzt");
      knobRight.write(valueString.toInt()*4/100);
    }
    Serial.println(); promt();
  }
  else if (inputString.startsWith("u2min")) {
    if(inputString.length() <= 7) {//mit NewLine & CR oder eben nur NL
      Serial.print((knobRightMitte.read()>>2)*100); 
    }
    else{
      Serial.print("u2min wird auf "); Serial.print(valueString.toInt()); Serial.print("mV gesetzt");
      knobRightMitte.write(valueString.toInt()*4/100);
    }
    Serial.println(); promt();
  }
  else if (inputString.startsWith("endcdc")) {
    if(inputString.length() <= 8) {//mit NewLine & CR oder eben nur NL
      Serial.print("der aktuelle Wert von endcdc wird ausgegeben "); 
    }
    else{
      if(valueString.toInt()){
        Serial.print("Der DCDC-Wandler wird eingeschaltet "); 
        dcdcRUN();
      }else {
        Serial.print("Der DCDC-Wandler wird ausgeschaltet ");
        dcdcSTOP();
      }
    }
    Serial.println(); promt();
  }
  else if (inputString.startsWith("i1")) {
    Serial.print(lastI1); //Serial.print(valueString.toInt()); Serial.print("mV gesetzt");
    Serial.println(); promt();
  }
  else if (inputString.startsWith("i2")) {
    Serial.print(lastI2); //Serial.print(valueString.toInt()); Serial.print("mV gesetzt");
    Serial.println(); promt();
  }
  else if (inputString.startsWith("u1")) {
    Serial.print(lastU1); //Serial.print(valueString.toInt()); Serial.print("mV gesetzt");
    Serial.println(); promt();
  }
  else if (inputString.startsWith("u2")) {
    Serial.print(lastU2); //Serial.print(valueString.toInt()); Serial.print("mV gesetzt");
    Serial.println(); promt();
  }
  else if (inputString.startsWith("test1")) {
    Serial.print("u2max wird auf "); Serial.print("12"); Serial.println("V gesetzt");
    knobRight.write(120*4);
    Serial.print("i2soll wird auf "); Serial.print("2"); Serial.print("A gesetzt");
    knobUnten.write(200*4);
    
   
    Serial.println(); promt();
  }
  else if (inputString.startsWith("statusLED")) {
    if(inputString.length() <= 11) {//mit NewLine & CR oder eben nur NL
      Serial.print(digitalRead(LED4)); 
    }
    else{
      if(valueString.toInt()){
        Serial.print("Die LED des Status-Tasters wird eingeschaltet "); 
        digitalWrite(LED4, HIGH);
      }else {
        Serial.print("Die LED des Status-Tasters wird ausgeschaltet "); 
        digitalWrite(LED4, LOW);
      }
    }
    Serial.println(); promt();
  }
  else {
    Serial.print("type: ? or: help");
    Serial.println(); promt();
  }
}

void promt(void){
  //Serial.println();
  Serial.print("E2W V");Serial.print((float)VERSION/100);Serial.print("> ");
}

void tftPrintFloatMilli(int x, int y, int Textsize, int Color,long WertInMilliEinheiten, char* Einheit, int Offset){
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
    tft.print((float) WertInMilliEinheiten / 1000);
    tft.print(Einheit);
}

long tftTextWertUpdate(int x, int y, int Textsize, int Color,long lastWert,long newWert, int digits, char* Einheit){
 long aktuellerText, xOff;

    if (digits == 4) xOff = -2;
    if (digits == 5) xOff = -1;
    
    tftPrintFloatMilli(x, y, Textsize, HX8357_BLACK, lastWert, Einheit, xOff);
    tftPrintFloatMilli(x, y, Textsize, Color, newWert, Einheit, xOff);
    
    aktuellerText = newWert;
    return aktuellerText;
}

void dcdcRUN(void){
  pinMode(DCDCENTRI, OUTPUT);
  
  digitalWrite(DCDCENTRI,HIGH);
  delayMicroseconds(10);
  pinMode(DCDCENTRI, INPUT);
  runState = true;
  tft.drawRect(7, BOXSIZE/3+20, 2*BOXSIZE-14, BOXSIZE*2/3-25, 0xD1CC);
  tft.setCursor(11,BOXSIZE/2+25);
  tft.setTextSize(2);
  tft.setTextColor(HX8357_RED);
  tft.print(runText); 
}

void dcdcSTOP(void){
  digitalWrite(DCDCENTRI,LOW);
  pinMode(DCDCENTRI, OUTPUT);
  digitalWrite(DCDCENTRI,LOW);
  runState = false;
  tft.drawRect(7, BOXSIZE/3+20, 2*BOXSIZE-14, BOXSIZE*2/3-25, HX8357_BLACK);
  tft.setCursor(11,BOXSIZE/2+25);
  tft.setTextSize(2);
  tft.setTextColor(HX8357_BLACK);
  tft.print(runText); 
}

void blink(int onMillis, int offMillis, int port){
  int t;
  if (digitalRead(port)){
    t = millis() - taster4BlinkONTimeStamp;
    if (t >= onMillis) {
      taster4BlinkOFFTimeStamp = millis();
      digitalWrite(port,LOW); 
    }
  }else{
    t = millis() - taster4BlinkOFFTimeStamp;
    if (t >= offMillis) {
      taster4BlinkONTimeStamp = millis();
      digitalWrite(port,HIGH); 
    }
  }
  
  
 
  
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

    tft.setCursor(11,BOXSIZE/2);
    tft.setTextSize(2);
    tft.setTextColor(0x7542);
    tft.print(thingName); tft.print((float) VERSION/100);
    tft.setCursor(11,BOXSIZE*4/5+4);
    tft.print(hiUser);
}


