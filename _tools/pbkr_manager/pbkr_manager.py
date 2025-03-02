#/bin/python3
python_version = int(str(range(3))[-2])
print("Detected python %d"%python_version)

if python_version == 3:
    import tkinter as tk
    import tkinter.filedialog
    from tkinter import messagebox
    from tkinter.simpledialog import askstring
    from tkinter import ttk
    try:from idlelib.tooltip import Hovertip
    except:pass # for python 3.6
    import socket
elif python_version == 2:
    raise Exception("This code requires adaptations to run under python2")
    import Tkinter as tk
    import tkFileDialog
    
import sys, os, re, time
from threading import Thread
from tools_src.pbkr_params import PBKR_Params
from tools_src.pbkr_ssh import PBKR_SSH, SSHCommander, SSHUploader, SSHDownloader
from tools_src.pbkr_utils import DEBUG, P_NoteBook, WavFileChecker, InvalidWavFileFormat, \
    target_path_join, askTitle

###################
# CONFIG
PROMPT_FORM_EXIT = False

###################
# CONSTANTS
TARGET_PATH ="/mnt/mmcblk0p2"
TARGET_WWW ="/home/www"
TARGET_WWW_RES =  target_path_join (TARGET_WWW, "res")
TARGET_WWW_CMD =  target_path_join (TARGET_WWW, "cmd")
TARGET_PROJECTS = target_path_join (TARGET_PATH, "pbkr.projects")
TARGET_CONFIG = target_path_join (TARGET_PATH, "pbkr.config")
TARGET_TRASH = target_path_join (TARGET_PATH, "pbkr.trash","")
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
        self._btns=[]
        
        self.readOnly = tk.BooleanVar(value = True)
        fr = tk.Frame(self.win)
        self.frame = fr
        # TODO : The frame containing songs needs a scrollbar
        
        fr.pack(side = tk.TOP)
        
        self.makeInactive("No project loaded")
        
    def clearProject(self):
        for song, var in self._vars :
            del var
        for wgt in self._wgt :
            wgt.destroy()
        self._wgt = []
        self._vars = []
        self._btns = []
        self.songs = []
        
    def makeInactive(self, message, fg = "#E0A0E0"):
        self.readOnly.set(True)
        self.clearProject()
        var = tk.StringVar(value = message)
        fr0 = tk.Frame(self.frame)
        
        tk.Label(fr0, textvariable=var, fg="#A01010",font=("Arial", 25)).pack()
        
        fr0.pack()
        self._wgt = [fr0]
        self._vars = [(None,var)]
        
        
    def setupProject(self, songs={}):
        '''
         @param songs :_PropertiedFileList
        '''
        # 1: delete existing items
        self.clearProject()
        
        
        fr0 = tk.Frame(self.frame)
        fr = tk.Frame(fr0)
        
        btn = tk.Button (fr, text="Refresh", command = self.reload)
        btn.pack(side = tk.LEFT)
        
        btn = tk.Button (fr, text="Backup", command = self.backup)
        btn.pack(side = tk.LEFT)
        
        btn = tk.Button (fr, text="Reorder", command = self.reorder)
        btn.pack(side = tk.LEFT)
        self._btns.append(btn)
        
        tk.Label(fr, text=" ").pack(side = tk.LEFT)
        
        btn = tk.Button (fr, text="Delete", command = lambda self=self : self.__delete())
        btn.pack(side = tk.LEFT)
        self._btns.append(btn)
        
        tk.Label(fr, text=" ").pack(side = tk.LEFT)
        
        btn = tk.Checkbutton (fr, text="ReadOnly", variable = self.readOnly, command=self.onReadOnlyChange)
        btn.pack(side = tk.LEFT)
        
        fr.pack(side = tk.TOP)
        
        
        self.songs=sorted([song for song in songs.values()], key=lambda s:s.getTrackIdx())
        
        fr = tk.Frame(fr0)
        
        if self.songs:
            y = 1
            for song in self.songs + [""]:
                btn = tk.Button(fr,text="Insert", command = lambda self=self, y=y: self.insertSong(at = y - 1))
                btn.grid(row = y, pady=2)
                self._btns.append(btn)
                y += 1
        else:
            btn = tk.Button(fr,text="Upload a first playback file :)",
                             command = lambda self=self: self.insertSong(at = 0))
            btn.pack()
            self._btns.append(btn)
            
            
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
            try: Hovertip(wgt,'%s'%song.name)
            except:pass
            
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
        for btn in self._btns:
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
            
            sizeKb = os.stat(fullfilename).st_size / 1024
            if sizeKb > 1024*1024:
                raise InvalidWavFile("File too large (limit is 1Gb)")
            
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
            
            self.makeInactive("Uploading in progress...")
            
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
    
    def backup(self):
        name = self.mgr.currentProjectName()
        path = tk.filedialog.askdirectory(parent=self.win,
                                         # initialdir= self.mgr.param_LastOpenPath.get(),
                                         title= "Select folder for <%s> backup"%name,
                                         )
        if not path: return
        self.makeInactive("Backup download in progress...")
        self.mgr.backupCurrentProject(path)
    
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

