#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IMidiQueue.h"
#include "LedConf.h"


class IPlugLedViewer : public IPlug
{
public:
	IPlugLedViewer(IPlugInstanceInfo instanceInfo);
  ~IPlugLedViewer();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  bool HostRequestingAboutBox();

  int GetNumKeys();
  bool GetKeyStatus(int key);
  void ProcessMidiMsg(IMidiMsg* pMsg);

private:
	IBitmapOverlayControl* mAboutBox;

	double mSampleRate;

	ITextControlRefr* dbg_ctrl;

	CLedMap ledMap;

	ITimeInfo mTimeInfo;
};

enum ELayout
{
	kWidth = GUI_WIDTH,  // width of plugin window
	kHeight = GUI_HEIGHT, // height of plugin window

	kKeybX = 1,
	kKeybY = 233,

	kGainX = 100,
	kGainY = 100,
	kKnobFrames = 60,

	kIRadioButtonsControl_N = 2,
	kIRBC_W = 24,  // width of bitmap
	kIRBC_H = 24,  // height of one of the bitmap images
	kIRBC_VN = 2,  // number of vertical buttons

	kISwitchControl_OffNotes_X = 100,
	kISwitchControl_OffNotes_Y = 110,
};