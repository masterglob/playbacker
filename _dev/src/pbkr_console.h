#pragma once

#include "pbkr_config.h"
#include "pbkr_utils.h"

using namespace std;

namespace PBKR
{

/*******************************************************************************
 *  CONSOLE
 *******************************************************************************/

class Console:public Thread
{
public:
    Console(void);
    virtual ~Console(void);
    static Console* instance(){return m_instance;}
    virtual void body(void);
    bool doSine;
    float volume(void);
    void changeVolume (float v, const float duration = volumeFaderDurationS);
private:
    static const float volumeFaderDurationS;
    static Console* m_instance;
    float _volume;
};

}
