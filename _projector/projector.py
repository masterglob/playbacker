
import subprocess
import ctypes
import os

import Tkinter as tk
import tkFileDialog
import ttk


def LOG(x):
    print (x )
    
# Drive types
DRIVE_UNKNOWN     = 0  # The drive type cannot be determined.
DRIVE_NO_ROOT_DIR = 1  # The root path is invalid; for example, there is no volume mounted at the specified path.
DRIVE_REMOVABLE   = 2  # The drive has removable media; for example, a floppy drive, thumb drive, or flash card reader.
DRIVE_FIXED       = 3  # The drive has fixed media; for example, a hard disk drive or flash drive.
DRIVE_REMOTE      = 4  # The drive is a remote (network) drive.
DRIVE_CDROM       = 5  # The drive is a CD-ROM drive.
DRIVE_RAMDISK     = 6  # The drive is a RAM disk.

# Map drive types to strings
DRIVE_TYPE_MAP = { DRIVE_UNKNOWN     : 'DRIVE_UNKNOWN',
                   DRIVE_NO_ROOT_DIR : 'DRIVE_NO_ROOT_DIR',
                   DRIVE_REMOVABLE   : 'DRIVE_REMOVABLE',
                   DRIVE_FIXED       : 'DRIVE_FIXED',
                   DRIVE_REMOTE      : 'DRIVE_REMOTE',
                   DRIVE_CDROM       : 'DRIVE_CDROM',
                   DRIVE_RAMDISK     : 'DRIVE_RAMDISK'}

EXT_TRACK = 'track'

def getDriveName(driveletter):
    return subprocess.check_output(["cmd","/c vol "+driveletter]).split("\r\n")[0].split(" ").pop()

def get_usbs ():
    # Return list of tuples mapping drive letters to drive types
    def get_drive_info():
        result = []
        bitmask = ctypes.windll.kernel32.GetLogicalDrives()
        for i in range(26):
            bit = 2 ** i
            if bit & bitmask:
                drive_letter = '%s:' % chr(65 + i)
                drive_type = ctypes.windll.kernel32.GetDriveTypeA('%s\\' % drive_letter)
                result.append((drive_letter, drive_type))
        return result

    drive_info = get_drive_info()
    
    # for drive_letter, drive_type in drive_info:
    #     print '%s = %s' % (drive_letter, DRIVE_TYPE_MAP[drive_type])
    removable_drives = [drive_letter for drive_letter, drive_type in drive_info if drive_type == DRIVE_REMOVABLE]
    return removable_drives

class File():
    def __init__(self,path, name):
        # LOG ('WAV file : %s'%name)
        self.name=name
        self.path=path
        self.filename=os.path.join(self.path, self.name)
        self.exts={}
        self.revert()
        # LOG ("Exts:%s"%self.exts)
        
    def __repr__(self):
        return "%s"%(self.name)
        #return "%s(%s)"%(self.name,self.exts)
    @staticmethod
    def extType(): return [EXT_TRACK, 'title']
    
    def revert (self):
        self.exts={k:self.readExt(k) for k in self.extType()}
        self._modified = False
        
    def readExt(self,ext):
        filename = "%s.%s"%(self.filename,ext)
        try:res = open(filename).readline()
        except:return None
        return res
    
    def apply(self):
        LOG ("Save: %s"%self.exts)
        for e in File.extType():
            if e in self.exts:
                self.__writeExt (e, self.exts[e])
            else:
                LOG("No value provided for %s"%e)
        self._modified = False
        
    def modified (self): return self._modified
    
    def setProperty (self, ext, value):
        self._modified = True
        self.exts[ext] = value   
    
    def __writeExt(self, ext, value):
        self.exts[ext] = str(value)
        filename = "%s.%s"%(self.filename,ext)
        try:
            f = open(filename, "w")
            f.write (str(value))
        except:
            LOG ("Failed to set resource %s"%ext)
        
    def popup_properties(self, parent):
        fPopup = tk.Toplevel()
        fPopup.title("Properties of %s"%self.name)
        fontH=(None, 15)
        font=(None, 12)
        vars={}
        row = 1
        tk.Label (fPopup, text="Name", font=font).grid(row=row, column=1)
        tk.Label(fPopup, text=self.name , font=font).grid(row=row, column=2, columnspan=2)
        row += 1
        
        tk.Label(fPopup, text= "Name : %s"%self.name , font=fontH)
        for e in File.extType():
            state = tk.NORMAL if e != EXT_TRACK else tk.DISABLED
            var = tk.StringVar(value=self.exts[e])
            tk.Label (fPopup, text=e, font=font).grid(row=row, column=1)
            entry =tk.Entry(fPopup, textvariable=var, font=font, state=state)
            entry.bind("<FocusIn>", lambda event, entry=entry:entry.selection_range(0, tk.END))
            entry.grid(row=row, column=2, columnspan=2)
            vars[e] = var
            row += 1
            
        def popup_reset ():
            for e in File.extType():
                vars[e].set(self.exts[e])
        def popup_cancel ():
            fPopup.destroy()
            
        tk.Button(fPopup, text='Save', font=font, command=fPopup.destroy).grid(row=row, column=1)
        tk.Button(fPopup, text='Reset', font=font, command=popup_reset).grid(row=row, column=2)
        tk.Button(fPopup, text='Cancel', font=font, command=popup_cancel).grid(row=row, column=3)
        fPopup.transient(parent)
        parent.wait_window(fPopup)
        
        for e in File.extType():
            val = vars[e].get()
            if val:
                self.setProperty(e, val)
            
