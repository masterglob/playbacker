#/bin/python3
python_version = int(str(range(3))[-2])
print("Detected python %d"%python_version)

if python_version == 3:
    import tkinter as tk
    from tkinter import messagebox
    from tkinter.simpledialog import askstring
    from tkinter import ttk
elif python_version == 2:
    raise Exception("This code requires adaptations to run under python2")
    import Tkinter as tk
    import tkFileDialog
import sys, os, re

from pbkr_params import PBKR_Params
from pbkr_ssh import PBKR_SSH, SSHCommander
from pbkr_utils import DEBUG, P_NoteBook

###################
# CONFIG
PROMPT_FORM_EXIT = False

###################
# CONSTANTS
TARGET_PATH ="/mnt/mmcblk0p2"
TARGET_PROJECTS = TARGET_PATH + "/pbkr.projects"
TARGET_CONFIG = TARGET_PATH + "/pbkr.config"
WIN_WIDTH=560
WIN_HEIGHT=600
MAX_PROPERTIES = 18

PROP_TITLE = "title"
PROP_TRACK = "track"
    
class _ProjectUI():
    def __init__(self, mgr, win):
        self.mgr, self.win = mgr, win
        self._wgt=[]
        self._btns=[]
        self.setup()
        
    def setup(self, songs={}):
        '''
         @param songs :_PropertiedFileList
        '''
        
        # 1: delete existing items
        for song, var, fr in self._wgt :
            del var
            fr.destroy()
        self._wgt = []
        self._btns = []
        
        fr = tk.Frame(self.win, width=WIN_WIDTH - 15)
        btn = tk.Button (fr, text="Reorder", command = self.reorder)
        btn.grid(row = 1)
        self._btns.append(btn)
        
        btn = tk.Button (fr, text="Delete", command = lambda self=self : self.__delete())
        btn.grid(row = 1, column=2)
        self._btns.append(btn)
        
        self.readOnly = tk.BooleanVar(value = True)
        btn = tk.Checkbutton (fr, text="ReadOnly", variable = self.readOnly, command=self.onReadOnlyChange)
        btn.grid(row = 2)
        
        fr.grid(row = 1)
        
        fr = tk.Frame(self.win, width=WIN_WIDTH - 15)
        self.songs=sorted([song for song in songs.values()], key=lambda s:s.getTrackIdx())
        
        # 2: reorder songs (! handle multiple identical tracks Id)
        # 3: create new items
        y = 1
        prevSong = None
        for song in self.songs:
            var = tk.StringVar(value = song.getTitle())
            tId = "#%02d"%song.getTrackIdx() if song.getTrackIdx() != None else '#??'
            wgt = tk.Label(fr, text = tId, width = 5)
            wgt.grid(row =y, column = 0)
            
            wgt = tk.Entry(fr, textvariable = var, width = 28, state="readonly")
            wgt.grid(row =y, column = 1, padx=2, pady=2)
            
            btn = tk.Button(fr,text="Edit title", command = lambda self=self, song=song : self.onTitleEdit(song))
            btn.grid(row = y, column = 2, padx=2, pady=2)
            self._btns.append(btn)
            
            if prevSong:
                cmd = lambda self=self, song=song, prevSong=prevSong: self.onMove(song, prevSong)
                btn = tk.Button(fr,text="Move Up", command = cmd)
                btn.grid(row = y, column = 3, padx=2, pady=2)
                self._btns.append(btn)            
            
            self._wgt.append((song, var, fr))
            y += 1
            prevSong = song
        fr.grid(row =2)
        self.onReadOnlyChange()
        
    def onReadOnlyChange(self):
        state = "disabled" if self.readOnly.get() else "normal"
        for btn in self._btns:
            btn.config(state=state)
        
    def refresh(self, propName):
        if propName == PROP_TRACK:
            self.mgr.refresh(False)
        else:
            y = 1
            for song, var, fr in self._wgt :
                var.set(value = song.getTitle())
            
    def onMove(self, song, prevSong):
        if self.readOnly.get():return 
        idx1 = song.getTrackIdx()
        idx2 = prevSong.getTrackIdx()
        song.setTrackIdx(self.mgr, idx2)
        prevSong.setTrackIdx(self.mgr, idx1)
        self.mgr.refresh(False)
    
    def reorder(self):
        if self.readOnly.get():return 
        y=1
        for song in self.songs:
            song.setTrackIdx(self.mgr, y)
            y += 1
        self.mgr.refresh(False)
    
    def onTitleEdit(self, song):
        if self.readOnly.get():return 
        title = askstring('Edit Title (%s)'%song.name, 'Enter new title for file (%s)'%song.name, initialvalue=song.getTitle())
        if title:
            if '\\' in title:
                messagebox.showerror("Bad title", "Cannot use '\\' in title")
                return                
            title = title.replace ("'", "\\'")
            if song.getTitle() != title:
                print("New title : <%s>"%title)
                song.setTitle(self.mgr, title)
    def __delete(self):
        # delete current project
        if self.readOnly.get():return 
        confirm = askstring('Confirm deletion', 'Enter the project title to confirm its deletion',
                             initialvalue="")
        if confirm == self.mgr.currentProjectName():
            self.mgr.deleteCurrentProject()
        else:
            messagebox.showerror("Bad confirmation", "bad confirmation: not deleted")
            
        
