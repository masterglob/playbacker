#include "LedConf.h"
#include "resource.h"
#include "IControl.h"

#include <math.h>


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
fillPoly(point4array& arr)const
{
	const double t = atan2(y2 - y1, x2 - x1);
	const double c = width * cos(t);
	const double s = width * sin(t);
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

		static const int wLarge(5);
		static const int wSmall(3);
		static const int pixSpace(2);
		const CLedWiring wiring(0, 2, 4, 6);

		int x1, x2, y1, y2;

		// SQ
		x1 = X_PERCENT(49);
		x2 = X_PERCENT(51);
		y1 = Y_PERCENT(55);
		y2 = Y_PERCENT(95);
		result.emplace_back("SQ", x1, y1, x1, y2, wLarge, wiring, LINE_SQ);
		result.emplace_back("SQs", x2, y1, x2, y2, wLarge, wiring, LINE_SQs);

		// M / Ms
		x1 = X_PERCENT(10);
		x2 = X_PERCENT(90);
		y1 = Y_PERCENT(30);
		y2 = Y_PERCENT(60);
		result.emplace_back("M", x1, y1, x1, y2, wSmall, wiring, LINE_M);
		result.emplace_back("Ms", x2, y1, x2, y2, wSmall, wiring, LINE_Ms);

		// G / Gs
		x1 = X_PERCENT(10);
		x2 = X_PERCENT(40);
		y1 = Y_PERCENT(28);
		y2 = Y_PERCENT(10);
		result.emplace_back("G", x1, y1, x2, y2, wSmall, wiring, LINE_G);
		x1 = X_PERCENT(90);
		x2 = X_PERCENT(60);
		result.emplace_back("Gs", x1, y1, x2, y2, wSmall, wiring, LINE_Gs);

		// SH1
		x1 = X_PERCENT(25);
		x2 = X_PERCENT(75);
		y1 = Y_PERCENT(55);
		y2 = Y_PERCENT(80);
		result.emplace_back("SH1", x1, y1, x1, y2, wSmall, wiring, LINE_SH1);
		result.emplace_back("SH1s", x2, y1, x2, y2, wSmall, wiring, LINE_SH1s);
		// SH2
		x1 = X_PERCENT(26);
		x2 = X_PERCENT(74);
		y1 = Y_PERCENT(65);
		y2 = Y_PERCENT(90);
		result.emplace_back("SH2", x1, y1, x1, y2, wSmall, wiring, LINE_SH2);
		result.emplace_back("SH2s", x2, y1, x2, y2, wSmall, wiring, LINE_SH2s);

	}
	return result;
}

ILedViewControl::
ILedViewControl(IPlugBase* pPlug, const CLedConf& ledCfg, int paramIdx)
	: IControl(pPlug, ledCfg.rect(), paramIdx, IChannelBlend::kBlendNone),
	mLedCfg(ledCfg){}

ILedViewControl::
ILedViewControl(IPlugBase* pPlug, const CLedConf& ledCfg)
	: IControl(pPlug, ledCfg.rect(), -1, IChannelBlend::kBlendNone),
	mLedCfg(ledCfg) {}

bool ILedViewControl::Draw(IGraphics* pGraphics)
{
	point4array points;
	mLedCfg.fillPoly(points);
	const IColor color(10, 250, 130, 250);
	return pGraphics->FillIConvexPolygon(&color, points.x, points.y, 4);
}