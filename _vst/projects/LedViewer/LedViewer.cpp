#include "LedViewer.h"
#include "LedConf.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include <sstream>


const int kNumPrograms = 8;


static const double maxLevel(0.5);
enum EParams
{
	kNoOffNotes = 0,
	kNumParams
};

void ITextControlRefr::CheckDirty() {
	if (mLast != text)
	{
		mLast = text;
		SetDirty();
	}
}

bool ITextControlRefr::Draw(IGraphics* pGraphics)
{
	const char* cStr = mLast.c_str();
	if (CSTR_NOT_EMPTY(cStr))
	{
		return pGraphics->DrawIText(&mText, cStr, &mRECT);
	}
	return true;
}

void ITextControlRefr::OnGUIIdle() {
	// dbg_ctrl->SetTextFromPlug(text.c_str());
}

IPlugLedViewer::IPlugLedViewer(IPlugInstanceInfo instanceInfo)
	: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo),
	mSampleRate(44100.),
	dbg_ctrl(nullptr)
{
	TRACE;


	GetParam(kNoOffNotes)->InitBool("Send Off Notes", false, "button");

	IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
	try {
		pGraphics->AttachBackground(BG_STAGE1_ID, BG_STAGE1_FN);
	}
	catch (...) {
		const IColor bgColor(255, 150, 150, 180);
		pGraphics->AttachPanelBackground(&bgColor);
		std::cout  << "Failed to load " BG_STAGE1_FN << std::endl;
	}

	// Create leds (WIP)
	ILedViewControl* led_ctrl;
	const LedVect_t& cfg = getConf();
	for (const CLedConf& ledCfg : cfg) {
		const IColor color(255, 250, 250, 250);
		std::cout << "LED: " << ledCfg.name << std::endl;

		led_ctrl = new ILedViewControl(this, ledCfg);
		ledMap.insert(ledCfg, led_ctrl);
		pGraphics->AttachControl(led_ctrl);
		//point4array points;
		//led.fillPoly(points);
		//pGraphics->FillIConvexPolygon(&color, points.x , points.y, 4);
	}

	
	// Debug info
	{
		IRECT tmpRect((GUI_WIDTH)/10, (GUI_HEIGHT * 9) / 10, 200, 30);
		IText textProps(24, &COLOR_YELLOW, "Arial", IText::kStyleBold, IText::kAlignNear, 0, IText::kQualityDefault);
		dbg_ctrl = new ITextControlRefr(this, tmpRect, &textProps, "Info");
		pGraphics->AttachControl(dbg_ctrl);
	}



	/****************************/
	// param kNoOffNotes	
/*	IRECT tmpRect(kISwitchControl_OffNotes_X + 60, kISwitchControl_OffNotes_Y, 200, 30);
	IBitmap bitmap = pGraphics->LoadIBitmap(IRADIOBUTTONSCONTROL_ID, IRADIOBUTTONSCONTROL_FN, kIRadioButtonsControl_N);
	pGraphics->AttachControl(new ISwitchControl(this, kISwitchControl_OffNotes_X, kISwitchControl_OffNotes_Y, kNoOffNotes, &bitmap));
	IText textProps(24, &COLOR_BLACK, "Arial", IText::kStyleBold, IText::kAlignNear, 0, IText::kQualityDefault);
	pGraphics->AttachControl(new ITextControl(this, tmpRect, &textProps, "Off notes"));
	//*/
	

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

IPlugLedViewer::~IPlugLedViewer() {}

void IPlugLedViewer::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
	// Mutex is already locked for us.
  //  double* in1 = inputs[0];
  //  double* in2 = inputs[1];
	double* outM = outputs[0];
	// double* out2 = outputs[1];

	GetTime(&mTimeInfo);

	for (int offset = 0; offset < nFrames; ++offset, /*++in1, ++in2,*/ ++outM/*, ++out2*/)
	{
		*outM = 0.0;
		// peakR = IPMAX(peakR, fabs(*out2));
	}
	ledMap.CheckDirty();
	dbg_ctrl->CheckDirty();
}

void IPlugLedViewer::Reset()
{
	TRACE;
	IMutexLock lock(this);

	mSampleRate = GetSampleRate();
}

void IPlugLedViewer::OnParamChange(int paramIdx)
{
	IMutexLock lock(this);

	switch (paramIdx)
	{
	case kNoOffNotes:
		//mSendOffNotes = GetParam(paramIdx)->Bool();
		break;
	default:
		break;
	}
}


void IPlugLedViewer::ProcessMidiMsg(IMidiMsg* pMsg)
{
	if (dbg_ctrl)
	{
		std::stringstream ss;
		ss << "Rcv Midi ch=0x" << std::hex << pMsg->Channel();
		int i = pMsg->Program();
		if (i >= 0) {
			ss << ", PC #" << i;
			ledMap.SetPC(i);
		}
		else
		{
			i = pMsg->NoteNumber();
			if (i >= 0) {
				ss << ", Note #" << i << "(Vel:" << pMsg->Velocity() << ")";
			}
			else
			{
				IMidiMsg::EControlChangeMsg cc = pMsg->ControlChangeIdx();
				if (cc >= 0) {
					const double ccV = pMsg->ControlChange(cc);
					ss << ", CC #" << i << "(Val:" << ccV << ")";
					ledMap.SetCC(cc, ccV);
				}
				else
				{
					ss << ", ???";
				}
			}
		}
		ss << std::endl;

		dbg_ctrl->text = ss.str();

	}
}

// Should return non-zero if one or more keys are playing.
int IPlugLedViewer::GetNumKeys()
{
	IMutexLock lock(this);
	return 0;
}

// Should return true if the specified key is playing.
bool IPlugLedViewer::GetKeyStatus(int key)
{
	IMutexLock lock(this);
	return false;
}

//Called by the standalone wrapper if someone clicks about
bool IPlugLedViewer::HostRequestingAboutBox()
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