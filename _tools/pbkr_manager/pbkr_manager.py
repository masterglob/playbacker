#/bin/python3
python_version = int(str(range(3))[-2])
print("Detected python %d"%python_version)

if python_version == 3:
    import tkinter as tk
    import tkinter.filedialog
    from tkinter import messagebox
    from tkinter.simpledialog import askstring
    from tkinter import ttk
    from idlelib.tooltip import Hovertip
elif python_version == 2:
    raise Exception("This code requires adaptations to run under python2")
    import Tkinter as tk
    import tkFileDialog
import sys, os, re

from pbkr_params import PBKR_Params
from pbkr_ssh import PBKR_SSH, SSHCommander, SSHUploader
from pbkr_utils import DEBUG, P_NoteBook, WavFileChecker, InvalidWavFileFormat, \
    target_path_join, askTitle

###################
# CONFIG
PROMPT_FORM_EXIT = False

###################
# CONSTANTS
TARGET_PATH ="/mnt/mmcblk0p2"
TARGET_PROJECTS = target_path_join (TARGET_PATH, "pbkr.projects")
TARGET_CONFIG = target_path_join (TARGET_PATH, "pbkr.config")
TARGET_TRASH = target_path_join (TARGET_PATH, "pbkr.trash")
WIN_WIDTH=620
WIN_HEIGHT=600
MAX_PROPERTIES = 18
NO_TRACK_ID = 999
PROP_TITLE = "title"
PROP_TRACK = "track"
     