class _UI():
    def __init__(self, mgr, win, params):
        self.win = win
        self.mgr = mgr
            
        ##################
        # Target selection
        fr= tk.LabelFrame(self.win, text = "Connection",width=WIN_WIDTH/2, height=150)
        tk.Label(fr, text="Select PBKR device").grid(row =1, column = 1)
        
        self.__param_RemoteIp = params.createStr("remote_ip")
        w = tk.Entry(fr, textvariable = self.__param_RemoteIp, width = 25, state="normal")
        w.grid(row = 2, column = 1)
        self.__entryIp = w
        
        self.__param_RemotePort = params.createInt("remote_port", 22)
        w = tk.Entry(fr, textvariable = self.__param_RemotePort, width = 10, state="normal")
        w.grid(row = 2, column = 2)
        self.__entryPort = w
        
        w = tk.Button(fr,text="Connect", command = self.__cbConn , width = 10)
        w.grid(row = 2, column = 3)
        self.__btnConnect = w
        w = tk.Button(fr,text="Disconnect", command = self.__cbDisconn , width = 10)
        w.grid(row = 2, column = 4)
        self.__btnDisconnect = w
            
        fr.place(x=5, y = 5)
        
        #############################
        # TABS
        tabs0W ,tabs0H = WIN_WIDTH-10, WIN_HEIGHT -150
        with P_NoteBook(self.win,width=tabs0W, height=tabs0H, label ="Connection", x=5, y = 80) as tabs0:
             
            #############################
            # TAB 1 : projects
            tab1 = tabs0.addTab('Projects')
            w = tk.Button(tab1,text="Refresh List", command = self.mgr.getProjList , width = 10)
            w.grid(row = 1, column = 1)
            self.__btnGetList = w
             
            w = tk.Button(tab1,text="Debug", command = self.mgr.debug , width = 10)
            w.grid(row = 1, column = 2)
            self.__btnDebug = w             
             
            w = ttk.Combobox(tab1, values=[], state="readonly", width = 40)
            w.grid(row = 1, column = 3, columnspan=2)
            w.bind("<<ComboboxSelected>>", self.onProjectSelect)
            self.__cbbProjList = w
            
            self.projectFrame= tk.LabelFrame(tab1, text = "Active project", width=tabs0W - 10, height=tabs0H -60)
            self.project = _ProjectUI(mgr, self.projectFrame)
            self.projectFrame.grid(row = 2, column = 1, columnspan=10)
            
            #############################
            # TAB 2 : config
            tab2 = tabs0.addTab('Config')
            for i in range(MAX_PROPERTIES):
                tkVarName = tk.StringVar(value = "")
                tkVarVal = tk.StringVar(value = "")
                w = tk.Entry(tab2, textvariable=tkVarName , width = 27, state="readonly")
                w.grid(row = i, column = 1)
                w = tk.Entry(tab2, textvariable=tkVarVal , width = 39, state="readonly")
                w.grid(row = i, column = 2)
                self.mgr.pbkrProps[i] = (w, tkVarName, tkVarVal)
                
              
