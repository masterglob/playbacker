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
SoundPlayer::SoundPlayer(const char * device_name)
{
	/* Allocate the snd_pcm_hw_params_t structure on the stack. */
	snd_pcm_hw_params_alloca (&hwparams);
	_pcm_name = strdup (device_name);

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

	unsigned int exact_rate;   /* Sample rate returned by */
	/* snd_pcm_hw_params_set_rate_near */
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
	exact_rate = FREQUENCY_HZ;
	if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &exact_rate, 0) < 0) {
		fprintf(stderr, "Error setting rate.\n");
		fail("Error setting rate.");
	}
	if (FREQUENCY_HZ != exact_rate) {
		fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n"
				"==> Using %u Hz instead.\n", FREQUENCY_HZ, exact_rate);
	}

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
	printf("SoundPlayer(%s) opened at %d Hz\n",pcm_name(),exact_rate);

	_samples =  (StereoSample *)malloc(SND_BUFFER_SIZE);
	_sampleFill = _samples;
	_sampleToFill = 0;
} // SoundPlayer::SoundPlayer

/*******************************************************************************/
SoundPlayer::~SoundPlayer(void)
{
	/* Stop PCM device and drop pending frames */
	snd_pcm_drop(pcm_handle);

	/* Stop PCM device after pending frames have been played */
	//snd_pcm_drain(pcm_handle);
	printf("SoundPlayer(%s) closed!\n",pcm_name());
	free (_pcm_name);
	free (_samples);
} // SoundPlayer::~SoundPlayer

/*******************************************************************************/
void SoundPlayer::fail(const std::string& msg)
{
	std::string s (_pcm_name);
	fprintf(stderr, "%s\n",msg.c_str());
	throw std::runtime_error (s + std::string(":") + msg);
} // SoundPlayer::fail

/*******************************************************************************/
void SoundPlayer::play(unsigned char * data, int frames)
{
	snd_pcm_sframes_t pcmreturn;
	while ((pcmreturn = snd_pcm_writei(pcm_handle, data, frames)) < 0) {
		snd_pcm_prepare(pcm_handle);
		fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
	}
} // SoundPlayer::play

/*******************************************************************************/
void SoundPlayer::fill(const StereoSample* samples, snd_pcm_uframes_t count)
{
	// TODO : convert to a non-blocking call...
	/*printf("fill(%p,%u)\n",samples,count);*/
	snd_pcm_sframes_t pcmreturn;
	while (count)
	{
		snd_pcm_uframes_t n (count);
		if (n + _sampleToFill > SND_CHANNEL_SAMPLES)
			n = SND_CHANNEL_SAMPLES - _sampleToFill;
		/*printf("_sampleToFill=%u (%u)\n",_sampleToFill,SND_CHANNEL_SAMPLES);
		printf("memcpy(%p,%p,%u)\n",
				_sampleFill, (void*)samples, n * BYTE_PER_STEREO_SAMPLE);*/

		memcpy ((void*)(_sampleFill), samples, n * BYTE_PER_STEREO_SAMPLE);
		_sampleToFill += n;
		_sampleFill += n;
		samples += n;
		count -= n;
		/*printf("_sampleToFill=%u (%u)\n",_sampleToFill,SND_CHANNEL_SAMPLES);*/
		if (_sampleToFill == SND_CHANNEL_SAMPLES)
		{
			while ((pcmreturn = snd_pcm_writei(pcm_handle, _samples, _sampleToFill )) < 0) {
				snd_pcm_prepare(pcm_handle);
				fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
			}
			_sampleToFill = 0;
			_sampleFill = _samples;
		}
	}
} // SoundPlayer::fill

} // namespace SOUND
} // namespace PBKR
