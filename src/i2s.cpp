/* to compile:


g++ src/i2s.cpp -o out/i2s -lossredir -laoss -ldl

 */
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

#define PI 3.14159265

namespace PBKR
{
/*******************************************************************************
 *
 *******************************************************************************/

typedef uint16_t MonoSample;
typedef uint32_t StereoSample;


#define MAX_VOLUME        ((float)0x7FFF)
#define SND_FREQ          44100u
#define SND_FRAGMENTS     2
#define SND_BUFFER_SIZE   4096
#define SND_BUFF_SAMPLES  (SND_BUFFER_SIZE / 2)
#define SND_CHANNEL_SAMPLES  (SND_BUFF_SAMPLES / 2) // number of samples on each channel in  a buffer
#define BYTE_PER_STEREO_SAMPLE     (SND_BUFFER_SIZE / SND_CHANNEL_SAMPLES)
#define ZERO_LEVEL        ((PBKR::MonoSample) 0x0000u)
#define ZERO_LEVEL_ST     ((PBKR::StereoSample) 0x80008000u)

#define FLOAT_TO_SAMPLE16(F) (( (int) (MAX_VOLUME * (F))  ) & 0xFFFF)
#define TO_STEREO(L,R) (FLOAT_TO_SAMPLE16(L) | ((FLOAT_TO_SAMPLE16(R))<<16))


class SoundPlayer
{
public:
	SoundPlayer(const char * device_name = "hw:0");
	virtual ~SoundPlayer(void);
	const char* pcm_name(void)const{return _pcm_name;}
	void fill(const StereoSample* samples, snd_pcm_uframes_t count);
	void play(unsigned char * data, int frames);
private:
	void fail(const std::string& msg);
	/* Handle for the PCM device */
	snd_pcm_t *pcm_handle;
	/* Playback stream */
	const snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	/* This structure contains information about    */
	/* the hardware and can be used to specify the  */
	/* configuration to be used for the PCM stream. */
	snd_pcm_hw_params_t *hwparams;
	/* Name of the PCM device, default = hw:0,0          */
	char* _pcm_name;

	StereoSample* _samples;
	StereoSample* _sampleFill;

	/* 0 = buffer empty */
	size_t _sampleToFill;

};

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
	int dir;          /* exact_rate == rate --> dir = 0 */
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
	exact_rate = SND_FREQ;
	if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &exact_rate, 0) < 0) {
		fprintf(stderr, "Error setting rate.\n");
		fail("Error setting rate.");
	}
	if (SND_FREQ != exact_rate) {
		fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n"
				"==> Using %u Hz instead.\n", SND_FREQ, exact_rate);
	}
	else
	{
		fprintf(stdout,"Frequency set to %u Hz\n",exact_rate);
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

	_samples =  (StereoSample *)malloc(SND_BUFFER_SIZE);
	_sampleFill = _samples;
	_sampleToFill = 0;
}

/*******************************************************************************/
SoundPlayer::~SoundPlayer(void)
{
	/* Stop PCM device and drop pending frames */
	snd_pcm_drop(pcm_handle);

	/* Stop PCM device after pending frames have been played */
	//snd_pcm_drain(pcm_handle);
	free (_pcm_name);
	free (_samples);
}

/*******************************************************************************/
void SoundPlayer::fail(const std::string& msg)
{
	std::string s (_pcm_name);
	fprintf(stderr, "%s\n",msg.c_str());
	throw std::runtime_error (s + std::string(":") + msg);
}

/*******************************************************************************/
void SoundPlayer::play(unsigned char * data, int frames)
{
	snd_pcm_sframes_t pcmreturn;
	while ((pcmreturn = snd_pcm_writei(pcm_handle, data, frames)) < 0) {
		snd_pcm_prepare(pcm_handle);
		fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
	}

}
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
}

} // namespace PBKR


int main (int argc, char**argv)
{
	PBKR::SoundPlayer player (argc < 2 ? "hw:0" : argv[1]);

	printf("%s open!\n",player.pcm_name());

	try
	{
		/* Write num_frames frames from buffer data to    */
		/* the PCM device pointed to by pcm_handle.       */
		/* Returns the number of frames actually written. */
		// snd_pcm_sframes_t snd_pcm_writei(pcm_handle, data, num_frames);
#if 0
		unsigned char *data;
		int pcmreturn, l1, l2;
		short s1, s2;
		int frames;

		data = (unsigned char *)malloc(SND_BUFFER_SIZE);
		frames = SND_BUFFER_SIZE >> 2;
		for(l1 = 0; l1 < 100; l1++) {
			for(l2 = 0; l2 < frames; l2++) {
				s1 = (l2 % 128) * 100 - 5000;
				s2 = (l2 % 256) * 100 - 5000;
				data[4*l2] = (unsigned char)s1;
				data[4*l2+1] = s1 >> 8;
				data[4*l2+2] = (unsigned char)s2;
				data[4*l2+3] = s2 >> 8;
			}
			player.play( data, frames);
		}
#else
		static const int sinLen (200); // for 440Hz: 100.22 samples
		static const float volume (0.3);
		PBKR::StereoSample *sine = (PBKR::StereoSample *)malloc (sinLen*sizeof(*sine));

#define DO_SINE
#ifdef DO_SINE
		for (int i(0) ; i < sinLen ; i++)
		{
			const float s(sin(((float) (PI *2 * i))/sinLen));
			sine[i] = TO_STEREO(volume * s,volume * s);
		}
#else
		const int vol(MAX_VOLUME /(sinLen  / 2));
		for (int i(0) ; i < sinLen  / 2; i++)
		{
			float r(i);
			r /= sinLen  / 2;
			r -= 0.5;
			r *= 2;
			sine[i] = TO_STEREO(volume * r,volume * 0);
		}
		for (int i(sinLen  / 2) ; i < sinLen  ; i++)
		{
			float r(sinLen -i);
			r /= sinLen  / 2;
			r -= 0.5;
			r *= 2;
			sine[i] = TO_STEREO(volume * r,volume * 0);
		}
#endif

		for (int i(0) ; i < sinLen ; i++)
		{
			printf ("Sine[%02X]=%08X\n",i,sine[i]);
		}
		while (1)
		{
			player.fill(sine, sinLen);
		}

#endif
	}
	catch (std::exception& e) {
		printf("Exception :%s\n",e.what());
	}

	printf("%s closed!\n",player.pcm_name());
	return 0;
}