#             tab2 = ttk.Frame(tabs0)
#             tabs0.add(tab2, text ='Config2')
#             tabs0.pack(expand = 1, fill ="both")
#             
            
        
        ##################
        # Status bar
        fr= tk.LabelFrame(self.win, text = "",width = tabs0W- 60, height=40)
        self.statusBar = tk.StringVar(value = "")
        l = tk.Label(fr, textvariable = self.statusBar, width = 60)
        l.grid(row =1, column = 1)
        
        fr.place(x=10, y = WIN_HEIGHT-50)
        
    def __cbConn(self):
        self.mgr.ssh.connect(self.__param_RemoteIp.get(), 
                                self.__param_RemotePort.get(), 
                                "tc","wtamot2019" )
#         self.__btnConnect.config(state="disabled")
    def __cbDisconn(self):
        self.setProjList([])
        self.mgr.ssh.disconnect()
    
    def __cbDemo1(self, success = False, result = None):
        if result == None:
            self.mgr.ssh.command("ls -1 %s"%TARGET_PROJECTS, self.__cbDemo1)
            return
        if success:
            DEBUG("Demo OK projects=%s"%result.split("\n"))
        else:
            DEBUG("Demo OK false=%s"%result)
        
    def setStatus(self, msg):
        if msg:
            self.statusBar.set(msg)
        connectedButtons=[self.__btnDisconnect, self.__btnGetList, self.__btnDebug]
        disconnectedButtons=[self.__btnConnect, self.__entryIp, self.__entryPort]
        connectingButtons=[]
        
        connectedState = "disabled"
        connectingState = "disabled"
        disconnectedState = "disabled"

        if self.mgr.ssh.isConnected():
            connectedState = "normal"
            self.mgr.setConnected(True)
        elif self.mgr.ssh.isConnecting():
            connectingState = "normal"
        else:
            disconnectedState = "normal"
            self.mgr.setConnected(False)
        
        for btn in disconnectedButtons:
            btn.config(state=disconnectedState)
        for btn in connectingButtons:
            btn.config(state=connectingState)
        for btn in connectedButtons:
            btn.config(state=connectedState)
            
    def setProjList(self, l):
        self.__cbbProjList["values"] = l[:]
        if l :
            self.__cbbProjList.current(0)
        else:
            self.__cbbProjList.config(state="normal")
            self.__cbbProjList.delete(0, "end")
            self.__cbbProjList.config(state="disabled")
        self.onProjectSelect()
            
    def onProjectSelect(self, event = None):
        curr = self.__cbbProjList.current()
        if curr >= 0:
            name = self.__cbbProjList.get()
        else: name = None
        self.mgr.onProjectSelect(name)
        
class SSHFileSizeReader(SSHCommander):
    def __init__(self, ssh, filename, file):
        self.filename = filename
        self.file = file
        SSHCommander.__init__(self, ssh, "du -k %s|cut -f 1"%filename, self.event)
    def event(self, result):
        try:
            sizeKb = int(result)
        except:
            DEBUG ("Failed to read file size(%s): <%s>"%(self.filename, result))
            sizeKb = -1
        DEBUG("File size:%s = %d Kb"%(self.filename,sizeKb))
        self.file.setSizeKb (sizeKb)
       
class SSHFilePropReader(SSHCommander):
    def __init__(self, mgr, filename, file, propName):
        self.mgr = mgr
        self.propName = propName
        self.filename = filename
        self.file = file
        cmd = "cat %s.%s"%(filename,propName)
        DEBUG("command is `%s`"%cmd)
        SSHCommander.__init__(self, mgr.ssh, cmd, self.event)
    def event(self, result):
        self.file.props[self.propName] = str(result)
        self.mgr.ui.project.refresh(self.propName)
        DEBUG ("%s of %s is %s"%(self.propName,self.filename,result))

