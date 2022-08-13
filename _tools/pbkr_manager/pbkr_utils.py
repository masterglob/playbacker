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
            