class Keyboard:
    def nameof(c):
        if c==' ': return "Space"
        if c=='!': return "Excl"
        if c=='/': return "Slash"
        if c==",": return "Comma"
        if c==";": return "SemiColon"
        if c==":": return "Colon"
        if c=="\\": return "BSlash"
        if ord(c) == 8 : return "DEL" 
        if ord(c) == 13 : return "Enter" 
        if c >= 'a' and c<="z" : return c.upper()
        if c >= 'A' and c<="Z" : return c.upper()
        if c >= '0' and c<="9" : return c
        return ""
    def __init__(self, osc, win):
        self.osc=osc
        BTN_W=1
        BTN_H=2
        SPC=5
        
        fr  = tk.Frame(win)
        self.varFB = tk.StringVar()
        self.wgtFB = tk.Entry(fr, textvariable=self.varFB, width = 60,
                 # anchor  = "w", 
                 bg="#1010EF", fg= "#F0F0EF", bd=1,
                 font=("Courier new", 10))
        _cmd = self.wgtFB.register(self.validateFB)
        self.wgtFB.configure(validatecommand=(_cmd, '%P'), validate='key')
        self.wgtFB.grid(row=1, column=1)
        self.wgtFB.bind("<Key>", self.simulateFB)                 
        fr.pack(side = tk.TOP)
        
        fr  = tk.Frame(win)
        maxX=0
        keybLines=["1234567890","AZERTYUIOP", "QSDFGHJKLM", "WXCVBN/,:!"]
        
        y=1
        for line in keybLines:
            x=y
            b=tk.Button(fr, text="", height=BTN_H, width=BTN_W*y, state=tk.DISABLED,
                        highlightthickness = 0, bd = 0)
            b.grid(row = y, column=0, columnspan=x)
            for c in line:
                tk.Button(fr, text=c, height=BTN_H, width=BTN_W*SPC,
                          command = lambda self=self,c=c: self.sendKey(Keyboard.nameof(c))
                          ).grid(row = y, column=x, columnspan=SPC)
                maxX = max(maxX,x)
                x+=SPC
            y+=1
            
        # DEL Key
        tk.Button(fr, text="DEL", height=BTN_H, width=BTN_W*SPC*2,
                          command = lambda self=self,c=c: self.sendKey("DEL")
                  ).grid(row = 1, column=maxX+SPC, rowspan=1)
        # Enter Key
        tk.Button(fr, text="ENTER", height=BTN_H*2, width=BTN_W*SPC*2,
                          command = lambda self=self,c=c: self.sendKey("Enter")
                  ).grid(row = 2, column=maxX+SPC, columnspan=SPC*3, rowspan=2)
        # Space bar              
        tk.Button(fr, text=" ", height=BTN_H, width=BTN_W*SPC*10,
                          command = lambda self=self,c=c: self.sendKey("Space")
                  ).grid(row = y, column=y-(SPC-1), columnspan=SPC*10)
            
        fr.pack(side = tk.TOP)
        # Feedback lines
        
        fr  = tk.Frame(win)

        x=1
        y=1
        self.kbdLines=[]
        for i in range(5):
            y+=1
            var = tk.StringVar(value="")
            self.kbdLines.append(var)
            
            tk.Label(fr, textvariable=var, width = 60,
                     anchor  = "w", 
                     bg="#1010FF", fg= "#F0F0FF", bd=1,
                      font=("Courier new", 10)).grid(row=y, column=x)
        
        fr.pack(side = tk.BOTTOM)
    
    def validateFB(self,valeur):return False
    def simulateFB(self, event): 
        try: v = Keyboard.nameof(event.char)
        except Exception as E:
            return
        if v:
            self.sendKey(v)
        
    def sendKey(self, key):
        msg = OSC.build(OSC.OSC_KBD, [key], 1)
        self.osc.send(msg)
    def updateFB(self, text):
        self.varFB.set(text)    
    def updateOutput(self, i, text:str):
        try:
            self.kbdLines[i].set(text)
        except:pass    
         
               
