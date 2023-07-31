#include "LedConf.h"
#include "resource.h"
#include "IControl.h"
#include "LedViewer.h"

#include <math.h>

#define DRAW_FRAMES 1
#define DRAW_METHOD 3
namespace
{
	inline uint8_t To_MIDI_Level(double d) {
		d *= 0x80;
		int result = static_cast<int>(d);
		if (result < 0) return 0;
		if (result > 127) return 0x7F;
		return result;
	}


}

CLedWiring::
CLedWiring(int rOffset, int gOffset, int bOffset, int wOffset):
	iR(rOffset),
	iG(gOffset),
	iB(bOffset),
	iW(wOffset)
{}


CLedConf::CLedConf(const std::string& _name, int _x1, int _y1, int _x2, int _y2, const int _width, const CLedWiring& _wiring, const int _line):
	name(_name),
	x1(_x1),
	x2(_x2),
	y1(_y1),
	y2(_y2),
	width(_width),
	line(_line),
	wiring(_wiring)
{
}

void
CLedConf::
fillPoly(point4array& arr, double k)const
{
	const double t = atan2(y2 - y1, x2 - x1);
	const double kd = k * width;
	const double c = kd * cos(t);
	const double s = kd * sin(-t);
	arr.x[0] = static_cast<int>(x1 + s);
	arr.x[1] = static_cast<int>(x1 - s);
	arr.x[2] = static_cast<int>(x2 - s);
	arr.x[3] = static_cast<int>(x2 + s);
	arr.y[0] = static_cast<int>(y1 + c);
	arr.y[1] = static_cast<int>(y1 - c);
	arr.y[2] = static_cast<int>(y2 - c);
	arr.y[3] = static_cast<int>(y2 + c);
}

IRECT
CLedConf::
rect(void)const {
	return IRECT{std::min((int)x1,(int)x2) - width ,std::min((int)y1,(int)y2) - width, std::max((int)x1,(int)x2) + width,  std::max((int)y1,(int)y2) + width};
}

#define Y_PERCENT(y) ((GUI_HEIGHT * (100 - (y))) / 100)
#define X_PERCENT(x) ((GUI_WIDTH * (x)) / 100)

// GUI_WIDTH GUI_HEIGHT
const LedVect_t& getConf(){
	static LedVect_t result;
	if (result.empty()){
		static const int LINE_M(9);
		static const int LINE_Ms(113);
		static const int LINE_G(17);
		static const int LINE_Gs(105);
		static const int LINE_SH1(25);
		static const int LINE_SH1s(97);
		static const int LINE_SH2(33);
		static const int LINE_SH2s(89);
		static const int LINE_SQ(41);
		static const int LINE_SQs(81);

		static const int wLarge(10);
		static const int wSmall(8);
		static const int pixSpace(2);
		const CLedWiring wiring(0, 2, 4, 6);

		int x1, x2, y1, y2;

		// SQ
		x1 = X_PERCENT(49);
		x2 = X_PERCENT(51);
		y1 = Y_PERCENT(45);
		y2 = Y_PERCENT(95);
		result.emplace_back("SQ", x1, y1, x1, y2, wLarge, wiring, LINE_SQ);
		result.emplace_back("SQs", x2, y1, x2, y2, wLarge, wiring, LINE_SQs);

		// M / Ms
		x1 = X_PERCENT(10);
		x2 = X_PERCENT(90);
		y1 = Y_PERCENT(25);
		y2 = Y_PERCENT(65);
		result.emplace_back("M", x1, y1, x1, y2, wSmall, wiring, LINE_M);
		result.emplace_back("Ms", x2, y1, x2, y2, wSmall, wiring, LINE_Ms);

		// G / Gs
		x1 = X_PERCENT(10);
		x2 = X_PERCENT(40);
		y1 = Y_PERCENT(23);
		y2 = Y_PERCENT(7);
		result.emplace_back("G", x1, y1, x2, y2, wSmall, wiring, LINE_G);
		x1 = X_PERCENT(90);
		x2 = X_PERCENT(60);
		result.emplace_back("Gs", x1, y1, x2, y2, wSmall, wiring, LINE_Gs);

		// SH1
		x1 = X_PERCENT(25);
		x2 = X_PERCENT(75);
		y1 = Y_PERCENT(50);
		y2 = Y_PERCENT(85);
		result.emplace_back("SH1", x1, y1, x1, y2, wSmall, wiring, LINE_SH1);
		result.emplace_back("SH1s", x2, y1, x2, y2, wSmall, wiring, LINE_SH1s);
		// SH2
		x1 = X_PERCENT(27);
		x2 = X_PERCENT(73);
		y1 = Y_PERCENT(60);
		y2 = Y_PERCENT(95);
		result.emplace_back("SH2", x1, y1, x1, y2, wSmall, wiring, LINE_SH2);
		result.emplace_back("SH2s", x2, y1, x2, y2, wSmall, wiring, LINE_SH2s);

	}
	return result;
}

