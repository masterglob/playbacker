#ifndef _pbkr_wav_h_
#define _pbkr_wav_h_

#include "pbkr_utils.h"
#include "pbkr_config.h"

#include <mutex>
#include <iostream>
#include <fstream>

#define PACK __attribute__((__packed__))
namespace PBKR
{

static const size_t WAV_NB_BUFFERS(2);
static const size_t WAV_BUFFER_SAMPLES(1024);

/*******************************************************************************
 * WavFile
 *******************************************************************************/
struct WavFile
{
    WavFile (const std::string& filename);
    virtual  ~WavFile (void);
    virtual void reset(void);

    bool is_open(void);
    bool eof(void);

    const std::string mFilename;

protected:
    /** WAV file format (https://fr.wikipedia.org/wiki/Waveform_Audio_File_Format)
     * http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html */
    struct PACK WAVHdr
    {
        // Full size = 44
        // Hdr WAVE
        uint32_t FileTypeBlocID; // «RIFF»  (0x52,0x49,0x46,0x46)
        uint32_t FileSize; // FileSize minus 8 bytes
        uint32_t FileFormatID; // «WAVE»  (0x57,0x41,0x56,0x45)
    };

    struct PACK AudioHdr
    {
        // HDR Audio
        uint32_t ckID; // Chunk ID: "fmt "
        uint32_t cksize; // Chunk size: 16, 18 or 40 (after that field)
        uint16_t wFormatTag; // Format code 'FFFFE'= see subformat)
        uint16_t nChannels; // Number of interleaved channels
        uint32_t nSamplesPerSec; // Sampling rate (blocks per second)
        uint32_t nAvgBytesPerSec; // Data rate
        uint16_t nBlockAlign; // Data block size (bytes)
        uint16_t wBitsPerSample; //  Bits per sample
    };

    struct PACK AddHdr
    {
        uint16_t wValidBitsPerSample;
        uint16_t dwChannelMask1;
        uint16_t dwChannelMask2;
        uint16_t wFormatTag; // Format code
        uint8_t  SubFormat[16 - 2];
    };

    struct PACK DataHdr
    {
        // Data HDR
        uint32_t DataBlocID; // Constante «data»  (0x64,0x61,0x74,0x61)
        uint32_t DataSize; // Nombre d'octets des données de Data (= full size - 44)
        uint16_t Data[];
    };
protected:

    std::ifstream mIf;
    size_t mHdrLen;
    size_t mLen;

    WAVHdr mWavHdr;
    AudioHdr mAudioHdr;
    AddHdr mAddHdr;

};

/*******************************************************************************
 * WemosFileSender
 *******************************************************************************/
/** reading & chacking 3 track WAV file */
class WavFileLRC : public WavFile
{
public:
    /* Check LRC format:
     * - Sample rate : check in "Utils::SampleRate"
     * - 3 channels
     * - 16 bits
     */
    WavFileLRC (const std::string& path, const std::string& filename);
    virtual  ~WavFileLRC (void);
    /**
     * Return True if the file is still reading. false when terminated
     */
    bool getNextSample(float & l, float & r, int16_t& midi);

    /**
     * false = backward.
     */
    void fastForward(bool forward, const int nbSeconds);
    string getTimeCode(void);

    inline SampleRate::Frequency frequency(void)const{return mAudioHdr.nSamplesPerSec;}
    virtual void reset(void);
private:
    Fader* _eof;  // Fade out
    Fader* _bof;  // Fade in


    struct PACK LRC_Sample
    {
        int16_t l;
        int16_t r;
        int16_t midi;
    };

    struct Buffer
    {
        Buffer (void):_pos(WAV_BUFFER_SAMPLES){ZERO(_data);}
        size_t _pos; // Reading Pos
        LRC_Sample _data[WAV_BUFFER_SAMPLES];
    };
    Buffer _buffers[WAV_NB_BUFFERS];
    size_t _bufferIdx;
    std::mutex m_mutex;
    void readBuffer(size_t index);
};

/*******************************************************************************
 * WavFile8Mono
 *******************************************************************************/
class WavFile8Mono : public WavFile
{
public:
    WavFile8Mono (const std::string& filename);
    virtual  ~WavFile8Mono (void);
    uint8_t readSample(void);
};
} // namespace PBKR
#endif // _pbkr_wav_h_
