
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "user_interface.h"
#include <i2s.h>
#include <i2s_reg.h>
#include "slave_consts.h"

#define DEBUG_LRC_I2C false

extern void commmgt_rcv(const uint8_t c);
extern void set_volume(const  float & volume);

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
static float s_volume(0.8);
static float clic_volume(0.5);
static SoftwareSerial SerialRx(SOFT_SER_RX, SOFT_SER_TX);
String s;

void set_volume(const  float & volume)
{
  s_volume = volume;
}


/*******************************************************************************
 *        EXTERNAL FUNCTIONS
 *******************************************************************************/
static const int16_t* notePlayingPtr(NULL);
static int notePlayingLen(0);

void on_MIDI_note(const uint8_t noteNum, const uint8_t velocity)
{
  PRINTF (("Play note %d at level %d\n",noteNum, velocity));
  if (noteNum == 24)
  {
    notePlayingLen = sizeof(SAMPLES_Clic1) /sizeof(*SAMPLES_Clic1);
    notePlayingPtr = SAMPLES_Clic1;
    clic_volume = velocity / 128.0;
  }
  if (noteNum == 25)
  {
    notePlayingLen = sizeof(SAMPLES_Clic2) /sizeof(*SAMPLES_Clic2);
    notePlayingPtr = SAMPLES_Clic2;
    clic_volume = velocity / 128.0;
  }
}

/*******************************************************************************
 *        SETUP
 *******************************************************************************/
void setup() {
  // put your setup code here, to run once:

  system_update_cpu_freq(CPU_FREQ);
  
  delay (100);
  Serial.begin(SERIAL_BAUDRATE);
  SerialRx.begin(RX_PRG_BAUDRATE);
  pinMode(I2S_PIN_BCK, OUTPUT);
  pinMode(I2S_PIN_LRCK, OUTPUT);
  pinMode(I2S_PIN_DATA, OUTPUT);
  digitalWrite(I2S_PIN_DATA,0);
  pinMode(BTN_TEST, INPUT);
  
  delay (200);
  i2s_begin();
  i2s_set_rate(I2S_HZ_FREQ);
  const String s (static_cast<int>(i2s_get_real_rate()));
  PRINTF(("F=%s Hz\n",s.c_str()));

  secPrec = (millis()/1000);

  while (secPrec == millis()/1000);
  secPrec ++;
  phase = 0;
  dtMax = -1;
  s_volume = 0.5;
  sineTest = false;

}

/*******************************************************************************
 *        LOOP
 *******************************************************************************/

void loop() {

  if (SerialRx.available() > 0)
  {
    const uint8_t c = uint8_t (SerialRx.read());
    PRINTF(("Rcv 0x%02X\n",c));
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
    LRC_i2s_writeDAC(v*clic_volume*s_volume,v*clic_volume*s_volume);
    
    phase += I2S_HZ_FREQ_DIV;
  }
  const int t2(micros()/10);
  
  const int sec=millis()/1000;
  
  //if (t1-t0 > dtMax)
  {
    dtMax = t1 - t0;
    s = static_cast<int>(dtMax);
    s += "/";
    s += static_cast<int>(t2-t0);
    s += " Vol=";
    s += s_volume;
  }
  
  if (secPrec != sec)
  {
    PRINTLN ((s.c_str()));
    secPrec = sec;
    secCnt++;
    dtMax = -1;
    s= "No info";
  }
  
}
