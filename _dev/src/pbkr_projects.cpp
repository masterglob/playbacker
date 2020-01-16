#include "pbkr_projects.h"

#include <dirent.h>
#include <stdio.h>
#include <bits/stdc++.h>
#include <algorithm>
#include <string>

/*******************************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************************/
extern "C"
{
extern int toupper(int);
}

namespace
{
using namespace PBKR;

/*******************************************************************************/
int readIntInFile(const string& name, int defVal)
{
    int value(defVal);
    std::ifstream myfile (name);
    try
    {
        myfile >> value;
    }
    catch(...){}
    myfile.close();

    return value;
} // readIntInFile

/*******************************************************************************/
string readStrInFile(const string& name, const string& defVal)
{
    string value(defVal);
    std::ifstream myfile (name);
    try
    {
        getline( myfile, value );
    }
    catch(...){}
    myfile.close();
    return value;
} // readStrInFile

/*******************************************************************************/
StringVect
listFilesOfType (const string & path, const int fileType)
{
    StringVect result;
    DIR *dir = opendir(path.c_str());
    if (dir)
    {
        struct dirent *entry = readdir(dir);
        while (entry != NULL)
        {
            if (entry->d_type == fileType)
            {
                result.push_back(entry->d_name);
            }
            entry = readdir(dir);
        }
        closedir(dir);
    }
    return result;
}

/*******************************************************************************/
string stringToUpper(string oString){
   for(size_t i = 0; i < oString.length(); i++){
       oString[i]  = (char):: toupper((int)oString[i]);
    }
    return oString;
} // stringToUpper

/*******************************************************************************/
StringVect
listFilesWithExtension(const string & path, const string & ext)
{
    static const size_t extLen(ext.length());
    const StringVect fileList (::listFilesOfType(path, DT_REG));

    StringVect result;
    for (auto it(fileList.begin());it != fileList.end(); it++)
    {
        // Find WAV files
        const string& name(*it);
        const size_t len (name.length());
        const string uppername(stringToUpper(name));
        if (len > extLen)
        {
            const string upperExt(uppername.substr(len - extLen,extLen));
            if (upperExt == ext)
            {
                result.push_back(name);
            }
        }
    }
    return result;
} // listFilesWithExtension

} // namespace


/*******************************************************************************
 * EXTERNAL FUNCTIONS
 *******************************************************************************/
