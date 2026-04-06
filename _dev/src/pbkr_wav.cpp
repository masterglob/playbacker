
#include "pbkr_wav.h"
#include "pbkr_utils.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <map>



// #define DEBUG_WAV printf
#define DEBUG_WAV(...)

/*******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************/
namespace
{

static const int WAV_HDR_RIFF (0x46464952u);
static const int WAV_HDR_WAVE (0x45564157u);
static const int WAV_HDR_FMT  (0x20746D66u);
static const int WAV_FMT_PCM  (0x0001u);
static const int NB_BYTES_PER_SAMPLE (2u);
static const int SUB_FORMAT_MASK (0xFFFEu);
size_t fileLength(std::ifstream& f)
{
    const size_t intPos ((size_t) f.tellg());
    f.seekg(0,std::ios::end);
    const size_t result ((size_t) f.tellg());
    f.seekg(intPos,std::ios::beg);
    return result;
}
} // namespace


#define DEBUG (void)

#define CHECK_FIELD(label, found, exp) do{\
        if ((found) != (exp)) \
        {\
            printf("%s : found 0x%X, exp 0x%X\n", label, (unsigned int)found, (unsigned int)exp);\
            throw std::invalid_argument(label);\
        }else{\
            /*DEBUG_WAV("%s OK: 0x%X\n", label, (unsigned int)found);*/\
        }}while(0)

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{
/*******************************************************************************/
WavFile::WavFile (const std::string& filename):
                mFilename(filename),
                mIf(mFilename),
                mLen(fileLength(mIf))
{
    ZERO(mWavHdr);
    ZERO(mAudioHdr);
    ZERO(mAddHdr);
    DEBUG_WAV("opened file %s, len =%d\n",_filename.c_str(), _len);
    fread(mIf, (char*)&mWavHdr, sizeof(mWavHdr));
    CHECK_FIELD("File truncated1", mIf.gcount(), sizeof(mWavHdr));

    // CHECK WAV HEADER
    CHECK_FIELD("Bad RIFF HDR", mWavHdr.FileTypeBlocID, WAV_HDR_RIFF);
    CHECK_FIELD("Bad WAVE HDR", mWavHdr.FileFormatID, WAV_HDR_WAVE);
    CHECK_FIELD("Bad file size", mWavHdr.FileSize + 8, mLen);

    // CHECK AUDIO HEADER
    do
    {
        fread(mIf, (char*)&mAudioHdr, sizeof(mAudioHdr));
        CHECK_FIELD("File truncated2", mIf.gcount(), sizeof(mAudioHdr));
        if (mAudioHdr.ckID != WAV_HDR_FMT)
        {
            printf("Found unknown header '%.4s' at pos #%08X (%u bytes)\n",
                    reinterpret_cast<const char*>(&mAudioHdr.ckID),
                    static_cast<int>(mIf.tellg())-sizeof(mAudioHdr),
                    mAudioHdr.cksize);
            // Try to remove unknown headers
            // Any header is always composed of a 4bytes identifier then a 4 bytes value (len)
            // followed by 'len' bytes

            // just skip remaining header
            int toSkip(mAudioHdr.cksize + 8- sizeof(mAudioHdr));
            mIf.seekg(toSkip, ios_base::cur);
        }
    } while (mAudioHdr.ckID != WAV_HDR_FMT);
    CHECK_FIELD("FMT type", mAudioHdr.ckID, WAV_HDR_FMT);
    CHECK_FIELD("cksize >= 16", mAudioHdr.cksize >= 16, true);
    const int addParams (mAudioHdr.cksize - 16);

    if (addParams > (int) sizeof(AddHdr))
    {
        uint16_t cbSize;
        fread(mIf, (char*)&cbSize, sizeof(cbSize));
        CHECK_FIELD("File truncated3", mIf.gcount(), sizeof(cbSize));
        if (cbSize == 22)
        {

            fread(mIf, (char*)&mAddHdr, sizeof(mAddHdr));
            CHECK_FIELD("File truncated4", mIf.gcount(), sizeof(mAddHdr));
        }
        else if  (cbSize == 0)
        {
            //  No additional HDR
        }
        else
        {
            CHECK_FIELD("cbSize!=0|22",false,true);
        }
    }
    else if (addParams > 0)
    {
        char extraParams[addParams];
        fread(mIf, extraParams, sizeof(extraParams));
        printf("Extra params, size = %d\n", sizeof(extraParams));
    }
    // actual format (
    if (mAudioHdr.wFormatTag != SUB_FORMAT_MASK)
    {
        CHECK_FIELD("PCM type", mAudioHdr.wFormatTag, WAV_FMT_PCM);
    }
    // CHECK DATA HEADER
    uint32_t dataMark;
    uint32_t dataSize;
    fread(mIf, (char*)&dataMark, sizeof(dataMark));
    CHECK_FIELD("File truncated5", mIf.gcount(), sizeof(dataMark));
    fread(mIf, (char*)&dataSize, sizeof(dataSize));
    CHECK_FIELD("File truncated6", mIf.gcount(), sizeof(dataSize));

    mHdrLen = (size_t) mIf.tellg();

    reset();

};

/*******************************************************************************/
WavFile::~WavFile (void)
{
    mIf.close();
    DEBUG_WAV("Closed %s\n", mFilename.c_str());
}

/*******************************************************************************/
void
WavFile::reset(void)
{
    mIf.close();
    mIf.open(mFilename);
    mIf.seekg(mHdrLen,std::ios::beg);
}

/*******************************************************************************/
bool
WavFile::eof(void)
{
    return mIf.eof();
}

/*******************************************************************************/
bool
WavFile::is_open(void)
{
    return (mIf && mIf.is_open());
}

/*******************************************************************************/
WavFileLRC::WavFileLRC (const std::string& path, const std::string& filename):
                WavFile(path + "/" + filename),
                _eof(NULL),
                _bof(NULL),
                _hasAudioClickTrack(mAudioHdr.nChannels == 4),
                _eltSize(_hasAudioClickTrack ? sizeof(LRC_SampleWithAudioClick) :
                        sizeof(LRC_SampleStereo)),
                _bufferIdx(0)
{
    if (mAudioHdr.nChannels < 2 || mAudioHdr.nChannels > 4)
    {
        CHECK_FIELD("NB Channels  in [2 .. 4]", mAudioHdr.nChannels, -1);
    }

    CHECK_FIELD("Sampling rate", actualSampleRate.supported(mAudioHdr.nSamplesPerSec), true);

    CHECK_FIELD("bit depth", mAudioHdr.wBitsPerSample, 8 * NB_BYTES_PER_SAMPLE);
    CHECK_FIELD("Blk size", mAudioHdr.nBlockAlign, mAudioHdr.nChannels * NB_BYTES_PER_SAMPLE);

    if (mAddHdr.wValidBitsPerSample != 0)
    {
        CHECK_FIELD("bit depth2", mAddHdr.wValidBitsPerSample, 8 * NB_BYTES_PER_SAMPLE);
        CHECK_FIELD("Sub PCM type", mAddHdr.wFormatTag, WAV_FMT_PCM);
    }

}

/*******************************************************************************/
WavFileLRC::~WavFileLRC (void)
{
    if (_eof) delete(_eof);
    if (_bof) delete (_bof);
}

/*******************************************************************************/
void
WavFileLRC::reset(void)
{
    m_mutex.lock();
    if (_bof) delete (_bof);
    _bof = new Fader(0.2, 0.0, 1.0 );
    if (_eof) delete(_eof);
    _eof=NULL;
    WavFile::reset();
    // read first buffer
    ZERO(_buffers[0]._samples);
    fread(mIf, &_buffers[0]._samples._buffer, _eltSize * WAV_BUFFER_SAMPLES);

    _buffers[0]._pos = 0;
    for (size_t i(0) ; i < WAV_NB_BUFFERS ; i++)
    {
        readBuffer(i);
    }
    _bufferIdx = 0;
    m_mutex.unlock();
}

/*******************************************************************************/
void
WavFileLRC::fastForward(bool forward, const int nbSeconds)
{
    m_mutex.lock();
    // Note : 6 bytes per sample
    static const int bytesPerSeconde (mAudioHdr.nSamplesPerSec * 6);
    streamoff offset(bytesPerSeconde * nbSeconds);
    const int curPos = mIf.tellg();
    cout << "FF, TELLG() =" << curPos << ",OFF=" << offset << endl;
    if (curPos < 0)
    {
        cout << "FF, TELLG failed, GOOD=" << mIf.good() << endl;
    }
    else
    {
        if (forward)
        {
            mIf.seekg(offset, ios_base::cur);
        }
        else
        {
            if (curPos - offset > mHdrLen)
            {
                mIf.seekg(-offset, ios_base::cur);
            }
            else
            {
                mIf.seekg(mHdrLen, ios_base::beg);
            }
            cout << "FF, sec= "<< nbSeconds << ", dir=" << (forward ? "FWD" : "BWD" ) << ", EOF =" << mIf.eof() << ", GOOD=" << mIf.good() << endl;
        }
        if (mIf.good())
        {
            printf("Pos=%s\n",getTimeCode().c_str());
        }
    }
    m_mutex.unlock();
}

/*******************************************************************************/
string
WavFileLRC::getTimeCode(void)
{
    if (mIf.good())
    {
        const size_t currBytePos ((size_t)mIf.tellg());
        const size_t samplePos ((currBytePos - mHdrLen) / (mAudioHdr.nChannels * NB_BYTES_PER_SAMPLE));
        const size_t posSec ((samplePos) / mAudioHdr.nSamplesPerSec);
        char s[10];
        snprintf(s, 10, "%02d:%02d\n", (posSec/60) % 100, posSec%60);
        return s;
    }
    else return "  :  ";
}

/*******************************************************************************/
inline double
WavFileLRC::toTimePos(size_t sampleIdx) const
{
    return static_cast<double>(sampleIdx) / static_cast<double>(mAudioHdr.nSamplesPerSec);
}

/*******************************************************************************/
bool
WavFileLRC::getNextSample(float & l, float & r, float& l2, float & r2, double& timePos)
{
    static const float READ_VOLUME (1.0 / 0x8000);
    Buffer* b(&_buffers[_bufferIdx]);
    if (b->_pos >= WAV_BUFFER_SAMPLES)
    {
        m_mutex.lock();
        readBuffer(_bufferIdx);
        // get next buffer
        _bufferIdx++;
        _bufferIdx%=WAV_NB_BUFFERS;
        b = &_buffers[_bufferIdx];
        m_mutex.unlock();
        if (b->_pos >= WAV_BUFFER_SAMPLES)
        {
            // underrun from input (EOF)?
            l = 0.0;
            r = 0.0;
            l2 = 0.0;
            r2 = 0.0;
            timePos = -1.0;
            return false;
        }
    }

    timePos = toTimePos(b->_pos);
    if (_hasAudioClickTrack)
    {
        LRC_SampleWithAudioClick& sample (b->_samples._data4[b->_pos++]);
        l = ((float)sample.l) * READ_VOLUME;
        r = ((float)sample.r) * READ_VOLUME;
        l2 = ((float)sample.lClick) * READ_VOLUME;
        r2 = ((float)sample.rClick) * READ_VOLUME;
    }
    else
    {
        LRC_SampleStereo& sample (b->_samples._data2[b->_pos++]);
        l = ((float)sample.l) * READ_VOLUME;
        r = ((float)sample.r) * READ_VOLUME;
        l2 = 0.0;
        r2 = 0.0;
    }

    if (_bof)
    {
        const float fadevol(_bof->position());
        l *= fadevol;
        r *= fadevol;
        l2 *= fadevol;
        r2 *= fadevol;
        if (_bof->done())
        {
            delete _bof;
            _bof = nullptr;
        }
    }
    if (_eof)
    {
        // Apply fade out
        const float fadevol(_eof->position());
        l *= fadevol;
        r *= fadevol;
        l2 *= fadevol;
        r2 *= fadevol;
        if (_eof->done()) return false;
    }
    return true;
}

/*******************************************************************************/
void
WavFileLRC::readBuffer(size_t index)
{
    Buffer& b(_buffers[index]);
    ZERO(b._samples);
    if (!_eof)
    {
        fread(mIf, &b._samples._buffer, _eltSize * WAV_BUFFER_SAMPLES);
        //fread (_f, (char*)b._data, sizeof(b._data));
        if (!mIf)
        {
            _eof = new Fader (0.01, 1.0, 0.0);
            DEBUG_WAV("End Of file\n");
        }
    }
    b._pos = 0;
} // WavFileLRC::readNextBuffer

/*******************************************************************************
 * WavFileLRC
 *******************************************************************************/

/*******************************************************************************/
WavFile8Mono::WavFile8Mono (const std::string& filename):
                WavFile(filename)
{
    CHECK_FIELD("NB Channels", mAudioHdr.nChannels, 1);
    CHECK_FIELD("Sampling rate", mAudioHdr.nSamplesPerSec, 11025);

    CHECK_FIELD("bit depth", mAudioHdr.wBitsPerSample, 8);
    CHECK_FIELD("Blk size", mAudioHdr.nBlockAlign, mAudioHdr.nChannels * 1);

    CHECK_FIELD("wFormatTag", mAudioHdr.wFormatTag, 1);

    if (mAddHdr.wValidBitsPerSample != 0)
    {
        CHECK_FIELD("bit depth2", mAddHdr.wValidBitsPerSample, 8);
        CHECK_FIELD("Sub PCM type", mAddHdr.wFormatTag, WAV_FMT_PCM);
    }
}

/*******************************************************************************/
WavFile8Mono::~WavFile8Mono (void){}

/*******************************************************************************/
uint8_t WavFile8Mono::readSample(void)
{
    uint8_t result;
    fread (mIf, (char*) &result, 1);
    return result;
}

/*******************************************************************************/

// --- Binary helpers -----------------------------------------------------------

uint32_t MidiFile::readBE32(const uint8_t* p) {
    return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];
}
uint16_t MidiFile::readBE16(const uint8_t* p) {
    return uint16_t((p[0]<<8)|p[1]);
}
uint32_t MidiFile::readVarLen(const uint8_t*& p, const uint8_t* end) {
    uint32_t v = 0;
    for (int i = 0; i < 4 && p < end; ++i) {
        uint8_t b = *p++;
        v = (v << 7) | (b & 0x7F);
        if (!(b & 0x80)) return v;
    }
    throw std::runtime_error("Invalid VarLen");
}

