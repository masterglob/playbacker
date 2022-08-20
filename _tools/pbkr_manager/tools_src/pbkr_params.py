#/bin/python3

import sys, os
import json
python_version = int(str(range(3))[-2])

if python_version == 3:
    import tkinter as tk
elif python_version == 2:
    import Tkinter as tk

def DEBUG(x):
    print(x)
    
class PBKR_Params:
    def __init__(self, initPath, filename):
        self._data={}
        self._initpath = initPath
        self._filename = filename
        self._file = os.path.join (initPath, filename)
        self.loadFromFile()
        
    def __prefFile(self):return 
    
    def createStr(self, key, value = ""):
        if key not in self._data:
            self._data[key] = tk.StringVar(value = value) 
        return self._data[key]
    def createInt(self, key, value = 0):
        if key not in self._data:
            self._data[key] = tk.IntVar(value = value)
        return self._data[key]
    
    def saveToFile(self):
        if not os.path.exists(self._initpath):return
        data ={ k : self._data[k].get() for k in self._data }
        with open(self._file, 'w') as outfile:
            json.dump(data, outfile)
        DEBUG("Saved %d parameters to %s"%(len(data), self._file))
                  
    def loadFromFile(self):
        if not os.path.exists(self._initpath):return
        try:
            DEBUG("Loading parameters from %s"%self._file)
            nbParams = 0
            with open(self._file) as json_file:
                data = json.load(json_file)
                for k in data:
                    nbParams += 1
                    val = data[k]
                    if isinstance(val, int): self.createInt(k, val)
                    else: self.createStr(k, val)
            DEBUG("Loaded %d parameters"%nbParams)
        except Exception as e:
            print ("Failed to open prefs file : %s"%self._filename)
            print(e)
        