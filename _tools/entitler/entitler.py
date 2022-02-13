#/bin/python3
python_version = int(str(range(3))[-2])
print("Detected python %d"%python_version)

import Tkinter as tk
import tkFileDialog
import sys, os
import json

class _Title:
    def __init__(self, name= "", title =""):
        self.title, self.name = tk.StringVar(value=title), tk.StringVar(value=name)
        self.track = tk.IntVar(value = -1)
class _TrackedFile:
    def __repr__(self):return "[%s(%d)]"%(self.name,self.track)
    def __init__(self, mgr, name= ""):
        self.track = int(mgr.readOneLineFile('%s.track'%name,0))
        self.title = mgr.readOneLineFile('%s.title'%name,name)
        self.name = name
    def __lt__(self, b):
        if  self.track != b.track: return self.track < b.track
        return self.name < b.name
    
class _Manager:
    def __init__(self, args):
        self._initpath=os.path.abspath (os.path.dirname(args[0]))
        self._args = args
        self.win = tk.Tk()
        self.win.title("Playbacker offline tool")
        self.win.minsize(560, 560)
        self.data={}
        self.data["inFolder"] = tk.StringVar(value = os.getcwd())
        self.titles= []
        self.wtitles= []
        self.wtids= []
        greeting = tk.Label(self.win, text="Hello, Tkinter")
        greeting.place(x= 0, y=0)
        bExit = tk.Button(self.win, text="Exit", command = self._quit)
        bExit.place(x = 500, y = 510)
        
        # Select INPUT folder
        fr= tk.LabelFrame(self.win, text = "Local config")
        b = tk.Button(fr, text="Select...", command = self._selectInputFolder)
        b.grid(row =1, column = 2)
        l = tk.Entry(fr, textvariable = self.data["inFolder"], width = 65, state="readonly")
        l.grid(row =1, column = 1)
        fr.place(x=10, y = 30)
        
        # Files arrangement
        fr = tk.LabelFrame(self.win, text = "Tracks",width=500, height=408)
        MAX_TRACK = 20
        for i in range(MAX_TRACK):
            fr2 = tk.Frame(fr, width = 480, height= 20)
            title= _Title()
            self.titles.append(title)
            trId = tk.Label(fr2, text = str(i), width = 3)
            trId.grid(row =1, column = 1)
            l = tk.Entry(fr2, textvariable = title.name, width = 25, state="readonly")
            l.grid(row =1, column = 2)
            wTitle = tk.Entry(fr2, textvariable = title.title, width = 20, state="normal")
            wTitle.grid(row =1, column = 3)
            wTitle.bind('<Return>', lambda event,i=i:self.onWidgetEnterKey(i))
            self.wtitles.append(wTitle)
            wTid = tk.Entry(fr2, textvariable = title.track, width = 10, state="normal")
            wTid.grid(row =1, column = 4)
            wTid.bind('<Return>', lambda event,i=i:self.onTIDEnterKey(i))
            self.wtids.append(wTid)
            fr2.grid(row = i, column=1)
        fr.place(x=10, y = 80)
        
        self.loadprefs()
        self.refresh()
            
    
    def readOneLineFile(self, name, defval):
        try:
            with open(os.path.join(self.data["inFolder"].get(),name),'r') as f:
                return f.readline()
        except : return defval
    def _writeOneLineFile(self, name, value):
        try:
            with open(os.path.join(self.data["inFolder"].get(),name),'w') as f:
                return f.write(value)
        except Exception as e:
            print ("FILE update failed!(%s)"%e)
    def onTIDEnterKey(self,i):
        if i >= len(self.titles): return
        # change title
        f = self.titles[i].name.get()
        track = self.titles[i].track.get()
        if track >= 1:
            print ("Track Id of %s changed to %s"%(f,track))
            self._writeOneLineFile("%s.track"%f,str(track))
            self.refresh()
        
    def onWidgetEnterKey(self,i):
        if i >= len(self.titles): return
        # change title
        f = self.titles[i].name.get()
        t = self.titles[i].title.get()
        print ("title of %s changed to %s"%(f,t))
        if t:
            self._writeOneLineFile("%s.title"%f,t)

    def _quit(self):
        self.saveprefs()
        self.win.destroy()
    def _selectInputFolder(self):
        folder_selected = tkFileDialog.askdirectory(
            title = "Select input folder",
            initialdir = self.data["inFolder"].get(),
            parent = self.win,
            mustexist = True
            )
        if folder_selected:
            self.data["inFolder"].set(folder_selected)
        self.refresh()
        
    def __prefFile(self):return os.path.join (self._initpath, '.prefs.json')
     
    def loadprefs(self):
        if not os.path.exists(self._initpath):return
        try:
            with open(self.__prefFile()) as json_file:
                data = json.load(json_file)
                for k in data:
                    if k in self.data:
                        self.data[k].set(data[k])
        except:
            print ("Failed to open prefs file : %s"%self.__prefFile())
    def saveprefs(self):
        if not os.path.exists(self._initpath):return
        data ={ k : self.data[k].get() for k in self.data }
        with open(self.__prefFile(), 'w') as outfile:
            json.dump(data, outfile)
        
    def refresh(self, recurse= False):
        f = self.data["inFolder"].get()
        if not f:return
        try:
            tFiles = [_TrackedFile(self,file) for file in os.listdir(f) if file.upper().endswith(".WAV")]
        except:
            tFiles = []
        tFiles.sort(reverse=False)
        print (tFiles)
        
#         # Re-sort using TID (check duplicates):
#         trackId = 0
#         newFiles = []
#         changed = False
#         while tFiles:
#             tFile = tFiles.pop()
#             if tFile.track <= trackId:
#                 print ("Changed track Id %s/%s"%(tFile.track,trackId) )
#                 tFile.track = trackId + 1
#                 self._writeOneLineFile("%s.track"%tFile.name,str(tFile.track))
#                 changed = True
#             else : tFile.track = int (trackId)
#             trackId = tFile.track
#             newFiles.append(tFile)
#         tFiles= newFiles
#         
#         if changed:
#             if recurse: raise Exception("???")
#             else: return self.refresh(recurse = True)
        # 
        for i in range(len(self.titles)):
            title=self.titles[i]
            if i < len(tFiles):
                title.title.set(tFiles[i].title)
                title.track.set(tFiles[i].track)
                title.name.set(tFiles[i].name)
                self.wtitles[i].config(state=tk.NORMAL)
            else:
                title.title.set("")
                title.name.set("")
                title.track.set(-1)
                self.wtitles[i].config(state=tk.DISABLED)
                
    def run(self): self.win.mainloop()
    
if __name__ == '__main__':
    mgr = _Manager(sys.argv[:])
    mgr.run()

