#include <Audio.h>

#define I2C   // uncomment this to include I2C code

#ifdef I2C
#include <Wire.h>
#include <SPI.h>
#endif

#define FILTERS  // uncomment this for Filters that do averaging of vibration for the level of background rainfall

#ifdef FILTERS
#include <Filters.h>
// vibration averaging with filter
#define VIB_MEAN_DUR 5     // in Seconds, how long to average the signal, for statistics
RunningStatistics vibStats;  // create running statistics to smooth these values
#endif


//----------------------------
// Vibration sensing
//#define VIBPIN A0       // use A0 for Pav X
#define VIB_PIN A1
#define VIB_MIN 5

int vibNow = 0;
int vibPrev = 0;
int vibDiff = 0;

//-------------------
// thunder

// WAV files converted to code by wav2sketch

#include "AudioSampleHihat.h"        // http://www.freesound.org/people/mhc/sounds/102790/
#include "AudioSampleGong.h"         // http://www.freesound.org/people/juskiddink/sounds/86773/
#include "AudioSampleDrip0.h"
#include "AudioSampleDrip1.h"
#include "AudioSampleDrip2.h"
#include "AudioSampleDrain0.h"
#include "AudioSampleDrain1.h"
#include "AudioSampleRain0.h"
#include "AudioSampleRain1.h"
#include "AudioSampleRain2.h"
#include "AudioSampleThunder0.h"
#include "AudioSampleThunder1.h"
#include "AudioSampleThunder2.h"
/*
#include "AudioSampleSnare.h"        // http://www.freesound.org/people/KEVOY/sounds/82583/
#include "AudioSampleTomtom.h"       // http://www.freesound.org/people/zgump/sounds/86334/
#include "AudioSampleKick.h"         // http://www.freesound.org/people/DWSD/sounds/171104/
*/

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioPlayMemory    sound0;
AudioPlayMemory    sound1;  // memory players, so we can play
AudioPlayMemory    sound2;  // all sounds simultaneously
AudioPlayMemory    sound3;  // loop the rainfall on this channel
#define N_CH 3

AudioMixer4        mix1;    // two 4-channel mixers are needed in
AudioOutputI2S     headphones;

// Create Audio connections between the components
//
//AudioConnection c1(sound0, 0, headphones, 0);
//AudioConnection c2(sound1, 0, headphones, 1);
AudioConnection  c1(sound0, 0, mix1, 0);
AudioConnection  c2(sound1, 0, mix1, 1);
AudioConnection  c3(sound2, 0, mix1, 2);
AudioConnection  c4(sound3, 0, mix1, 3);
AudioConnection  c8(mix1, 0, headphones, 0);
//AudioConnection  c9(mix1, 0, headphones, 1);

//const float mixgain = 1.0;
const float mixgain = 0.4;
float loudness = 1.0;

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;

//------------------
// lightning
/* The audio board uses the following pins.
 6 - MEMCS
 7 - MOSI
 9 - BCLK
10 - SDCS
11 - MCLK
12 - MISO
13 - RX
14 - SCLK
15 - VOL
18 - SDA
19 - SCL
22 - TX
23 - LRCLK
*/

#define BLU0_PIN  3
#define GRN0_PIN  4
#define RED0_PIN  5

#define LIGHTNING HIGH

unsigned int lightDur = 50;
unsigned int flash_delay = 0;
unsigned int flash_prev = 0;

//--------------------------------
// Mode

int MODE = 2; // 0=quiet, 1=lights only, 2=thor mode
int MODE_OLD = MODE; // 0=quiet, 1=lights only, 2=thor mode
boolean ASLEEP = true;

