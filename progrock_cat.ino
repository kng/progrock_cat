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
 *  version 1.1 2018-01-07 support multiple commands from rigcat
 *  version 1.0 2018-01-06 initial release
 */

#include <SoftwareSerial.h>
#define BUFFLEN 16
#define VFOLEN 11
#define VFOPIN 12

SoftwareSerial mySerial(10, 11); // RX, TX

// rx = CAT commands from PC, pr = progrock communication
unsigned char rxbuf[BUFFLEN], prbuf[BUFFLEN], rigcat[BUFFLEN], vfoa[VFOLEN], vfob[VFOLEN];
unsigned int rxpos, rxread, prpos, opmode=2, rxs;
bool rxcomplete, prcomplete, rx_ab, tx_ab;

void setup() {
  pinMode(VFOPIN, OUTPUT); // VFO A/B
  digitalWrite(VFOPIN, !rx_ab); // out HIGH = A
  Serial.begin(9600);
  //while (!Serial) { ; } // wait for host to connect
  //Serial.println(F("Kenwood CAT"));
  mySerial.begin(9600);
  prclear();
  for(int i=0;i<VFOLEN;i++){
    vfoa[i]='0';
    vfob[i]='0';
  }
}

void loop() {
  int i;

  if (mySerial.available()) { // softserial polling, does not handle multiple answers
    i = mySerial.read();
    if(prpos<BUFFLEN && !prcomplete && i != ',' && i != '\r'){ // filter out irrelevant stuff
      prbuf[prpos++]=(char)i;
    }
    if(i == '\r' || rxpos>=BUFFLEN){
      prcomplete = true;
    }
  }

  if(prcomplete){ // complete sentance from the progrock
/*    Serial.print(F("PR: ")); // debugging output
    Serial.print(prpos);
    Serial.print(F(" "));
    for(i=0;i<prpos;i++){
      Serial.write(prbuf[i]);
    }
    Serial.println(';');*/

    if(prbuf[0]=='4' && prbuf[1]==':'){ // answer on get/set VFO A
      for(i=0;i<VFOLEN;i++){ // fill with zeroes
        vfoa[i]='0';
      }
      for(i=2;i<prpos;i++){
        vfoa[VFOLEN-prpos+i]=prbuf[i];
      }
    }else if(prbuf[0]=='7' && prbuf[1]==':'){ // answer on get/set VFO B
      for(i=0;i<VFOLEN;i++){ // fill with zeroes
        vfob[i]='0';
      }
      for(i=2;i<prpos;i++){
        vfob[VFOLEN-prpos+i]=prbuf[i];
      }
//    }else{
//      Serial.print(F("E;"));
    }
    if(rxs==100){
      Serial.print(F("FA"));
      for(i=0;i<VFOLEN;i++){
        Serial.write(vfoa[i]);
      }
      Serial.print(F(";"));
    }else if(rxs==101){
      Serial.print(F("FB"));
      for(i=0;i<VFOLEN;i++){
        Serial.write(vfob[i]);
      }
      Serial.print(F(";"));
    }else if(rxs==110){
      Serial.print(F("IF"));
      //Serial.print(F("IF00000000000000000000000000000000000;"));
      for(i=0;i<VFOLEN;i++){ // P1 11 digit freq
        Serial.write(vfoa[i]);
      }
      Serial.print(F("     ")); // P2 five spaces
      Serial.print(F("00000000000")); // P3-P8
      Serial.print(opmode); // P9 opmode
      Serial.print(F("000000 ;")); // P10-P15
    }
    rxs=0;
    prclear();
  }

  if(rxavail()){ // parsing of the cat commands
/*    Serial.print(F("CAT available: "));
    for(i=0;i<BUFFLEN;i++){
      Serial.write(rigcat[i]);
      if(rigcat[i]==';') exit;
    }
    Serial.println();*/

    rxs=255; // status 255, unhandled packet
    if(rigcat[0] == 'A'){
      if(rigcat[1]=='I' && (rigcat[2]==';' || rigcat[3]==';')){ // AI, auto information
        rxs=0; // no further processing
        Serial.print(F("AI0;"));
      }
    }else if(rigcat[0] == 'F'){
      if(rigcat[1] == 'A'){ // FA, get/set VFO A
        rxs=254;
        if(rigcat[2] == ';'){ // get
          rxs=100; // request FAxxx;
          mySerial.print(F("4?\r"));
        }else if(rigcat[13] == ';'){ // set
          rxs=100; // request FAxxx;
          mySerial.print(F("4="));
          for(i=2;i<13;i++){
            mySerial.write(rigcat[i]);
          }
          mySerial.write('\r');
        }
      }else if(rigcat[1] == 'B'){ // FB, get/set VFO B
        rxs=254;
        if(rigcat[2] == ';'){ // get
          rxs=101; // request FBxxx;
          mySerial.print(F("7?\r"));
        }else if(rigcat[13] == ';'){ // set
          rxs=101; // request FBxxx;
          mySerial.print(F("7="));
          for(i=2;i<13;i++){
            mySerial.write(rigcat[i]);
          }
          mySerial.write('\r');
        }
      }else if(rigcat[1] == 'R'){ // FR, get/set receiver VFO A/B
        rxs=254;
        if(rigcat[2] == ';'){ // get
          rxs=0;
          Serial.print(F("FR"));
          Serial.print(rx_ab ? '1' : '0');
          Serial.print(F(";"));
        }else if(rigcat[3] == ';'){ // set
          rxs=0;
          Serial.print(F("FR"));
          rx_ab = rigcat[2] == '1';
          digitalWrite(VFOPIN, !rx_ab);
          Serial.print(rx_ab ? '1' : '0');
          Serial.print(F(";"));
        }
      }else if(rigcat[1] == 'T'){ // FT, get/set transmitter VFO A/B
        rxs=254;
        if(rigcat[2] == ';'){ // get
          rxs=0;
          Serial.print(F("FT"));
          Serial.print(tx_ab ? '1' : '0');
          Serial.print(F(";"));
        }else if(rigcat[3] == ';'){ // set
          rxs=0;
          Serial.print(F("FT"));
          tx_ab = rigcat[2] == '1';
          //digitalWrite(VFOPIN, !tx_ab);
          Serial.print(tx_ab ? '1' : '0');
          Serial.print(F(";"));
        }
      }
    }else if(rigcat[0] == 'I'){
      if(rigcat[1] == 'D' && rigcat[2] == ';'){ // ID, read transceiver identity
        rxs=0;
        Serial.print(F("ID020;"));
      }else if(rigcat[1] == 'F' && rigcat[2] == ';'){ // IF, read transceiver status
        rxs=110; // request IFxxx;
        mySerial.print(F("4?\r")); // check vfo A/B
        //Serial.print(F("IF00000000000000000000000000000000000;"));
      }
    }else if(rigcat[0] == 'M'){
      if(rigcat[1] == 'D' && rigcat[2] == ';'){ // MD, operating mode
        rxs=0;
        Serial.print(F("MD"));
        Serial.print(opmode);
        Serial.print(F(";"));
      }else if(rigcat[3] == ';'){ // set
        rxs=254;
        if(rigcat[2] >= '0' && rigcat[2] <= '9'){
          rxs=0;
          opmode = rigcat[2]-'0';
          Serial.print(F("MD"));
          Serial.print(opmode);
          Serial.print(F(";"));
        }
      }
    }else if(rigcat[0] == 'N'){
      if(rigcat[1] == 'A' && rigcat[2] == ';'){ // NA, radio name
        rxs=0;
        Serial.print(F("NAProgRock;"));
      }
    }else if(rigcat[0] == 'P'){
      if(rigcat[1] == 'S' && rigcat[2] == ';'){ // PS, power status
        rxs=0;
        Serial.print(F("PS1;"));
      }else if(rigcat[1] == 'T' && rigcat[2] == ';'){ // PT, pitch ?
        rxs=0;
        Serial.print(F("PT00;"));
      }
    }else if(rigcat[0] == 'R'){
      if(rigcat[1] == 'C' && rigcat[2] == ';'){ // RC, clear rit
        rxs=0;
        //Serial.print(F("RC;"));
      }else if(rigcat[1] == 'S' && rigcat[2] == ';'){ // RS
        rxs=0;
        Serial.print(F("RS0;"));
      }else if(rigcat[1] == 'T' && rigcat[2] == ';'){ // RT
        rxs=0;
        Serial.print(F("RT0;"));
      }
    }else if(rigcat[0] == 'S'){
      if(rigcat[1] == 'M' && rigcat[3] == ';'){ // SM, signal meter
        rxs=0;
        Serial.print(F("SM00000;"));
      }
    }
    if(rxs==255){
      rxs=0;
      Serial.print(F("?;")); // bad command syntax
    }else if(rxs==254){
      rxs=0;
      Serial.print(F("O;")); // processing not completed
    }
  }
}

void serialEvent(){
  int i;
  while(Serial.available()){
    i = Serial.read();
    if(i != '\r' && i != '\n'){
      rxbuf[rxpos]=(char)i;
      rxpos = (rxpos + 1) % BUFFLEN;
    }
  }
}

bool rxavail(void){
  int i, len, com;
  if(rxpos==rxread) return false; // no new data, or complete overrun
  com = false;
  len = (BUFFLEN - rxread + rxpos) % BUFFLEN; // total length of new data
  for(i=0;i<len;i++){ // find next ';'
    if(rxbuf[(rxread+i)%BUFFLEN]==';'){
      com=true;
      len=i; // length to first ';'
      exit;
    }
  }
  if(com){
    for(i=0;i<BUFFLEN;i++){
      rigcat[i]=0;
    }
    for(i=0;i<=len;i++){
      rigcat[i]=rxbuf[(rxread+i)%BUFFLEN];
    }
    rxread = (rxread+i)%BUFFLEN;
    return true;
  }
  return false;
}

void prclear(void){
  for(int i=0;i<BUFFLEN;i++){
    prbuf[i]=0;
  }
  prpos=0;
  prcomplete=false;
}

