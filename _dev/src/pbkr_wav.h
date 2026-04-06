#ifndef _pbkr_wav_h_
#define _pbkr_wav_h_

#include "pbkr_utils.h"
#include "pbkr_config.h"

#include <mutex>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <stdexcept>

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
        uint32_t FileTypeBlocID; // "RIFF"  (0x52,0x49,0x46,0x46)
        uint32_t FileSize; // FileSize minus 8 bytes
        uint32_t FileFormatID; // "WAVE"  (0x57,0x41,0x56,0x45)
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
        uint32_t DataBlocID; // Constante "data"  (0x64,0x61,0x74,0x61)
        uint32_t DataSize; // Nombre d'octets des donnees de Data (= full size - 44)
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
/** reading & checking 3 track WAV file */
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
    bool getNextSample(float & l, float & r, float& l2, float & r2, double& timePos);

    /**
     * false = backward.
     */
    void fastForward(bool forward, const int nbSeconds);
    string getTimeCode(void);
    double toTimePos(size_t sampleIdx) const;

    inline SampleRate::Frequency frequency(void)const{return mAudioHdr.nSamplesPerSec;}
    virtual void reset(void);
private:
    Fader* _eof;  // Fade out
    Fader* _bof;  // Fade in

    const bool _hasAudioClickTrack;
    const size_t _eltSize;

    struct PACK LRC_SampleWithAudioClick
    {
        int16_t l;
        int16_t r;
        int16_t lClick;
        int16_t rClick;
    };

    struct PACK LRC_SampleStereo
    {
        int16_t l;
        int16_t r;
    };

    struct Buffer
    {
        Buffer (void):_pos(WAV_BUFFER_SAMPLES){ZERO(_samples);}
        size_t _pos; // Reading Pos
        union
        {
            char _buffer;
            LRC_SampleStereo            _data2[WAV_BUFFER_SAMPLES];
            LRC_SampleWithAudioClick    _data4[WAV_BUFFER_SAMPLES];
        } _samples;
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

/*******************************************************************************
 * MidiFile
 *******************************************************************************/
enum class MidiEventType : uint8_t {
    NoteOff         = 0x80,
    NoteOn          = 0x90,
    PolyPressure    = 0xA0,
    ControlChange   = 0xB0,
    ProgramChange   = 0xC0,
    ChannelPressure = 0xD0,
    PitchBend       = 0xE0,
    SysEx           = 0xF0,
    Meta            = 0xFF,
};

// Channel events only — Meta and SysEx are used internally during parsing
// (tempo map) but are not exposed. This keeps MidiEvent a trivial flat struct
// with no heap allocation, safe for vector reallocation on old GCC ABI.
struct MidiEvent {
    uint32_t      tick     = 0;    // absolute position in MIDI ticks
    double        time_sec = 0.0;  // position in seconds (after tempo resolution)
    MidiEventType type     = MidiEventType::NoteOff;
    uint8_t       channel  = 0;    // 0-15
    uint8_t       data1    = 0;    // note / CC number / etc.
    uint8_t       data2    = 0;    // velocity / CC value / etc.
};

// ─── MIDI parser (no external dependencies) ───────────────────────────────────
class MidiFile {
public:
    explicit MidiFile(const std::string path);  // see .cpp

    // Resolves ticks -> seconds and returns a flat list of events sorted by time_sec.
    // Iterate or index it directly — no wrapper needed.
    std::vector<MidiEvent> buildEventList() const;

    uint16_t format()     const noexcept { return format_; }
    uint16_t numTracks()  const noexcept { return num_tracks_; }
    uint16_t ticksPerQN() const noexcept { return ticks_per_qn_; }

private:
    struct RawEvent {
        uint32_t tick;
        uint8_t  status;
        uint8_t  data[2];
        uint8_t  meta_type;
        std::vector<uint8_t> sysex_or_meta;
    };

    uint16_t format_       = 0;
    uint16_t num_tracks_   = 0;
    uint16_t ticks_per_qn_ = 480;

    std::vector<std::vector<RawEvent>> tracks_;  // one vector per track

    static uint32_t readVarLen(const uint8_t*& p, const uint8_t* end);
    static uint32_t readBE32(const uint8_t* p);
    static uint16_t readBE16(const uint8_t* p);
    void parseTrack(const uint8_t* p, size_t len, std::vector<RawEvent>& out);
};

} // namespace PBKR
#endif // _pbkr_wav_h_
