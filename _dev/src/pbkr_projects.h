#ifndef _pbkr_projects_h_
#define _pbkr_projects_h_

#include "pbkr_config.h"
#include "pbkr_types.h"

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace PBKR
{
using namespace std;
/*******************************************************************************
 * TRACKS
 *******************************************************************************/

class Track
{
public:
    Track(const string& title,const int index,const string filename)
    :m_title(title),m_index(index),m_filename(filename){}
    string   m_title;
    int      m_index;
    string   m_filename;
};
typedef vector<Track,allocator<Track>> TrackVect;

/*******************************************************************************
 * PROJECT
 *******************************************************************************/
struct ProjectSource
{
    explicit ProjectSource(const string &name,
            const string &path):pName(name),pPath(path){};
    const string pName;
    const string pPath;
    StringVect findProjects(void)const;
};
typedef vector<ProjectSource,allocator<ProjectSource>> ProjectSourceVect;
extern const ProjectSourceVect projectSources;

/*******************************************************************************
 * PROJECT
 *******************************************************************************/

class Project
{
public:
    struct BadProject : public exception{};
    explicit Project(const string& name, const ProjectSource source);
    ~Project(void);
    const string m_title;
    const ProjectSource m_source;
    void debug(void)const;
    Track getByTrackId(int id);
    int getNbTracks(void)const;
    int getFirstTrackIndex(void)const; // -1 = no track, 0=first track
    void setInuse(bool inUse){m_inUse = inUse;}
    void setToClose(void){m_toClose = true;}
    bool inUse(void)const{return m_inUse;}
    bool toClose(void)const{return m_toClose;}
private:
    TrackVect m_tracks;
    int getNewTrackIndex(int fromId)const;
    bool m_inUse;
    bool m_toClose;
};

typedef vector<Project*,allocator<Project*>> ProjectVect;
/**
 * Scan for all projects on USB or internal.
 * On each call, pointers are kept for unchanged projects.
 * - New projects are added.
 * - removed projects are deleted when m_inUse is false
 */
ProjectVect getAllProjects(void);

/**
 * Find a project in a list using its name
 */
Project* findProjectByTitle(const ProjectVect& pVect, const string& title);

/*******************************************************************************
 *
 *******************************************************************************/

} // namespace PBKR
#endif // _pbkr_projects_h_
