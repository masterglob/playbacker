
#include <unistd.h>

#include "pbkr_menu.h"
#include "pbkr_cfg.h"
#include "pbkr_osc.h"
#include "pbkr_display_mgr.h"
#include "pbkr_projects.h"
#include "pbkr_utils.h"
#include "pbkr_version.h"

namespace
{
using namespace PBKR;

// http://patorjk.com/software/taag/#p=display&f=Banner3&t=NET%20MENU

// All menus:
struct ListMenuItem;
struct PlayMenuItem;
struct NetShowMenuItem;
struct SettingsMenuItem;
struct SelectProjectShowMenuItem;
struct ClicSettingsMenuItem;
struct CopyFromUSBMenuItem;
struct DeleteInternalProjectMenuItem;


static const char* label_empty ("<EMPTY>");
static string boolToString(const bool b)
{
    return (b ? "<YES>" : "<NO>");
}

static inline void increment (uint32_t& val, const int incr, const uint32_t max)
{
    val = (val + max + incr) % max;
}

static const std::string edit_me ("->");
/*******************************************************************************/
static const std::string addVertArrow(const std::string& title, bool canLeft=true, bool canRight=true)
{
    char result[DISPLAY::DISPLAY_WIDTH+1];
    snprintf(result, DISPLAY::DISPLAY_WIDTH-2, title.c_str());

    size_t i;
    for (i = strlen(result); i < DISPLAY::DISPLAY_WIDTH-2;i++)
    {
        result[i]=' ';
    }
    if (i > DISPLAY::DISPLAY_WIDTH-2) i = DISPLAY::DISPLAY_WIDTH-2;
    result[i++] = (canLeft ? 0x7F : ' ');
    result[i++] = (canRight ? 0x7E : ' ');
    result[i++] = 0;
    return result;
}

/*******************************************************************************/
struct NumParam
{
    NumParam (const std::string& name, const std::string& unit,
                        const int max_val,const int min_val,const int inc_val, const int val):
            m_name(name), m_unit(unit),
            m_max_val(max_val), m_min_val(min_val), m_inc_val(inc_val),m_val(val) {}
    const std::string m_name;
    const std::string m_unit;
    const int m_max_val;
    const int m_min_val;
    const int m_inc_val;
    int m_val;
    bool canEdit(void)const{return m_min_val < m_max_val;}
};

/*******************************************************************************/
struct FixedNumParam :NumParam
{
    FixedNumParam (const std::string& name,const std::string& unit,const int val):
            NumParam(name,unit, val,val,val,val){}
};

/*******************************************************************************
 * SUB MENUS
 *******************************************************************************/
struct PlayMenuItem : MenuItem
{
    PlayMenuItem(void): MenuItem("PLAY Menu"){}
    virtual ~PlayMenuItem(void){}
    virtual void onExit(void);
    virtual void onEnter(const bool longPressed=false);
    virtual void onUpDownPress(const bool isUp);
    virtual void onLeftRightPress(const bool isLeft);
    virtual const std::string menul1(void){return "";}
    virtual const std::string menul2(void){return "";}
};
PlayMenuItem playMenuItem;

MenuItem mainMenuItem("Main Menu", &playMenuItem);
MenuItem netMenuItem ("Network", &mainMenuItem);

/*******************************************************************************/
struct ListMenuItem : MenuItem
{
    ListMenuItem(const std::string & title, MenuItem* parent, const uint32_t maxItems);
    virtual ~ListMenuItem(void){}
    virtual void onLeftRightPress(const bool isLeft);
    virtual const std::string menul1(void);
protected:
    const uint32_t m_lrIdx_Max;
    uint32_t m_lrIdx;
};

/*******************************************************************************/
struct NetShowMenuItem : ListMenuItem
{
    NetShowMenuItem(const std::string & title, MenuItem* parent);
    virtual ~NetShowMenuItem(void){}
    virtual void onUpDownPress(const bool isUp);
    virtual const std::string menul1(void);
    virtual const std::string menul2(void);
private:
    const std::string menul(const bool line1);
    uint32_t m_upDownIdx;
    uint32_t m_upDownIdxMax;
};
NetShowMenuItem netShowMenuItem ("Show config", &netMenuItem);


/*******************************************************************************/
struct NetConnMenuItem : ListMenuItem
{
    NetConnMenuItem(const std::string & title, MenuItem* parent);
    virtual ~NetConnMenuItem(void){}
    virtual void onUpDownPress(const bool isUp);
    virtual const std::string menul1(void);
    virtual const std::string menul2(void);
private:
    uint32_t m_upDownIdx;
    uint32_t m_upDownIdxMax;
};
NetConnMenuItem netConnMenuItem ("Show conns", &netMenuItem);


/*******************************************************************************/
struct SettingsMenuItem : MenuItem
{
    SettingsMenuItem(const std::string & title, MenuItem* parent);
    virtual ~SettingsMenuItem(void){}
    virtual void onLeftRightPress(const bool isLeft){}
    virtual const std::string menul1(void);
    virtual const std::string menul2(void);
protected:
};
SettingsMenuItem settingsMenuItem ("Settings", &mainMenuItem);

/*******************************************************************************/
struct SongParamsMenuItem : ListMenuItem
{
    SongParamsMenuItem(const std::string & title, MenuItem* parent);
    virtual ~SongParamsMenuItem(void) = default;
    virtual const std::string menul1(void);
    virtual const std::string menul2(void);
    virtual void onShow(void);
    virtual void onEnter(const bool longPressed=false);
    virtual void onUpDownPress(const bool isUp);
private:
    typedef enum {
        ID_VOLUME_SAMPLES = 0,
        ID_VOLUME_CLIC,
        ID_COUNT
    } ItemId;
    float m_currentValue;
    size_t m_indexPlaying;
};
SongParamsMenuItem songParamsMenuItem ("Song", &mainMenuItem);

/*******************************************************************************/
struct SelectProjectShowMenuItem : ListMenuItem
{
    SelectProjectShowMenuItem(const std::string & title, MenuItem* parent);
    virtual ~SelectProjectShowMenuItem(void){}
    virtual const std::string menul2(void);
    virtual void onEnter(const bool longPressed=false);
    void setDefaultProject(void);
    static const string m_saveSection;
private:
    string m_projectTitle;
    ProjectVect m_allproj;
};
const string SelectProjectShowMenuItem::m_saveSection("SelectProjectShowMenuItem");
SelectProjectShowMenuItem selectProjectShowMenuItem ("Select show", &mainMenuItem);

/*******************************************************************************/
struct CopyFromUSBMenuItem : MenuItem
{
    CopyFromUSBMenuItem(MenuItem* parent);
    virtual ~CopyFromUSBMenuItem(void);
    virtual const std::string menul1(void);
    virtual const std::string menul2(void);
    virtual void onLeftRightPress(const bool isLeft);
    virtual void onExit(void);
    virtual void onEnter(const bool longPressed=false);
private:
    typedef enum  {S_INIT ,S_CONFIRM, S_COPY_MODE, S_COPY, S_FAILED, S_CANCELED,S_SUCCESS} State;

