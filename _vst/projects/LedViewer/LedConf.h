#pragma once

#include <string>
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
	void fillPoly(point4array& arr)const;
	IRECT rect(void)const;
};
using LedVect_t = std::vector<CLedConf>;


// Draws a Led depending on the current value.
class ILedViewControl : public IControl
{
public:
	ILedViewControl(IPlugBase* pPlug, const CLedConf& ledCfg, int paramIdx);

	ILedViewControl(IPlugBase* pPlug, const CLedConf& ledCfg);

	virtual ~ILedViewControl() {}

	virtual bool Draw(IGraphics* pGraphics);

protected:
	const CLedConf mLedCfg;
};


const LedVect_t& getConf();

