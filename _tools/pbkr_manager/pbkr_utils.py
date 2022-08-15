#/bin/python3
python_version = int(str(range(3))[-2])
# print("Detected python %d"%python_version)

import struct, re
if python_version == 3:
    import tkinter as tk
    from tkinter import messagebox
    from tkinter import ttk
    from tkinter.simpledialog import askstring
elif python_version == 2:
    import Tkinter as tk
    import tkFileDialog

def DEBUG(x):print(x)

class P_NoteBook:
    def __init__(self, parent, label = ""):
        self.parent = parent
        self.label =label
    def __enter__(self):
        self.fr= tk.LabelFrame(self.parent, text = self.label)
        self.tabs = ttk.Notebook(self.fr)
        return self
    def __exit__(self ,type, value, traceback):
        self.fr.pack(side = tk.TOP, expand = True, fill=tk.BOTH)
        self.tabs.pack(side = tk.TOP, expand = True, fill=tk.BOTH)
    def frame(self):return self.fr
    def addTab(self, title):
        t = ttk.Frame(self.tabs)
        self.tabs.add(t, text =title)
        return t
   
##################################################
def askTitle(parent, name, title):
    result = None
    while not result:
        result = askstring(parent = parent,
                          title = 'Set Title (%s)'%name,
                          prompt = 'Enter new title for file (%s)'%name,
                          initialvalue = title).strip()
        if result:
            if not re.match(r"^[ 'A-Z0-9_+-]+$", result, flags=re.IGNORECASE): 
                messagebox.showerror("Bad title", "Do not use special chars in title")
                continue
            result = result.replace ("'", "\\'")
            return result.strip()
            

##################################################
def printable(c):return "%c"%c if c >= 0x20 and c<0x80 else "."

##################################################
def target_path_join(*args): return ("/".join(args))

##################################################
class InvalidWavFileFormat(Exception):
    def __init__(self, msg):
        Exception.__init__(self, "Invalid WAV file (Debug info:%s)"%msg)

##################################################
class WavFileChecker:
    ### See http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
    def __init__(self, filename):
        assert(filename)
        # Open file
        try:
            fh = open(filename, 'rb')
        except:
            raise InvalidWavFileFormat("Could not open input file %s" % (filename))
        
        try:
            # RIFF     Header
            ckId, cksize, hType = struct.unpack('<4sI4s', fh.read(12)) 
            
            if ckId != b"RIFF" :
                raise InvalidWavFileFormat("ckId=<%s>"%ckId)
            if hType != b"WAVE":
                raise InvalidWavFileFormat("File Header Type")
            cksize -= 4
            
            # fmt Chunk
            fmt, fmtSize = struct.unpack('<4sI', fh.read(8)) 
            if fmt != b"fmt ":
                raise InvalidWavFileFormat("FMT")
            
            if fmtSize < 16:
                raise InvalidWavFileFormat("fmtSize")
            wFormatTag, nChannels, nSamplesPerSec, nAvgBytesPerSec, nBlockAlign, wBitsPerSample = \
                struct.unpack('<HHIIHH', fh.read(16)) 
            
            if nChannels < 2 or nChannels > 4:
                raise InvalidWavFileFormat("nChannels=%d"%nChannels)
            
            if fmtSize > 16:
                if fmtSize < 18:
                    raise InvalidWavFileFormat("fmtSize=%s"%fmtSize)
                cbSize, = struct.unpack('<H', fh.read(2))
                if cbSize != 0 and cbSize != 22:
                    raise InvalidWavFileFormat("cbSize=%d"%cbSize)
                if cbSize == 22:
                    wValidBitsPerSample, dwChannelMask, subFormat = \
                        struct.unpack('<HI16s', fh.read(cbSize)) 
            else:        
                wValidBitsPerSample, dwChannelMask, subFormat = 0,0, ""
            DEBUG ("%s is a valid compatible WAV file"%filename)
        finally:
            fh.close()
        