#ifndef _pbkr_cfg_h_
#define _pbkr_cfg_h_

#include "pbkr_config.h"

#include <string>
#include <vector>

namespace PBKR
{
using namespace std;
using StringVect = vector<string>;

/*******************************************************************************
 * GLOBAL CONSTANTS
 *******************************************************************************/
extern const char* config_filename;

/*******************************************************************************
 * CONFIG
 *******************************************************************************/
bool checkOrCreateDir(const string& path);

class Config
{
public:
    static Config& instance();
    void saveInt(const string& name, const int value);
    int loadInt(const string& name, const int defaultValue);
    void saveStr(const string& name, const string& value);
    string loadStr(const string& name, const string& defaultValue = "", bool absPath=false);
    static StringVect listDirs(const string& path);
    static StringVect listFiles(const string& path);
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