class _ProjectUI():
    def __init__(self, mgr, win):
        self.mgr, self.win = mgr, win
        self._wgt=[]
        self._vars=[]
        self._statBtns=[]
        self._btns=[]
        
        fr = tk.Frame(self.win)
        
        btn = tk.Button (fr, text="Refresh", command = self.reload)
        btn.pack(side = tk.LEFT)
        
        btn = tk.Button (fr, text="Reorder", command = self.reorder)
        btn.pack(side = tk.LEFT)
        self._statBtns.append(btn)
        
        tk.Label(fr, text=" ").pack(side = tk.LEFT)
        
        btn = tk.Button (fr, text="Delete", command = lambda self=self : self.__delete())
        btn.pack(side = tk.LEFT)
        self._statBtns.append(btn)
        
        tk.Label(fr, text=" ").pack(side = tk.LEFT)
        
        self.readOnly = tk.BooleanVar(value = True)
        btn = tk.Checkbutton (fr, text="ReadOnly", variable = self.readOnly, command=self.onReadOnlyChange)
        btn.pack(side = tk.LEFT)
        
        fr.pack(side = tk.TOP)
        
        fr = tk.Frame(self.win)
        self.frame = fr
        # The frame containing songs needs a scrollbar
        self.setupProject()
        
        #self.frame.pack(side = tk.RIGHT, fill = tk.BOTH )
        fr.pack(side = tk.TOP)
        
        
    def clearProject(self):
        for song, var in self._vars :
            del var
        for wgt in self._wgt :
            wgt.destroy()
        self._wgt = []
        self._vars = []
        self._btns = []
        
        
    def setupProject(self, songs={}):
        '''
         @param songs :_PropertiedFileList
        '''
        # 1: delete existing items
        self.clearProject()
        
        self.songs=sorted([song for song in songs.values()], key=lambda s:s.getTrackIdx())
        
        fr0 = tk.Frame(self.frame)
        fr = tk.Frame(fr0)
        
        # 2: reorder songs (! handle multiple identical tracks Id)
        # 3: create new items
        y = 1
        prevSong = None
        for song in self.songs + [""]:
            btn = tk.Button(fr,text="Insert", command = lambda self=self, y=y: self.insertSong(at = y - 1))
            btn.grid(row = y, pady=2)
            self._btns.append(btn)
            y += 1
            
        fr.pack(side = tk.LEFT)
        fr = tk.Frame(fr0)
        y = 1
        prevSong = None
        for song in self.songs:
            var = tk.StringVar(value = song.getTitle())
            tId = "#%02d"%song.getTrackIdx() if song.getTrackIdx() != None else '#??'
            wgt = tk.Label(fr, text = tId, width = 5)
            wgt.grid(row =y, column = 0)
            
            wgt = tk.Entry(fr, textvariable = var, width = 35, state="readonly")
            wgt.grid(row =y, column = 1, padx=2, pady=2)
            myTip = Hovertip(wgt,'%s'%song.name)
            
            btn = tk.Button(fr,text="Edit title", command = lambda self=self, song=song : self.onTitleEdit(song))
            btn.grid(row = y, column = 2, padx=2, pady=2)
            self._btns.append(btn)
            
            if prevSong:
                cmd = lambda self=self, song=song, prevSong=prevSong: self.onMove(song, prevSong)
                btn = tk.Button(fr,text="Move Up", command = cmd)
                btn.grid(row = y, column = 3, padx=2, pady=2)
                self._btns.append(btn)   
                
            btn = tk.Button(fr,text="Delete", command = lambda self=self, song=song : self.onDeleteSong(song))
            btn.grid(row = y, column = 4, padx=2, pady=2)
            self._btns.append(btn)
                     
            self._vars.append((song,var))
            y += 1
            
            prevSong = song
        
        self.onReadOnlyChange()
        fr.pack(side = tk.RIGHT)
        fr0.pack()
        self._wgt.append(fr0)
        
    def onReadOnlyChange(self):
        state = "disabled" if self.readOnly.get() else "normal"
        for btn in self._btns + self._statBtns:
            btn.config(state=state)
       
    def onDeleteSong(self, song):
        if self.readOnly.get():return 
        SSHFileDeleter(self.mgr, song.fullPathName, self.mgr.currentProjectName())
            
    def insertSong(self, at):
        class InvalidWavFile(Exception):pass
        
        if self.readOnly.get():return 
        try:
            projName = self.mgr.currentProjectName()
            if not projName : return
            fullfilename = tk.filedialog.askopenfilename(parent=self.win,
                                                     initialdir= self.mgr.param_LastOpenPath.get(),
                                                     title= "Select file to insert in project %s"%projName,
                                                     filetypes= (('WAV files', '*.wav'),('All files', '*.*'), )
                                                     )
            if not fullfilename: return
            srcDir, filename = os.path.split(os.path.abspath(fullfilename))
            self.mgr.param_LastOpenPath.set(srcDir)
            
            # Check that File format is correct
            try:WavFileChecker( os.path.join(srcDir, filename))
            except InvalidWavFileFormat as e:
                raise InvalidWavFile(str(e))
            
            # Check for invalid chars
            if not re.match(r"^[A-Z0-9_+-]+[.]wav$", filename, flags=re.IGNORECASE):
                raise InvalidWavFile("Invalid file name. Please use only basic characters and '.wav' extension.")
            
            # ensure that there is not already a file with same name
            for song in self.songs:
                if song.name == filename:
                    raise InvalidWavFile("There is already one song with that filename (#%d : %s)"%
                                         (song.getTrackIdx(), song.getTitle()))
        
            # Ask for song name
            title = askTitle(self.win, filename, filename.split(".")[0])
            if not title:
                raise InvalidWavFile("Cancelled")
            
            DEBUG("Valid title <%s>"%title)
            
            # Create song object
            newSong = _PropertiedFile(target_path_join(TARGET_PROJECTS, projName), filename)
            self.songs.insert(at, newSong)
            # To create track Id, we may have to increments next songs ids...
            try: expTrackId = self.songs[at - 1].getTrackIdx() + 1
            except:  expTrackId = 1
            if expTrackId >= NO_TRACK_ID : expTrackId = 1
            DEBUG("Start at pos=%d, with id=%d"%(at,expTrackId))
            
            newSong.setTrackIdx(self.mgr, expTrackId)
            
            for song in self.songs[at + 1:]:
                currTrackId = song.getTrackIdx()
                if expTrackId > currTrackId:
                    DEBUG("Change track Id from %d to %d for %s"%(currTrackId,expTrackId, song.name))
                    song.setTrackIdx(self.mgr, expTrackId)
                
                expTrackId += 1
                
            newSong.setTitle(self.mgr, title)
            
            # inactivate screen...
            self.readOnly.set(True)
            self.clearProject()
            var = tk.StringVar(value = "Uploading in progress...")
            fr0 = tk.Frame(self.frame)
            
            tk.Label(fr0, textvariable=var, fg="#A01010",font=("Arial", 25)).pack()
            
            fr0.pack()
            self._wgt = [fr0]
            self._vars = [(None,var)]
            
            
            # Install file
            dstName = target_path_join(TARGET_PROJECTS, projName, filename) 
            SSHUploader(self.mgr.ssh, fullfilename, dstName, event = self.__insertSongDone)
                                
        except InvalidWavFile as e:
            messagebox.showerror("Installation failed", str(e))

    def __insertSongDone(self, success, result):
        DEBUG("__insertSongDone")
        if not success: 
            messagebox.showerror("Installation failed", "Copy failed : %s"%result)
        self.reload()
        self.readOnly.set(False)
        
    def reload(self):
        self.mgr.ui.onProjectSelect()
        
    def refresh(self, propName = ""):
        if propName == PROP_TRACK:
            self.mgr.refresh(False)
        else:
            y = 1
            for song, var in self._vars :
                if song:
                    var.set(value = song.getTitle())
            
    def onMove(self, song, prevSong):
        if self.readOnly.get():return 
        idx1 = song.getTrackIdx()
        idx2 = prevSong.getTrackIdx()
        song.setTrackIdx(self.mgr, idx2)
        prevSong.setTrackIdx(self.mgr, idx1)
        self.mgr.checkRefreshStatus()
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
        title = askTitle(self.win, song.name, song.getTitle())
        if title:
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
        fr0= tk.LabelFrame(self.win, text = "Connection",width=WIN_WIDTH/2, height=150)
        fr= tk.Frame(fr0)
        tk.Label(fr, text="Select PBKR device").pack(side = tk.LEFT)
        
        self.__param_RemoteIp = params.createStr("remote_ip")
        w = tk.Entry(fr, textvariable = self.__param_RemoteIp, width = 20, state="normal")
        w.pack(side = tk.LEFT, expand = True, fill = tk.X)
        self.__entryIp = w
        
        tk.Label(fr, text=" port ").pack(side = tk.LEFT)
        
        self.__param_RemotePort = params.createInt("remote_port", 22)
        w = tk.Entry(fr, textvariable = self.__param_RemotePort, width = 6, state="normal")
        w.pack(side = tk.LEFT, expand = True)
        self.__entryPort = w
                
        tk.Label(fr, text=" User:").pack(side = tk.LEFT)
        self.__param_targetUserName = params.createStr("targetUserName", value="tc")
        w = tk.Entry(fr, textvariable = self.__param_targetUserName, width = 6, state="normal")
        w.pack(side = tk.LEFT, expand = True, fill = tk.X)
        self.__entryUserName = w
        
        tk.Label(fr, text=" Pass:").pack(side = tk.LEFT)
        self.__param_targetPassword = params.createStr("targetPassword")
        w = tk.Entry(fr, textvariable = self.__param_targetPassword, width = 14, state="normal", show="*")
        w.pack(side = tk.LEFT, expand = True)
        self.__entryPassword = w
        fr.pack(side = tk.TOP, fill = tk.X)
        
        fr= tk.Frame(fr0)        
        w = tk.Button(fr,text="Connect", command = self.__cbConn , width = 10)
        w.pack(side = tk.LEFT)
        self.__btnConnect = w
        w = tk.Button(fr,text="Disconnect", command = self.__cbDisconn , width = 10)
        w.pack(side = tk.LEFT)
        self.__btnDisconnect = w
            
        fr.pack(side = tk.TOP, fill = tk.X)
        fr0.pack(side = tk.TOP, fill = tk.X)
        
        #############################
        # TABS
        tabs0W ,tabs0H = WIN_WIDTH-10, WIN_HEIGHT -150
        with P_NoteBook(self.win, label ="Connection") as tabs0:
             
            #############################
            # TAB 1 : projects
            tab1 = tabs0.addTab('Projects')
            fr= tk.Frame(tab1)
            w = tk.Button(fr,text="Refresh List", command = self.mgr.getProjList , width = 10)
            w.pack(side = tk.LEFT, expand = False)
            self.__btnGetList = w
             
            w = tk.Button(fr,text="Debug", command = self.mgr.debug , width = 10)
            w.pack(side = tk.LEFT, expand = False)
            self.__btnDebug = w             
             
            w = ttk.Combobox(fr, values=[], state="readonly", width = 40)
            w.pack(side = tk.LEFT, expand = True, fill=tk.X)
            w.bind("<<ComboboxSelected>>", self.onProjectSelect)
            self.__cbbProjList = w
            fr.pack(side = tk.TOP, expand = False, fill=tk.X)
            
            fr= tk.Frame(tab1)
            self.projectFrame = tk.LabelFrame(fr,text = "Active project", width=tabs0W - 100, height=tabs0H -60)
            self.project = _ProjectUI(mgr, self.projectFrame)
            self.projectFrame.pack(side = tk.TOP, expand = True, fill=tk.BOTH)
            fr.pack(side = tk.TOP, expand = True, fill=tk.BOTH)
            
            #############################
            # TAB 2 : config
            tab2 = tabs0.addTab('Config')
            for i in range(MAX_PROPERTIES):
                fr= tk.Frame(tab2)
                tkVarName = tk.StringVar(value = "")
                tkVarVal = tk.StringVar(value = "")
                w = tk.Entry(fr, textvariable=tkVarName , width = 27, state="readonly")
                w.pack(side = tk.LEFT, expand = True, fill=tk.X)
                w = tk.Entry(fr, textvariable=tkVarVal , width = 39, state="readonly")
                w.pack(side = tk.LEFT, expand = True, fill=tk.X)
                self.mgr.pbkrProps[i] = (w, tkVarName, tkVarVal)
                fr.pack(side = tk.TOP, expand = True, fill=tk.X)
                
        ##################
        # Status bar
        fr= tk.LabelFrame(self.win, text = "")
        self.statusBar = tk.StringVar(value = "")
        l = tk.Label(fr, textvariable = self.statusBar)
        l.pack(side = tk.LEFT, expand = True, fill = tk.X)
        
        
        bExit = tk.Button(win, text="Exit", command = mgr.on_closing)
        bExit.pack(side = tk.RIGHT)
        fr.pack(side = tk.TOP, expand = False, fill = tk.X)
        
    def __cbConn(self):
        self.mgr.ssh.connect(self.__param_RemoteIp.get(), 
                                self.__param_RemotePort.get(), 
                                self.__param_targetUserName.get(),
                                self.__param_targetPassword.get())
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
        disconnectedButtons=[self.__btnConnect, self.__entryIp, self.__entryPort, 
                             self.__entryUserName, self.__entryPassword]
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
        self.__cbbProjList.config(state="normal")
        self.__cbbProjList.delete(0, "end")
        self.__cbbProjList["values"] = l[:]
        if l :
            self.__cbbProjList.current(0)
            self.__cbbProjList.config(state="readonly")
        else:
            self.__cbbProjList.config(state="disabled")
        self.onProjectSelect()
            
    def onProjectSelect(self, event = None):
        curr = self.__cbbProjList.current()
        if curr == 0:
            pass # <None>
        if curr >= 1:
            name = self.__cbbProjList.get()
        else: name = None
        self.mgr.onProjectSelect(name)
       