class OSC (Thread):
    OSC_PAGE = b"/pbkrctrl/"
    OSC_KBD  = b"/pbkrkbd/"
    OSC_MENU = b"/pbkrmenu/"
    def __init__(self, mgr, win):
        Thread.__init__(self)
        self.mgr = mgr
        fr  = tk.Frame(win)
        
        fr2  = tk.LabelFrame(fr, text= "OSC connection")
        self.varIP = tk.StringVar(value="192.168.7.80")
        tk.Label(fr2, text="Address").grid(row=1, column=1)
        self.wgt_IP= tk.Entry(fr2, textvariable=self.varIP, width = 16, font=("Courier new", 10))
        self.wgt_IP.grid(row=1, column=2)
        tk.Label(fr2, text="Status:").grid(row=2, column=1)
        self.varStatus = tk.StringVar(value="")
        self.wgt_Status= tk.Label(fr2, textvariable=self.varStatus, fg='red')
        self.wgt_Status.grid(row=2, column=2)
        
        self.varPortIn = tk.IntVar(value=8000)
        self.varPortOut = tk.IntVar(value=9000)
        tk.Label(fr2, text="Port In").grid(row=1, column=3)
        self.wgt_portIn = tk.Entry(fr2, textvariable=self.varPortIn, width = 8, font=("Courier new", 10))
        self.wgt_portIn.grid(row=1, column=4)
        tk.Label(fr2, text="Port Out").grid(row=2, column=3)
        self.wgt_portOut=tk.Entry(fr2, textvariable=self.varPortOut, width = 8, font=("Courier new", 10))
        self.wgt_portOut.grid(row=2, column=4)
        
        self.btnCnx = tk.Button(fr2, text="Connect", command=self._connect, width = 12)
        self.btnCnx.grid(row=1, column=5)
        self.btnDiscnx = tk.Button(fr2, text="Disconnect", command=self._disconnect, width = 12)
        self.btnDiscnx.grid(row=2, column=5)
        fr2.pack(side = tk.TOP)
        
        #fr2  = tk.LabelFrame(fr, text=  "OSC connection")
        #fr2.pack(side = tk.TOP)

        fr.pack(side = tk.TOP)
        
        # Keyboard
        fr  = tk.LabelFrame(win)
        self.kbd = Keyboard(self, fr)
        fr.pack(side = tk.TOP)
        
        self._refState=None
        self._disconnect()
        self.refresh()
        self.start()
        
    @staticmethod
    def build(section, path :[], value):
        msg1 = section + b"/".join([v.encode("utf-8") for v in path])
        # complete path
        msg2 = b"\0" * (4- (len(msg1) % 4))
        
        if isinstance(value, int):
            msg3=b",i\0\0"
            msg4=value.to_bytes(4, byteorder='big')
        elif isinstance(value, float):
            msg3=b",f\0\0"
            msg4=value.to_bytes(4, byteorder='big')
        else:
            msg3=b",\0\0\0"
            msg4=b""


        return msg1+msg2+msg3+msg4
        
    def refresh(self):
        
        discState=[tk.NORMAL, tk.NORMAL, tk.DISABLED]
        connState=[tk.DISABLED, tk.DISABLED, tk.NORMAL]
        statusColor=["#2020C0", "#30A020", "red"]
        statusName=["Connecting...", "Connected", "Disconnected"]
        prevState= self._refState
        if self.connecting:
            self._refState = 0
        elif self.connection:
            self._refState = 1
        else:
            self._refState = 2
         
        if prevState != self._refState:
            prevState = self._refState
            self.btnDiscnx.config(state=discState[prevState])
            self.btnCnx.config(state=connState[prevState])
            self.wgt_IP.config(state=connState[prevState])
            self.wgt_portOut.config(state=connState[prevState])
            self.wgt_portIn.config(state=connState[prevState])
            self.wgt_Status.config(fg=statusColor[prevState])
            self.varStatus.set(statusName[prevState])
        
            
    def _connect(self):
        self.connecting=(self.varIP.get(), self.varPortIn.get(), self.varPortOut.get())
        self.refresh()
        
    def _disconnect(self):
        self.toSend=[]
        self.connecting=None
        self.connection=None
        self.refresh()
        self.kbd.updateFB("")
    
    def run(self):
        sOut=None
        sIn=None
        while not self.mgr.quitting:
            if sIn == None and self.connecting:
                try:
                    ip, pIn, pOut = self.connecting
                    addrIn=("", pOut)
                    addrOut=(ip,pIn)
                    sIn = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                    sOut = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                    sIn.settimeout(0.01)
                    print("Bind socket on %s:%d"%(addrIn))
                    sIn.bind(addrIn)
                except Exception as E:
                    print (E)
                    print("Failed to bind socket on %s:%d"%(addrIn))
                    sIn,sOut = None,None
                    self.connecting = None
                    
            if sOut != None and self.connecting:
                try:
                    # print("send '/ping' to %s:%d"%(addrOut))
                    sOut.sendto(b"/ping", addrOut)
                except Exception as E:
                    print (E)
                    print("Failed to send socket on %s:%d"%(addrOut))
                
            if sOut:
                while self.toSend:
                    msg=self.toSend.pop(0)
                    try:
                        sOut.sendto(msg, addrOut)
                    except:    
                        print ("Failed to send msg [%s]", msg)
                    
            if sIn != None:
                while True:
                    try:
                        data, address = sIn.recvfrom(4096)
                    except Exception as E:
                        break
                    if data:
                        if self.connecting:
                            self.connecting=None
                            self.connection=(sIn,sOut)
                        self._onMsgRecv(data)
                    
            if sIn != None and not self.connection and not self.connecting:
                sIn.close()
                sIn = None
                sOut.close()
                sOut=None
                self._disconnect()
                print("Close socket")
                    
            self.refresh()
            time.sleep(0.1)
     
    def send(self, msg):
        if self.connection:
            self.toSend.append(msg)
            
    def _onMsgRecv(self,data):
        current=""
        path=None
        params=[]
        try:
            for c in data:
                if c != 0:
                    current+="%c"%c
                elif current:
                    if path == None:
                        path=current
                    else:
                        params.append(current)
                    current=""
        except:
            print ("Decoding failed!")
            return
        l=len(params)
        if params:
            if params[0] == ",s" and l >= 2:
                self.mgr.onOSC(path=path, param=str(params[1]))
        
    def onOSC(self, path:[], param):
        if not path : return
        cmd=path[0]
        if cmd == 'InputFB':
            self.kbd.updateFB(param)
        if cmd == 'Output1':
            self.kbd.updateOutput(0, param)
        if cmd == 'Output2':
            self.kbd.updateOutput(1, param)
        if cmd == 'Output3':
            self.kbd.updateOutput(2, param)
        if cmd == 'Output4':
            self.kbd.updateOutput(3, param)
        if cmd == 'Output5':
            self.kbd.updateOutput(4, param)
        