//------------------
// setup
void setup() 
{

#ifdef FILTERS
  vibStats.setWindowSecs(VIB_MEAN_DUR);
#endif

  AudioMemory(5);
  // turn on the output
  audioShield.enable();
  audioShield.volume(loudness);

  // reduce the gain on mixer channels, so more than 1
  // sound can play simultaneously without clipping
  
  mix1.gain(0, mixgain);
  mix1.gain(1, mixgain);
  mix1.gain(2, mixgain);
  mix1.gain(3, mixgain);

// turn the lights off
  pinMode(BLU0_PIN, OUTPUT);
  analogWrite(BLU0_PIN, 0);
  pinMode(RED0_PIN, OUTPUT);
  analogWrite(RED0_PIN, 0);
  pinMode(GRN0_PIN, OUTPUT);
  analogWrite(GRN0_PIN, 0);

// communications
#ifdef I2C
  Wire.begin(7); // begin I2C and define slave address   MUST BE SET DIFFERENT FOR EACH SLAVE
  Wire.onReceive(receiveEvent); // register event
#endif
  Serial.begin(9600);           // start serial for output

  flash_prev = millis();
}


//--------------------
// loop

unsigned char ch=0;

boolean WIREFLAG = false; //  flag while receiving
boolean LIGHTFLAG = false; //  flag while receiving