// --- Parse a single MTrk chunk ------------------------------------------------
void MidiFile::parseTrack(const uint8_t* p, size_t len,
                           std::vector<RawEvent>& out) {
    const uint8_t* end = p + len;
    uint32_t abs_tick = 0;
    uint8_t  running_status = 0;

    while (p < end) {
        uint32_t delta = readVarLen(p, end);
        abs_tick += delta;

        uint8_t status = *p;
        if (status & 0x80) {          // new status byte
            if (status != 0xF0 && status != 0xFF) running_status = status;
            ++p;
        } else {                       // running status (reuse previous)
            status = running_status;
        }

        RawEvent ev;
        ev.tick   = abs_tick;
        ev.status = status;
        ev.data[0] = ev.data[1] = 0;
        ev.meta_type = 0;

        if (status == 0xFF) {          // Meta event
            ev.meta_type = *p++;
            uint32_t mlen = readVarLen(p, end);
            ev.sysex_or_meta.assign(p, p + mlen);
            p += mlen;
        } else if (status == 0xF0 || status == 0xF7) {  // SysEx
            uint32_t slen = readVarLen(p, end);
            ev.sysex_or_meta.assign(p, p + slen);
            p += slen;
        } else {                       // Channel event
            uint8_t type = status & 0xF0;
            ev.data[0] = *p++;
            // ProgramChange and ChannelPressure have only 1 data byte
            if (type != 0xC0 && type != 0xD0) ev.data[1] = *p++;
        }
        out.push_back(std::move(ev));
    }
}