class SSHFilePropWriter(SSHCommander):
    def __init__(self, mgr, filename, file, propName, propValue):
        self.mgr = mgr
        self.propName = propName
        self.filename = filename
        self.file = file
        cmd="echo '%s' >  %s.%s"%(propValue,filename,propName)
        DEBUG("command is `%s`"%cmd)
        SSHCommander.__init__(self, mgr.ssh, cmd, self.event)
    def event(self, result):
        # Read back value
        SSHFilePropReader(self.mgr, self.filename, self.file, self.propName)

class SSHSettingsPropReader(SSHCommander):
    def __init__(self, ssh, name, result):
        self.name = name
        self.result = result
        SSHCommander.__init__(self, ssh, "cat %s/%s.sav"%(TARGET_CONFIG,name), self.event)
    def event(self, result):
        wgt, tkName, tkVal = self.result
        tkVal.set(str(result.strip()))
        wgt.config(state= "normal")
        DEBUG ("Global setting <%s> is <%s>"%(self.name,result))
       
class _PropertiedFile:     
    def __init__(self, path, name):
        self.props={}   
        self.sizeKb=-1
        self.name=name
        self.fullPathName="%s/%s"%(path,name)
    def setSizeKb(self, sizeKb):self.sizeKb = sizeKb
    def getTitle(self):
        try: t = self.props[PROP_TITLE].strip()
        except: t= ""
        return t if t else "<%s>"%self.name
    def getTrackIdx(self):
        try:return int(self.props[PROP_TRACK])
        except: return 999
    def setTitle(self, mgr, newTitle):
        SSHFilePropWriter(mgr, self.fullPathName, self, PROP_TITLE, newTitle)
    def setTrackIdx(self, mgr, newTrack):
        SSHFilePropWriter(mgr, self.fullPathName, self, PROP_TRACK, newTrack)
        
class _PropertiedFileList:
    def __init__(self, mgr):
        self.mgr = mgr
        self.clear()
    def clear(self):
        self._path = ""
        self._list={}
    
    def byTrack(self, trackId):
        for name in self._list:
            song = self._list[name]
            if song.getTrackIdx() == trackId: return song
        return None
    
    def getFiles(self): return self._list
    def setPath(self, path):self._path = path
    def addFile(self, filename):
        m = re.match(r"(.*)([.]wav)[.](.*)", filename, flags=re.IGNORECASE)
        if m:
            n, ext = (m.group(1)+m.group(2)), m.group(3)
        else:
            m = re.match(r"(.*)([.]wav)$", filename, flags=re.IGNORECASE)
            n, ext = (m.group(1)+m.group(2)), ""
        if m:
            if n not in self._list:
                f = _PropertiedFile(self._path, n)
                self._list[n] = f
            else: f = self._list[n]
            
            filename = f.fullPathName
            
            if ext:
                f.props[ext] = None
                SSHFilePropReader(self.mgr, filename, f, PROP_TITLE)
                SSHFilePropReader(self.mgr, filename, f, PROP_TRACK)
            else:
                # read file size
                # "du -k AMOT_4T_02-HA.wav|cut -f 1"
                SSHFileSizeReader(self.mgr.ssh, filename, f)
    