class RemoteControl (Thread):
    def __init__(self, mgr, win):
        Thread.__init__(self)
        self.mgr, self.win = mgr, win
        BTN_W=5
        BTN_H=3
        
        fr0  = tk.Frame(win)
        fr  = tk.Frame(fr0)
        
        self.menul1= tk.StringVar(value="Menu L1")
        self.menul2= tk.StringVar(value="Menu L2")
        tk.Label(fr, textvariable=self.menul1, width = 16,
                 anchor  = "w", 
                 bg="#1010FF", fg= "#F0F0FF", bd=1,
                  font=("Courier new", 14)).grid(row=1, column=1)
        tk.Label(fr, text=" ", width=1).grid(row=1, column=2)
        tk.Label(fr, textvariable=self.menul2, width = 16,
                 anchor  = "w", 
                 bg="#1010FF", fg= "#F0F0FF", bd=1,
                  font=("Courier new", 14)).grid(row=2, column=1)
        tk.Label(fr, text=" ", width=1).grid(row=2, column=2)
        
        fr.pack(side = tk.RIGHT)
        
        fr  = tk.Frame(fr0)
        tk.Button(fr, text="OK", height=BTN_H, width=BTN_W,
                  command = lambda self=self: self.sendControl("pOk")
                  ).grid(row = 1, column=1)
        tk.Label(fr, text=" ", width=1).grid(row=1, column=2)
        tk.Button(fr, text="Cancel", height=BTN_H, width=BTN_W,
                  command = lambda self=self: self.sendControl("pClose")
                  ).grid(row = 1, column=3)
        tk.Label(fr, text=" ", width=1).grid(row=1, column=4)
        fr.pack(side = tk.LEFT)
        
        fr  = tk.Frame(fr0)
        tk.Button(fr, text="Left", height=BTN_H, width=BTN_W,
                  command = lambda self=self: self.sendControl("pLeft")
                  ).grid(row = 2, column=1, columnspan=2)
        tk.Button(fr, text="Right", height=BTN_H, width=BTN_W,
                  command = lambda self=self: self.sendControl("pRight")
                  ).grid(row = 2, column=3, columnspan=2)
        tk.Button(fr, text="Up", height=BTN_H, width=BTN_W,
                  command = lambda self=self: self.sendControl("pUp")
                  ).grid(row = 1, column=2, columnspan=2)
        tk.Button(fr, text="Down", height=BTN_H, width=BTN_W,
                  command = lambda self=self: self.sendControl("pDown")
                  ).grid(row = 3, column=2, columnspan=2)
        fr.pack(side = tk.LEFT)
        fr0.pack(side = tk.TOP)
        
        self.start()

    def sendControl(self, cmd):
        if self.mgr.ssh.isConnected() :
            cmd = "echo -n '%s' > '%s'"%(cmd, TARGET_WWW_CMD)
            SSHCommander(self.mgr.ssh, cmd, None)
        
        
    def onUpdate(self, var, value):
        # convert displayable chars
        value = value.replace("%c"%0x7F, "<")
        value = value.replace("%c"%0x7E, ">")
        if var.get() != value:
            var.set(value)
            print (" ".join(["%02X"%ord(c) for c in value]))
             
        
    def run(self):
        wgtMap=[
            ("lMenuL1", self.menul1),
            ("lMenuL2", self.menul2),
            ]
        while not self.mgr.quitting:
            for name, var in wgtMap:
                cmd = "cat %s/%s"%(TARGET_WWW_RES, name)
                if self.mgr.ssh.isConnected() :
                    SSHCommander(self.mgr.ssh, cmd, lambda result,self=self, var=var :self.onUpdate(var, result))
                
                time.sleep(0.5)
            
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


