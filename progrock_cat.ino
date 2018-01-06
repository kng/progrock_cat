/*  progrock_cat
 *  Basic rig control of qrp-labs progrock 1.01a
 *  By: Daniel Ekman SA2KNG <knegge@gmail.com>
 *  
 *  Tested on Arduino UNO
 *  Emulates Kenwood rig with a subset of it's CAT commands.
 *  Implements VFO A/B wired to the progrock's bank selection
 *  
 *  Use the bridged voltage regulator to power it from the arduino if you want, if the progrock is powered in some rig you do not need to connect the 5V from the arduino.
 *  Solder leads to the RX+TX on the progrock and wire RX to 10, TX to 11, B0 to 12, GND to GND
 *  See operating manual page 5
 *  
 *  version 1.0 2018-01-06
 */

#include <SoftwareSerial.h>
#define BUFFLEN 40
#define VFOPIN 12

SoftwareSerial mySerial(10, 11); // RX, TX

// rx = CAT commands from PC, pr = progrock communication
unsigned char rxbuf[BUFFLEN], prbuf[BUFFLEN];
unsigned int rxpos, prpos, opmode=2;
bool rxcomplete, prcomplete, vfoab;

void setup() {
  pinMode(VFOPIN, OUTPUT); // VFO A/B
  digitalWrite(VFOPIN, !vfoab); // out HIGH = A
  Serial.begin(9600);
  //while (!Serial) { ; } // wait for host to connect
  //Serial.println(F("Kenwood CAT"));
  mySerial.begin(9600);
  rxclear();
  prclear();
}

void loop() {
  int i;
  byte rxs;

  if (mySerial.available()) { // softserial polling
    i = mySerial.read();
    if(prpos<BUFFLEN && !prcomplete && i != ','){ // filter out irrelevant stuff ?
      prbuf[prpos++]=(char)i;
    }
    if(i == '\r' || rxpos>=BUFFLEN){
      prcomplete = true;
    }
  }

  if(prcomplete){ // complete sentance from the progrock
/*    Serial.print(F("PR Ans: ")); // debugging output
    for(i=0;i<prpos;i++){
      Serial.write(prbuf[i]);
    }
    Serial.println(';');*/
    if(prbuf[0]=='4' && prbuf[1]==':'){ // answer on get/set VFO A
      Serial.print(F("FA"));
      for(i=0;i<=13-prpos;i++){ // pad with zeroes
        Serial.write('0');
      }
      for(i=2;i<prpos;i++){
        Serial.write(prbuf[i]);
      }
      Serial.write(';');
    }else if(prbuf[0]=='7' && prbuf[1]==':'){ // answer on get/set VFO B
      Serial.print(F("FB"));
      for(i=0;i<=13-prpos;i++){ // pad with zeroes
        Serial.write('0');
      }
      for(i=2;i<prpos;i++){
        Serial.write(prbuf[i]);
      }
      Serial.write(';');
    }else{
      Serial.println(F("E;"));
    }
    prclear();
  }

  if(rxcomplete){ // parsing of the cat commands
/*    Serial.print(F("RX Ans: ")); // debugging output
    for(i=0;i<rxpos;i++){
      Serial.write(rxbuf[i]);
    }
    Serial.println(';');*/
    rxs=255; // status 255, unhandled packet
    if(rxbuf[0] == 'A'){
      if(rxbuf[1] == 'I' && rxbuf[2] == ';'){ // AI, auto information
        rxs=0; // no further processing
        Serial.print(F("AI0;"));
      }
    }else if(rxbuf[0] == 'F'){
      if(rxbuf[1] == 'A'){ // FA, get/set VFO A
        rxs=254;
        if(rxbuf[2] == ';'){ // get
          rxs=0;
          mySerial.print(F("4?\r"));
        }else if(rxbuf[13] == ';'){ // set
          rxs=0;
          mySerial.print(F("4="));
          for(i=2;i<13;i++){
            mySerial.write(rxbuf[i]);
          }
          mySerial.write('\r');
        }
      }else if(rxbuf[1] == 'B'){ // FB, get/set VFO B
        rxs=254;
        if(rxbuf[2] == ';'){ // get
          rxs=0;
          mySerial.print(F("7?\r"));
        }else if(rxbuf[13] == ';'){ // set
          rxs=0;
          mySerial.print(F("7="));
          for(i=2;i<13;i++){
            mySerial.write(rxbuf[i]);
          }
          mySerial.write('\r');
        }
      }else if(rxbuf[1] == 'R'){ // FR, get/set receiver VFO A/B
        rxs=254;
        if(rxbuf[2] == ';'){ // get
          rxs=0;
          Serial.print(F("FR"));
          Serial.print(vfoab ? '1' : '0');
          Serial.print(F(";"));
        }else if(rxbuf[3] == ';'){ // set
          rxs=0;
          Serial.print(F("FR"));
          vfoab = rxbuf[2] == '1';
          digitalWrite(VFOPIN, !vfoab);
          Serial.print(vfoab ? '1' : '0');
          Serial.print(F(";"));
        }
      }
    }else if(rxbuf[0] == 'I'){
      if(rxbuf[1] == 'D' && rxbuf[2] == ';'){ // ID, read transceiver identity
        rxs=0;
        Serial.print(F("ID020;"));
      }
    }else if(rxbuf[0] == 'M'){
      if(rxbuf[1] == 'D' && rxbuf[2] == ';'){ // MD, operating mode, always USB
        rxs=0;
        Serial.print(F("MD"));
        Serial.print(opmode);
        Serial.print(F(";"));
      }else if(rxbuf[3] == ';'){ // set
        rxs=254;
        if(rxbuf[2] >= '0' && rxbuf[2] <= '9'){
          rxs=0;
          opmode = rxbuf[2]-'0';
          Serial.print(F("MD"));
          Serial.print(opmode);
          Serial.print(F(";"));
        }
      }
    }else if(rxbuf[0] == 'N'){
      if(rxbuf[1] == 'A' && rxbuf[2] == ';'){ // NA, radio name
        rxs=0;
        Serial.print(F("NAProgRock;"));
      }
    }
    if(rxs==255){
      Serial.print(F("?;")); // bad command syntax
    }else if(rxs==254){
      Serial.print(F("O;")); // processing not completed
    }
    rxclear();
  }
}

void serialEvent(){
  int i;
  while(Serial.available()){
    i = Serial.read();
    if(rxpos<BUFFLEN && !rxcomplete && i != '\r' && i != '\n'){
      rxbuf[rxpos++]=(char)i;
    }
    if(i == ';' || rxpos>=BUFFLEN){
      rxcomplete = true;
    }
  }
}

void rxclear(void){
  for(int i=0;i<BUFFLEN;i++){
    rxbuf[i]=0;
  }
  rxpos=0;
  rxcomplete=false;
}

void prclear(void){
  for(int i=0;i<BUFFLEN;i++){
    prbuf[i]=0;
  }
  prpos=0;
  prcomplete=false;
}


