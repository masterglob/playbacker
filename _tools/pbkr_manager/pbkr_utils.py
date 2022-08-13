#/bin/python3
python_version = int(str(range(3))[-2])
print("Detected python %d"%python_version)

if python_version == 3:
    import tkinter as tk
    from tkinter import messagebox
    from tkinter import ttk
elif python_version == 2:
    import Tkinter as tk
    import tkFileDialog

def DEBUG(x):print(x)

class P_NoteBook:
    def __init__(self, parent, width, height, x, y, label = ""):
        self.parent = parent
        self.x,self.y= x, y
        self.w, self.h, self.label = width, height, label
    def __enter__(self):
        self.fr= tk.LabelFrame(self.parent,width=self.w, height=self.h, text = self.label)
        self.tabs = ttk.Notebook(self.fr, width=self.w - 5, height=self.h - 30)
        return self
    def __exit__(self ,type, value, traceback):
        self.fr.place(x=self.x, y = self.y)
        self.tabs.pack(expand = True)
    def frame(self):return self.fr
    def addTab(self, title):
        t = ttk.Frame(self.tabs, width=self.w, height=self.h)
        self.tabs.add(t, text =title)
        return t
            