class SSHFileDeleter(SSHCommander):
    def __init__(self, mgr, filename, title, confirm = True):
        self.filename = filename
        self.mgr = mgr
        paths=filename.split("/")
        name = paths[-1]
        tPath = "/".join(paths[:-1])
        trashName = target_path_join(TARGET_TRASH,"")
        cmd = "find '%s' -name '%s*' -exec mv {} '%s' \;"%(tPath, name, trashName)
        if confirm:
            if not tk.messagebox.askokcancel("Delete file %s"%name, 
                                             "Confirm deletion of file %s from project %s"%
                                             (name,title)):
                return 
        #DEBUG ("cmd=<%s>"%cmd)
        SSHCommander.__init__(self, mgr.ssh, cmd, self.event)
    def event(self, result):
        self.mgr.ui.project.reload()
   
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
        #DEBUG("File size:%s = %d Kb"%(self.filename,sizeKb))
        self.file.setSizeKb (sizeKb)
       
class SSHFilePropReader(SSHCommander):
    def __init__(self, mgr, filename, file, propName):
        self.mgr = mgr
        self.propName = propName
        self.filename = filename
        self.file = file
        cmd = "cat %s.%s"%(filename,propName)
        #DEBUG("command is `%s`"%cmd)
        SSHCommander.__init__(self, mgr.ssh, cmd, self.event)
    def event(self, result):
        self.file.props[self.propName] = str(result)
        self.mgr.ui.project.refresh(self.propName)
        #DEBUG ("%s of %s is %s"%(self.propName,self.filename,result))

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
        except: return NO_TRACK_ID
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
        
        self.__norefresh = False
        self._projList=[]
        self._projFiles =_PropertiedFileList(self)
        self._currProject = None
        self._isConnected = False
        self.pbkrProps=[(None, None, None) for _ in range(MAX_PROPERTIES)]
        
        win = tk.Tk()
        win.protocol("WM_DELETE_WINDOW", self.on_closing)
        win.title("Playbacker MANAGER tool")
        win.minsize(WIN_WIDTH, WIN_HEIGHT)
        # win.resizable(False, False)   
        self.win = win       
        
        self._initpath=os.path.abspath (os.path.dirname(args[0]))
        self._args = args
        self._params = PBKR_Params (self._initpath, '.pbkmgr.json')
        self.ui = _UI(self, win, self._params)          
        
        self.__param_NbExec = self._params.createInt("nb_exec")
        self.__param_NbExec.set(self.__param_NbExec.get() + 1)
        
        self.param_LastOpenPath = self._params.createStr("lastOpenPath", value = os.getcwd())
        
        self.refresh(True)
        self.ssh.start()
        
    def quit(self):
        self.ssh.quit()
        self._params.saveToFile()
        self.win.destroy()
        
    def refresh(self, reload= True):
        if reload:
            self.ui.project.setupProject()
            self.getProjFiles()
        if not self.__norefresh:
            self.ui.project.setupProject(self._projFiles.getFiles())
        
    def on_closing(self):
        DEBUG ("on_closing")
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
            self._projList = ("<NONE>\n%s"%result).split("\n")
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
        self.__norefresh = True
        if result == None:
            self._projFiles.clear() 
            self.ssh.command("ls -1 %s"%(path), self.getProjFiles)
            return
        if success:
            self._projFiles.setPath(path)
            for f in result.split("\n"):
                self._projFiles.addFile(f) 
                print("found file %s"%f)
            # Eventually append an empty event to trigger refresh on completion
            self.checkRefreshStatus()
    
    def checkRefreshStatus(self, success = False, result = None):
        if not self._isConnected: return
        if not self._currProject: return
        if result == None:
            self.ssh.command("echo", self.checkRefreshStatus)
            return
        else:
            self.__norefresh = False
            self.refresh(False)
        
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
