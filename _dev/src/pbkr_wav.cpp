
#include "pbkr_wav.h"
#include "pbkr_utils.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


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
static const int NB_INTERLEAVED_CHANNELS  (3u);
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
    fread(mIf, (char*)&mAudioHdr, sizeof(mAudioHdr));
    CHECK_FIELD("File truncated2", mIf.gcount(), sizeof(mAudioHdr));
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
                _hasMidiTrack(mAudioHdr.nChannels == 3),
                _eltSize(_hasMidiTrack ? sizeof(LRC_SampleWithMidi) : sizeof(LRC_SampleWithoutMidi) ),
                _bufferIdx(0)
{
    if (mAudioHdr.nChannels != 2 && mAudioHdr.nChannels != 3)
    {
        CHECK_FIELD("NB Channels =2 or 3", mAudioHdr.nChannels, -1);
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
    const streamoff offset(bytesPerSeconde * nbSeconds);
    const int sign(forward ? 1 : -1);
    mIf.seekg(offset * sign, ios_base::cur);
    cout << "FF, sec= "<< nbSeconds << ", dir=" << sign << ", EOF =" << mIf.eof() << ", GOOD=" << mIf.good() << endl;
    if (mIf.good())
    {
        printf("Pos=%s\n",getTimeCode().c_str());
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
        const size_t samplePos ((currBytePos - mHdrLen) / (NB_INTERLEAVED_CHANNELS * NB_BYTES_PER_SAMPLE));
        const size_t posSec ((samplePos) / mAudioHdr.nSamplesPerSec);
        char s[10];
        snprintf(s, 10, "%02d:%02d\n",posSec/60, posSec%60);
        return s;
    }
    else return "  :  ";
}

/*******************************************************************************/
bool
WavFileLRC::getNextSample(float & l, float & r, int16_t& midi)
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
            midi = -1;
            return false;
        }
    }
    if (_hasMidiTrack)
    {
        LRC_SampleWithMidi& sample (b->_samples._data3[b->_pos++]);
        l = ((float)sample.l) * READ_VOLUME;
        r = ((float)sample.r) * READ_VOLUME;

        midi = sample.midi;
    }
    else
    {
        LRC_SampleWithoutMidi& sample (b->_samples._data2[b->_pos++]);
        l = ((float)sample.l) * READ_VOLUME;
        r = ((float)sample.r) * READ_VOLUME;
        midi = 0;
    }

    if (_bof)
    {
        const float fadevol(_bof->position());
        l *= fadevol;
        r *= fadevol;
        if (_bof->done())
        {
            delete _bof;
            _bof = NULL;
        }
    }
    if (_eof)
    {
        // Apply fade out
        const float fadevol(_eof->position());
        l *= fadevol;
        r *= fadevol;
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

} // namespace PBKR