    ProjectVect m_projects;
    uint32_t m_projIdx;
    State m_step;
    bool m_confirm;
    ProjectCopier::CopyMode m_copyMode;
    ProjectCopier* m_copier;
};

CopyFromUSBMenuItem copyFromUSBMenuItem (&mainMenuItem);


/*******************************************************************************/
struct DeleteInternalProjectMenuItem : MenuItem
{
    DeleteInternalProjectMenuItem(MenuItem* parent);
    virtual ~DeleteInternalProjectMenuItem(void);
    virtual const std::string menul1(void);
    virtual const std::string menul2(void);
    virtual void onLeftRightPress(const bool isLeft);
    virtual void onExit(void);
    virtual void onEnter(const bool longPressed=false);
private:
    typedef enum  {S_CHOOSE ,S_CONFIRM, S_DELETE, S_FAILED, S_CANCELED, S_SUCCESS} State;

    ProjectVect m_projects;
    uint32_t m_projIdx;
    State m_step;
    bool m_confirm;
    ProjectDeleter* m_deleter;
};
DeleteInternalProjectMenuItem deleteInternalProjectMenuItem (&mainMenuItem);


/*******************************************************************************/
struct ClicSettingsMenuItem : ListMenuItem
{
    ClicSettingsMenuItem(const std::string & title, MenuItem* parent);
    virtual ~ClicSettingsMenuItem(void){}
    virtual void onEnter(const bool longPressed=false);
    virtual void onUpDownPress(const bool isUp);
    virtual const std::string menul1(void);
    virtual const std::string menul2(void);
    void sendVolume(void);
    void refreshLatency(void);
private:
    static int paramToVolume(const int param){return (param * 127) /100;}
    void setLatency(void);
    typedef enum {
        ID_VOLUME = 0,
        ID_LATENCY,
        ID_CHANN_L,
        ID_CHANN_R,
        ID_PRIM_NOTE,
        ID_SECN_NOTE,
        ID_COUNT
    } ItemId;
    NumParam m_pVolume;
    NumParam m_pLatency;
    NumParam m_pChannL;
    NumParam m_pChannR;
    NumParam m_pPriNote;
    NumParam m_pSecNote;
    NumParam* m_param[ID_COUNT];
};
ClicSettingsMenuItem clicSettingsMenuItem ("Clic settings", &settingsMenuItem);

/*******************************************************************************
 *******************************************************************************
 * IMPLEMENTATIONS
 *******************************************************************************
 *******************************************************************************/

/*******************************************************************************
 * ListMenuItem
 *******************************************************************************/

ListMenuItem::ListMenuItem(const std::string & title, MenuItem* parent, const uint32_t maxItems):
    MenuItem(title, parent),
    m_lrIdx_Max(maxItems),
    m_lrIdx(0)
{}

/*******************************************************************************/
const std::string ListMenuItem::menul1(void)
{
    return addVertArrow(subMenuName(), m_lrIdx > 0, m_lrIdx + 1 < m_lrIdx_Max);
}

/*******************************************************************************/
void ListMenuItem::onLeftRightPress(const bool isLeft)
{
    if (isLeft)
    {
        if (m_lrIdx > 0)
            m_lrIdx--;
    }
    else
    {
        if (m_lrIdx + 1 < m_lrIdx_Max)
            m_lrIdx++;
    }
    onShow();
}


/*******************************************************************************


########  ##          ###    ##    ##    ##     ## ######## ##    ## ##     ##
##     ## ##         ## ##    ##  ##     ###   ### ##       ###   ## ##     ##
##     ## ##        ##   ##    ####      #### #### ##       ####  ## ##     ##
########  ##       ##     ##    ##       ## ### ## ######   ## ## ## ##     ##
##        ##       #########    ##       ##     ## ##       ##  #### ##     ##
##        ##       ##     ##    ##       ##     ## ##       ##   ### ##     ##
##        ######## ##     ##    ##       ##     ## ######## ##    ##  #######


 *******************************************************************************/
void PlayMenuItem::onEnter(const bool longPressed)
{
    fileManager.startReading();
}

/*******************************************************************************/
void PlayMenuItem::onExit(void)
{
    if (fileManager.reading())
    {
        fileManager.stopReading();
    }
    else
    {
        globalMenu.setMenu(&mainMenuItem);
    }
}

/*******************************************************************************/
void PlayMenuItem::onUpDownPress(const bool isUp)
{
    if (!isUp)
        fileManager.prevTrack();
    else
        fileManager.nextTrack();
}

/*******************************************************************************/
void PlayMenuItem::onLeftRightPress(const bool isLeft)
{
    if (isLeft)
    {
        fileManager.backward();
    }
    else
    {
        fileManager.fastForward();
    }
}

/*******************************************************************************
########  ######## ##       ######## ######## ########    ########  ########   #######        ##
##     ## ##       ##       ##          ##    ##          ##     ## ##     ## ##     ##       ##
##     ## ##       ##       ##          ##    ##          ##     ## ##     ## ##     ##       ##
##     ## ######   ##       ######      ##    ######      ########  ########  ##     ##       ##
##     ## ##       ##       ##          ##    ##          ##        ##   ##   ##     ## ##    ##
##     ## ##       ##       ##          ##    ##          ##        ##    ##  ##     ## ##    ##
########  ######## ######## ########    ##    ########    ##        ##     ##  #######   ######
*******************************************************************************/
DeleteInternalProjectMenuItem::DeleteInternalProjectMenuItem (MenuItem* parent)
:
        MenuItem("Delete proj.", parent),
        //m_projects(getAllProjects()),
        m_projIdx(0),
        m_step(S_CHOOSE),
        m_confirm(false),
        m_deleter(NULL)
{
}

/*******************************************************************************/
DeleteInternalProjectMenuItem::~DeleteInternalProjectMenuItem(void)
{
    if (m_deleter) delete m_deleter;
}

/*******************************************************************************/
const std::string
DeleteInternalProjectMenuItem::menul1(void)
{ // S_CHOOSE ,S_CONFIRM, S_FAILED,S_SUCCESS S_DELETE
    switch (m_step) {
    case S_CANCELED:
    case S_CHOOSE: return "Delete project:";
    case S_CONFIRM: return "Confirm?";
    case S_DELETE:
    {
        if (m_deleter)
        {
            if (m_deleter->failed())
            {
                m_step = S_FAILED;
            }
            else if (m_deleter->done())
            {
                m_step = S_SUCCESS;
            }
        }
        else
        {
            m_step = S_FAILED;
        }
        return "Deleting...";
    }
    case S_FAILED:  return "Deletion...";
    case S_SUCCESS: return "Deletion...";
    default: return label_empty;
    }
}

/*******************************************************************************/
const std::string
DeleteInternalProjectMenuItem::menul2(void)
{
    m_projects.clear();
    getProjects (m_projects, projectSourceInternal);
    if (m_projIdx >= m_projects.size())
    {
        m_projIdx = 0;
        return "No USB projects!";
    }
    if (m_projIdx < 0 && !m_projects.empty())
    {
        m_projIdx = 0;
    }

    switch (m_step) {
    case S_CHOOSE:  return m_projects[m_projIdx]->m_title;
    case S_CONFIRM: return boolToString (m_confirm);
    case S_DELETE:  return "<CANCEL>";
    case S_CANCELED: return "  ...canceled";
    case S_FAILED:  return "  ...failed";
    case S_SUCCESS: return "  ...success";
    default: return label_empty;
    }
}

/*******************************************************************************/
void
DeleteInternalProjectMenuItem::onLeftRightPress(const bool isLeft)
{
    const int delta (isLeft ? -1 : +1);
    if (m_step == S_CHOOSE)
    {
        increment (m_projIdx, delta, m_projects.size());
    }
    else if (m_step == S_CONFIRM) {m_confirm = not m_confirm;}
}

/*******************************************************************************/
void
DeleteInternalProjectMenuItem::onExit(void)
{
    switch (m_step) {
    case S_FAILED:
    case S_SUCCESS:
    case S_CANCELED:
    {
        m_step = S_CHOOSE;
        if (m_deleter)
            m_deleter->cancel();
        globalMenu.setMenu(m_parent);
    }
    break;
    default:
        m_step = S_CANCELED;
    }
}

/*******************************************************************************/
void
DeleteInternalProjectMenuItem::onEnter(const bool longPressed)
{
    // S_CHOOSE ,S_CONFIRM, S_FAILED,S_SUCCESS S_DELETE
    if (m_projIdx >= m_projects.size())
    {
        m_projIdx = 0;
        m_step = S_FAILED;
    }
    switch (m_step) {
    case S_CHOOSE:
    {
        m_confirm = true;
        m_step = S_CONFIRM;
    }
    break;
    case S_CONFIRM:
    {
        if (m_confirm)
        {
            m_step = S_DELETE;
            if (m_deleter) delete m_deleter;
            m_deleter = new ProjectDeleter (m_projects[m_projIdx]);
            m_deleter->begin();
        }
        else m_step = S_CANCELED;
    }
    break;
    case S_DELETE:
    {
        m_step = S_FAILED;
        m_deleter->cancel();
    }
    break;
    case S_FAILED:
    case S_CANCELED:
    case S_SUCCESS:
    {
        m_step = S_CHOOSE;
        globalMenu.setMenu(m_parent);
        if (m_deleter)
            m_deleter->cancel();
    }
    break;
    default: break;
    }
}
/*******************************************************************************
 ######   #######  ########  ##    ##
##    ## ##     ## ##     ##  ##  ##
##       ##     ## ##     ##   ####
##       ##     ## ########     ##
##       ##     ## ##           ##
##    ## ##     ## ##           ##
 ######   #######  ##           ##
*******************************************************************************/
CopyFromUSBMenuItem::CopyFromUSBMenuItem (MenuItem* parent)
:
        MenuItem("Copy from USB", parent),
        m_projIdx(0),
        m_step(S_INIT),
        m_confirm(false),
        m_copyMode(ProjectCopier::CM_OVERWRITE),
        m_copier(NULL)
{
}

/*******************************************************************************/
CopyFromUSBMenuItem::~CopyFromUSBMenuItem(void)
{
    if (m_copier)
        delete m_copier;
}

/*******************************************************************************/
const std::string
CopyFromUSBMenuItem::menul1(void)
{
    switch (m_step) {
    case S_INIT: return "Select USB Proj.";
    case S_COPY_MODE: return "Delete existing?";
    case S_CONFIRM: return "Confirm?";
    case S_COPY: // Copying
        {
            if (m_copier)
            {
                if (m_copier->failed())
                {
                    m_step = S_FAILED;
                }
                else if (m_copier->done())
                {
                    m_step = S_SUCCESS;
                }
                return m_copier->progress();
            }
            m_step = S_FAILED;
            return "Copy:";
        }
    case S_CANCELED:
    case S_FAILED:
    case S_SUCCESS: return "Copy result:";
    default: return label_empty;
    }
}

/*******************************************************************************/
const std::string
CopyFromUSBMenuItem::menul2(void)
{
    m_projects.clear();
    getProjects (m_projects, projectSourceUSB);
    if (m_projIdx >= m_projects.size())
    {
        m_projIdx = 0;
        return "No USB projects!";
    }
    if (m_projIdx < 0 && !m_projects.empty())
    {
        m_projIdx = 0;
    }
    switch (m_step) {
    case S_INIT: return m_projects[m_projIdx]->m_title;
    case S_COPY_MODE:
    {
        switch (m_copyMode) {
        // CM_OVERWRITE ,CM_COMPLETE, CM_BACKUP, CM_CANCEL
        case ProjectCopier::CM_OVERWRITE:return "<Overwrite>";
        case ProjectCopier::CM_COMPLETE:return "<Complete>";
        case ProjectCopier::CM_BACKUP:return "<Backup>";
        case ProjectCopier::CM_CANCEL:return "<Cancel>";
        default: break;
        }
        break;
    }
    case S_CONFIRM: return boolToString (m_confirm);
    case S_COPY: return "<Cancel>";
    case S_FAILED: return "Failed...";
    case S_SUCCESS: return "Success...";
    case S_CANCELED: return "Canceled.";
    default: break;
    }
    return label_empty;
}

/*******************************************************************************/
void
CopyFromUSBMenuItem::onLeftRightPress(const bool isLeft)
{
    const int delta (isLeft ? -1 : +1);
    if (m_step == S_INIT)
    {
        increment (m_projIdx, delta, m_projects.size());
    }
    else if (m_step == S_CONFIRM) {m_confirm = not m_confirm;}
    else if (m_step == S_COPY_MODE)
    {
        uint32_t mode ((uint32_t)m_copyMode);
        increment (mode, delta, (uint32_t)ProjectCopier::NB_CM);
        m_copyMode = (ProjectCopier::CopyMode)mode;
    }
}

/*******************************************************************************/
void
CopyFromUSBMenuItem::onExit(void)
{
    if (m_copier) delete m_copier;
    m_copier = NULL;

    switch (m_step) {
    case S_FAILED:
    case S_SUCCESS:
    case S_CANCELED:
    {
        m_step = S_INIT;
        globalMenu.setMenu(m_parent);
    }
    break;
    default:
        m_step = S_CANCELED;
    }
}

/*******************************************************************************/
void
CopyFromUSBMenuItem::onEnter(const bool longPressed)
{
    if (m_projIdx >= m_projects.size())
    {
        m_projIdx = 0;
        m_step = S_FAILED;
    }
    switch (m_step) {
    case S_INIT:
    {
        if (m_copier) delete m_copier;
        m_copier = new ProjectCopier (m_projects[m_projIdx], projectSourceInternal);
        m_step = (m_copier->willOverwrite() ? S_COPY_MODE : S_CONFIRM);
        m_confirm = true;
    }
    break;
    case S_COPY_MODE:
    {
        m_step = (m_copyMode == ProjectCopier::CM_CANCEL ? S_FAILED : S_CONFIRM);
    }
    break;
    case S_CONFIRM:
    {
        if (m_confirm)
        {
            m_step = S_COPY;
            m_copier->begin(m_copyMode);
        }
        else
        {
            m_step = S_CANCELED;
        }
    }
    break;
    case S_COPY: // Copying
    {
        // Action = stop
        if (m_copier) m_copier->cancel();
        m_step = S_FAILED;
    }
    break;
    case S_FAILED:
    case S_CANCELED:
    case S_SUCCESS: // OK
    {
        if (m_copier) delete m_copier;
        m_copier = NULL;
        m_step = S_INIT;
        globalMenu.setMenu(m_parent);
    }
    break;
    default: break;
    }
}


/*******************************************************************************
 *
 ######   #######  ##    ##  ######      ########     ###    ########     ###    ##     ##  ######
##    ## ##     ## ###   ## ##    ##     ##     ##   ## ##   ##     ##   ## ##   ###   ### ##    ##
##       ##     ## ####  ## ##           ##     ##  ##   ##  ##     ##  ##   ##  #### #### ##
 ######  ##     ## ## ## ## ##   ####    ########  ##     ## ########  ##     ## ## ### ##  ######
      ## ##     ## ##  #### ##    ##     ##        ######### ##   ##   ######### ##     ##       ##
##    ## ##     ## ##   ### ##    ##     ##        ##     ## ##    ##  ##     ## ##     ## ##    ##
 ######   #######  ##    ##  ######      ##        ##     ## ##     ## ##     ## ##     ##  ######

*******************************************************************************/
SongParamsMenuItem::SongParamsMenuItem (const std::string & title, MenuItem* parent)
:
        ListMenuItem(title, parent, 2),
        m_currentValue(-1.0f),
        m_indexPlaying(0)
{
}

/*******************************************************************************/
const std::string
SongParamsMenuItem::menul1(void)
{
    m_lrIdx %= m_lrIdx_Max; // Just for robustness

    static const std::string P_NAMES[] = {"Volume OUT", "Volume CLIC"};
    string result = P_NAMES[m_lrIdx];
    if (fileManager.fileAreParamsModified(m_indexPlaying))
        result += "*";
    return addVertArrow(result, m_lrIdx > 0, m_lrIdx + 1 < m_lrIdx_Max);
}

/*******************************************************************************/
const std::string
SongParamsMenuItem::menul2(void)
{
    if (m_currentValue < 0) return "<N.A.>";
    int iVal = static_cast<int>(100.0 * m_currentValue);
    return std::to_string(iVal)+ "\%";
}

/*******************************************************************************/
void
SongParamsMenuItem::onUpDownPress(const bool isUp)
{
    if (m_indexPlaying == 0) return;
    if (m_currentValue < 0) return;

    const float ratio(m_currentValue < 0.5f ? 1.125 : 1.05);
    if (isUp)
    {
        m_currentValue *= ratio;
        if (m_currentValue > 1.0f) m_currentValue = 1.0f;
    }
    else
    {
        m_currentValue /= ratio;
        if (m_currentValue < 0.05f) m_currentValue = 0.05f;
    }

    m_lrIdx %= m_lrIdx_Max; // Just for robustness
    switch (m_lrIdx) {
    case ID_VOLUME_SAMPLES:
        fileManager.fileSetVolumeSamples(m_indexPlaying, m_currentValue);
        break;
    case ID_VOLUME_CLIC:
        fileManager.fileSetVolumeClic(m_indexPlaying, m_currentValue);
        break;
    default:
        break;
    }
}

/*******************************************************************************/
void
SongParamsMenuItem::onShow(void)
{
    m_lrIdx %= m_lrIdx_Max; // Just for robustness
    m_currentValue = 1.0f;
    m_indexPlaying = fileManager.indexPlaying();

    if (m_indexPlaying == 0) return;

    switch (m_lrIdx) {
    case ID_VOLUME_SAMPLES:
        m_currentValue = fileManager.fileGetVolumeSamples(m_indexPlaying);
        break;
    case ID_VOLUME_CLIC:
        m_currentValue = fileManager.fileGetVolumeClic(m_indexPlaying);
        break;
    default:
        break;
    }
}

/*******************************************************************************/
void
SongParamsMenuItem::onEnter(const bool longPressed)
{
    m_lrIdx %= m_lrIdx_Max; // Just for robustness

    fileManager.fileSaveParamsModification(m_indexPlaying);

    m_currentValue = -1.0f;
    onExit();
}


/*******************************************************************************
 *
########  ########   #######        ## ########  ######  ########
##     ## ##     ## ##     ##       ## ##       ##    ##    ##
##     ## ##     ## ##     ##       ## ##       ##          ##
########  ########  ##     ##       ## ######   ##          ##
##        ##   ##   ##     ## ##    ## ##       ##          ##
##        ##    ##  ##     ## ##    ## ##       ##    ##    ##
##        ##     ##  #######   ######  ########  ######     ##

*******************************************************************************/
SelectProjectShowMenuItem::SelectProjectShowMenuItem (const std::string & title, MenuItem* parent)
:
        ListMenuItem(title, parent, 8),
        m_projectTitle("")
{
}


/*******************************************************************************/
void
SelectProjectShowMenuItem::setDefaultProject(void)
{
    m_projectTitle = Config::instance().loadStr(m_saveSection);

    m_allproj.clear();
    getProjects (m_allproj, projectSourceInternal);
    if (m_projectTitle.length() > 0)
    {
        printf("Trying to auto load previous show: '%s'\n",m_projectTitle.c_str());
        onEnter();
    }
}


/*******************************************************************************/
const std::string
SelectProjectShowMenuItem::menul2(void)
{
    m_allproj.clear();
    getProjects (m_allproj, projectSourceInternal);
    if (m_lrIdx < m_allproj.size())
    {
        m_projectTitle = m_allproj[m_lrIdx]->m_title;
        return m_projectTitle;
    }
    m_projectTitle = "";
    return std::string("<EMPTY") + std::to_string(m_lrIdx + 1) +">";
}

/*******************************************************************************/
void
SelectProjectShowMenuItem::onEnter(const bool longPressed)
{
    if (m_projectTitle.length() > 0)
    {
        Project* p(findProjectByTitle(m_allproj, m_projectTitle));
        if (p)
        {
            fileManager.loadProject(p);
            globalMenu.setMenu(&playMenuItem);
            Config::instance().saveStr(m_saveSection, m_projectTitle);
        }
        else
        {
            printf("Failed to load:%s\n",m_projectTitle.c_str());
        }
    }
}

/*******************************************************************************
 *

##    ## ######## ########    ##     ## ######## ##    ## ##     ##
###   ## ##          ##       ###   ### ##       ###   ## ##     ##
####  ## ##          ##       #### #### ##       ####  ## ##     ##
## ## ## ######      ##       ## ### ## ######   ## ## ## ##     ##
##  #### ##          ##       ##     ## ##       ##  #### ##     ##
##   ### ##          ##       ##     ## ##       ##   ### ##     ##
##    ## ########    ##       ##     ## ######## ##    ##  #######


*******************************************************************************/
NetShowMenuItem::NetShowMenuItem (const std::string & title, MenuItem* parent)
:
        ListMenuItem(title, parent, 4),
        m_upDownIdx(0),
        m_upDownIdxMax(3)
{}

/*******************************************************************************/
void NetShowMenuItem::onUpDownPress(const bool isUp)
{
    if (not isUp)
    {
        if (m_upDownIdx > 0) m_upDownIdx --;
    }
    else
    {
        if (m_upDownIdx + 1 < m_upDownIdxMax) m_upDownIdx ++;
    }
}

/*******************************************************************************/
const std::string NetShowMenuItem::menul(const bool line1)
{
    static const char * ETH_NAMES[] = {"ETH ", "WIFI"};
    static const char * ETH_DEVS[] = {NET_DEV_ETH,NET_DEV_WIFI};
    static const char * SUBMENU_NAME[] = {"IP ", "NETMSK", "NAME"};

    static const std::string sharp("#");
    static const std::string colon(":");
    static const std::string slash("/");

    if (m_lrIdx >= 2)
    {
        return (line1 ? sharp + std::to_string(m_lrIdx + 1) : "##noitem##");
    }
    const char* name = ETH_NAMES[m_lrIdx];
    const char* dev = ETH_DEVS[m_lrIdx];
    if (m_upDownIdx >= 3)
    {
        return (line1 ? name + colon : "##noitem##");
    }
    const std::string submenuname (SUBMENU_NAME[m_upDownIdx]);
    if (line1)
    {
        return addVertArrow( name + slash + submenuname + colon);
    }
    else
    {
        switch (m_upDownIdx)
        {
        case 0:
            return getIPAddr(dev);
        case 1:
            return getIPNetMask(dev);
        case 2:
            return dev;
        default:
            return "##noitem##";
        }
    }
}

/*******************************************************************************/
const std::string NetShowMenuItem::menul1(void)
{
    return menul(true);
}

/*******************************************************************************/
const std::string NetShowMenuItem::menul2(void)
{
    return menul(false);
}

/*******************************************************************************
*******************************************************************************/
NetConnMenuItem::NetConnMenuItem (const std::string & title, MenuItem* parent)
:
        ListMenuItem(title, parent, 1),
        m_upDownIdx(0),
        m_upDownIdxMax(0)
{}

/*******************************************************************************/
void NetConnMenuItem::onUpDownPress(const bool isUp)
{}

/*******************************************************************************/
const std::string NetConnMenuItem::menul1(void)
{
    return "OSC control:";
}

/*******************************************************************************/
const std::string NetConnMenuItem::menul2(void)
{
    using namespace OSC;
    OSC_Controller* inst = p_osc_instance;
    if (inst)
    {
        uint32_t addr = inst->clientConnected();
        static char res[17];
        memset(res, 0, sizeof(res));
        snprintf(res, sizeof(res), "%d.%d.%d.%d",
                (uint8_t)(addr),
                (uint8_t)(addr>>8),
                (uint8_t)(addr>>16),
                (uint8_t)(addr>>24));
        return res;
    }
    return "";
}


/*******************************************************************************
 *
 ######  ######## ######## ######## #### ##    ##  ######    ######
##    ## ##          ##       ##     ##  ###   ## ##    ##  ##    ##
##       ##          ##       ##     ##  ####  ## ##        ##
 ######  ######      ##       ##     ##  ## ## ## ##   ####  ######
      ## ##          ##       ##     ##  ##  #### ##    ##        ##
##    ## ##          ##       ##     ##  ##   ### ##    ##  ##    ##
 ######  ########    ##       ##    #### ##    ##  ######    ######
*******************************************************************************/
SettingsMenuItem::SettingsMenuItem(const std::string & title, MenuItem* parent)
:
        MenuItem(title,parent)
{

}
const std::string SettingsMenuItem::menul1(void)
{
    return "Version";
}
const std::string SettingsMenuItem::menul2(void)
{
    static const std::string version (PBKR_VERSION);
    static const std::string build (std::to_string(PBKR_BUILD_ID));
    return version+" b"+build;
}

/*******************************************************************************
 *
 ######  ##       ####  ######     ##     ## ######## ##    ## ##     ##
##    ## ##        ##  ##    ##    ###   ### ##       ###   ## ##     ##
##       ##        ##  ##          #### #### ##       ####  ## ##     ##
##       ##        ##  ##          ## ### ## ######   ## ## ## ##     ##
##       ##        ##  ##          ##     ## ##       ##  #### ##     ##
##    ## ##        ##  ##    ##    ##     ## ##       ##   ### ##     ##
 ######  ######## ####  ######     ##     ## ######## ##    ##  #######
*******************************************************************************/
ClicSettingsMenuItem::ClicSettingsMenuItem (const std::string & title, MenuItem* parent)
:
        ListMenuItem(title, parent, ID_COUNT),
         m_pVolume(NumParam("Volume", "", 100, 10, 10, 90)),
         m_pLatency(NumParam("Latency", "ms", 50, -60, 2, 2)),
         m_pChannL(FixedNumParam ("Left Chann.", "", 15)),
         m_pChannR(FixedNumParam ("Right Chann.", "", 16)),
         m_pPriNote(FixedNumParam ("Primary Note", "", 24)),
         m_pSecNote(FixedNumParam ("Second. Note", "", 25)),
        m_param{&m_pVolume, &m_pLatency,&m_pChannL,&m_pChannR, &m_pPriNote,&m_pSecNote}
{
    m_pVolume.m_val = Config::instance().loadInt("MIDI_VOLUME", 60);
    m_pLatency.m_val = ( Config::instance().loadInt("MIDI_LATENCY", 4));
    setLatency ();
}

/*******************************************************************************/
void ClicSettingsMenuItem::onEnter(const bool longPressed)
{
    onExit();
}

/*******************************************************************************/
void  ClicSettingsMenuItem::sendVolume(void)
{
    const uint8_t vol8 (paramToVolume(m_pVolume.m_val));

    MidiOutMsg msg;
    msg.push_back(vol8); // Volume value
    wemosControl.pushSysExMessage(WemosControl::SYSEX_COMMAND_VOLUME, msg);
}

/*******************************************************************************/
void  ClicSettingsMenuItem::refreshLatency(void)
{
    const int prevLatency (m_pLatency.m_val);
    m_pLatency.m_val = 0;
    setLatency();
    m_pLatency.m_val = prevLatency;
    setLatency();
}

/*******************************************************************************/
void  ClicSettingsMenuItem::setLatency(void)
{
    leftLatency.setMs(m_pLatency.m_val);
    rightLatency.setMs(m_pLatency.m_val);
    leftClicLatency.setMs(-m_pLatency.m_val);
    rightClicLatency.setMs(-m_pLatency.m_val);
    midiLatency.setMs(-m_pLatency.m_val);
}

/*******************************************************************************/
void ClicSettingsMenuItem::onUpDownPress(const bool isUp)
{
    if (! m_param[m_lrIdx]) return;
    NumParam& param(* m_param[m_lrIdx]);
    int& value (param.m_val);

    // update param
    if (isUp)
    {
        if (value + param.m_inc_val >= param.m_max_val)
            value = param.m_max_val;
        else
            value += param.m_inc_val;
    }
    else
    {
        if (value <= param.m_min_val + param.m_inc_val)
            value = param.m_min_val;
        else
            value -= param.m_inc_val;
    }

    // Processing
    switch (m_lrIdx)
    {
    case ID_VOLUME:
    {
        Config::instance().saveInt("MIDI_VOLUME", value);
        sendVolume ();
        break;
    }
    case ID_LATENCY:
        Config::instance().saveInt("MIDI_LATENCY", value);
        setLatency();
        break;
    default:
        break;
    }
}

/*******************************************************************************/
const std::string ClicSettingsMenuItem::menul1(void)
{
    NumParam* param( m_param[m_lrIdx]);
    if (!param) return label_empty;
    return (m_lrIdx < ID_COUNT ? addVertArrow (param->m_name) : label_empty);
}

/*******************************************************************************/
const std::string ClicSettingsMenuItem::menul2(void)
{
    NumParam* param( m_param[m_lrIdx]);
    if (!param) return label_empty;
    return (param->canEdit() ? edit_me : std::string (""))
            + std::to_string(param->m_val) + param->m_unit;
}

/*******************************************************************************
 *
##     ## ######## #### ##        ######
##     ##    ##     ##  ##       ##    ##
##     ##    ##     ##  ##       ##
##     ##    ##     ##  ##        ######
##     ##    ##     ##  ##             ##
##     ##    ##     ##  ##       ##    ##
 #######     ##    #### ########  ######

*******************************************************************************/

} // namespace


