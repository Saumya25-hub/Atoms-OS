import os
import json
import time
import subprocess
from datetime import datetime
import threading
from abc import ABC, abstractmethod

class CliBackend(ABC):
    @abstractmethod
    def trigger_investigation(self, task_file, log_callback): pass
    
    @abstractmethod
    def trigger_verification(self, report_file, log_callback): pass

class SubprocessCliBackend(CliBackend):
    def __init__(self, cli_path):
        self.cli_path = cli_path
        self.active_process = None

    def _run_worker(self, prompt, role, log_callback):
        log_callback(f"[{role}] Starting CLI Worker process...")
        start_time = time.time()
        
        cmd = [
            self.cli_path,
            "chat",
            "-m", "agent",
            prompt
        ]
        
        try:
            self.active_process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                cwd=os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
            )
            
            pid = self.active_process.pid
            log_callback(f"[{role}] Process spawned with PID: {pid}")
            
            stdout, stderr = self.active_process.communicate()
            end_time = time.time()
            exit_code = self.active_process.returncode
            
            duration = end_time - start_time
            log_callback(f"[{role}] Process exited with code {exit_code} after {duration:.2f} seconds.")
            
            if stdout:
                log_callback(f"[{role}] STDOUT:\n{stdout}")
            if stderr:
                log_callback(f"[{role}] STDERR:\n{stderr}")
                
        except Exception as e:
            log_callback(f"[{role}] Error executing subprocess: {e}")
        finally:
            self.active_process = None

    def trigger_investigation(self, task_file, log_callback):
        prompt = f"/goal You are the CLI Investigator. Read {task_file}. Perform deep Git/codebase forensic investigation. Do NOT modify source code. Write your findings to CLI_REPORT.md containing BRIDGE_STATUS: INVESTIGATION_COMPLETE."
        # Run in a separate thread so daemon polling isn't blocked completely, 
        # though state machine ensures we only trigger this once.
        t = threading.Thread(target=self._run_worker, args=(prompt, "INVESTIGATOR", log_callback))
        t.daemon = True
        t.start()

    def trigger_verification(self, verify_file, log_callback):
        prompt = f"/goal You are the CLI Verifier. Read {verify_file}. Verify the implementation matches the plan. Write your findings back to VERIFICATION_REPORT.md containing BRIDGE_STATUS: VERIFICATION_COMPLETE."
        t = threading.Thread(target=self._run_worker, args=(prompt, "VERIFIER", log_callback))
        t.daemon = True
        t.start()

    def trigger_implementation(self, report_file, impl_file, log_callback):
        prompt = f"/goal You are the IDE Implementer. Read {report_file} and execute the implementation. Write your findings to {impl_file} containing BRIDGE_STATUS: IMPLEMENTATION_COMPLETE."
        t = threading.Thread(target=self._run_worker, args=(prompt, "IMPLEMENTER", log_callback))
        t.daemon = True
        t.start()


