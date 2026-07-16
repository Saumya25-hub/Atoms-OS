import subprocess
import time
import sys
import os

print("Starting autonomous subprocess test...")
cmd = [
    r"C:\Users\Saumya Chaudhari\AppData\Local\Programs\Antigravity IDE\bin\antigravity-ide.cmd",
    "chat",
    "-m", "agent",
    "Write exactly the word 'SUCCESS' to a new file named AI_BRIDGE_TEST.txt in the current directory, then exit."
]

start_time = time.time()
try:
    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        cwd=os.path.abspath(os.path.dirname(__file__))
    )
    
    print(f"Process spawned with PID {process.pid}")
    
    # Wait for completion or timeout
    try:
        stdout, stderr = process.communicate(timeout=30)
        exit_code = process.returncode
        end_time = time.time()
        
        print(f"Process exited with code {exit_code} after {end_time - start_time:.2f}s")
        print(f"Captured STDOUT: {stdout.strip()}")
        print(f"Captured STDERR: {stderr.strip()}")
        
        # Check if file was created
        if os.path.exists("AI_BRIDGE_TEST.txt"):
            print("TEST PASSED: File was created autonomously.")
            with open("AI_BRIDGE_TEST.txt", 'r') as f:
                print(f"File content: {f.read()}")
        else:
            print("TEST FAILED: Process exited but file was NOT created. The command did not run autonomously or exit deterministically.")
            
    except subprocess.TimeoutExpired:
        print("TEST FAILED: Process timed out after 30 seconds.")
        process.kill()
        
except Exception as e:
    print(f"TEST FAILED: Exception occurred: {e}")
