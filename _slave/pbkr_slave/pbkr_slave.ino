
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "user_interface.h"
#include <i2s.h>
#include <i2s_reg.h>
#include "slave_consts.h"

#define DEBUG_LRC_I2C true

extern void commmgt_rcv(const uint8_t c);

///////////////////////////////////////////////////////
// LOCAL FUNCTIONS
///////////////////////////////////////////////////////
 inline void LRC_i2s_writeDAC(int16_t leftData, int16_t rightData) {
  uint32_t value;
  int16_t* pval((int16_t*)(void*)&value);
  *pval = leftData;
  pval++;
  *pval = rightData;
  i2s_write_sample(value);
}

static int secPrec;

static uint8_t phase;
static int dtMax(-1);
static bool sineTest(false);
static int secCnt(0);
static float volume(0.5);
static SoftwareSerial SerialRx(SOFT_SER_RX, SOFT_SER_TX);
String s;



/*******************************************************************************
 *        EXTERNAL FUNCTIONS
 *******************************************************************************/
static const int16_t* notePlayingPtr(NULL);
static int notePlayingLen(0);

void on_MIDI_note(const uint8_t noteNum, const uint8_t velocity)
{
  //Serial.printf("Play note %d at level %d\n",noteNum, velocity); // TODO remove log
  if (noteNum == 30)
  {
    notePlayingLen = sizeof(SAMPLES_Clic1) /sizeof(*SAMPLES_Clic1);
    notePlayingPtr = SAMPLES_Clic1;
    volume = velocity / 128.0;
  }
  if (noteNum == 31)
  {
    notePlayingLen = sizeof(SAMPLES_Clic2) /sizeof(*SAMPLES_Clic2);
    notePlayingPtr = SAMPLES_Clic2;
    volume = velocity / 128.0;
  }
}

/*******************************************************************************
 *        SETUP
 *******************************************************************************/
void setup() {
  // put your setup code here, to run once:

#if OVERCLOCK
  system_update_cpu_freq(160);
#endif

  
  delay (100);
  //Serial.begin(SERIAL_BAUDRATE);
  Serial.begin(MIDI_BAUD_RATE);
  SerialRx.begin(RX_PRG_BAUDRATE);
  pinMode(I2S_PIN_BCK, OUTPUT);
  pinMode(I2S_PIN_LRCK, OUTPUT);
  pinMode(I2S_PIN_DATA, OUTPUT);
  digitalWrite(I2S_PIN_DATA,0);
  pinMode(BTN_TEST, INPUT);
  
  delay (200);
  i2s_begin();
  i2s_set_rate(I2S_HZ_FREQ);
  //Serial.printf("");
  //Serial.printf("F=");
  const String s (static_cast<int>(i2s_get_real_rate()));
  //Serial.printf(s.c_str());
  //Serial.printf(" Hz\n");

  secPrec = (millis()/1000);

  while (secPrec == millis()/1000);
  secPrec ++;
  phase = 0;
  dtMax = -1;
  volume = 0.1;
  sineTest = false;
  //Serial.println("I2S SINE test");

}

/*******************************************************************************
 *        LOOP
 *******************************************************************************/

void loop() {

  if (SerialRx.available() > 0)
  {
    const uint8_t c = uint8_t (SerialRx.read());
    commmgt_rcv(c);
  }
  
  if (digitalRead(BTN_TEST))
  {
    sineTest = true;
    secCnt = 0;
  }
  
  // put your main code here, to run repeatedly:
  if (sineTest == false && notePlayingPtr == NULL) return;
  
  if (secCnt>=DEBUG_LRC_SINE_TEST) sineTest = false;

  const int t0(micros()/10);
  while (i2s_is_full()){yield();}
  const int t1(micros()/10);
  while (!i2s_is_full()){
    int16_t v (0);
    
    if (notePlayingPtr)
    {
      v=* (notePlayingPtr);
      notePlayingPtr += I2S_HZ_FREQ_DIV;
      notePlayingLen -= I2S_HZ_FREQ_DIV;
      if(notePlayingLen <= 0)
      {
        notePlayingPtr = NULL;
      }
    }
    else if (sineTest)
    {
      v = LRC_wavSine[phase];
    }
    LRC_i2s_writeDAC(v*volume,v*volume);
    
    phase += I2S_HZ_FREQ_DIV;
  }
  const int t2(micros()/10);
  
  const int sec=millis()/1000;
  
  if (t1-t0 > dtMax)
  {
    dtMax = t1 - t0;
    s = static_cast<int>(dtMax);
    s += "/";
    s += static_cast<int>(t2-t0);
    s += " Vol=";
    s += volume;
  }
  
  if (secPrec != sec)
  {
    //Serial.println(s.c_str());
    secPrec = sec;
    secCnt++;
    dtMax = -1;
  }
  
}