class Project():
    def __init__(self,path):
        self.name=os.path.basename(path)
        self.path = path
        self.files=[]
        self.reload()
        LOG ('Project %s (%s)'%(self.name,self.path))
                
    def __repr__(self):return "Project %s"%self.name
    def reload(self):
        self.files=[]
        # Parse project
        for file in os.listdir(self.path):
            ext = file.split(".")[-1].upper()
            if ext == "WAV":
                self.files.append(File(self.path, file))
            else:
                LOG ("Ignore file %s (%s)"%(file,ext))
        for file in self.files: LOG(file)
    def contains (self, filename):
        for file in self.files:
            if file.name == filename : return True
        return False
    
    def delete (self, name):
        for file in os.listdir(self.path):
            if file.startswith (name):
                os.remove (os.path.join(self.path,file))
        LOG ("self.files=%s"%self.files)
        for file in self.files[:]:
            if file.name == name:
                LOG ("Removed %s"%name)
                self.files.remove(file)
        LOG ("self.files=%s"%self.files)
        self.reload()
        
class Main_Window(tk.Frame):
    
    def __init__(self):
        self.__root= tk.Tk()
        tk.Frame.__init__(self, self.__root)
        self.__icon=None
        self.__name="Projector"
        self.currUsb=None
        self.currProj=None
        self.currFile=None
        
    def on_closing(self):
        LOG ("Closed requested")
        self.__root.destroy()

    def run(self):
        # hide main windows
        # self.__root.overrideredirect(1)
        # self.__root.withdraw()    
        self.__root.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.__root.title (self.__name)
        self.wgtUSB=None
        
        self.setup()
        if self.__icon:
            self.iconbitmap(self.__icon)
        self.__root.mainloop()

    def reloadUSB (self):
        self.wgtUSB.after(1000, self.reloadUSB)
        
        usbList = self.wgtUSB.get(0, tk.END)
        usbs =  get_usbs()
        
        for u in usbs:
            u=str(u)
            if u not in usbList:
                # New Key, just add it
                print ("Key inserted : %s"%u)
                self.wgtUSB.insert(tk.END, u)
                
        for u in usbList:
            u=str(u)
            if u not in usbs:
                print ("Key removed : %s (current was %s)"%(u,self.currUsb))
                # Key removed
                self.wgtUSB.delete(usbList.index(u))
                if u == str (self.currUsb):
                    print ("Current project Key removed!")
                    self.usbSelect()
            
        
    def setup(self):
        root = self.__root
        W=640
        H=440
        font=(None, 11)
        root.geometry('%sx%s'%(W,H))
        
        ### main FRAMES
        fusb = tk.LabelFrame (text="USB devices")
        
        fProj = tk.LabelFrame (text="Projects")
        
        fFil = tk.LabelFrame (text="Files")
        
        fProps = tk.LabelFrame (text="Properties")
        
        #### LIST OF USB INTERFACES
        tk.Label(fusb, text = "Select device", font=font).pack()
        
        self.wgtUSB = tk.Listbox(fusb,  width = 20, font=font,height=5,
                                  activestyle='dotbox', selectmode=tk.SINGLE)
        self.wgtUSB.bind("<<ListboxSelect>>", self.usbSelect)
        self.wgtUSB.pack()
        self.wgtUSB.after(1000, self.reloadUSB)
        
        
        #### LIST OF PROJECTS
        tk.Button (fProj, text= "Create", font=font,
                   command=self.onProjectCreate).grid(padx=3,pady=3,row = 1, column = 1)
        tk.Button (fProj, text= "Delete", font=font,
                    command=self.onProjectDelete).grid(padx=3,pady=3,row = 1, column = 2)
        self.combProjects = tk.Listbox(fProj,  width = 20, font=font,height=10,
                                  activestyle='dotbox', selectmode=tk.SINGLE)
        self.combProjects.bind("<<ListboxSelect>>", self.onProjectSelect)
        self.combProjects.grid(padx=3,pady=3,row = 2, column = 1, columnspan=2)
        
        #### LIST OF FILES IN PROJECT
        self.varProjName = tk.StringVar(value="None")
        tk.Label (fFil, text= "Current project:", font=font).grid(padx=3,pady=3,row = 0, column = 1)
        tk.Label (fFil, textvariable= self.varProjName, font=font).grid(padx=3,pady=3,row = 0, column = 2)
        tk.Button (fFil, text= "Import", font=font,
                   command=self.onFileImport).grid(padx=3,pady=3,row = 1, column = 1)
        tk.Button (fFil, text= "Remove", font=font,
                    command=self.onFileRemove).grid(padx=3,pady=3,row = 1, column = 2)
        self.combFiles = tk.Listbox(fFil, font=font,height=20,  width = 25,
                                    activestyle='dotbox', selectmode=tk.SINGLE)
        self.combFiles.bind("<<ListboxSelect>>", self.onFileSelect)
        self.combFiles.bind("<Button-3>", self.onFileProperties)
        self.combFiles.bind('<Double-Button-1>', self.onFileProperties)
        self.combFiles.grid(padx=3,pady=3,row = 2, column = 1, columnspan=2)
        
        
        #### LIST OF PROPERTIES
        f2 = tk.Frame (fProps)
        tk.Label(f2, text="").grid(row=1, column=1, columnspan=3)
        tk.Button (f2, text="Move Up", font = font, 
                   command= lambda self=self: self.onFileMove(up = True)).grid(row=2, column=1)
        tk.Button (f2, text="Move Down", font = font, 
                   command= lambda self=self: self.onFileMove(up = False)).grid(row=2, column=2)
        tk.Button (f2, text="Renumber", font = font, 
                   command= lambda self=self: self.renumberTracks()).grid(row=2, column=3)
        f2.grid(padx=10,pady=10,row=0, column=1)
        
        fProps2 = tk.Frame (fProps)
        self.varProp={}
        self.varPropEntries={}
        row=1
        for e in File.extType():
            var = tk.StringVar(value="")
            state = tk.NORMAL if e != EXT_TRACK else tk.DISABLED
            tk.Label (fProps2, text = e.capitalize(), font = font,  width = 8).grid(row=row, column=1)
            entry = tk.Entry (fProps2, textvariable = var, font = font,  width = 20, state=state)
            entry.grid(row=row, column=2, columnspan=2)
            entry.bind('<Return>', lambda event, self=self, e=e :self.OnPropEnter(e))
            entry.bind('<Escape>', lambda event, self=self, e=e :self.OnPropReload(e))
            entry.bind('<FocusOut>', lambda event, self=self, e=e :self.OnPropEnter(e))
            row += 1
            self.varPropEntries[e] = entry
            self.varProp[e] = var
        tk.Button(fProps2, text= "Apply", font=font, 
                  command = lambda self=self, e=e :self.OnPropApply(None)).grid(row=row, column=2)
        tk.Button(fProps2, text= "Reload", font=font,
                   command = lambda self=self, e=e :self.OnPropReload(None)).grid(row=row, column=3)
        fProps2.grid(padx=10,pady=10,row=1, column=1)
        
        fProps2 = tk.Frame (fProps)
        self.wgtModified = tk.Label (fProps2, font=font, fg='red', text='Modification not saved!')
        fProps2.grid(padx=10,pady=10,row=2, column=1)
        
        # GLOBAL LAYOUT
        fusb.grid(row = 1, column = 1)
        fProj.grid(row = 2, column = 1)
        fFil.grid(row = 1, column = 2, rowspan=2)
        fProps.grid(row = 1, column = 3, rowspan=2)
    
    def chooseFile(self):
        return tkFileDialog.askopenfilename(initialdir = "",
                                             title = "Select file",
                                             filetypes = (("wav files","*.wav"),)
                                             )
        
    def popup (self, title, value, confirmType=False):
        fPopup = tk.Toplevel()
        fPopup.title(title)
        newVal = tk.StringVar(value=value)
        def popup_cancel ():
            newVal.set("")
            fPopup.destroy()
        font=(None, 15)
        
        tk.Button(fPopup, text='Ok', font=font, command=fPopup.destroy).grid(row=1, column=1)
        btnCancel = tk.Button(fPopup, text='Cancel', font=font, command=popup_cancel)
        btnCancel.grid(row=1, column=3)
        
        if confirmType:
            newVal.set ("YES")
            wgt =tk.Label (fPopup, text=value, font=font)
            btnCancel.focus_set()
        else:
            wgt =tk.Entry(fPopup, textvariable=newVal, font=font)
            wgt.bind("<FocusIn>", lambda event:entry.selection_range(0, tk.END))
            wgt.bind("<Return>", lambda event:fPopup.destroy())
            wgt.bind("<Escape>", lambda event:popup_cancel())
            wgt.focus_set()
        wgt.grid(row=0, column=1, columnspan=2)
        
        fPopup.transient(self.__root)       # 
        fPopup.grab_set()          # Interaction avec fenetre jeu impossible
        
        fPopup.geometry("+%d+%d" % (self.__root.winfo_rootx()+50,
                          self.__root.winfo_rooty()+50
                         )
             ) 
        self.__root.wait_window(fPopup)   #
        if confirmType:
            return newVal.get() != ""
        return newVal.get()
    
    def onProjectCreate (self):
        name =  self.popup("Enter project Name", "")
        if name :
            LOG ("TODO : onProjectCreate %s" % name)
    def onProjectDelete (self):
        w = self.combProjects
        sel =  w.curselection()
        if not sel:return
        
        s = w.get (sel [0])
        res = self.popup("Confirm", "Confirm deletion of %s"%s, True)
        LOG ("TODO : onProjectDelete : %s"%res)
        
    def onFileImport(self):
        if not self.currProj: return 
        f = self.chooseFile()
        name, path = os.path.basename(f), os.path.dirname(f)
        
        # Check if file exists in project
        if self.currProj.contains (name):
            res = self.popup("Overwrite?", 
                             "File '%s' already exists in the project.\n"%name +
                             " Continuing will overwrite existing file. Continue?", True)
            if not res: return
            self.currProj.delete (name)
        
        
        LOG ("TODO : onFileImport %s from %s" % (name, path))
        self.onProjectSelect()
        
    def onFileRemove (self):
        if not self.currProj: return 
        w = self.combFiles
        sel =  w.curselection()
        if not sel:return
        
        s = w.get (sel [0])
        res = self.popup("Confirm", "Confirm deletion of file %s"%s, True)
        if not res: return
        self.currProj.delete (s)
        self.refreshProject()
        
    def onProjectSelect(self, event = None):
        sel =  self.combProjects.curselection()
        if not sel:
            LOG ("NO Project selected")
            return
        
        idx = int (sel[0])
        proj = self.projects[idx]
        if proj != self.currProj:
            self.currProj = proj
            self.varProjName.set (proj.name)
            LOG ("Select: %s"% self.currProj)
            self.onFileSelect(None)
            self.refreshProject()
        
    def refreshProject(self):
        # Load file list
        if not self.currProj: return
        self.currProj.reload()
        self.combFiles.delete (0, tk.END)
        for file in self.currProj.files:
            self.combFiles.insert(tk.END, file)
        self.modifiedRefresh()
        
    def onFileSelect (self, event):
        sel =  self.combFiles.curselection()
        if sel and self.currProj:
            idx = int ( sel[0])
            file = self.currProj.files[idx]
            self.currFile = file
            for e in File.extType():
                state = tk.NORMAL if e != EXT_TRACK else tk.DISABLED
                self.varProp[e].set (file.exts[e])
                self.varPropEntries[e].config(state=state)
        else:
            self.currFile = None
            for e in File.extType():
                self.varProp[e].set ("")
                self.varPropEntries[e].config(state=tk.DISABLED)
        self.modifiedRefresh()
            
    def onFileMove (self, up):
        sel =  self.combFiles.curselection()
        if sel and self.currProj:
            idx = int ( sel[0])
            file = self.currProj.files[idx]
            newIdx = idx + (-1 if up else 1)
            if newIdx < 0 or newIdx >= len(self.currProj.files): return
            LOG ("onFileMove %s %s"%("UP" if up else "DOWN", file))
            f = self.currProj.files
            f[newIdx], f[idx] = f[idx] , f[newIdx]
            self.combFiles.delete (idx)
            self.combFiles.insert (newIdx, f[newIdx])
            self.combFiles.selection_set(newIdx)
            
            self.renumberTracks()

    def renumberTracks(self):
        idx = 1
        for f in self.currProj.files:
            f.setProperty (EXT_TRACK, str(idx))
            idx += 1
        self.OnPropReload(EXT_TRACK)
        
    def modifiedRefresh(self):
        if self.currProj:
            modified = False
            for f in self.currProj.files:
                if f.modified() :
                    LOG ("%s modified"%f)
                    modified = True
            if modified:
                self.wgtModified.pack()
            else:
                self.wgtModified.pack_forget()
        
    def OnPropReload(self, e = None):
        if self.currFile :
            self.currFile.revert()
            for e in File.extType():
                self.varProp[e].set(self.currFile.exts[e])
        self.modifiedRefresh()
          
    def OnPropEnter(self, e):
        file = self.currFile
        if file :
            if e in File.extType():
                file.setProperty(e, self.varProp[e].get())
            else:
                for e in File.extType():
                    file.setProperty(e, self.varProp[e].get())
        self.modifiedRefresh()
        
    def OnPropApply(self, e):
        self.OnPropEnter(None)
        proj = self.currProj
        if proj:
            for file in proj.files[:]:
                file.apply()
            self.modifiedRefresh()
        
           
    def onFileProperties(self, event):
        sel =  event.widget.curselection()
        if sel:
            idx = int ( sel[0])
            file = self.currProj.files[idx]
            file.popup_properties(self.__root)
            self.modifiedRefresh()
            
        
    def renameByIndex(self,i):
        n = self.lablFile[i]
        newName = self.popup("Rename",i)
        LOG ("Rename %s => %s"%(n,newName))
        
    def usbSelect(self, event = None):
        w = self.wgtUSB
        sel = w.curselection()
        if sel:
            newUsb = w.get (sel [0])
            if self.currUsb != newUsb:
                self.setUsb (newUsb)
        else:
            self.setUsb (None)
            
    def setUsb(self, drive):
        self.currUsb = drive
        self.combProjects.delete(0, tk.END)
        if drive:
            path = os.path.join(drive,"PBKR")
            if (os.path.isdir (path)):
                self.projects=[]
                LOG ("Search %s"%path)
                
                for file in os.listdir(path): 
                    proj=Project(os.path.join(path,file))
                    self.projects.append(proj)
                    self.combProjects.insert(tk.END,proj)
                LOG ("Projects:%s"%self.projects)
            else:
                LOG ("No folder %s"%path)
        
        self.onProjectSelect()
        
        
        
def main():
    LOG ("PROJECTOR")
    usbs =  get_usbs()
    print ('removable_drives = %r' % usbs)
    win = Main_Window()
    win.run()

if __name__ == "__main__":
    # execute only if run as a script
    main()