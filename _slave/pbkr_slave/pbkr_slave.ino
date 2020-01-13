
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
struct SoundPlay;
inline int16_t advanceSound (SoundPlay& snd);

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
struct SoundPlay
{
  const int16_t* playingPtr;
  int playingLen;
  SoundPlay(void):playingPtr(NULL),playingLen(0){}
};
static SoundPlay channel1Sound;
static SoundPlay channel2Sound;

/*******************************************************************************
 *        on_MIDI_note
 *******************************************************************************/
void on_MIDI_note(const uint8_t noteNum, const uint8_t velocity, const bool onChannel1)
{
  PRINTF (("Play note %d (ch %d) at level %d\n",noteNum, (onChannel1? 1: 2),velocity));
  SoundPlay& snd = (onChannel1 ? channel1Sound : channel2Sound);
  if (noteNum == 24)
  {
    snd.playingLen = sizeof(SAMPLES_Clic1) /sizeof(*SAMPLES_Clic1);
    snd.playingPtr = SAMPLES_Clic1;
    clic_volume = velocity / 128.0;
  }
  if (noteNum == 25)
  {
    snd.playingLen = sizeof(SAMPLES_Clic2) /sizeof(*SAMPLES_Clic2);
    snd.playingPtr = SAMPLES_Clic2;
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
#ifdef LED_OUT
  pinMode(LED_OUT,OUTPUT);
  digitalWrite(LED_OUT,LOW);
#endif
  
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
inline int16_t advanceSound (SoundPlay& snd)
{
   int16_t res= 0;
    if (snd.playingPtr)
    {
      res = *(snd.playingPtr);
      snd.playingPtr += I2S_HZ_FREQ_DIV;
      snd.playingLen -= I2S_HZ_FREQ_DIV;
      if(snd.playingLen <= 0)
      {
        snd.playingPtr = NULL;
      }
    }
    return res;
}

static int last_rcv(0);
static bool hasSerialRcv(false);
inline void process_MIDI_in(void)
{
  hasSerialRcv = SerialRx.available();
  if (hasSerialRcv > 0)
  {
    const uint8_t c = uint8_t (SerialRx.read());
    //PRINTF(("Rcv 0x%02X\n",c));
    commmgt_rcv(c);
    last_rcv = millis();
  }
}

void loop() {

  do
  {
    process_MIDI_in();
  } while (hasSerialRcv);

#ifdef LED_OUT
  const int ms (millis());
  const int dms (ms - last_rcv);
  
  if (last_rcv == 0 || dms < 25)
  {
      digitalWrite(LED_OUT,(ms % 2 ? HIGH : LOW));
  }
  else if (dms > 5000)
  {
      digitalWrite(LED_OUT,HIGH);
  }
  else
  {
      digitalWrite(LED_OUT,LOW);
  }
#endif

  if (digitalRead(BTN_TEST))
  {
    //PRINTLN(("BTN pressed"));
    sineTest = true;
    secCnt = 0;
  }
  else
  {
    sineTest = false;
  }
  
  // put your main code here, to run repeatedly:
  if (sineTest == false && channel1Sound.playingPtr == NULL && channel2Sound.playingPtr == NULL) return;
  
  if (secCnt>=DEBUG_LRC_SINE_TEST) sineTest = false;

  const int t0(micros()/10);
  while (i2s_is_full()){yield();}
  const int t1(micros()/10);
  while (!i2s_is_full()){

    unsigned int nb_ech;
    
    if (sineTest)
    {
      const int16_t v = LRC_wavSine[phase];
      LRC_i2s_writeDAC(v*clic_volume*s_volume,v*clic_volume*s_volume);
      phase += I2S_HZ_FREQ_DIV;
    }
    else
    {
      // process a minimum samples to avoid underflow,
      // but allows MIDI management
      for (nb_ech = 20 ; nb_ech -- ; (!i2s_is_full()) && nb_ech > 0)
      {
        const int16_t v1 = advanceSound (channel1Sound);
        const int16_t v2 = advanceSound (channel2Sound);
        LRC_i2s_writeDAC(v1*clic_volume*s_volume,v2*clic_volume*s_volume);
      }
      process_MIDI_in();
    }
  }
  const int t2(micros()/10);
}
