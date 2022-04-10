/*************************************************************************************
 SDR++ Controller 
 by Bartłomiej Marcinkowski

 Designed and tested on:
 - Arduino Leonardo
 - DFRobot DFR0009 LCD + Keys shield 
 - DFRobot EC11 rotary encoder button x2 

 Encoder support based on source code taken from DFRobot site: 
 https://wiki.dfrobot.com/EC11_Rotary_Encoder_Module_SKU__SEN0235 (no author info)

 LCD and Keys support based on source code taken from Botland store site: 
 https://botland.com.pl/arduino-shield-klawiatury-i-wyswietlacze/2729-dfrobot-lcd-keypad-shield-v11-wyswietlacz-dla-arduino-5903351243780.html
 by Mark Bramwell, July 2010

 Greetings to RTL-SDR Polska group @ Facebook
 https://www.facebook.com/groups/2628926590655863
**************************************************************************************/

#include <LiquidCrystal.h>
#include <stdlib.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);           // select the pins used on the LCD panel

// define some values used by the panel and buttons (and knobs)
int lcd_key     = 0;
int adc_key_in  = 0;
int isbtnPressed = 0;
int incomingByte = 0;
int startCommand = 0;
int bytePos = 0;
int adc_value = 0;
int knobCommand = 0;
int knobKey = 0;
int dSNR = 0;
int lastSNR = 0;

int coolDown = 0;

int encoderPinA = 0;
int encoderPinB = 1;
int buttonPin = 11;

int encoderPinC = 2;
int encoderPinD = 3;

int ledPin = 13;

//left encoder knob
volatile int lastEncodedL = 0;
//long lastencoderValueL = 0;
int lastMSBL = 0;
int lastLSBL = 0;

//right encoder knob
volatile int lastEncodedR = 0;
//long lastencoderValueR = 0;
int lastMSBR = 0;
int lastLSBR = 0;

//volatile long encoderValueL = 0;
//volatile long encoderValueR = 0;

//unsigned long currentReadtime = 0;
//unsigned long lastReadtime = 0;

char freq[16] = "0";
char snr[5] = "0";


#define inputNONE  0

#define knobRIGHT 1
#define knobUP    2
#define knobDOWN  3
#define knobLEFT  4

#define btnSELECT 5
#define btnRIGHT  6
#define btnUP     7
#define btnDOWN   8
#define btnLEFT   9
#define btnKNOB   0xa

#define commandCTRL   67 // C - general command for extra actions
#define commandDISP   68 // D - display debug message 
#define commandFREQ   70 // F - display frequency
#define commandMODE   77 // M - display mode
#define commandRST    82 // R - reset
#define commandSNR    83 // S - SNR level
#define commandEOL    10 // 0x0a - std end of line

enum modes_t {
    NFM,
    WFM,
    AM,
    DSB,
    USB,
    CW,
    LSB,
    RAW
};

char *modeStr[8] = {
  "NFM","WFM","AM","DSB","USB","CW","LSB","RAW"
};

modes_t dMode;

int read_knob_button() {
  if(!digitalRead(buttonPin)) {
    return btnKNOB;
  } else {
    return inputNONE;
  }
}

int read_LCD_buttons(){               // read the buttons
    adc_key_in = analogRead(0);       // read the value from the sensor 

    // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
    // we add approx 50 to those values and check to see if we are close
    // We make this the 1st option for speed reasons since it will be the most likely result

    if (adc_key_in > 1000) return inputNONE; 

    // For V1.1 us this threshold
    if (adc_key_in < 50)   return btnRIGHT;  
    if (adc_key_in < 250)  return btnUP; 
    if (adc_key_in < 450)  return btnDOWN; 
    if (adc_key_in < 650)  return btnLEFT; 
    if (adc_key_in < 850)  return btnSELECT;  

   // For V1.0 comment the other threshold and use the one below:
   /*
     if (adc_key_in < 50)   return btnRIGHT;  
     if (adc_key_in < 195)  return btnUP; 
     if (adc_key_in < 380)  return btnDOWN; 
     if (adc_key_in < 555)  return btnLEFT; 
     if (adc_key_in < 790)  return btnSELECT;   
   */
    return inputNONE;                // when all others fail, return this.
}

