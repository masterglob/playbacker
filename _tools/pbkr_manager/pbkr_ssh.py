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
        self._sftp = None
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
    def installFile(self, srcFile, dstFile, resultCb):
        self.lock.acquire()
#         print ("Received command:%s"%str(cmdStr))
        self.cmds.append((self._doInstall,(srcFile, dstFile, resultCb)))
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
#             print (">%s"%cmd)
#             print (">%s"%resultCb)
            self.sendEvent()
            if resultCb:
                resultCb(True, stdout.read().decode().strip())
            else:
                print ("No result CB for %s"%cmd)
        except paramiko.ssh_exception.SSHException as e:
            self.sendEvent("Command <%s> failed"%cmd, str(e))
            if resultCb:
                resultCb(False, stderr.read().decode().strip())
        
    def _doDisconnect(self, **kwargs):
        self.ssh.close()
        self._isConnected = False
        self._isConnecting  = False
        self._sftp = None
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
        self._sftp = self.ssh.open_sftp()
          
    def sendEvent(self, status= "", error = None):
        self.onEvent (status, error) 
    
    def _doInstall(self, srcFile, dstFile, resultCb):
        if not self._sftp:
            if resultCb:
                resultCb(False, "SFTP not open")
            return
        
        try:
            attr = self._sftp.put(srcFile, dstFile)
            # print("Command: _sftp.put(%s, %s)"%(srcFile,dstFile))
            self.sendEvent()
            resultCb(True, "")
        except IOError as e:
            self.sendEvent("Installing file <%s> to path <%s> failed"%
                           (srcPath, destPath), str(e))
            if resultCb:
                resultCb(False, stderr.read().decode().strip())
                   
class SSHExecuter:
    def __init__(self, name, event = None, **kwargs):
        self.__event, self.__name  = event, name
    def __call__(self, success, result):
        if success:
            try:
                if self.__event : self.__event(result)
            except:
                DEBUG ("Failed to apply SSH result (%s): <%s>"%(self.__name, result))
                raise
        else:
            DEBUG ("Failed to exec SSH command. ERR=<%s>"%(result))
            DEBUG ("Note: failed command was <%s>"%(self.__name))
              
class SSHCommander(SSHExecuter):
    def __init__(self, ssh, command, event):
        SSHExecuter.__init__(self, name=command[:], event= event)
        self.command = command
        ssh.command(command ,self)
        
class SSHUploader(SSHExecuter):
    def __init__(self, ssh, srcFile, dstFile, event):
        SSHExecuter.__init__(self, name="copy %s to %s"%(srcFile,dstFile), event= event)
        self.srcFile = srcFile
        ssh.installFile(srcFile ,dstFile ,event)
        