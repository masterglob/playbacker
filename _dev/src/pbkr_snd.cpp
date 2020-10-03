#include "pbkr_snd.h"

#include <sys/select.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <exception>
#include <math.h>

#include <alsa/asoundlib.h>
#include <alloca.h>


#define DEBUG_SND printf
// #define DEBUG_SND(...)

/*******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************/
namespace
{

// Sound buffer size (in bytes). Contains L + R samples
#define SND_BUFFER_SIZE   4096
// number of samples on in SND_BUFFER_SIZE (L + R)
#define SND_BUFF_SAMPLES  (SND_BUFFER_SIZE / 2)
// number of samples on in SND_BUFFER_SIZE (in one channel)
#define SND_CHANNEL_SAMPLES  (SND_BUFF_SAMPLES / 2)

#define SND_FRAGMENTS     2

#define BYTE_PER_STEREO_SAMPLE     (SND_BUFFER_SIZE / SND_CHANNEL_SAMPLES)
} // namespace


/*******************************************************************************
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{
namespace SOUND
{
/*******************************************************************************/
SoundPlayer::SoundPlayer(const char * device_name):
    _pcm_name (strdup (device_name))
{
    snd_pcm_hw_params_malloc (&hwparams);
    mSampleRate = DEFAULT_FREQUENCY_HZ;

	setup();
	_samples =  (StereoSample *)malloc(SND_BUFFER_SIZE);
	_sampleFill = _samples;
	_sampleToFill = 0;
} // SoundPlayer::SoundPlayer

/*******************************************************************************/
SoundPlayer::~SoundPlayer(void)
{
    clear();

	/* Stop PCM device after pending frames have been played */
	//snd_pcm_drain(pcm_handle);
	DEBUG_SND("SoundPlayer(%s) closed!\n",pcm_name());
	free (_pcm_name);
	free (_samples);
	free (hwparams);
} // SoundPlayer::~SoundPlayer

/*******************************************************************************/
void SoundPlayer::clear(void)
{
    if (pcm_handle)
    {
        /* Stop PCM device and drop pending frames */
        snd_pcm_drop(pcm_handle);
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
    }
} // SoundPlayer::clear

/*******************************************************************************/
void SoundPlayer::setup(void)
{

    /* Open PCM. The last parameter of this function is the mode. */
    /* If this is set to 0, the standard mode is used. Possible   */
    /* other values are SND_PCM_NONBLOCK and SND_PCM_ASYNC.       */
    /* If SND_PCM_NONBLOCK is used, read / write access to the    */
    /* PCM device will return immediately. If SND_PCM_ASYNC is    */
    /* specified, SIGIO will be emitted whenever a period has     */
    /* been completely processed by the soundcard.                */
    if (snd_pcm_open(&pcm_handle, _pcm_name, stream, 0) < 0) {
        fail ("Bad PCM device");
    }

    /* Init hwparams with full configuration space */
    if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
        fail ("PCM device configure failed");
    }

    // int dir;          /* exact_rate == rate --> dir = 0 */
    /* exact_rate < rate  --> dir = -1 */
    /* exact_rate > rate  --> dir = 1 */

    /* Set access type. This can be either    */
    /* SND_PCM_ACCESS_RW_INTERLEAVED or       */
    /* SND_PCM_ACCESS_RW_NONINTERLEAVED.      */
    /* There are also access types for MMAPed */
    /* access, but this is beyond the scope   */
    /* of this introduction.                  */
    if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        fail ("Error setting access");
    }

    /* Set sample format */
    if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE) < 0) {
        fail("Error setting format.");
    }

    /* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.         */
    if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &mSampleRate, 0) < 0) {
        err_printf("Error setting rate.\n");
        fail("Error setting rate.");
    }
    /*
    if (DEFAULT_FREQUENCY_HZ != mSampleRate) {
        err_printf("The rate %d Hz is not supported by your hardware.\n"
                "==> Using %u Hz instead.\n", DEFAULT_FREQUENCY_HZ, mSampleRate);
    }*/

    /* Set number of channels */
    if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 2) < 0) {
        fail("Error setting channels.");
    }


    /* Set number of periods. Periods used to be called fragments. */
    if (snd_pcm_hw_params_set_periods(pcm_handle, hwparams, SND_FRAGMENTS, 0) < 0) {
        fail ("Error setting periods.");
    }

    /* Set buffer size (in frames). The resulting latency is given by */
    /* latency = periodsize * periods / (rate * bytes_per_frame)     */
    if (snd_pcm_hw_params_set_buffer_size(pcm_handle, hwparams, (SND_BUFFER_SIZE * SND_FRAGMENTS)>>2) < 0) {
        fail("Error setting buffersize.");
    }
    /* Apply HW parameter settings to */
    /* PCM device and prepare device  */
    if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
        fail("Error setting HW params.");
    }
    DEBUG_SND("SoundPlayer(%s) opened at %d Hz\n",pcm_name(),mSampleRate);

} // SoundPlayer::setup

/*******************************************************************************/
void SoundPlayer::fail(const std::string& msg)
{
	std::string s (_pcm_name);
	err_printf( "%s\n",msg.c_str());
	throw std::runtime_error (s + std::string(":") + msg);
} // SoundPlayer::fail

/*******************************************************************************/
void SoundPlayer::set_sample_rate(const SampleRate::Frequency freq)
{
    if (mSampleRate != freq)
    {
        mMutex.lock();
        // Successfully updated sample rate
        /* Set sample rate. If the exact rate is not supported */
        /* by the hardware, use nearest possible rate.         */
        mSampleRate = freq;
        clear();
        setup();
        mMutex.unlock();
    }
    else
    {
        err_printf("Cannot use sample rate %s: Unknown\n", freq);
    }
} // SoundPlayer::set_sample_rate

/*******************************************************************************/
void SoundPlayer::fill(const StereoSample* samples, snd_pcm_uframes_t count)
{
	// TODO : convert to a non-blocking call...
	/*DEBUG_SND("fill(%p,%u)\n",samples,count);*/

    if (mMutex.try_lock())
    {
        snd_pcm_sframes_t pcmreturn;
        while (count)
        {
            snd_pcm_uframes_t n (count);
            if (n + _sampleToFill > SND_CHANNEL_SAMPLES)
                n = SND_CHANNEL_SAMPLES - _sampleToFill;
            /*DEBUG_SND("_sampleToFill=%u (%u)\n",_sampleToFill,SND_CHANNEL_SAMPLES);
            DEBUG_SND("memcpy(%p,%p,%u)\n",
                    _sampleFill, (void*)samples, n * BYTE_PER_STEREO_SAMPLE);*/

            memcpy ((void*)(_sampleFill), samples, n * BYTE_PER_STEREO_SAMPLE);
            _sampleToFill += n;
            _sampleFill += n;
            samples += n;
            count -= n;
            /*DEBUG_SND("_sampleToFill=%u (%u)\n",_sampleToFill,SND_CHANNEL_SAMPLES);*/
            if (_sampleToFill == SND_CHANNEL_SAMPLES)
            {
                while ((pcmreturn = snd_pcm_writei(pcm_handle, _samples, _sampleToFill )) < 0) {
                    snd_pcm_prepare(pcm_handle);
                    err_printf( "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
                }
                _sampleToFill = 0;
                _sampleFill = _samples;
            }
        }
        mMutex.unlock();
    }
} // SoundPlayer::fill

} // namespace SOUND
} // namespace PBKR