void loop() 
{
#ifdef I2C
  if (WIREFLAG)
  {
    delay(100);
    return;
  }
#endif
  if (MODE != MODE_OLD)
  {
    Serial.print("MODE set to ");
    Serial.println(MODE);
    MODE_OLD = MODE;
  }
       
  if (MODE == 0)
  {
    if (ASLEEP == false)
    {
      analogWrite(BLU0_PIN, 0);
      analogWrite(GRN0_PIN, 0);
      analogWrite(RED0_PIN, 0);     
      
 //     audioShield.volume(0.1);
      ASLEEP = true;
    }
    return;
  }
  ASLEEP = false;
  // slow pulse colours

  if (!LIGHTFLAG)
  {
 //   bluPulse();  
    grnPulse();  
    redPulse();
  }

  
  if (MODE == 1) return; // just lights

//  LIGHTFLAG = false;

  
//  long vibNow = random(1024);
  vibNow = analogRead(VIB_PIN); // read the level of vibration from A0
  vibDiff = vibNow - vibPrev;
if (vibNow > 10) Serial.println(vibNow);

  if (vibDiff > VIB_MIN+900) 
  {
//   PlayChannel(ch, AudioSampleThunder2, mixgain);
    PlayChannel(ch, AudioSampleDrip2, mixgain);
    ch = (ch+1)%N_CH;
      digitalWrite(BLU0_PIN, LIGHTNING);
      digitalWrite(RED0_PIN, LIGHTNING);
      digitalWrite(GRN0_PIN, LIGHTNING);
    LIGHTFLAG = true;
  }
  else if (vibDiff > VIB_MIN+850) 
  {
    PlayChannel(ch, AudioSampleThunder1, mixgain);
    ch = (ch+1)%N_CH;
      digitalWrite(BLU0_PIN, LIGHTNING);
      digitalWrite(RED0_PIN, LIGHTNING);
      digitalWrite(GRN0_PIN, LIGHTNING);
    LIGHTFLAG = true;
  }
  else if (vibDiff > VIB_MIN+800) 
  {
    PlayChannel(ch, AudioSampleGong, mixgain);
    ch = (ch+1)%N_CH;
      digitalWrite(BLU0_PIN, LIGHTNING);
      digitalWrite(RED0_PIN, LIGHTNING);
      digitalWrite(GRN0_PIN, LIGHTNING);
    LIGHTFLAG = true;
  }
  else if (vibDiff > VIB_MIN+750) 
  {
    PlayChannel(ch, AudioSampleThunder0, mixgain);
    ch = (ch+1)%N_CH;
      digitalWrite(BLU0_PIN, LIGHTNING);
      digitalWrite(RED0_PIN, LIGHTNING);
      digitalWrite(GRN0_PIN, LIGHTNING);
    LIGHTFLAG = true;
  }
  else if (vibDiff > VIB_MIN+700) 
  {
    PlayChannel(ch, AudioSampleDrain1, mixgain);
    ch = (ch+1)%N_CH;
      digitalWrite(BLU0_PIN, LIGHTNING);
      digitalWrite(RED0_PIN, LIGHTNING);
      digitalWrite(GRN0_PIN, LIGHTNING);
    LIGHTFLAG = true;
 }
  else if (vibDiff > VIB_MIN+600) 
  {
    PlayChannel(ch, AudioSampleDrain0, mixgain);
    ch = (ch+1)%N_CH;
      digitalWrite(BLU0_PIN, LIGHTNING);
      digitalWrite(GRN0_PIN, LIGHTNING);
//      digitalWrite(RED0_PIN, LIGHTNING);
    LIGHTFLAG = true;
  }
  else if (vibDiff > VIB_MIN+500) 
  {
    PlayChannel(ch, AudioSampleThunder1, mixgain);
    ch = (ch+1)%N_CH;
//PlayChannel(ch, AudioSampleRain1, mixgain);
      digitalWrite(BLU0_PIN, LIGHTNING);
      digitalWrite(RED0_PIN, LIGHTNING);
//      digitalWrite(RED0_PIN, LIGHTNING);
    LIGHTFLAG = true;
  }
  else if (vibDiff > VIB_MIN+400) 
  {
    PlayChannel(ch, AudioSampleHihat, mixgain);
    ch = (ch+1)%N_CH;
      digitalWrite(BLU0_PIN, LIGHTNING);
      digitalWrite(RED0_PIN, LIGHTNING);
//      digitalWrite(RED0_PIN, LIGHTNING);
      LIGHTFLAG = true;
  }
  else if (vibDiff > VIB_MIN+300) 
  {
//    PlayChannel(ch, AudioSampleDrip0, mixgain);
    PlayChannel(ch, AudioSampleDrain0, mixgain);
    ch = (ch+1)%N_CH;
      digitalWrite(BLU0_PIN, LIGHTNING);
      digitalWrite(RED0_PIN, LIGHTNING);
//      digitalWrite(RED0_PIN, LIGHTNING);
    LIGHTFLAG = true;
  }
  else if (vibDiff > VIB_MIN+200) 
  {
    PlayChannel(ch, AudioSampleDrain1, mixgain);
    ch = (ch+1)%N_CH;
//    PlayChannel(ch, AudioSampleDrip1, mixgain);
      digitalWrite(GRN0_PIN, LIGHTNING);
//      digitalWrite(GRN0_PIN, LIGHTNING);
    LIGHTFLAG = true;
  }
  else if (vibDiff > VIB_MIN)
  {
    PlayChannel(ch, AudioSampleThunder1, mixgain);
    ch = (ch+1)%N_CH;
//    PlayChannel(ch, AudioSampleDrip2, mixgain);
      digitalWrite(GRN0_PIN, LIGHTNING);
//      digitalWrite(GRN0_PIN, LIGHTNING);
    LIGHTFLAG = true;
  }


  if (LIGHTFLAG)
  {
//    ch = (ch+1)%N_CH;
    flash_delay = millis();
    if (flash_delay - flash_prev > lightDur)
    {
      digitalWrite(BLU0_PIN, 0);
      digitalWrite(RED0_PIN, 0);
      digitalWrite(GRN0_PIN, 0);
//      digitalWrite(BLU0_PIN, LOW);
//      digitalWrite(GRN0_PIN, LOW);
//      digitalWrite(RED0_PIN, LOW);
      flash_prev = flash_delay;
      LIGHTFLAG = false;
    }
  }


  if (vibDiff > VIB_MIN) Serial.println(vibDiff);

#ifdef FILTERS
// change the level of background rain to match long term mean of the vibration
  vibStats.input(vibNow);
  rainLoop(vibStats.mean());
#else
  rainLoop(20);
#endif

  vibPrev = vibNow;

}

