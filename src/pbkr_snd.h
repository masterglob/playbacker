#ifndef _pbkr_snd_h_
#define _pbkr_snd_h_


#include <sys/select.h>
#include <sys/types.h>
#include <string>

#include <alsa/asoundlib.h>

namespace PBKR
{
namespace SOUND
{

typedef uint16_t MonoSample;
typedef uint32_t StereoSample;

// Output sound frequency
static const unsigned int FREQUENCY_HZ (44100u);
#define SND_MAX_LEVEL        ((float)0x7FFF)

/*******************************************************************************
 * SOUNDPLAYER CLASS
 *******************************************************************************/
class SoundPlayer
{
public:
	SoundPlayer(const char * device_name = "hw:0");
	virtual ~SoundPlayer(void);
	const char* pcm_name(void)const{return _pcm_name;}
	/* Append samples in buffer.
	 * Plays only when buffer is full
	 * @param count number of elemetns in samples
	 */
	void fill(const StereoSample* samples, snd_pcm_uframes_t count);
	/*
	 * Directly plays a buffer
	 */
	void play(unsigned char * data, int frames);

	/* Append 1 sample (L+R) in buffer. Applies the volume level.
	 * @param l,r :value between -1 and 1. values above will saturate
	 */
	inline void write_sample (float l, float r);
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

// Force float between -1 and 1
#define SND_NORMALIZE(f) f = (f > 1 ? 1 : (f < -1 ? -1 : f))
#define SND_FLOAT_TO_SAMPLE16(F) (( (int) (SND_MAX_LEVEL * (F))  ) & 0xFFFF)
inline void SoundPlayer::write_sample (float l, float r)
{
	StereoSample s;
	SND_NORMALIZE(l);
	SND_NORMALIZE(r);
	s = SND_FLOAT_TO_SAMPLE16(l);
	s |= SND_FLOAT_TO_SAMPLE16(r)<<16;
	fill(&s,1);
}

} // namespace SOUND
} // namespace PBKR
#endif // _pbkr_snd_h_
