import os
import stat
import tkinter as tk
from tkinter import ttk
import paramiko
import sys
from datetime import datetime
import threading

# ==============================================================================
# CONFIGURATION / CONSTANTS
# ==============================================================================
DEFAULT_IP = "192.168.7.80"
DEFAULT_USER = "tc"
DEFAULT_PPK = os.path.join(os.environ["USERPROFILE"], ".ssh", "id_rsa_openssh.key")

# ---- AUTOMATIC PATH DETECTION ----
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LOCAL_PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))

LOCAL_SRC_SUBDIR = "_dev"
LOCAL_SYNC_DIR = os.path.join(LOCAL_PROJECT_ROOT, LOCAL_SRC_SUBDIR)
REMOTE_PROJECT_DIR = "/tmp/_dev"

WINDOW_WIDTH = 450
WINDOW_HEIGHT = 400

REQUIRED_PACKAGES = ["gcc", "gdb", "make", "compiletc", "wiringpi-dev",
    "wiringpi-dev", "libasound-dev", "samba4", "libusb-dev"
]

# ==============================================================================
# APPLICATION CODE
# ==============================================================================

class PiCoreDeployerApp:

    def __init__(self, root):
        self.root = root
        self.root.title("piCore Dev Deployer")
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        self.root.resizable(False, False)

        self.ssh_client = None
        self.is_connected = False
        self.abort_requested = False  # Abort flag

        self.ip_var = tk.StringVar(value=DEFAULT_IP)
        self.user_var = tk.StringVar(value=DEFAULT_USER)
        self.ppk_var = tk.StringVar(value=DEFAULT_PPK)

        self.create_widgets()

    def create_widgets(self):
        # Connection Frame
        conn_frame = ttk.LabelFrame(self.root, text=" Connection Settings ", padding=10)
        conn_frame.pack(fill="x", padx=15, pady=(10, 10))

        # IP Address
        ttk.Label(conn_frame, text="IP Address:").grid(row=0, column=0, sticky="w", pady=5)
        self.entry_ip = ttk.Entry(conn_frame, textvariable=self.ip_var, width=25)
        self.entry_ip.grid(row=0, column=1, sticky="w", pady=5, padx=5)

        # Username
        ttk.Label(conn_frame, text="Username:").grid(row=1, column=0, sticky="w", pady=5)
        self.entry_user = ttk.Entry(conn_frame, textvariable=self.user_var, width=25)
        self.entry_user.grid(row=1, column=1, sticky="w", pady=5, padx=5)

        # Private Key
        ttk.Label(conn_frame, text="Private Key:").grid(row=2, column=0, sticky="w", pady=5)
        self.entry_ppk = ttk.Entry(conn_frame, textvariable=self.ppk_var, width=35)
        self.entry_ppk.grid(row=2, column=1, sticky="w", pady=5, padx=5)

        # Connect / Disconnect Button
        self.btn_connect = ttk.Button(self.root, text="Connect", command=self.toggle_connection, width=15)
        self.btn_connect.pack(pady=5)

        # Actions Frame
        actions_frame = ttk.LabelFrame(self.root, text=" Actions ", padding=10)
        actions_frame.pack(fill="x", padx=15, pady=5)

        self.btn_setup = ttk.Button(actions_frame, text="Setup Target Environment (TCZ)", command=self.run_setup, state="disabled")
        self.btn_setup.pack(fill="x", pady=5)

        self.btn_clean = ttk.Button(actions_frame, text="🧹 Clean", command=self.start_clean_thread, state="disabled")
        self.btn_clean.pack(fill="x", pady=5)
        
        # Note: command points to 'start_deploy_thread' now
        self.btn_deploy = ttk.Button(actions_frame, text="🚀 Compile", command=self.start_deploy_thread, state="disabled")
        self.btn_deploy.pack(fill="x", pady=5)

        # Dynamic Status Banner
        self.lbl_status = tk.Label(
            self.root, text="Ready", font=("Helvetica", 10, "bold"),
            fg="black", bg="#f0f0f0", anchor="w", padx=10, pady=5
        )
        self.lbl_status.pack(fill="x", side="bottom")

    def show_status(self, message, is_error=False):
        color = "#cc0000" if is_error else "#008800"
        self.lbl_status.config(text=message, fg="white", bg=color)
        self.root.update_idletasks()

    def clear_status(self, message="Ready"):
        self.lbl_status.config(text=message, fg="black", bg="#f0f0f0")

    def toggle_connection(self):
        if not self.is_connected:
            self.connect_ssh()
        else:
            self.disconnect_ssh()

    def connect_ssh(self):
        ip = self.ip_var.get()
        user = self.user_var.get()
        ppk_path = self.ppk_var.get()

        if not os.path.exists(ppk_path):
            self.show_status(f"Error: Key file not found ({ppk_path})", is_error=True)
            return

        self.show_status("Connecting...")

        try:
            try:
                key = paramiko.RSAKey.from_private_key_file(ppk_path)
            except paramiko.ssh_exception.SSHException:
                key = paramiko.Ed25519Key.from_private_key_file(ppk_path)
        except Exception as e:
            self.show_status("Error: Unrecognized key format (OpenSSH required)", is_error=True)
            return

        try:
            self.ssh_client = paramiko.SSHClient()
            self.ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            self.ssh_client.connect(hostname=ip, username=user, pkey=key, timeout=10)

            self.is_connected = True
            self.btn_connect.config(text="Disconnect")
            self.btn_setup.config(state="normal")
            self.btn_clean.config(state="normal")
            self.btn_deploy.config(state="normal")
            self.set_inputs_state("disabled")
            
            self.show_status(f"Successfully connected to {user}@{ip}", is_error=False)

        except Exception as e:
            self.ssh_client = None
            self.show_status(f"SSH connection failed", is_error=True)

    def disconnect_ssh(self):
        if self.ssh_client:
            self.ssh_client.close()
        self.is_connected = False
        self.btn_connect.config(text="Connect")
        self.btn_setup.config(state="disabled")
        self.btn_clean.config(state="disabled")
        self.btn_deploy.config(state="disabled")
        self.set_inputs_state("normal")
        self.clear_status("Disconnected from target.")

    def set_inputs_state(self, state):
        self.entry_ip.config(state=state)
        self.entry_user.config(state=state)
        self.entry_ppk.config(state=state)

    # ==========================================================================
    # STREAMING REMOTE COMMAND EXECUTION (REAL-TIME CONSOLE)
    # ==========================================================================
    def execute_remote_command(self, cmd):
        """Executes a command and streams stdout/stderr to the local console in real-time."""
        try:
            transport = self.ssh_client.get_transport()
            channel = transport.open_session()
            channel.exec_command(cmd)

            while True:
                # Check if user requested an abort from the UI thread
                if self.abort_requested:
                    channel.close()
                    sys.stderr.write("\n[LOCAL] Build aborted by user.\n")
                    return -1

                if channel.recv_ready():
                    output = channel.recv(1024).decode("utf-8")
                    sys.stdout.write(output)
                    sys.stdout.flush()
                
                if channel.recv_stderr_ready():
                    errors = channel.recv_stderr(1024).decode("utf-8")
                    sys.stderr.write(errors)
                    sys.stderr.flush()

                if channel.exit_status_ready() and not channel.recv_ready() and not channel.recv_stderr_ready():
                    break

            exit_status = channel.recv_exit_status()
            channel.close()
            return exit_status
        except Exception as e:
            sys.stderr.write(f"Execution Error: {e}\n")
            return -1

    def run_setup(self):
        self.btn_setup.config(state="disabled")
        self.show_status("Installing production environment on Pi (please wait)...")

        cmd_packages = f"tce-load -i {' '.join(REQUIRED_PACKAGES)}"
        cmd_samba = "sudo /usr/local/sbin/smbd -D || true"

        print("\n--- STARTING TARGET SETUP ---")
        status1 = self.execute_remote_command(cmd_packages)
        status2 = self.execute_remote_command(cmd_samba)
        print("--- TARGET SETUP FINISHED ---\n")

        if status1 == 0:
            self.show_status("Target setup completed successfully!", is_error=False)
        else:
            self.show_status(f"Setup finished (Verify console logs, Code {status1})", is_error=True)

        self.btn_setup.config(state="normal")

    # ==========================================================================
    # SMART SFTP SYNCHRONIZATION LOGIC
    # ==========================================================================
    def smart_sftp_sync(self, sftp):
        files_sent = 0
        if not os.path.exists(LOCAL_SYNC_DIR):
            raise FileNotFoundError(f"Local sync directory not found: {LOCAL_SYNC_DIR}")

        self.create_remote_dir_recursive(sftp, REMOTE_PROJECT_DIR)

        for root_dir, dirs, files in os.walk(LOCAL_SYNC_DIR):
            for file in files:
                # Quick check if cancel was requested during SFTP loop
                if self.abort_requested:
                    return files_sent

                local_path = os.path.join(root_dir, file)
                rel_path = os.path.relpath(local_path, LOCAL_SYNC_DIR).replace('\\', '/')
                remote_path = f"{REMOTE_PROJECT_DIR}/{rel_path}"
                remote_dir = os.path.dirname(remote_path)

                local_stat = os.stat(local_path)
                local_size = local_stat.st_size
                local_mtime = int(local_stat.st_mtime)

                need_upload = False
                try:
                    remote_stat = sftp.stat(remote_path)
                    local_mtime = int(os.path.getmtime(local_path))
                    remote_mtime = int(remote_stat.st_mtime)
                    
                    local_size = os.path.getsize(local_path)
                    remote_size = remote_stat.st_size
                
                    # FIX DE DÉTECTION : On synchronise si la taille est différente 
                    # OU si les dates ne sont pas strictement identiques (à 1s près)
                    need_upload = (local_size != remote_size) or (abs(local_mtime - remote_mtime) > 1)
                except IOError:
                    need_upload = True

                if need_upload:
                    self.create_remote_dir_recursive(sftp, remote_dir)
                    self.show_status(f"Sending: {rel_path}...")
                    print(f"Sending: {rel_path}...")
                    sftp.put(local_path, remote_path)
                    sftp.utime(remote_path, (local_mtime, local_mtime))
                    files_sent += 1

        return files_sent

    def create_remote_dir_recursive(self, sftp, remote_dir):
        parts = remote_dir.split('/')
        current = ""
        for part in parts:
            if not part:
                continue
            current += f"/{part}"
            try:
                sftp.mkdir(current)
            except IOError:
                pass 
    
    # ==========================================================================
    # CLEAN PROJECT MULTI-THREADING HANDLERS
    # ==========================================================================
    def start_clean_thread(self):
        """Triggered by UI 'Clean' button. Runs inside a background thread."""
        self.abort_requested = False
        # Disable buttons during operation
        self.btn_clean.config(state="disabled")
        self.btn_deploy.config(state="disabled")
        
        t = threading.Thread(target=self.run_clean, daemon=True)
        t.start()

    def run_clean(self):
        """Executes 'make clean' on the target Pi."""
        self.show_status("Cleaning build artifacts on target...")
        
        print("\n--- REMOTE CLEAN START ---")
        clean_cmd = f"cd {REMOTE_PROJECT_DIR} && make clean"
        status = self.execute_remote_command(clean_cmd)
        print(f"--- REMOTE CLEAN FINISHED (Exit Code: {status}) ---\n")
        
        if status == 0:
            self.show_status("Project cleaned successfully!", is_error=False)
        else:
            self.show_status(f"Clean failed (Code {status})", is_error=True)
            
        # Safely re-enable buttons in the UI main thread
        self.root.after(0, self.restore_after_clean)

    def restore_after_clean(self):
        """Restores the UI buttons safely back in the main thread."""
        self.btn_clean.config(state="normal")
        self.btn_deploy.config(state="normal")
    # ==========================================================================
    # MULTI-THREADING HANDLERS
    # ==============================================================================
    def start_deploy_thread(self):
        """Triggered by UI button. Starts the execution inside a background thread."""
        self.abort_requested = False
        # Switch button appearance immediately to Cancel
        self.btn_deploy.config(text="🛑 Cancel / Abort Build", command=self.request_abort)
        self.btn_clean.config(state="disabled")
        
        # Launch the actual heavy method in a separate thread
        t = threading.Thread(target=self.run_deploy, daemon=True)
        t.start()

    def request_abort(self):
        """Instantly called by UI Main Thread when 'Cancel' is clicked."""
        self.abort_requested = True
        self.show_status("Aborting operations... Please wait.", is_error=True)

    def run_deploy(self):
        """Runs inside the background thread. Keeps UI un-frozen."""
        self.show_status("Analyzing modified files...")

        try:
            sftp = self.ssh_client.open_sftp()
            count = self.smart_sftp_sync(sftp)
            sftp.close()
            
            if self.abort_requested:
                self.show_status("Deployment and build cancelled.", is_error=True)
                return

            self.show_status(f"Sync complete ({count} files synchronized). Building on Pi...")
            
            # --- CORRECTION DE L'HORLOGE DU PI ---
            # On récupère l'heure actuelle de Windows au format local (MMDDhhmmYYYY) requis par la commande 'date' de BusyBox
            now_utc = datetime.utcnow()
            date_str = now_utc.strftime("%Y%m%d%H%M.%S")
            
            # On force le Raspberry Pi à se mettre à l'heure exacte de Windows
            print(f"sudo date -s {date_str}")
            date_status = self.execute_remote_command(f"sudo date -s {date_str}")
            if date_status != 0:
                self.show_status("Clock synchronization failed! Build aborted.", is_error=True)
                print("[LOCAL ERROR] 'sudo date' command failed on target. Check sudo privileges or BusyBox version.")
                return # We stop execution right here, skipping the make command
            # -------------------------------------

            # Trigger Make on target with Real-Time Console Output and multi-core compilation
            print("\n--- REMOTE BUILD START (MULTI-CORE) ---")
            
            # --- LOCAL BUILD ID INCREMENTATION ---
            build_id_path = os.path.join(LOCAL_SYNC_DIR, "build_id")
            build_id = "1" # Default value if file doesn't exist yet
            
            if os.path.exists(build_id_path):
                try:
                    with open(build_id_path, "r") as f:
                        content = f.read().strip()
                        if content.isdigit():
                            build_id = str(int(content) + 1)
                except Exception as e:
                    self.show_status(f"[LOCAL WARNING] Failed to read build_id: {e}", is_error=True)
                    return
    
            # Save the incremented build ID back to Windows local file
            try:
                with open(build_id_path, "w") as f:
                    f.write(build_id)
                print(f"[LOCAL] Build ID updated to: {build_id}")
            except Exception as e:
                print(f"[LOCAL WARNING] Failed to write build_id: {e}")
            # -------------------------------------
    
            # Inject BUILD_ID directly into the remote make command
            build_cmd = f"cd {REMOTE_PROJECT_DIR} && make -j$(grep -c '^processor' /proc/cpuinfo) BUILD_ID={build_id}"
            
            status = self.execute_remote_command(build_cmd)
            print(f"--- REMOTE BUILD FINISHED (Exit Code: {status}) ---\n")
                        
            if self.abort_requested:
                self.show_status("Deployment and build cancelled.", is_error=True)
            elif status == 0:
                self.show_status("Deployment & Compilation successful!", is_error=False)
            else:
                self.show_status(f"Compilation failed (Code {status})", is_error=True)

        except Exception as e:
            self.show_status(f"SFTP transfer error: {e}", is_error=True)
            
        finally:
            # Restore button state cleanly back inside the UI loop safely
            self.root.after(0, self.restore_deploy_button)

    def restore_deploy_button(self):
        """Restores the button safely back in the main UI thread."""
        self.btn_deploy.config(text="🚀 Compile", command=self.start_deploy_thread)
        self.btn_deploy.config(state="normal")
        self.btn_clean.config(state="normal")


if __name__ == "__main__":
    root = tk.Tk()
    app = PiCoreDeployerApp(root)
    root.mainloop()