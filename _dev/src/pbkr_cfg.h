#ifndef _pbkr_cfg_h_
#define _pbkr_cfg_h_

#include "pbkr_config.h"

#include <string>

namespace PBKR
{
using namespace std;
/*******************************************************************************
 * GLOBAL CONSTANTS
 *******************************************************************************/
extern const char* config_filename;

/*******************************************************************************
 * CONFIG
 *******************************************************************************/

class Config
{
public:
    static Config& instance();
    void saveInt(const string& name, const int value);
    int loadInt(const string& name, const int defaultValue);
    void saveStr(const string& name, const string& value);
    string loadStr(const string& name, const string& defaultValue = "", bool absPath=false);
protected:
    Config(void);
	virtual ~Config(void);
private:
    bool m_ready;
    bool m_error;
    void prepare (void);

};

}
#endif // _pbkr_utils_h_