//long readEncoderValue(void){  // not used , taken from DFRobot example 
//    return encoderValue/4;    // we are not counting impulses 
//}


// void(* resetFunc) (void) = 0;
// ^^ this works different on Leonardo. Serial waits for disconnection? not sure how to handle that

void readCommand() {
    char rchar;
    if (Serial.available() > 0) {
        incomingByte = Serial.read();

        if (incomingByte == commandEOL){
            startCommand = 0;
            bytePos = 0;
        }

       switch (startCommand) {
            case 0:
                break;
            case commandFREQ:
                if(bytePos < sizeof(freq) - 1) {
                    freq[bytePos] = incomingByte;
                    bytePos++;
                }
                //Serial.println(freq);
                break;
            case commandSNR:
                if(bytePos < sizeof(snr) - 1) {
                    snr[bytePos] = incomingByte;
                    bytePos++;
                }
                //Serial.println(freq);
                break;
            case commandMODE:
                dMode = char(incomingByte) - '0';
                if(dMode > 7 || dMode < 0) {
                  Serial.println("D Wrong mode");
                  dMode = 0;
                } else {
                    Serial.println(modeStr[dMode]);
                }
                break;
            default:
                break;
        }


        if(startCommand == 0) {
            switch(incomingByte){
                case commandRST:
                    //Serial.println("D Reset command received");
                    delay(200);
                    Serial.println("D SDR++ Controller init");
                    //resetFunc(); // again this doesn't work on Leonardo
                    break;
                case commandFREQ:
                    //Serial.println("D Receinve frequency start");
                    memset(freq,0,sizeof(freq));
                    startCommand = commandFREQ;
                    //delay(200);
                    break;
                case commandMODE:
                    startCommand = commandMODE;
                    break;
                case commandSNR:
                    memset(snr,0,sizeof(snr));
                    startCommand = commandSNR;
                    break;
                case commandEOL:
                    //Serial.println("D EOL");
                    break;
                default:
                    Serial.println("D Unknown command");
                    Serial.println(incomingByte, DEC);
                    Serial.println(incomingByte);

            }
        }
        //Serial.print("I received: ");
        //Serial.println(incomingByte, DEC);
    }
}

// This part is a modified example code from DFRobot site.
// Adjusted to handle two knobs at the same time.
void updateEncoder(){

  int MSBR = digitalRead(encoderPinA); //MSB = most significant bit
  int LSBR = digitalRead(encoderPinB); //LSB = least significant bit

  int encodedR = (MSBR << 1) |LSBR; //converting the 2 pin value to single number
  int sumR  = (lastEncodedR << 2) | encodedR; //adding it to the previous encoded value
  
  if(sumR == 0b1101 || sumR == 0b0100 || sumR == 0b0010 || sumR == 0b1011) { 
      knobCommand = knobRIGHT;
  }
  if(sumR == 0b1110 || sumR == 0b0111 || sumR == 0b0001 || sumR == 0b1000) { 
      knobCommand = knobLEFT;
  }
  lastEncodedR = encodedR; //store this value for next time

  int MSBL = digitalRead(encoderPinC); //MSB = most significant bit
  int LSBL = digitalRead(encoderPinD); //LSB = least significant bit

  int encodedL = (MSBL << 1) |LSBL; //converting the 2 pin value to single number  
  int sumL  = (lastEncodedL << 2) | encodedL; //adding it to the previous encoded value
  if(sumL == 0b1101 || sumL == 0b0100 || sumL == 0b0010 || sumL == 0b1011) { 
      knobCommand = knobUP;
  }
  if(sumL == 0b1110 || sumL == 0b0111 || sumL == 0b0001 || sumL == 0b1000) { 
      knobCommand = knobDOWN;
  }

  lastEncodedL = encodedL; //store this value for next time

  if (coolDown < 10) { // this strange, added to fix garbage on boot, I don't like it
      coolDown++;
      knobCommand = inputNONE;
  }
}