#ifdef I2C
//--------------------------
//
// I2C recieve function
//
void receiveEvent(int howMany)
{
  WIREFLAG = true;
  Serial.println("**********");
  
  Serial.println(howMany);
  while(1 < Wire.available()) // loop through all but the last
  {
    char c = Wire.read(); // receive byte as a character
    Serial.print(c);         // print the character
  }
  int x = Wire.read();    // receive byte as an integer
  Serial.println(x);         // print the integer

  if (x >= 0) 
  {
    MODE = x;
    Serial.println(MODE);
  }
  WIREFLAG = false;
}
#endif

//---------------------
// allocate channels in sequence

void PlayChannel(unsigned char ch, const unsigned int * sample, float level) 
{
  switch (ch) {
    case 0:
      if (!sound0.isPlaying())
      {
        mix1.gain(0, level);
        sound0.play(sample);
        Serial.println("sound0");
      }   
      break;
    case 1:
      if (!sound1.isPlaying())
     {
        mix1.gain(1, level);
        sound1.play(sample);
        Serial.println("sound1");
      }   
      break;
    case 2:
      if (!sound2.isPlaying())
     {
        mix1.gain(2, level);
        sound2.play(sample);
        Serial.println("sound2");
      }   
      break;
    case 3:
      if (!sound3.isPlaying())
     {
        mix1.gain(3, level);
        sound3.play(sample);
      }   
      break;
    default: 
      break;
  }  
}

//-------------
// rain loop
float gainRain = 0;

void rainLoop(int level) // 0..1023
{
  long n = map(level, 0, 1023, 1, 70);
//  unsigned float g = n/100.0;
  PlayChannel(3, AudioSampleRain0, n/100.0);
}


//-----------------------
// pulsing lights
#define FADE_LO 0
#define FADE_HI 1000
#define RGB_LO 0
#define RGB_HI 200


int bluBright = FADE_LO;    // how bright the LED is
#define BLU_FADE 1   // how many points to fade the LED by
int bluFade = BLU_FADE;
#define BLU_LOOPS 5000
int bluI = 0;

void bluPulse() 
{
  if (bluI++ < BLU_LOOPS) return;
  bluI=0;
 // reverse the direction of the fading at the ends of the fade:
  if (bluBright <= FADE_LO) bluFade = BLU_FADE;
  else if (bluBright >= FADE_HI) bluFade = -BLU_FADE;
  
  bluBright = bluBright + bluFade;
  int v = map(bluBright, FADE_LO, FADE_HI, RGB_LO, RGB_HI);

  analogWrite(BLU0_PIN, v);
//  Serial.print("B=");
//  Serial.println(v);
}

int grnBright = FADE_LO;    // how bright the LED is
#define GRN_FADE 1   // how many points to fade the LED by
int grnFade = GRN_FADE;
#define GRN_LOOPS 300
int grnI = 0;

void grnPulse() 
{
  if (grnI++ < GRN_LOOPS) return;
  grnI=0;

 // reverse the direction of the fading at the ends of the fade:
  if (grnBright <= FADE_LO) grnFade = GRN_FADE;
  else if (grnBright >= FADE_HI) grnFade = -GRN_FADE;
  
  grnBright = grnBright + grnFade;
  int v = map(grnBright, FADE_LO, FADE_HI, RGB_LO, RGB_HI);

  analogWrite(GRN0_PIN, v);
 // Serial.print("G=");
 // Serial.println(v);
}

int redBright = FADE_LO;    // how bright the LED is
#define RED_FADE 1   // how many points to fade the LED by
int redFade = RED_FADE;
#define RED_LOOPS 400
int redI = 0;

void redPulse() 
{
  if (redI++ < RED_LOOPS) return;
  redI=0;
 // reverse the direction of the fading at the ends of the fade:
  if (redBright <= FADE_LO) redFade = RED_FADE;
  else if (redBright >= FADE_HI) redFade = -RED_FADE;
  
  redBright = redBright + redFade;
  int v = map(redBright, FADE_LO, FADE_HI, RGB_LO, RGB_HI);

  analogWrite(RED0_PIN, v);
//  Serial.print("R=");
//  Serial.println(v);
}