ILedViewControl::
ILedViewControl(IPlugBase* pPlug, IPlugLedViewer* pPlugView, const CLedConf& ledCfg, int paramIdx)
	: IControl(pPlug, ledCfg.rect(), paramIdx, IChannelBlend::kBlendNone),
	mViewer(pPlugView),
	mLedCfg(ledCfg),
	mWBlend(IChannelBlend::kBlendAdd){}

ILedViewControl::
ILedViewControl(IPlugBase* pPlug, IPlugLedViewer* pPlugView, const CLedConf& ledCfg)
	: IControl(pPlug, ledCfg.rect(), -1, IChannelBlend::kBlendNone),
	mViewer(pPlugView),
	mLedCfg(ledCfg),
	mWBlend(IChannelBlend::kBlendAdd) {}

inline uint8_t 
ILedViewControl::
To_LED_Level(uint32_t l, float level) {
	uint32_t result = l;
 	if (mViewer->getColModel())
	{
		static const uint32_t blackThr = 30;
		result *= (256 - blackThr);
		result /= (128);
		result += blackThr;
	}
	else
	{
		result <<= 1;
	}
	static_cast<uint32_t>(result * level);
	if (result > 0xFF) return 0xFF;
	return result;
}
bool ILedViewControl::Draw(IGraphics* pGraphics)
{
	static uint32_t v=0;
	point4array points;
	mLedCfg.fillPoly(points, 1);
#if DRAW_METHOD == 1
 /* R,G,b and W are in [0..127]
 * We want to have at least some "gray" when completely off
 * => Add 1/4 of gray anyway
 * => *4/5 result to re-range value
 */
	static const int gray0(0x20); // 0x20 = 0x80 / 4)

	const int w = mArgb.u8[3] + gray0;
	uint8_t r = ((mArgb.u8[0] + w) * 5) / 4;
	uint8_t g = ((mArgb.u8[1] + w) * 5) / 4;
	uint8_t b = ((mArgb.u8[2] + w) * 5) / 4;
	const IColor color(255, r, g, b);
	pGraphics->FillIConvexPolygon(&color, points.x, points.y, 4);
#elif DRAW_METHOD == 2
	/*
	* Draw colors on the edge and white in the middle
	*/
	point4array pointsX;
	mLedCfg.fillPoly(points, 0.4);
	mLedCfg.fillPoly(pointsX, 1.2);
	static const float level = 1;
	uint8_t w = To_LED_Level(mArgb.u8[3], level);
	uint8_t r = To_LED_Level(mArgb.u8[0], level);
	uint8_t g = To_LED_Level(mArgb.u8[1], level);
	uint8_t b = To_LED_Level(mArgb.u8[2], level);
	const IColor colorRGB(40, r, g, b);
	const IColor colorW(255, w, w, w);

	pGraphics->FillIConvexPolygon(&colorW, pointsX.x, pointsX.y, 4);

	pGraphics->FillIConvexPolygon(&colorRGB, points.x, points.y, 4, &mWBlend);
#elif DRAW_METHOD == 3
	static const float level = 1;
	uint8_t bg = To_LED_Level(0);
  	uint8_t w = To_LED_Level(mArgb.u8[3], level);
	uint8_t r = To_LED_Level(mArgb.u8[0], level);
	uint8_t g = To_LED_Level(mArgb.u8[1], level);
	uint8_t b = To_LED_Level(mArgb.u8[2], level);
	const IColor colorBg(255, bg, bg, bg);
	const IColor colorW(255, w, w, w);
	const IColor colorRGB(255, r, g, b);
	pGraphics->FillIConvexPolygon(&colorBg, points.x, points.y, 4);
	
	// const double dXY = sqrt(dx * dx + dy * dy);
	int nIter;
	// points 0 and 1  are extrapolation of the same point - so they are close to each other
	// points 2 and 3 are extrapolation of the other point
	double x0 = points.x[0];
	double y0 = points.y[0];
	double x1 = points.x[1];
	double y1 = points.y[1];
	double x3 = points.x[3];
	double y3 = points.y[3];
	double dx = x1 - x0;
	double dy = y1 - y0;
	if (abs(dx) > abs(dy))
	{
		nIter= abs(dx);
		dx = dx>0 ? 1 : -1;
		for (int i = 0; i < mLedCfg.width; i++) {
			pGraphics->DrawLine(&colorW, x0, y0, x3, y3, &mWBlend, true);
			x0 += dx , x3 += dx;
			pGraphics->DrawLine(&colorRGB, x0, y0, x3, y3, &mWBlend, true);
			x0 += dx, x3 += dx;
		}
	}
	else
	{
		nIter = abs(dy);
		dy = dy > 0 ? 1 : -1;
		for (int i = 0; i < mLedCfg.width; i++) {
			pGraphics->DrawLine(&colorW, x0, y0, x3, y3, &mWBlend, true);
			y0 += dy, y3 += dy;
			pGraphics->DrawLine(&colorRGB, x0, y0, x3, y3, &mWBlend, true);
			y0 += dy, y3 += dy;
		}
	}
#define DRAW_FRAMES 0
#endif

#if DRAW_FRAMES
	// draw box frames
	mLedCfg.fillPoly(points, 1.2);
	static const IColor borderColor(10,0x50, 0x50, 0x50);
	for (int i =0 ; i < 4 ; i++)
	{
		pGraphics->DrawLine(&borderColor, points.x[i], points.y[i], points.x[(i + 1) % 4], points.y[(i + 1) % 4]);
	}
#endif
	 return true;
}