class _Manager:
    def __init__(self, args):
        self.ssh = PBKR_SSH(self.__sshEvent)
        
        self._projList=[]
        self._projFiles =_PropertiedFileList(self)
        self._currProject = None
        self._isConnected = False
        self.pbkrProps=[(None, None, None) for _ in range(MAX_PROPERTIES)]
        
        win = tk.Tk()
        win.protocol("WM_DELETE_WINDOW", self.__on_closing)
        win.title("Playbacker MANAGER tool")
        win.minsize(WIN_WIDTH, WIN_HEIGHT)
        win.resizable(False, False)   
        self.win = win       
        
        self._initpath=os.path.abspath (os.path.dirname(args[0]))
        self._args = args
        self._params = PBKR_Params (self._initpath, '.pbkmgr.json')
        self.ui = _UI(self, win, self._params)          
        bExit = tk.Button(win, text="Exit", command = self.__on_closing)
        bExit.place(x = WIN_WIDTH - 50, y = WIN_HEIGHT-40)
        
        self.__param_NbExec = self._params.createInt("nb_exec")
        self.__param_NbExec.set(self.__param_NbExec.get() + 1)
        
        self.refresh(True)
        self.ssh.start()
        
    def quit(self):
        self.ssh.quit()
        self._params.saveToFile()
        self.win.destroy()
        
    def refresh(self, reload= True):
        if reload:
            self.ui.project.setup()
            self.getProjFiles()
        self.ui.project.setup(self._projFiles.getFiles())
        
    def __on_closing(self):
        DEBUG ("__on_closing")
        if (PROMPT_FORM_EXIT == False ) or tk.messagebox.askokcancel("Quit", "Do you want to quit?"):
            self.quit()
    def __sshEvent(self, statusMsg, errorMsg):
        if errorMsg:
            self.ui.setStatus("[EE] %s (%s)"%(statusMsg, errorMsg))
        else: 
            self.ui.setStatus(statusMsg)
    def run(self): self.win.mainloop()
    
    def clearFiles(self):
        self._projFiles.clear()
        self._projList=[]
    def setConnected(self, isConnected):
        if self._isConnected == isConnected: return
        self._isConnected = isConnected
        if isConnected:
            self.getProjList()
            self.getConfigFiles()
        else:
            self.clearFiles()
            self.getConfigFiles(True, "")
            
    def debug(self):
        DEBUG("Projects = %s"%self._projList)
        DEBUG("Active Project = %s"%self._currProject)
        DEBUG("Files:")
        d= self._projFiles.getFiles()
        # Sort files by track
        ds = sorted(d, key=lambda f, d=d: d[f].getTrackIdx())
        
        trackIds=[]
        for f in ds:
            props = d[f]
            tId = props.getTrackIdx()
            if tId in trackIds: self.__sshEvent("Duplicate track Id: #%d"%tId)
            trackIds.append(tId)
            DEBUG(" - #%s : %s (%d Kb) File = %s"%(tId, props.getTitle(),props.sizeKb, f))
    def getProjList(self, success = False, result = None):
        self.clearFiles()
        if result == None:
            self.ssh.command("ls -1 %s"%TARGET_PROJECTS, self.getProjList)
            return
        if success:
            DEBUG("projects=%s"%result)
            self._projList = result.split("\n")
            self.ui.setProjList(self._projList)
            DEBUG("projects=%s"%self._projList)
    
    def currentProjectName(self):return self._currProject
    def deleteCurrentProject(self):
        messagebox.showerror("Not implemented", "Not implemented: project deletion") # TODO
    def onProjectSelect(self, name):
        self.clearFiles()
        self._currProject = name
        DEBUG("project=%s"%name)
        self.refresh()
        
    def getProjFiles(self, success = False, result = None):
        if not self._isConnected: return
        if not self._currProject: return
        path = TARGET_PROJECTS +"/" + self._currProject
        if result == None:
            self.ssh.command("ls -1 %s"%(path), self.getProjFiles)
            return
        if success:
            self._projFiles.setPath(path)
            for f in result.split("\n"):
                self._projFiles.addFile(f) 
        
    def getConfigFiles(self, success = False, result = None):
        if result == None:
            self.ssh.command("ls -1 %s"%TARGET_CONFIG, self.getConfigFiles)
            return
        if success:
            props = result.split("\n")
            DEBUG("Config files=%s"%props)
            iRes = 0
            for i in range(MAX_PROPERTIES):
                
                try: wgt, tkName, tkVal = self.pbkrProps[i]
                except: pass
                
                try: name = props[iRes]
                except: name = ""
                value=""
                nameState = "disabled"
                wgt.config(state="disabled")
                
                if name.endswith(".sav"):
                    iRes += 1
                    name = name[:-4]
                else: name = ""
                
                if name:
                    nameState ="normal"
                    value="<unknown>"
                    SSHSettingsPropReader(self.ssh, name, self.pbkrProps[i])
                    
                wgt.config(state=nameState)
                tkName.set(name)
                tkVal.set(value)
        else:
            DEBUG("Config files failed=%s"%result)
        
if __name__ == '__main__':
    mgr = _Manager(sys.argv[:])
    mgr.run()
    DEBUG("Program exited")
