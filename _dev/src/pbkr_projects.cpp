
#include <dirent.h>
#include <stdio.h>
#include <bits/stdc++.h>
#include <algorithm>
#include <unistd.h>
#include <string>
#include <sys/wait.h>

#include "pbkr_config.h"
#include "pbkr_types.h"
#include "pbkr_projects.h"

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
bool moveToStartOfLine(std::ifstream& fs)
{
    fs.seekg(-1, std::ios_base::cur);
    for(long i = fs.tellg(); i > 0; i--)
    {
        if(fs.peek() == '\n')
        {
            fs.get();
            return true;
        }
        fs.seekg(i, std::ios_base::beg);
    }
    return false;
}

/*******************************************************************************/
std::string getLastLineInFile(const string filename)
{
    std::ifstream fs(filename);
    if(!fs.is_open()) return "";

    // Go to the last character before EOF
    fs.seekg(-1, std::ios_base::end);
    if (!moveToStartOfLine(fs))
        fs.seekg(0, ios_base::beg);

    std::string lastline = "";
    getline(fs, lastline);
    fs.close();
    return lastline;
}

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
            const string upperExt(PBKR::substring (uppername, len - extLen,extLen));
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
void getProjects(ProjectVect& vect, const ProjectSource& from)
{
    const StringVect v (from.findProjects());
    FOR (i, v)
    {
        const string& s (*i);
        try
        {
            Project* proj = new Project(s, from);
            // proj->debug();
            vect.push_back(proj);
        }
        catch (Project::BadProject& e)
        {
            printf("Invalid project:%s\n", s.c_str());
        }
    }
}

/*******************************************************************************/
ProjectVect getAllProjects(void)
{
    ProjectVect newProjects;

    for (auto s(projectSources.begin()); s != projectSources.end(); s++)
    {
        // TODO REMOVE!
        const ProjectSource& ps(*s);
        getProjects (newProjects, ps);
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

/*******************************************************************************
########  ######## ##       ######## ######## ######## ########
##     ## ##       ##       ##          ##    ##       ##     ##
##     ## ##       ##       ##          ##    ##       ##     ##
##     ## ######   ##       ######      ##    ######   ########
##     ## ##       ##       ##          ##    ##       ##   ##
##     ## ##       ##       ##          ##    ##       ##    ##
########  ######## ######## ########    ##    ######## ##     ##
 *******************************************************************************/
ProjectDeleter::ProjectDeleter(const Project* source)
:
        m_source(source->m_source),
        m_name(source->m_title),
        m_failed (false),
        m_done(false)
{

}


/*******************************************************************************/
ProjectDeleter::~ProjectDeleter(void)
{
}

/*******************************************************************************/
void
ProjectDeleter::begin(void)
{
    const string rm_cmd ("\\rm -rf \'");
    const string cmd (rm_cmd + m_source.pPath + "/" + m_name + "\'");

    // SECURITY!!!

    static const string shouldStartWith ("\\rm -rf \'" INTERNAL_MOUNT_POINT "/pbkr.projects/");
    const string startWith (substring(cmd, 0, (size_t) shouldStartWith.length()));

    cout << "Delete command:" << cmd << endl;
    if (startWith == shouldStartWith)
    {

        const int res (system(cmd.c_str()));
        if (res != 0)
        {
            m_failed = true;
        }
    }
    else
    {
        cout << "Unexpected command: <" << shouldStartWith << "> was expected..." << endl;
        m_failed = true;
    }
    m_done = true;
}

/*******************************************************************************
 ######   #######  ########  #### ######## ########
##    ## ##     ## ##     ##  ##  ##       ##     ##
##       ##     ## ##     ##  ##  ##       ##     ##
##       ##     ## ########   ##  ######   ########
##       ##     ## ##         ##  ##       ##   ##
##    ## ##     ## ##         ##  ##       ##    ##
 ######   #######  ##        #### ######## ##     ##

 *******************************************************************************/
ProjectCopier::ProjectCopier (const Project* source, const ProjectSource& dest)
:
        Thread ("ProjectCopier"),
        m_source(source->m_source),
        m_name(source->m_title),
        m_dest(dest),
        m_doBackup(true),
        m_failed (false),
        m_done(false),
        m_mode(CM_CANCEL)
{
}


/*******************************************************************************/
ProjectCopier::~ProjectCopier(void)
{
}

/*******************************************************************************/
bool
ProjectCopier::willOverwrite(void)const
{
    ProjectVect vect;
    getProjects(vect, m_dest);
    return findProjectByTitle(vect,m_name) != NULL;
}

/*******************************************************************************/
void
ProjectCopier::begin(CopyMode mode)
{
    m_mode = mode;
    if (m_mode == CM_CANCEL) m_failed = true;
    else start();
}

/*******************************************************************************/
void
ProjectCopier::body(void)
{
    // Manage "BACKUP" case
    if  (m_mode == CM_BACKUP)
    {
        // TODO backup project
        const string newName (substring(m_name,0,12) + "-bak");
        const string oldPath (m_dest.pPath + "/" + m_name);
        const string newPath (m_dest.pPath + "/" + newName);
        const int res (rename (oldPath.c_str(), newName.c_str()));
        if (res != 0)
        {
            cout << "Failed to rename" << oldPath << " to " << newPath << endl;
            m_failed = true;
            return;
        }
    }

    // Create and clear file
    static const string filename = "/tmp/progress";
    ofstream of(filename);
    of << "  0%" << endl;

    const int cpid  = fork();
    int  status(-1);
    if (cpid  < 0) throw EXCEPTION("Fork failed");
    if (cpid  == 0)
    {
        const char *mode = (m_mode == CM_COMPLETE ? "-c" : "-r");
        // forked exec
        char* argv[] ={
                strdup("/root/pbkr/pbkr_cpy.sh"),
                strdup(m_source.pPath.c_str()),
                strdup (m_dest.pPath.c_str()),
                strdup (m_name.c_str()),
                strdup (mode),
                NULL};
        execv("/root/pbkr/pbkr_cpy.sh", argv);
        _exit (-1);
    }
    else
    {
        const int flag(WNOHANG);
        while (not (isExitting() or m_failed))
        {
            const int waitRes (waitpid(cpid, &status, flag));
            if (waitRes < 0) throw EXCEPTION("Fork failed2");
            if (WIFEXITED(status)) break;
            if (WIFSIGNALED(status)) break;
            usleep(1000 * 20);
        }
    }

    if (WEXITSTATUS(status) != 0) m_failed = true;
    m_done = true;
}

/*******************************************************************************/
std::string
ProjectCopier::progress (void)const
{
    static const string filename = "/tmp/progress";
    return getLastLineInFile (filename);
}

} // namespace PBKR