class BridgeDaemon:
    def __init__(self, bridge_dir):
        self.bridge_dir = bridge_dir
        self.config_path = os.path.join(bridge_dir, "bridge_config.json")
        self.state_path = os.path.join(bridge_dir, "bridge_state.json")
        
        with open(self.config_path, 'r') as f:
            self.config = json.load(f)
            
        self.log_path = os.path.join(bridge_dir, self.config["log_file"])
        self.task_file = os.path.join(bridge_dir, self.config["task_file"])
        self.report_file = os.path.join(bridge_dir, self.config["report_file"])
        self.impl_file = os.path.join(bridge_dir, self.config["impl_file"])
        self.verify_file = os.path.join(bridge_dir, self.config["verify_file"])
        
        self.load_state()
        
        cli_executable = r"C:\Users\Saumya Chaudhari\AppData\Local\Programs\Antigravity IDE\bin\antigravity-ide.cmd"
        self.backend = SubprocessCliBackend(cli_executable)

    def log(self, message):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        log_entry = f"[{timestamp}] {message}\n"
        with open(self.log_path, 'a') as f:
            f.write(log_entry)
        print(log_entry.strip())

    def load_state(self):
        if os.path.exists(self.state_path):
            with open(self.state_path, 'r') as f:
                self.state = json.load(f)
        else:
            self.state = {"current_state": "IDLE", "task_id": 0, "last_update": time.time()}
            self.save_state()

    def save_state(self):
        temp_path = self.state_path + ".tmp"
        self.state["last_update"] = time.time()
        with open(temp_path, 'w') as f:
            json.dump(self.state, f, indent=4)
        os.replace(temp_path, self.state_path)

    def transition(self, new_state):
        self.log(f"State transition: {self.state['current_state']} -> {new_state}")
        self.state['current_state'] = new_state
        self.save_state()

    def check_file_for_status(self, file_path, expected_status):
        if not os.path.exists(file_path):
            return False
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                return f"BRIDGE_STATUS: {expected_status}" in content
        except Exception as e:
            self.log(f"Error reading {file_path}: {e}")
            return False

    def remove_status_from_file(self, file_path, status):
        if not os.path.exists(file_path):
            return
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            content = content.replace(f"BRIDGE_STATUS: {status}", f"BRIDGE_STATUS: PROCESSED_{status}")
            
            temp_path = file_path + ".tmp"
            with open(temp_path, 'w', encoding='utf-8') as f:
                f.write(content)
            os.replace(temp_path, file_path)
        except Exception as e:
            self.log(f"Error updating {file_path}: {e}")

    def run(self):
        self.log("AI_BRIDGE Subprocess Daemon started.")
        while True:
            try:
                self.poll()
            except Exception as e:
                self.log(f"Exception in polling loop: {e}")
            time.sleep(self.config["poll_interval_seconds"])

    def poll(self):
        current_state = self.state["current_state"]

        if current_state == "IDLE":
            if self.check_file_for_status(self.task_file, "TASK_READY"):
                self.remove_status_from_file(self.task_file, "TASK_READY")
                self.state["task_id"] += 1
                self.transition("TASK_READY")
                
        elif current_state == "TASK_READY":
            self.log(f"Triggering automated Investigator for Task {self.state['task_id']}...")
            self.backend.trigger_investigation(self.task_file, self.log)
            self.transition("CLI_INVESTIGATING")
            
        elif current_state == "CLI_INVESTIGATING":
            if self.check_file_for_status(self.report_file, "INVESTIGATION_COMPLETE"):
                self.remove_status_from_file(self.report_file, "INVESTIGATION_COMPLETE")
                self.transition("REPORT_READY")
                
        elif current_state == "REPORT_READY":
            self.log(f"Triggering automated Implementer for Task {self.state['task_id']}...")
            self.backend.trigger_implementation(self.report_file, self.impl_file, self.log)
            self.transition("IDE_IMPLEMENTING")
            
        elif current_state == "IDE_IMPLEMENTING":
            # Backward compatibility state, fall through
            if self.check_file_for_status(self.impl_file, "IMPLEMENTATION_COMPLETE"):
                self.remove_status_from_file(self.impl_file, "IMPLEMENTATION_COMPLETE")
                self.transition("IMPLEMENTATION_READY")
                
        elif current_state == "IMPLEMENTATION_READY":
            self.log(f"Triggering automated Verifier for Task {self.state['task_id']}...")
            self.backend.trigger_verification(self.verify_file, self.log)
            self.transition("CLI_VERIFYING")
            
        elif current_state == "CLI_VERIFYING":
            if self.check_file_for_status(self.verify_file, "VERIFICATION_COMPLETE"):
                self.remove_status_from_file(self.verify_file, "VERIFICATION_COMPLETE")
                self.transition("VERIFICATION_READY")
                
        elif current_state == "VERIFICATION_READY":
            self.log("Workflow complete. Returning to IDLE.")
            self.transition("COMPLETE")
            
        elif current_state == "COMPLETE":
            self.transition("IDLE")

if __name__ == "__main__":
    bridge_dir = os.path.dirname(os.path.abspath(__file__))
    daemon = BridgeDaemon(bridge_dir)
    daemon.run()