void ILedViewControl::setLineColor(uint8_t line, uint8_t value)
{
	if (line < 4) {
		mArgb.u8[line] = value;
	}
}

CLedMap::
CLedMap(void) {
	memset(ccVal, 0, sizeof(ccVal));
	memset(mCtrl, 0, sizeof(mCtrl));
}

void
CLedMap::
insert(const CLedConf& cfg, ILedViewControl* viewCtrl) {
	Elt_Data_t data;
	data.ctrl = viewCtrl;
	data.line = 0;
	mCtrl[cfg.getRLine()] = data;
	data.line++;
	mCtrl[cfg.getGLine()] = data;
	data.line++;
	mCtrl[cfg.getBLine()] = data;
	data.line++;
	mCtrl[cfg.getWLine()] = data;
}

void
CLedMap::
SetPC(uint8_t pc)
{
	for (int cc = 0; cc < 127; cc++)
	{
		SetCC(cc, pc *(1.0/0x80));
	}
}

void
CLedMap::
SetCC(uint8_t cc, double val) {
	int iVal = ::To_MIDI_Level(val);
	if (cc < 0x80 && ccVal[cc] != iVal)
	{ 
		ccVal[cc] = iVal;
		const Elt_Data_t& data = mCtrl[cc];
		if (data.ctrl) {
			data.ctrl->setLineColor(data.line, iVal);
		}
		mMutex.lock();
 		mDirty.insert(cc);
		mMutex.unlock();
	}
}

void
CLedMap::
DirtyAll(void)
{
	for (uint32_t i = 0; i < 0x80; i++)
	{
		ILedViewControl* ctrl = mCtrl[i].ctrl;
		mDirty.insert(i);
	}
}

bool
CLedMap::
CheckDirty(void)
{
	bool result = false;
	mMutex.lock();
	std::set<int> copy(mDirty);
	mDirty.clear();
	mMutex.unlock();

	for (int cc : copy)
	{
		if (cc < 0 || cc > 0x80) continue; // argl?
		const Elt_Data_t& data = mCtrl[cc];
		if (data.ctrl) {
			result = true;
			data.ctrl->SetDirty();
		}
	}
	return result;
}
