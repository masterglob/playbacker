
#include "pbkr_wav.h"
#include "pbkr_utils.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>


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
static const int NB_SAMPLES_PER_SEC (44100u);
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
        if (found != exp) \
        {\
            printf("%s : found 0x%X, exp 0x%X\n", label, (unsigned int)found, (unsigned int)exp);\
            throw std::invalid_argument(label);\
        }else{\
            /*printf("%s OK: 0x%X\n", label, (unsigned int)found);*/\
        }}while(0)

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{

/*******************************************************************************/
WavFileLRC::WavFileLRC (const std::string& path, const std::string& filename):
                _filename(filename),
                _f(path + filename),
                _len(fileLength(_f)),
                _goodFormat(true),
                _eof(NULL),
                _bufferIdx(0)
{
    WAVHdr wavHdr;
    AudioHdr audioHdr;
    AddHdr addHdr;
    ZERO(wavHdr);
    ZERO(audioHdr);
    ZERO(addHdr);
    readFile((char*)&wavHdr, sizeof(wavHdr));
    CHECK_FIELD("File truncated1", _f.gcount(), sizeof(wavHdr));

    // CHECK WAV HEADER
    CHECK_FIELD("Bad RIFF HDR", wavHdr.FileTypeBlocID, WAV_HDR_RIFF);
    CHECK_FIELD("Bad WAVE HDR", wavHdr.FileFormatID, WAV_HDR_WAVE);
    CHECK_FIELD("Bad file size", wavHdr.FileSize + 8, _len);

    // CHECK AUDIO HEADER
    if (_goodFormat)
    {
        readFile((char*)&audioHdr, sizeof(audioHdr));
        CHECK_FIELD("File truncated2", _f.gcount(), sizeof(audioHdr));
        CHECK_FIELD("FMT type", audioHdr.ckID, WAV_HDR_FMT);
        CHECK_FIELD("NB Channels", audioHdr.nChannels, NB_INTERLEAVED_CHANNELS);
        CHECK_FIELD("Sampling rate", audioHdr.nSamplesPerSec, NB_SAMPLES_PER_SEC);
        CHECK_FIELD("Blk size", audioHdr.nBlockAlign, NB_INTERLEAVED_CHANNELS * NB_BYTES_PER_SAMPLE);
        CHECK_FIELD("bit depth", audioHdr.wBitsPerSample, 8 * NB_BYTES_PER_SAMPLE);
        if (wavHdr.FileSize > 0x10 + 1)
        {
            uint16_t cbSize;
            readFile((char*)&cbSize, sizeof(cbSize));
            CHECK_FIELD("File truncated3", _f.gcount(), sizeof(cbSize));
            if (cbSize == 22)
            {

                readFile((char*)&addHdr, sizeof(addHdr));
                CHECK_FIELD("File truncated4", _f.gcount(), sizeof(addHdr));
                CHECK_FIELD("bit depth2", addHdr.wValidBitsPerSample, 8 * NB_BYTES_PER_SAMPLE);
                CHECK_FIELD("Sub PCM type", addHdr.wFormatTag, WAV_FMT_PCM);
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
        // actual format (
        if (audioHdr.wFormatTag != SUB_FORMAT_MASK)
        {
            CHECK_FIELD("PCM type", audioHdr.wFormatTag, WAV_FMT_PCM);
        }
    }
    // CHECK DATA HEADER
    if (_goodFormat)
    {
        uint32_t dataMark;
        uint32_t dataSize;
        readFile((char*)&dataMark, sizeof(dataMark));
        CHECK_FIELD("File truncated5", _f.gcount(), sizeof(dataMark));
        readFile((char*)&dataSize, sizeof(dataSize));
        CHECK_FIELD("File truncated6", _f.gcount(), sizeof(dataSize));
        _eof = NULL;
        // read first buffer
        ZERO(_buffers[0]._data);
        readFile((char*)_buffers[0]._data, sizeof(_buffers[0]._data));
        _buffers[0]._pos = 0;
        for (size_t i(0) ; i < WAV_NB_BUFFERS ; i++)
        {
            readBuffer(i);
        }
        _bufferIdx = 0;
    }
    // DATA contains : [Chan1 sample 1, Chan2 Sample 1, Chan3 Sample 1, Chan1 Sample 2 ...]

    // Fill in buffers

}
#undef CHECK_FIELD

/*******************************************************************************/
WavFileLRC::~WavFileLRC (void)
{
    if (_eof) delete(_eof);
    printf("Closed %s\n", _filename.c_str());
}

/*******************************************************************************/
bool
WavFileLRC::isLRC(void)const
{
    return _goodFormat;
}

/*******************************************************************************/
bool
WavFileLRC::is_open(void)
{
    return (_f && _f.is_open());
}

/*******************************************************************************/
void
WavFileLRC::getNextSample(float & l, float & r, uint16_t midi)
{
    static const float READ_VOLUME (1.0 / 0x8000);
    Buffer* b(&_buffers[_bufferIdx]);
    if (b->_pos >= WAV_BUFFER_SAMPLES)
    {
        readBuffer(_bufferIdx);
        // get next buffer
        _bufferIdx++;
        _bufferIdx%=WAV_NB_BUFFERS;
        b = &_buffers[_bufferIdx];
        if (b->_pos >= WAV_BUFFER_SAMPLES)
        {
            // underrun from input (EOF)?
            l = 0.0;
            r = 0.0;
            midi = 0;
            return;
        }
    }
    LRC_Sample& sample (b->_data[b->_pos++]);
    l = ((float)sample.l) * READ_VOLUME;
    r = ((float)sample.r) * READ_VOLUME;
    /*printf("Sample = %d / %d\n",sample.l, sample.r);
    sleep (1.0);*/
    if (_eof)
    {
        // Apply fade out
        const float fadevol(_eof->position());
        l *= fadevol;
        r *= fadevol;
    }
    midi = sample.midi;
}

/*******************************************************************************/
void
WavFileLRC::readBuffer(size_t index)
{
    Buffer& b(_buffers[index]);
    ZERO(b._data);
    if (!_eof)
    {
        readFile((char*)b._data, sizeof(b._data));
        if (!_f)
        {
            _eof = new Fader (0.01, 1.0, 0.0);
            printf("End Of file (underrun from USB)\n");
        }
    }
    b._pos = 0;
} // WavFileLRC::readNextBuffer

/*******************************************************************************
 * WavFileLRC
 *******************************************************************************/

} // namespace PBKR