class _UI():
    def __init__(self, mgr, win, params):
        self.win = win
        self.mgr : _Manager = mgr
            
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
#              
#             w = tk.Button(fr,text="Debug", command = self.mgr.debug , width = 10)
#             w.pack(side = tk.LEFT, expand = False)
#             self.__btnDebug = w    

            tk.Label(fr, text=" ").pack(side = tk.LEFT)
            
            w = tk.Button(fr,text="Create", command = self.mgr.createNewProject , width = 8)
            w.pack(side = tk.LEFT, expand = False)
            self.__btnDebug = w  
            
            tk.Label(fr, text=" ").pack(side = tk.LEFT)
            w = tk.Button(fr,text="Rename", command = self.mgr.renameProject , width = 8)
            w.pack(side = tk.LEFT, expand = False)
            self.__btnRenameProj = w  
            
            tk.Label(fr, text=" ").pack(side = tk.LEFT)
                      
             
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
                
            #############################
            # TAB 3 : Remote control
            tab3 = tabs0.addTab('Remote Control')
            fr= tk.Frame(tab3)
            self.remoteControl = RemoteControl(mgr, fr)
            fr.pack(side = tk.TOP, expand = True, fill=tk.BOTH)
            
            #############################
            # TAB 4 :  OSC
            tab4 = tabs0.addTab('OSC')
            fr= tk.Frame(tab4)
            self.kbd = OSC(mgr, fr)
            fr.pack(side = tk.TOP, expand = True, fill=tk.BOTH)
            
            
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
        if not self.__param_RemoteIp.get(): return
        self.mgr.ssh.connect(self.__param_RemoteIp.get(), 
                                self.__param_RemotePort.get(), 
                                self.__param_targetUserName.get(),
                                self.__param_targetPassword.get())
