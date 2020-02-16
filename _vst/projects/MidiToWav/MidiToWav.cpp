#include "MidiToWav.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"


const int kNumPrograms = 8;


static const double maxLevel(0.5);
enum EParams
{
	kNoOffNotes = 0,
	kNumParams
};

RawSerializer::RawSerializer(bool& sendOffNotes):
	mSendOffNotes(sendOffNotes),
	mLen(0),
	mPos(-1),
	mStop(true)
{
	memset(toSend, 0, 3);
}

RawSerializer::~RawSerializer(void)
{
}

double RawSerializer::getNextValue(const int& offset)
{
	if ((mLen == 0) && !Empty())
	{
		if (mStop)
		{
			mStop = false;
			return 0.0;
		}
		// Currently not sending
		IMidiMsg*mCurMsg = Peek();

		if (mCurMsg->mOffset > offset) return 0.0;
		Remove();
		mStop = true;

		toSend[0] = mCurMsg->mStatus;
		toSend[1] = mCurMsg->mData1;
		toSend[2] = mCurMsg->mData2;
		mPos = -1;
		mLen = 3;
		const int cmd = mCurMsg->mStatus & 0xF0;
		switch (cmd)
		{
		case 0x80: // Note Off event
			if (!mSendOffNotes)
			{
				mLen = 0;
			}
		case 0x90: // Note On event. 
			if (mCurMsg->mData2 == 0 && !mSendOffNotes)
			{
				mLen = 0;
			}
			break;
		case 0xA0: // Polyphonic Key Pressure (Aftertouch). 
		case 0xB0: // Control Change. 
		case 0xE0: //Pitch Bend Change
			break;
		case 0xC0: // Program Change. 
		case 0xD0: // Channel Pressure (After-touch)
			mLen = 2;
			break;
		case 0xF0: // System : not managed
			mLen = 0;
			break;
		}
	}
	if (mLen == 0)
		return 0.0;

	if (mPos < 0)
	{
		mPos = 0;
		return -maxLevel;
	}

	if (mPos >= mLen)
	{
		mPos = -1;
		mLen = 0;
		return -maxLevel;
	}
	return ( ((double)toSend[mPos++]) * maxLevel ) / 0x100;
}

IPlugMidiToWav::IPlugMidiToWav(IPlugInstanceInfo instanceInfo)
	: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo),
	mSampleRate(44100.),
	mSerial(mSendOffNotes)
{
	TRACE;


	GetParam(kNoOffNotes)->InitBool("Send Off Notes", false, "button");

	const IColor bgColor(255, 150, 150, 180);
	IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
	pGraphics->AttachPanelBackground(&bgColor);
	//	pGraphics->AttachBackground(BG_ID, BG_FN);

	/****************************/
	// param kNoOffNotes	
	IRECT tmpRect(kISwitchControl_OffNotes_X + 60 , kISwitchControl_OffNotes_Y, 200, 30);
	IBitmap bitmap = pGraphics->LoadIBitmap(IRADIOBUTTONSCONTROL_ID, IRADIOBUTTONSCONTROL_FN, kIRadioButtonsControl_N);
	pGraphics->AttachControl(new ISwitchControl(this, kISwitchControl_OffNotes_X, kISwitchControl_OffNotes_Y, kNoOffNotes, &bitmap));
	IText textProps(24, &COLOR_BLACK, "Arial", IText::kStyleBold, IText::kAlignNear, 0, IText::kQualityDefault);
	pGraphics->AttachControl(new ITextControl(this, tmpRect, &textProps, "Off notes"));

	

	IBitmap knob = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
	IText text = IText(14);
	IBitmap regular = pGraphics->LoadIBitmap(WHITE_KEY_ID, WHITE_KEY_FN, 6);
	IBitmap sharp = pGraphics->LoadIBitmap(BLACK_KEY_ID, BLACK_KEY_FN);


	IBitmap about = pGraphics->LoadIBitmap(ABOUTBOX_ID, ABOUTBOX_FN);
	mAboutBox = new IBitmapOverlayControl(this, 100, 100, &about, IRECT(540, 250, 680, 290));
	pGraphics->AttachControl(mAboutBox);
	AttachGraphics(pGraphics);

	//MakePreset("preset 1", ... );
	MakeDefaultPreset((char*) "-", kNumPrograms);
}

IPlugMidiToWav::~IPlugMidiToWav() {}

void IPlugMidiToWav::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
	// Mutex is already locked for us.
  //  double* in1 = inputs[0];
  //  double* in2 = inputs[1];
	double* outM = outputs[0];
	// double* out2 = outputs[1];

	GetTime(&mTimeInfo);

	for (int offset = 0; offset < nFrames; ++offset, /*++in1, ++in2,*/ ++outM/*, ++out2*/)
	{
		*outM = mSerial.getNextValue(offset);
		// peakR = IPMAX(peakR, fabs(*out2));
	}

	mSerial.Flush(nFrames);
}

void IPlugMidiToWav::Reset()
{
	TRACE;
	IMutexLock lock(this);

	mSampleRate = GetSampleRate();
	mSerial.Resize(GetBlockSize());
}

void IPlugMidiToWav::OnParamChange(int paramIdx)
{
	IMutexLock lock(this);

	switch (paramIdx)
	{
	case kNoOffNotes:
		mSendOffNotes = GetParam(paramIdx)->Bool();
	default:
		break;
	}
}


void IPlugMidiToWav::ProcessMidiMsg(IMidiMsg* pMsg)
{

	mSerial.Add(pMsg);
}

// Should return non-zero if one or more keys are playing.
int IPlugMidiToWav::GetNumKeys()
{
	IMutexLock lock(this);
	return 0;
}

// Should return true if the specified key is playing.
bool IPlugMidiToWav::GetKeyStatus(int key)
{
	IMutexLock lock(this);
	return false;
}

//Called by the standalone wrapper if someone clicks about
bool IPlugMidiToWav::HostRequestingAboutBox()
{
	IMutexLock lock(this);
	if (GetGUI())
	{
		// get the IBitmapOverlay to show
		mAboutBox->SetValueFromPlug(1.);
		mAboutBox->Hide(false);
	}
	return true;
}
