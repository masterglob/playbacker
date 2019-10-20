/* to compile:


g++ src/i2s.cpp -o out/i2s -lasound

 */

#include "pbkr_snd.h"
#include "pbkr_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <exception>
#include <math.h>

#define SINE 0
#define SAW 1
#define TRIANGLE 2

#define TEST_TYPE SINE

using namespace PBKR;

class Console:public Thread
{
public:
	Console(void): Thread(),
	exitreq(false),
	doSine(false),
	_volume(0.01),
	_fader(NULL)
{}
	virtual ~Console(void){}
	virtual void body(void)
	{
		printf("Console Ready\n");
		while (not exitreq)
		{
			printf("> ");
			fflush(stdout);
			const char c ( getch());
			printf("%c\n",c);
			switch (c) {
			case 'q':
			case 'Q':
				printf("Exit requested\n");
				exitreq = true;
				break;
				// Sine on/off
			case 's':doSine = not doSine;
				break;
			case '+':
				changeVolume (_volume + 0.02);
				break;
			case '-':
				changeVolume (_volume - 0.02);
				break;
			default:
				printf("Unknown command :0x(%02X)\n",c);
				break;
			}
		}
		printf("Console Exiting\n");
	}
	bool exitreq;
	bool doSine;
	float volume(void);
	void changeVolume (float v, const float duration = volumeFaderDurationS);
private:
	static const float volumeFaderDurationS;
	float _volume;
	Fader* _fader;
};
const float Console::volumeFaderDurationS (0.1);

void Console::changeVolume (float v, const float duration)
{
	if (v > 1.0) v = 1.0;
	if (v < 0.0) v = 0.0;
	// printf("New volume : %d%%\n",(int)(_volume*100));
	const float v0(volume()); // !! Compute volume before destroying fader!
	if (_fader)
	{
		free (_fader);
	}
	_fader =  new Fader(duration,v0, v);
}

float Console::volume(void)
{
	if (_fader)
	{
		const float f(_fader->position());
		if (_fader->done())
		{
			_volume = f;
			// printf("FADER DONE\n");
			free (_fader);
			_fader=NULL;
		}
		else
			return f;
	}
	return _volume;
}

static Console console;

int main (int argc, char**argv)
{
	SOUND::SoundPlayer player1 ("hw:0");
	SOUND::SoundPlayer player2 ("hw:1");

	printf("%s open!\n",player1.pcm_name());
	printf("%s open!\n",player2.pcm_name());

	{
		console.start();

		float l,r;
		float phase = 0;
		const float phasestep=(TWO_PI * 440.0)/ 44100.0;
		Fader* fader(NULL);
		float faderVol(1.0);
		while (1)
		{
			if (console.exitreq && !fader)
			{
				printf("Exit cleany (no clip)\n");
				fader = new Fader(0.3, 1.0,0.0);
			}
			if (fader)
			{
				faderVol = fader->position();
				if (fader -> done()) break;
			}
			if (console.doSine)
			{
				l = sin (phase) * console.volume() * faderVol;
				r = l;
			}
			else
			{
				l=0;
				r=0;
			}
			phase += phasestep;
			if (phase > TWO_PI) phase -=TWO_PI;

			VirtualTime::elapseSample();
			player1.write_sample(l,r);
			player2.write_sample(l,r);
		}
	}
#if 0
	try
	{
		/* Write num_frames frames from buffer data to    */
		/* the PCM device pointed to by pcm_handle.       */
		/* Returns the number of frames actually written. */
		// snd_pcm_sframes_t snd_pcm_writei(pcm_handle, data, num_frames);
#if TEST_TYPE == SAW
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
		float *sine = (float *)malloc (sinLen*sizeof(*sine));


#if TEST_TYPE == SINE

		for (int i(0) ; i < sinLen ; i++)
		{
			const float s(sin(((float) (PI *2 * i))/sinLen));
			sine[i] = (volume * s);
		}
#endif

#if TEST_TYPE == TRIANGLE
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

		int i (0);
		while (1)
		{
			const float s=(sine[i++]);
			player1.write_sample(s,s);
			i %= sinLen;
		}

#endif
	}
	catch (std::exception& e) {
		printf("Exception :%s\n",e.what());
	}
#endif

	printf("%s closed!\n",player1.pcm_name());
	printf("%s closed!\n",player2.pcm_name());
	return 0;
}

