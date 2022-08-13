#/bin/python3
import threading, time
import cmd

try:
    import paramiko
except Exception as e:
    print("You need PARAMIKO to run this program")
    print(e)
    sys.exit(1)

def DEBUG(x):
    print(x)
    
class PBKR_SSH(threading.Thread):
    def __init__(self, onEvent):
        self.onEvent = onEvent
        self.ssh = paramiko.SSHClient() 
        self.cmds=[]
        self.error = None
        self.status = None
        self._isConnected = False
        self._isConnecting = False
        self.running = True
        self.lock = threading.Lock()
        threading.Thread.__init__(self)
    def start(self):
        threading.Thread.start(self)
        self._doDisconnect()

    def __del__(self):
        del self.ssh
    
    def isConnecting(self): return self._isConnecting 
    def isConnected(self): return self._isConnected 
    def command(self, cmdStr, resultCb):
        self.lock.acquire()
#         print ("Received command:%s"%str(cmdStr))
        self.cmds.append((self._doCommand,(cmdStr, resultCb)))
        self.lock.release()
    def connect(self, ip, port, username, password):
        self.lock.acquire()
        self.cmds.append((self._doConnect,(ip, port, username,password)))
        self.lock.release()
    def disconnect(self):
        self.lock.acquire()
        self.cmds.append((self._doDisconnect, ()))
        self.lock.release()
        
    def quit(self):
        self.running = False
    def run(self):
        while self.running:
            cmd = None
            self.lock.acquire()
            if self.cmds: cmd = self.cmds.pop(0)
            self.lock.release()
            if cmd:
#                 print ("Process command:%s"%str(cmd))
                try:
                    fcn, prms = cmd
                except:
                    print ("Failed to extract command:%s"%str(cmd))
                    continue
                try:
                    fcn(*prms)
                except:
                    print ("Failed to execute command:%s"%str(cmd))
                    raise
                
            else:
                time.sleep(0.01)
                
    def _doCommand(self, cmd, resultCb):
        try:
            stdin, stdout, stderr = self.ssh.exec_command(cmd)
            self.sendEvent()
            resultCb(True, stdout.read().decode().strip())
        except paramiko.ssh_exception.SSHException as e:
            self.sendEvent("Command <%s> failed"%cmd, str(e))
            resultCb(False, stderr.read().decode().strip())
        
    def _doDisconnect(self, **kwargs):
        self.ssh.close()
        self._isConnected = False
        self._isConnecting  = False
        self.sendEvent("Disconnected")
        
    def _doConnect(self, ip, port, username, password):
        self._isConnecting = True
        self.sendEvent("Connecting to %s@%s:%s"%(username,  ip, port))
        try:
            self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            self.ssh.connect(ip, username=username, password=password, port=port)
            self._isConnected = True
            self._isConnecting  = False
            self.sendEvent("Connected to %s@%s:%s"%(username,  ip, port))
        except paramiko.ssh_exception.SSHException as e:
            self._isConnecting  = False
            self.sendEvent("Failed to connect. SSHException =", str(e))
            print(self.error)
        except Exception as e:
            self._isConnecting  = False
            self.sendEvent("Failed to connect", str(e))
            print(self.error)
          
    def sendEvent(self, status= "", error = None):
        self.onEvent (status, error) 
        
class SSHCommander:
    def __init__(self, ssh, command, event):
        self.command = command
        self.event = event
        ssh.command(command ,self)
    def __call__(self, success, result):
        if success:
            try:
                self.event(result)
            except:
                DEBUG ("Failed to apply SSH result (%s): <%s>"%(self.command, result))
                raise
        else:
            DEBUG ("Failed to exec SSH command. ERR=<%s>"%(result))
            DEBUG ("Note: failed command was <%s>"%(self.command))
            