namespace PBKR
{

void refreshMidiVolume    (void)
{
    ::clicSettingsMenuItem.sendVolume();
}

void refreshLatency(void)
{
    ::clicSettingsMenuItem.refreshLatency();
}
/*******************************************************************************


##     ## ######## ##    ## ##     ##    #### ######## ######## ##     ##
###   ### ##       ###   ## ##     ##     ##     ##    ##       ###   ###
#### #### ##       ####  ## ##     ##     ##     ##    ##       #### ####
## ### ## ######   ## ## ## ##     ##     ##     ##    ######   ## ### ##
##     ## ##       ##  #### ##     ##     ##     ##    ##       ##     ##
##     ## ##       ##   ### ##     ##     ##     ##    ##       ##     ##
##     ## ######## ##    ##  #######     ####    ##    ######## ##     ##


 *******************************************************************************/
MenuItem::MenuItem (const std::string& title, MenuItem* parent):
        name(title),m_parent(parent),m_iter(m_subMenus.end())
{
    printf("MenuItem:%s\n",title.c_str());
    if (m_parent)
    {
        m_parent->m_subMenus.push_back(this);
        m_parent->m_iter = m_parent->m_subMenus.begin();
    }
}

/*******************************************************************************/
void MenuItem::onLeftRightPress(const bool isLeft)
{
    if (m_iter == m_subMenus.end()) return;
    if (isLeft)
    {
        if (m_iter != m_subMenus.begin())
            m_iter--;
        else
            m_iter = m_subMenus.end() - 1;
    }
    else
    {
        m_iter++;
        if (m_iter == m_subMenus.end()) m_iter = m_subMenus.begin();
    }
}

/*******************************************************************************/
void MenuItem::onEnter(const bool longPressed)
{
    if (m_iter != m_subMenus.end())
    {
        MenuItem* menuItem (*m_iter);
        globalMenu.setMenu(menuItem);
    }
}

/*******************************************************************************/
void MenuItem::onExit(void)
{
    if (m_parent)
    {
        globalMenu.setMenu(m_parent);
    }
}

/*******************************************************************************/
const std::string MenuItem::menul1(void)
{
    if (m_iter != m_subMenus.end() && m_subMenus.size() > 1)
    {
        return addVertArrow(subMenuName());
    }
    return subMenuName();
}

/*******************************************************************************/
const std::string MenuItem::menul2(void)
{
    if (m_iter != m_subMenus.end())
    {
        MenuItem* menuItem (*m_iter);
        return std::string ("<") + menuItem->subMenuName() + ">";
    }
    return label_empty;
}

MainMenu& globalMenu(MainMenu::instance());
void openCopyFromUSBMenu(){ globalMenu.setMenu(&copyFromUSBMenuItem);}
void openNetMenuFromUSBMenu(){ globalMenu.setMenu(&netMenuItem);}
void openDeleteProjectMenu(){ globalMenu.setMenu(&deleteInternalProjectMenuItem);}
void SelectProjectMenu(Project* proj)
{
    if (!proj) return;
    fileManager.loadProject(proj);
    Config::instance().saveStr(SelectProjectShowMenuItem::m_saveSection, proj->m_title);

}

/*******************************************************************************
 * MAIN MENU CONFIG
 *******************************************************************************/
static const float MAX_BTN_WAIT (1.0);
/*******************************************************************************/
MenuCfg::MenuCfg(void):
                leftGpio (GPIOs::GPIO_27_PIN13,MAX_BTN_WAIT,   "LEFT MENU"),
                rightGpio (GPIOs::GPIO_22_PIN15,MAX_BTN_WAIT,  "RIGHT MENU"),
                upGpio (GPIOs::GPIO_24_PIN18,MAX_BTN_WAIT,     "UP MENU"),
                downGpio (GPIOs::GPIO_23_PIN16,MAX_BTN_WAIT,   "DOWN MENU"),
                enterGpio (GPIOs::GPIO_06_PIN31,MAX_BTN_WAIT, "ENTER MENU"),
                exitGpio (GPIOs::GPIO_17_PIN11,MAX_BTN_WAIT, "EXIT MENU")
{
}

/*******************************************************************************
 * MAIN MENU
 *******************************************************************************/

/*******************************************************************************
 *MENU MANAGER
 *******************************************************************************/

/*******************************************************************************/
MainMenu::MainMenu(void):
        Thread("MainMenu"),
        m_cfg (MenuCfg()),
        m_currentMenu(&::playMenuItem)
{
    Thread::start();
} // MainMenu::MainMenu

/*******************************************************************************/
MainMenu& MainMenu::instance (void)
{
    static MainMenu ins;
    return ins;
} // MainMenu::instance

/*******************************************************************************/
void MainMenu::setMenu(MenuItem* menu)
{
    if (!menu) return;
    m_currentMenu = menu;
    m_currentMenu->onShow();
    if (OSC::p_osc_instance) OSC::p_osc_instance->setMenuName(menu->name);
    printf("New menu => %s\n",m_currentMenu->name.c_str());
} // MainMenu::setMenu

/*******************************************************************************/
void
MainMenu::pressKey(const Key& key)
{
    switch (key)
    {
    case KEY_LEFT:
        m_currentMenu->onLeftRightPress(true);
        break;
    case KEY_RIGHT:
        m_currentMenu->onLeftRightPress(false);
        break;
    case KEY_UP:
        m_currentMenu->onUpDownPress(true);
        break;
    case KEY_DOWN:
        m_currentMenu->onUpDownPress(false);
        break;
    case KEY_OK:
        m_currentMenu->onEnter();
        break;
    case KEY_CANCEL:
        m_currentMenu->onExit();
        break;
    default:
        break;
    }
}

/*******************************************************************************/
void MainMenu::body(void)
{
    while (not isExitting())
    {
        bool longPress(false);
        usleep(100*1000);
        if (m_cfg.leftGpio.pressed(longPress))
        {
            m_currentMenu->onLeftRightPress(true);
        }
        if (m_cfg.rightGpio.pressed(longPress))
        {
            m_currentMenu->onLeftRightPress(false);
        }

        if (m_cfg.upGpio.pressed(longPress))
        {
            m_currentMenu->onUpDownPress(true);
        }
        if (m_cfg.downGpio.pressed(longPress))
        {
            m_currentMenu->onUpDownPress(false);
        }

        if (m_cfg.enterGpio.pressed(longPress))
        {
            m_currentMenu->onEnter(longPress);
        }
        if (m_cfg.exitGpio.pressed(longPress))
        {
            m_currentMenu->onExit();
        }
    }
} // MainMenu::body

/*******************************************************************************/
void setDefaultProject(void)
{
    selectProjectShowMenuItem.setDefaultProject();
} // setDefaultProject


bool Button::pressed(bool& longPress)const
{
    longPress = false;
    if (digitalRead(m_gpio))
    {
        if (m_must_release)
        {
            return false;
        }
        if (m_pressed)
        {
            const float f = VirtualTime::toS(VirtualTime::now() - m_t0);
            if (m_maxWait < f)
            {
                // Auto release after a long press
                m_pressed = false;
                longPress = true;
                m_must_release = true;
                printf("Short press on %s\n", m_name.c_str());
                return true;
            }
        }
        else
        {
            m_t0 = VirtualTime::now();
            printf("start press on %s\n", m_name.c_str());
            m_pressed = true;
        }
    }
    else
    {
        m_must_release = false;
        if (m_pressed)
        {
            m_pressed = false;
            m_must_release = true;
            printf("Release on %s\n", m_name.c_str());
            return true;
        }
    }
    return false;
}

} // namespace PBKR

