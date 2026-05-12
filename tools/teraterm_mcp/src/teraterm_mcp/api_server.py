import asyncio
import os
import subprocess
import time
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import uvicorn
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("teraterm_api")

app = FastAPI(title="Tera Term API Server")

# In a real implementation, this would interact with a running Tera Term instance
# via ttpmacro.exe, COM ports, named pipes, or file-based I/O.
# For this harness phase, we provide a mock state manager to establish the API structure.

class SessionState:
    def __init__(self):
        self.connected = False
        self.connection_info = {}
        self.buffer = []
        self.last_command = ""

state = SessionState()

class ConnectRequest(BaseModel):
    target: str # e.g., COM3, or SSH target
    baud_rate: int = 9600
    encoding: str = "UTF-8"
    connection_type: str = "serial" # "serial" or "ssh"

class CommandRequest(BaseModel):
    command: str
    wait_for_prompt: bool = True
    timeout: int = 10

class CommandResponse(BaseModel):
    output: str
    prompt_found: bool

@app.post("/connect")
async def connect(req: ConnectRequest):
    if state.connected:
        return {"status": "error", "message": "Already connected to " + state.connection_info.get("target", "unknown")}

    # Mocking connection
    state.connected = True
    state.connection_info = req.dict()

    # Initialize mock buffer based on target (Yamaha or Linux)
    if "192.168." in req.target or "yamaha" in req.target.lower():
        state.buffer.append("Yamaha Router RT Series\r\nPassword: ")
    else:
        state.buffer.append("Ubuntu 22.04 LTS\r\njules@ubuntu:~$ ")

    logger.info(f"Connected to {req.target} via {req.connection_type}")
    return {"status": "success", "message": f"Connected to {req.target}"}

@app.post("/send")
async def send_command(req: CommandRequest):
    if not state.connected:
        raise HTTPException(status_code=400, detail="Not connected to any target")

    state.last_command = req.command
    logger.info(f"Sending command: {req.command}")

    # Mock command execution and output generation
    output = f"{req.command}\r\n"

    if req.command.strip() == "show config":
        output += "ip route default gateway 192.168.1.1\r\nip lan1 address 192.168.1.254/24\r\n"
    elif req.command.strip() == "administrator":
        output += "Password: "
    elif req.command.strip() == "ls -la":
        output += "total 12\r\ndrwxr-xr-x 2 jules jules 4096 May 11 10:00 .\r\ndrwxr-xr-x 3 root  root  4096 May 11 10:00 ..\r\n-rw-r--r-- 1 jules jules  123 May 11 10:00 config.txt\r\n"
    else:
        output += f"Mock execution of {req.command}\r\n"

    # Append prompt
    if "yamaha" in state.connection_info.get("target", "").lower():
        prompt = "# " if req.command.strip() == "administrator" else "> "
    else:
        prompt = "jules@ubuntu:~$ "

    output += prompt

    # In a real implementation, we would wait and read from Tera Term
    await asyncio.sleep(0.5) # Simulate network/processing delay

    return CommandResponse(output=output, prompt_found=True)

@app.get("/read")
async def read_buffer():
    if not state.connected:
        raise HTTPException(status_code=400, detail="Not connected")

    # Return what we have in the buffer (mock)
    if state.buffer:
        content = "".join(state.buffer)
        state.buffer = [] # Clear buffer after reading
        return {"output": content}
    return {"output": ""}

@app.post("/disconnect")
async def disconnect():
    if not state.connected:
        return {"status": "success", "message": "Already disconnected"}

    target = state.connection_info.get("target", "unknown")
    state.connected = False
    state.connection_info = {}
    state.buffer = []

    logger.info(f"Disconnected from {target}")
    return {"status": "success", "message": f"Disconnected from {target}"}

@app.get("/status")
async def get_status():
    return {
        "connected": state.connected,
        "connection_info": state.connection_info
    }

def main():
    logger.info("Starting Tera Term API Server on port 8000")
    uvicorn.run("teraterm_mcp.api_server:app", host="127.0.0.1", port=8000, reload=False)

if __name__ == "__main__":
    main()
