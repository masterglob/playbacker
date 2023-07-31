#pragma once

#include <string>
#include <map>
#include <set>
#include <mutex>
#include <vector>
#include "IControl.h"

class CLedWiring {
public:
	CLedWiring(int rOffset, int gOffset, int bOffset, int wOffset=-1);
	const int iR, iG, iB, iW;
};

struct point4array {
	int x[4];
	int y[4];
};

class CLedConf {
public:
	CLedConf(const std::string& _name, int _x1, int _y1, int _x2, int _y2, const int _width, const CLedWiring& _wiring, const int _line);
	const std::string name;
	const double x1, x2, y1, y2;
	const int width,line;
	const CLedWiring wiring;
	void fillPoly(point4array& arr, double k=1.0)const;
	IRECT rect(void)const;

	int getRLine(void)const { return wiring.iR >= 0 ? wiring.iR + line : -1; }
	int getGLine(void)const { return wiring.iG >= 0 ? wiring.iG + line : -1; }
	int getBLine(void)const { return wiring.iB >= 0 ? wiring.iB + line : -1; }
	int getWLine(void)const { return wiring.iW >= 0 ? wiring.iW + line : -1; }
};
using LedVect_t = std::vector<CLedConf>;


class ITextControlRefr : public ITextControl
{
public:
	ITextControlRefr(IPlugBase* pPlug, IRECT pR, IText* pText, const char* str = "")
		: ITextControl(pPlug, pR, pText, str)
	{
	}
	virtual ~ITextControlRefr() {}
	virtual void OnGUIIdle();
	void CheckDirty();
	virtual bool Draw(IGraphics* pGraphics);
	std::string text;
private:
	std::string mLast;
};

class IPlugLedViewer;

// Draws a Led depending on the current value.
class ILedViewControl : public IControl
{
public:
	ILedViewControl(IPlugBase* pPlug, IPlugLedViewer* pPlugView, const CLedConf& ledCfg, int paramIdx);

	ILedViewControl(IPlugBase* pPlug, IPlugLedViewer* pPlugView, const CLedConf& ledCfg);

	virtual ~ILedViewControl() {}

	virtual bool Draw(IGraphics* pGraphics);

	void setLineColor(uint8_t line, uint8_t value);
private:
	uint8_t To_LED_Level(uint32_t l, float level = 1.0);
protected:
	IPlugLedViewer* mViewer;
	const CLedConf mLedCfg;
	union {
		uint32_t u32;
		uint8_t u8[4]; // order:  R, G ,B, W
	} mArgb;
	IChannelBlend mWBlend;
};

class CLedMap {
public:
	CLedMap(void);
	void insert(const CLedConf& cfg, ILedViewControl* viewCtrl);
	void SetCC(uint8_t cc, double val);
	void SetPC(uint8_t pc);
	bool CheckDirty(void);
	void DirtyAll(void);
private:
	std::mutex mMutex;
	std::set<int> mDirty;

	int ccVal[0x80];
	struct Elt_Data_t {
		ILedViewControl* ctrl;
		uint8_t line;
	};
	Elt_Data_t mCtrl[0x80];
};

const LedVect_t& getConf();