namespace PBKR
{
using namespace std;
const ProjectSource projectSourceUSB("USB", USB_MOUNT_POINT);
const ProjectSource projectSourceInternal("Internal", INTERNAL_MOUNT_POINT "/pbkr.projects");
const ProjectSourceVect projectSources= {projectSourceUSB, projectSourceInternal};


/*******************************************************************************/
Project* findProjectByTitle(const ProjectVect& pVect, const string& title)
{
    for (auto it(pVect.begin()); it != pVect.end(); it++)
    {
        Project* pProj(*it);
        if (pProj && title == pProj->m_title) return pProj;
    }
    return NULL;
} // findProjectByTitle

/*******************************************************************************/
StringVect
ProjectSource::findProjects(void)const
{
    const StringVect projectsList (::listFilesOfType(pPath,DT_DIR));
    StringVect result;
    for (auto it(projectsList.begin()); it != projectsList.end(); it++)
    {
        const string& name (*it);
        const string fullPath (pPath + "/" + name);
        if (name == "." or name == "..") continue;
        const StringVect wavs (::listFilesWithExtension(fullPath, ".WAV"));
        if (wavs.size() == 0) continue;
        result.push_back(name);
    }
    return result;
}

/** Active list of projects */
static ProjectVect currentProjects;

/*******************************************************************************/
ProjectVect getAllProjects(void)
{
    ProjectVect newProjects;

    for (auto s(projectSources.begin()); s != projectSources.end(); s++)
    {
        // TODO REMOVE!
        const ProjectSource& ps(*s);
        // printf("Looking into %s\n",ps.pName.c_str());
        const StringVect v (ps.findProjects());
        for (auto i(v.begin()) ;i != v.end();i++)
        {
            const string& s (*i);
            // printf("Found project %s (%s)\n", s.c_str(), ps.pName.c_str());
            try
            {
                Project* proj = new Project(s, ps);
                // proj->debug();
                newProjects.push_back(proj);
            }
            catch (Project::BadProject& e)
            {
                printf("Invalid project:%s\n", s.c_str());
            }
        }
    }

    // Add new items  to currentProjects
    for (auto it(newProjects.begin()) ;it != newProjects.end();it++)
    {
        Project* proj (*it);
        if (! findProjectByTitle (currentProjects,  proj->m_title))
        {
            printf("New project: %s\n", proj->m_title.c_str());
            currentProjects.push_back(proj);
        }
        else
        {
            proj->setToClose();
        }
    }

    // Delete old items from currentProjects
    bool deleted;
    do
    {
        deleted = false;
        for (auto it(currentProjects.begin()) ;it != currentProjects.end();it++)
        {
            Project* proj (*it);
            if (! findProjectByTitle (newProjects,  proj->m_title))
            {
                if (!proj->inUse())
                {
                    printf("Project deleted: %s\n", proj->m_title.c_str());
                    currentProjects.erase(it);
                    delete proj;
                    deleted = true;
                    break; // force re-loop
                }
                else if (! proj->toClose())
                {
                    printf("Project should be deleted but in use: %s\n", proj->m_title.c_str());
                    proj->setToClose();
                }
            }
        }
    } while (deleted);

    for (auto it(newProjects.begin()) ;it != newProjects.end();it++)
    {
        Project* proj (*it);
        if (proj->toClose())
            delete proj;
    }
    return currentProjects;
}

/*******************************************************************************
########  ########   #######        ## ########  ######  ########
##     ## ##     ## ##     ##       ## ##       ##    ##    ##
##     ## ##     ## ##     ##       ## ##       ##          ##
########  ########  ##     ##       ## ######   ##          ##
##        ##   ##   ##     ## ##    ## ##       ##          ##
##        ##    ##  ##     ## ##    ## ##       ##    ##    ##
##        ##     ##  #######   ######  ########  ######     ##
 *******************************************************************************/
Project::Project(const std::string& name, const ProjectSource source):
        m_title(name),
        m_source(source),
        m_inUse(false),
        m_toClose(false)
{
    static const string wavExt(".WAV");
    const string fullPath(source.pPath + "/" + name);
    const StringVect wavs(::listFilesWithExtension(fullPath, wavExt));

    for (auto it(wavs.begin());it != wavs.end(); it++)
    {
        // Find WAV files
        int nextIndex = getNewTrackIndex (1);
        const string& filename(*it);
        const string fullFilename(fullPath + "/" + filename);
        //printf("fullFilename = %s,",fullFilename.c_str());
        const int index(readIntInFile(fullFilename + ".track", nextIndex));
        //printf("index = %d,",index);
        const string trackname(readStrInFile(fullFilename + ".title",filename));
        //printf("trackname = %s\n",trackname.c_str());
        m_tracks.push_back(Track(trackname,index,filename));
        //debug();
    }
    sort(m_tracks.begin(), m_tracks.end(), [ ]( const auto& lhs, const auto& rhs )
            {
               return lhs.m_index < rhs.m_index;
            });
} // Project::Project

/*******************************************************************************/
Project::~Project(void)
{
} // Project::~Project

/*******************************************************************************/
void
Project::debug(void)const
{
    printf("Project <%s> (%s)\n", m_title.c_str(), m_source.pName.c_str());
    for (auto it(m_tracks.begin()); it != m_tracks.end(); it++)
    {
        const Track& track(*it);
        printf(" - Track #%02d : %s (%s)\n",
                track.m_index,
                track.m_title.c_str(),
                track.m_filename.c_str());
    }
} // Project::debug

/*******************************************************************************/
Track
Project::getByTrackId(int id)
{
    static const Track noTrack ("",-1,"");
    // Remind tracks are sorted!
    for (auto it(m_tracks.begin()); it != m_tracks.end(); it++)
    {
        const Track& track(*it);
        if (track.m_index == id) return track;
        else if (track.m_index > id) break;
    }
    return noTrack;
}

/*******************************************************************************/
int
Project::getNbTracks(void)const
{
    if (m_tracks.size() == 0) return 0;
    const Track& lastTrack(*(m_tracks.end()-1));
    return lastTrack.m_index ;
}

/*******************************************************************************/
int
Project::getFirstTrackIndex(void)const
{
    if (m_tracks.size() == 0) return -1;
    const Track& track(*(m_tracks.begin()));
    return track.m_index;
}

/*******************************************************************************/
int
Project::getNewTrackIndex(int fromId)const
{
    while (true)
    {
        bool alreadyExists(false);
        for (auto it(m_tracks.begin()); it != m_tracks.end(); it++)
        {
            const Track& track(*it);
            if (fromId == track.m_index)
            {
                alreadyExists = true;
                break;
            }
        }
        if (not alreadyExists) return fromId;
        fromId++;
    }
}

} // namespace PBKR
