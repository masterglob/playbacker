/* to compile:


g++ src/i2s.cpp -o out/i2s -lasound

 */

#include <pbkr_snd.h>
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

#define PI 3.14159265

int main (int argc, char**argv)
{
	SOUND::SoundPlayer player (argc < 2 ? "hw:0" : argv[1]);

	printf("%s open!\n",player.pcm_name());

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
			player.write_sample(s,s);
			i %= sinLen;
		}

#endif
	}
	catch (std::exception& e) {
		printf("Exception :%s\n",e.what());
	}

	printf("%s closed!\n",player.pcm_name());
	return 0;
}