// --- Constructor: load and parse the .mid file --------------------------------

MidiFile::MidiFile(const std::string path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error(std::string("Cannot open: ") + path);

    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());
    const uint8_t* p   = buf.data();
    const uint8_t* end = p + buf.size();

    // MThd header
    if (std::memcmp(p, "MThd", 4)) throw std::runtime_error("Not a MIDI file");
    uint32_t hlen = readBE32(p+4);
    format_       = readBE16(p+8);
    num_tracks_   = readBE16(p+10);
    ticks_per_qn_ = readBE16(p+12);
    p += 8 + hlen;

    // MTrk chunks — unknown chunk types are skipped as per the MIDI spec
    tracks_.resize(num_tracks_);
    for (uint16_t t = 0; t < num_tracks_ && p + 8 <= end; ) {
        uint32_t tlen = readBE32(p + 4);
        if (std::memcmp(p, "MTrk", 4) != 0) {
            p += 8 + tlen;  // skip unknown chunk and keep going
            continue;
        }
        p += 8;
        parseTrack(p, tlen, tracks_[t]);
        p += tlen;
        ++t;
    }
}

// --- Tick -> seconds resolution (with tempo map support) ---------------------
std::vector<MidiEvent> MidiFile::buildEventList() const {
    // 1. Build the tempo map from track 0 (meta event 0x51)
    //    maps tick -> microseconds per beat
    std::map<uint32_t, uint32_t> tempo_map;
    tempo_map[0] = 500000;  // default: 120 BPM

    for (auto& track : tracks_) {
        for (auto& ev : track) {
            if (ev.status == 0xFF && ev.meta_type == 0x51
                && ev.sysex_or_meta.size() >= 3) {
                auto& d = ev.sysex_or_meta;
                uint32_t usec = (uint32_t(d[0])<<16)|(uint32_t(d[1])<<8)|d[2];
                tempo_map[ev.tick] = usec;
            }
        }
    }

    // 2. Convert a tick position to seconds using the tempo map
    auto tickToSec = [&](uint32_t tick) -> double {
        double sec = 0.0;
        uint32_t prev_tick = 0;
        uint32_t cur_tempo = 500000;

        for (auto& kv : tempo_map) {
            if (kv.first >= tick) break;
            sec += (double)(kv.first - prev_tick) / ticks_per_qn_ * cur_tempo * 1e-6;
            prev_tick = kv.first;
            cur_tempo = kv.second;
        }
        sec += (double)(tick - prev_tick) / ticks_per_qn_ * cur_tempo * 1e-6;
        return sec;
    };

    // 3. Merge all tracks into a single flat list (channel events only)
    //    Meta and SysEx are skipped — they were already consumed by the tempo map.
    // Count channel events first so reserve() is exact and push_back()
    // never triggers a reallocation (avoids GCC <7.1 _M_realloc_insert warning).
    size_t count = 0;
    for (auto& track : tracks_)
        for (auto& raw : track)
            if (raw.status != 0xFF && raw.status != 0xF0 && raw.status != 0xF7)
                ++count;

    // resize + index assignment avoids push_back, which always instantiates
    // _M_realloc_insert and triggers a GCC <7.1 ABI warning at compile time.
    std::vector<MidiEvent> events(count);
    size_t idx = 0;

    for (auto& track : tracks_) {
        for (auto& raw : track) {
            if (raw.status == 0xFF || raw.status == 0xF0 || raw.status == 0xF7)
                continue;

            MidiEvent& ev = events[idx++];
            ev.tick     = raw.tick;
            ev.time_sec = tickToSec(raw.tick);
            ev.type     = static_cast<MidiEventType>(raw.status & 0xF0);
            ev.channel  = raw.status & 0x0F;
            ev.data1    = raw.data[0];
            ev.data2    = raw.data[1];
        }
    }

    // 4. Sort by time_sec (multi-track files may interleave events)
    // Insertion sort avoids std::stable_sort / lower_bound internals that
    // trigger a GCC <7.1 iterator-passing ABI warning. Events from a well-formed
    // MIDI file are nearly sorted, so this is O(n) in the common case.
    for (size_t i = 1; i < events.size(); ++i) {
        MidiEvent key = events[i];
        size_t j = i;
        while (j > 0 && events[j - 1].time_sec > key.time_sec) {
            events[j] = events[j - 1];
            --j;
        }
        events[j] = key;
    }

    return events;
}

} // namespace PBKR
