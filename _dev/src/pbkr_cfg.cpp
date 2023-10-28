#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <fstream>

#include "pbkr_config.h"
#include "pbkr_cfg.h"

namespace
{
static const std::string dataPath(std::string (INTERNAL_MOUNT_POINT) + "/pbkr.config");

    static bool isMounted(const char* path)
    {
        std::string cmd ( std::string("grep -q ") + path + " /etc/mtab");
        const int res(system(cmd.c_str()));
        return res == 0;
    }
    static void doMount(const char* path)
    {
        std::string cmd ( std::string("mount ") + path);
        system(cmd.c_str());
    }
    bool checkDir(const std::string &path)
    {
      struct stat buffer;
      return (stat (path.c_str(), &buffer) == 0);
    }
    static inline const std::string toSavePath(const std::string& name)
    {
        static const std::string prefix (dataPath + "/");
        return prefix + name + ".sav";
    }
}

namespace PBKR
{
bool checkOrCreateDir(const string& path)
{
    // Check data path
    if (! checkDir (path))
    {
        // Creaate it
        mkdir(path.c_str(), 0777);
        printf ("Create path : %s\n", path.c_str());
        if (! checkDir (path))
        {
            printf ("Failed to create path : %s\n", path.c_str());
            return false;
        }
    }
    return true;
}

Config::Config(void):m_ready(false), m_error(false)
{
}
Config::~Config(void)
{
}

Config& Config::instance(void)
{
    static Config singleton;
    singleton.prepare();
    return singleton;
}

void Config::prepare (void)
{
    if (m_ready) return;
    m_ready = true;

    // check that DATA_PATH is mounted
    if (! ::isMounted(INTERNAL_MOUNT_POINT))
    {
        // partition not mounted. Try to do it
        ::doMount (INTERNAL_MOUNT_POINT);
        if (! ::isMounted(INTERNAL_MOUNT_POINT))
        {
            printf("Failed to mount SAVE path : " INTERNAL_MOUNT_POINT "\n");
            m_error = true;
            return;
        }
    }

    // Check data path
    if (! checkOrCreateDir(dataPath))
        m_error = true;
} // Config::prepare

void Config::saveInt(const std::string& name, const int value)
{
    if (m_error) return;
    std::ofstream myfile;
    myfile.open (toSavePath(name));
    myfile << value;
    myfile.close();
} // Config::saveInt

int Config::loadInt(const std::string& name, const int defaultValue)
{
    if (m_error) return defaultValue;
    int value;
    std::ifstream myfile;
    myfile.open (toSavePath(name));
    try
    {
        myfile >> value;
    }
    catch(...)
    {
        value = defaultValue;
    }
    myfile.close();

    return value;
} // Config::loadInt

StringVect Config::listDirs(const string& path)
{
    StringVect res;
    DIR *dir = opendir(path.c_str());
    struct dirent *entry = readdir(dir);
    while (entry != NULL)
    {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.')
        {
            res.push_back(entry->d_name);
        }
        entry = readdir(dir);
    }
    closedir(dir);
    return res;
}

StringVect Config::listFiles(const string& path)
{
    StringVect res;
    DIR *dir = opendir(path.c_str());
    struct dirent *entry = readdir(dir);
    while (entry != NULL)
    {
        if (entry->d_type == DT_REG && entry->d_name[0] != '.')
        {
            res.push_back(entry->d_name);
        }
        entry = readdir(dir);
    }
    closedir(dir);
    return res;
}

string Config::loadStr(const string& name, const string& defaultValue, bool absPath)
{
    if (m_error) return defaultValue;
    string value(defaultValue);
    std::ifstream myfile (absPath ? name : toSavePath(name));
    try
    {
        getline( myfile, value );
    }
    catch(...){}
    myfile.close();
    return value;
} // Config::loadStr

void Config::saveStr(const std::string& name, const string& value)
{
    if (m_error) return;
    std::ofstream myfile;
    myfile.open (toSavePath(name));
    myfile << value;
    myfile.close();
} // Config::saveStr

} // namespace PBKR
