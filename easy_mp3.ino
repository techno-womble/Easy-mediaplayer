/***************************************************
 Arduino based MP3 player with minimal radio style user interface 
 
 Created 11 Nov 2019
 Modified 18 Nov 2019
 V1.0
 By John Potter (techno-womble)

****************************************************/

#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

boolean serialDebug = false;                                                           // enable/disable troubleshooting messages
boolean randomTrackPlay = true;                                                            // randomise the track order

const int potPin = 0;                                                                 // volume control voltage on pin analog 0
const int encoderPinA = 4;                                                            // tuning encoder on digital pins 2 and 4
const int encoderPinB = 2; 
const int busyPin = 6;

int encoderPos;
int encoderLastPos = -1;
boolean encoderALast = LOW;
int potVal = 0;
int lastVolume = 999;                                                                 // impossible value to force initial volume update                  
int volume;

int stationCount;
int currentStation;
int currentTrack = 1;
int lastTrack = 1;

SoftwareSerial mySoftwareSerial(10,11);                                              // init SoftwareSerial control for DFR mp3 player on pins 10 & 11 (RX,TX)
DFRobotDFPlayerMini myDFPlayer;                                                      // init DFR Mp3 player module 

//********************************************************************************************************************

void checkVol() {                                                                     //read volume control voltage and update DFR software volume control
  
  potVal = analogRead(potPin);
  volume = map(potVal,0,1023,0,32);
  if (lastVolume != volume) {                                                         // update volume if changed
    myDFPlayer.volume(volume);
    delay(20);
    
    lastVolume = volume; 

    if (serialDebug)  {                                                               // debug code - show volume status
      Serial.print("Volume = ");    
      Serial.print(volume); 
      Serial.println("/32");   }
     }
}

//**************************************************************************************************************************

void checkTuning() {                                                                  // calculate "station number" (encoder position)
  
  boolean encoderA = digitalRead(encoderPinA);

  if ((encoderALast == HIGH) && (encoderA == LOW))
  {
    if (digitalRead(encoderPinB) == LOW) {
      encoderPos--;
    } else {
      encoderPos++;
    }
  }
  encoderALast = encoderA;
  
  if (encoderPos == 0) encoderPos = stationCount;
  if (encoderPos > stationCount) encoderPos = 1;
  if (encoderPos != encoderLastPos) {                                         // encoder position has changed
    encoderLastPos = encoderPos;

    if (serialDebug) {                                                       //  debug code - show encoder status
      Serial.print("Encoder position = ");    
      Serial.println(encoderPos);      
    }

     myDFPlayer.stop();                                                                  
     delay(50);

     currentStation = encoderPos;
     playStation();                                                          // play the selected station
     delay(500);                                                             // delay to audition selection                                                                                  
    }  
  
}

//*****************************************************************************************************************************

void playStation()  {
  int trackCount = 0;  

//  myDFPlayer.stop();                                                                  
//     delay(150);

  trackCount = (myDFPlayer.readFileCountsInFolder(currentStation));                                 // number of tracksin folder
   
  if (currentTrack > trackCount) currentTrack = 1;

  if (serialDebug) {Serial.print("Playing station ");
     Serial.print(currentStation);
     Serial.print(" track ");
     Serial.println(currentTrack);
  }
  myDFPlayer.playFolder(currentStation,currentTrack);                                               // play the selected station
  if (randomTrackPlay == false ) currentTrack++;
  else {
      lastTrack = currentTrack;
      currentTrack =   random(1,trackCount);                                                        // pick a random track
      if (currentTrack == lastTrack) currentTrack++;                                                // don't play the same track twice
  } 
}


//******************************************************************************************************************************

void setup () {

  int i;
  int count = 0;

 Serial.begin (9600);                                                                     
 mySoftwareSerial.begin(9600);

 if (serialDebug) { Serial.println("Debug messages");                                  // debug code - header text
                    Serial.println("Initializing DFPlayer.............");}

 pinMode(encoderPinA, INPUT);                                                              //set up rotary encoder 
 pinMode(encoderPinB, INPUT);
 pinMode(busyPin, INPUT);
 digitalWrite(encoderPinA, HIGH);
 digitalWrite(encoderPinB, HIGH);
 digitalWrite(busyPin, HIGH);

 
 if (!myDFPlayer.begin(mySoftwareSerial,false,true)) {                                               // Use software serial for player control and hardware serial for debug messages 
  if (serialDebug){                                                                       // debug code - show player initialisation status
     Serial.println("Failed :-(");                                                        // initialisation failed messages
     Serial.println("Check the software serial connection - digital pins 10 and 11");
     Serial.println("and check the SD card file structure");
     }
     while(true) {
        delay(0);                                                                          // Code to be compatible with ESP8266 watch dog - copied from DR Robot examples
     }
 }
 if (serialDebug) Serial.println("Success!");                                              // debug code - player initialised sucessfully

  myDFPlayer.setTimeOut(2000);                                                              //Set serial communictaion time out 500ms
  checkVol();                                                                              // set initial volume and tone controls
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);                                                      
//  myDFPlayer.EQ(DFPLAYER_EQ_POP);
//  myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
//  myDFPlayer.EQ(DFPLAYER_EQ_JAZZ);
//  myDFPlayer.EQ(DFPLAYER_EQ_CLASSIC);
//  myDFPlayer.EQ(DFPLAYER_EQ_BASS);

  count = 0;                                                               // calculate the number of "stations" (directories)
  for (i=1; count != -1; i++) {                                            // count non-empty directories
       if (serialDebug) Serial.print("Directory ");
       if (serialDebug) Serial.print(i);
       if (serialDebug) Serial.print(" contains ");
       count = (myDFPlayer.readFileCountsInFolder(i));                     //read number of files in folder 
       if (serialDebug) Serial.print(count);
       if (serialDebug) Serial.println(" files");
   }
  stationCount = i -2;                   
   randomSeed(analogRead(millis()));
   encoderPos = (random(1,stationCount));                                  // start up with as random station
   currentStation = encoderPos;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//*******************************************************************************************************************************

void loop () { 

  boolean busy =false;
 
  delay (10);  
  if (myDFPlayer.available())  myDFPlayer.read();                            // needed to keep ack buffer clean clear                                                              
  checkVol(); 
  checkTuning();   

  busy = digitalRead(busyPin);                                                 // check if current track is finished
 
  if (busy == 1 ) {
    playStation();
    delay(300);                                                                        // wait for busy pin
  }                     
}

//********************************************************************************************************************************