#         self.__btnConnect.config(state="disabled")
    def __cbDisconn(self):
        self.setProjList([])
        self.mgr.ssh.disconnect()
        
    def setStatus(self, msg):
        if msg:
            self.statusBar.set(msg)
        connectedButtons=[self.__btnDisconnect, self.__btnGetList, self.__btnDebug, self.__btnRenameProj]
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
        cmd = "find '%s' -name '%s*' -exec mv -f {} '%s' \;"%(tPath, name, TARGET_TRASH)
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
        if not filename: return
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
        self.quitting = False
        self.__currentBackup=None
        
        win = tk.Tk()
        win.protocol("WM_DELETE_WINDOW", self.on_closing)
        win.title("Playbacker MANAGER tool")
        win.minsize(WIN_WIDTH, WIN_HEIGHT)
        # win.resizable(False, False)   
        self.win = win       
        win.iconbitmap(default='images/PBKR_MGR.ico')
        
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
        self.quitting = True
        self.ssh.quit()
        self._params.saveToFile()
        self.win.destroy()
        
    def refresh(self, reload= True):
        if reload:
            self.ui.project.clearProject()
            self.getProjFiles()
        if not self.__norefresh:
            if self._currProject:
                self.ui.project.setupProject(self._projFiles.getFiles())
            else:
                self.ui.project.makeInactive("No project loaded")
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
            
    def createNewProject(self):
        if not self._isConnected: return
        
        name = askstring(parent = self.win,
                          title = 'Define project name',
                          prompt = 'Enter new project name',
                          initialvalue = "").strip()
        if not name : return
        if not re.match(r"^['A-Z0-9_+-]+$", name, flags=re.IGNORECASE): 
            messagebox.showerror("Bad project name", "Do not use special chars or spaces in name")
            return
        name = name.replace ("'", "\\'")
        
        # Check if project already exists
        if name in self._projList:
            messagebox.showerror("Bad project name", "A project with that name already exists")
            return
        
        DEBUG ("Creating project <%s>"%name)
        
        fullname = target_path_join(TARGET_PROJECTS, name)
        SSHCommander(self.ssh, "mkdir -p '%s'"%fullname, 
                     lambda result, self=self : self.getProjList() )
        
    # WIP
        
    def renameProject(self):
        
        currProj = self.currentProjectName()
        if not self._isConnected or not currProj: return
        
        name = askstring(parent = self.win,
                          title = f"Rename project '{currProj}'",
                          prompt =f" 'Enter new project name for '{currProj}'",
                          initialvalue = currProj).strip()
                          
        if not name: return
        
        # Ensure projet does not already exist!
        if name in self._projList:
            messagebox.showerror("Bad project name", "A project with that name already exists")
            return
        
        DEBUG (f"Renaming project {currProj} to '{name}'")
        
        initName = target_path_join(TARGET_PROJECTS, currProj)
        fullname = target_path_join(TARGET_PROJECTS, name)
        SSHCommander(self.ssh, f"mv {initName} {fullname}", 
                     lambda result, self=self : self.getProjList() )
        
        
    def currentProjectName(self):return self._currProject
    
    def deleteCurrentProject(self, success = False, result = None):
        if result == None:
            if not self._isConnected: return
            if not self._currProject: return
            path = TARGET_PROJECTS +"/" + self._currProject
            self.onProjectSelect(None)
            self.__norefresh = True
            self.clearFiles()
            cmd="mv -f '%s' '%s'"%(path,TARGET_TRASH)
            self.ssh.command(cmd, self.deleteCurrentProject)
            return
        # Eventually append an empty event to trigger refresh on completion
        print ("project deleted(%s)"%result)
        self.getProjList()
        self.checkRefreshStatus()
        
    def onProjectSelect(self, name):
        if name != self._currProject:
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
            
    def backupCurrentProject(self, path):
        if not self._currProject:return
        if self.__currentBackup:
            DEBUG("Backup already in progress") 
            return
        self.__currentBackup = os.path.join(path, self._currProject)
        
        if os.path.isdir(self.__currentBackup):
            messagebox.showerror("Backup error", "Folder %s already exists"%self.__currentBackup)
            self.__currentBackup = None
            return
        try:os.mkdir(self.__currentBackup)
        except:
            messagebox.showerror("Backup error", "Failed to create folder %s"%self.__currentBackup)
            self.__currentBackup = None
            return
        
        dstName = target_path_join(TARGET_PROJECTS, self._currProject) 
        self.ssh.command("ls -1 %s"%(dstName), self.__backupCurrentProject2)
        
    def __backupCurrentProject2(self, success = False, result = None):
        if not self._currProject:return
        if not self.__currentBackup:return
        
        # list files
        if success:
            self.__currentBackupList = result.split("\n")
            self.__currentBackupDone = 0
            for f in self.__currentBackupList:
                dstName = target_path_join(TARGET_PROJECTS, self._currProject, f) 
                localPath= os.path.join(self.__currentBackup,f)
                SSHDownloader(self.ssh, dstName, localPath, self.__backupUpdateStatus)
            self.ssh.command("echo" ,self.__backupCurrentProject3) # just for event
        else:
             self.__currentBackup = None
             print("FAILED")
        
    def __backupCurrentProject3(self, success = False, result = None):
        self.__currentBackup = None
        self.ui.project.reload()
        print("Done")
        
    def __backupUpdateStatus(self, success = False, result = None):
        if success: self.__currentBackupDone += 1
        self.ui.project.makeInactive("Backup in progress... (%d/%d)"%(
            self.__currentBackupDone, len(self.__currentBackupList)))
        
    def onOSC(self, path :str, param):
        if not path or path[0] != "/":return
        path=path[1:].split("/")
        if not path: return
        if path[0] == "pbkrkbd":
            self.ui.kbd.onOSC(path[1:],param)
        elif path[0] == "pbkrctrl":
            pass
        elif path[0] == "pbkrmenu":
            pass
        else:
            print ("Received '%s': %s(%s)"%(path,param,type(param)))
                   
if __name__ == '__main__':
    dir_path = os.path.dirname(os.path.realpath(__file__))
    os.chdir(dir_path)
    try:
        mgr = _Manager(sys.argv[:])
        mgr.run()
    except Exception as E:
        print(f"Error {E}")
        input()
    DEBUG("Program exited")
