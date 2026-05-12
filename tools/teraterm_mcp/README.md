# Tera Term MCP Plugin for Claude Code

This is a Claude Code plugin that provides a Model Context Protocol (MCP) server to interact with Tera Term. It allows Claude to automate interactions with target devices (such as Linux servers or Yamaha routers) over serial COM ports or SSH, acting as a HARNESS (test, execution, and monitoring framework).

## Architecture

This plugin uses a two-tier architecture to prevent locking the COM port and to isolate operations:
1. **API Server (`api_server.py`)**: A FastAPI server that runs in the background. It maintains the state of the Tera Term session and interacts with the terminal (using mocking in this initial version).
2. **MCP Server (`mcp_server.py`)**: An MCP standard server that exposes tools to Claude Code and communicates with the background API Server.

## Features

- **Jules Agent**: A specialized Claude Code Subagent designed to dynamically route skills based on the terminal prompt context (e.g., distinguishing between a Linux shell and a Yamaha router prompt).
- **Tools**:
  - `teraterm_connect`: Establish a connection.
  - `teraterm_read_buffer`: Monitor the output and prompt state.
  - `teraterm_send_command`: Inject a command into the terminal.
  - `teraterm_disconnect`: Safely terminate the connection.

## Installation and Usage

To test and use this plugin locally with Claude Code:

1. Install the required Python dependencies in your environment:
   ```bash
   pip install fastapi uvicorn mcp httpx pydantic
   ```

2. Run Claude Code with the plugin directory:
   ```bash
   claude --plugin-dir tools/teraterm_mcp
   ```

3. Once loaded, you can call the subagent:
   ```
   /jules-teraterm Please connect to the yamaha router on COM3 and check the configuration.
   ```
   Or use the tools directly if interacting with the main Claude agent.

## Future Enhancements
- Replace the mocked API responses with actual `ttpmacro.exe` execution or DDE/named-pipe communication with the Tera Term process.
- Implement robust character encoding handling (SJIS/UTF-8 conversions) within the API server.