void setup(){
  Serial.begin(9600);
  while (!Serial) {
    Serial.println("D SDR++ Controller init");
  }
  pinMode(encoderPinA, INPUT);
  pinMode(encoderPinB, INPUT);
  pinMode(encoderPinC, INPUT);
  pinMode(encoderPinD, INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin,OUTPUT); // led SNR

 
  digitalWrite(encoderPinA, INPUT_PULLUP); //turn pullup resistor on
  digitalWrite(encoderPinB, INPUT_PULLUP); //turn pullup resistor on

  digitalWrite(encoderPinC, INPUT_PULLUP); //turn pullup resistor on
  digitalWrite(encoderPinD, INPUT_PULLUP); //turn pullup resistor on

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3)
  // This works well for UNO with one knob.  Unfortunately there are only
  // two interrupts and we have to hande four of encoder lines. So... 
  // We do not use interrupts at all and just call updateEncoder() directly.
  // This version is for Leonardo so we can use interrupts!
  attachInterrupt(0, updateEncoder, CHANGE);
  attachInterrupt(1, updateEncoder, CHANGE);
  attachInterrupt(2, updateEncoder, CHANGE);
  attachInterrupt(3, updateEncoder, CHANGE);

   lcd.begin(16, 2);
   lcd.setCursor(0,0);
   lcd.print("SDR++ Controller");
   delay(1000);
   lcd.clear();
}

void loop(){

  readCommand();
  lcd.setCursor(0,0);
  lcd.print("F: "); 
  lcd.print(freq); 
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print("[");
  lcd.print(modeStr[dMode]);
  lcd.print("] ");
  lcd.setCursor(6,5);
  lcd.print("SNR: ");
  lcd.print(snr);
  lcd.print("     ");
  
  // update SNR led
  dSNR = atoi(snr);

  if (dSNR < 12) dSNR = 0; 
  else if (dSNR > 90) dSNR = 90;
  
  if (dSNR < lastSNR) {
      dSNR = lastSNR - 1; // just slow down with fade out 
  }
  lastSNR = dSNR;
  int pwmvalue = (dSNR * 255) / 90;
  analogWrite(ledPin,pwmvalue);
  
  
  lcd_key = read_LCD_buttons();
  knobKey = read_knob_button();
  //currentReadtime = millis();

  //updateEncoder(); // this should be called by interrupt

  if (lcd_key != inputNONE && isbtnPressed == 0) {
      Serial.write(commandCTRL);
      Serial.println(lcd_key);  
      Serial.flush();
      isbtnPressed = lcd_key;
  } else if (knobCommand != inputNONE) {
      Serial.write(commandCTRL);
      Serial.println(knobCommand);  
      Serial.flush(); 
      knobCommand = inputNONE;
  } else if (knobKey != inputNONE && isbtnPressed == 0) {
      Serial.write(commandCTRL);
      Serial.println(knobKey,HEX);  
      Serial.flush(); 
      isbtnPressed = knobKey;
  } else if (lcd_key == inputNONE && knobKey == inputNONE && isbtnPressed != 0) {
      isbtnPressed = 0;
   }

  
// This part may be used to handle extra actions for buttons.
// Left as it was in the original source sode
/*
 switch (lcd_key){               // depending on which button was pushed, we perform an action
       case btnRIGHT:{             //  push button "RIGHT" and show the word on the screen
            lcd.print("RIGHT ");
            break;
       }
       case btnLEFT:{
             lcd.print("LEFT   "); //  push button "LEFT" and show the word on the screen
             break;
       }    
       case btnUP:{
             lcd.print("UP    ");  //  push button "UP" and show the word on the screen
             break;
       }
       case btnDOWN:{
             lcd.print("DOWN  ");  //  push button "DOWN" and show the word on the screen
             break;
       }
       case btnSELECT:{
             lcd.print("SELECT");  //  push button "SELECT" and show the word on the screen
             break;
       }
       case btnNONE:{
             lcd.print("NONE  ");  //  No action  will show "None" on the screen
             break;
       }
   }
*/
}
