import asyncio
import httpx
import logging
from mcp.server import Server
from mcp.server.stdio import stdio_server
from mcp.types import Tool, TextContent

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("teraterm_mcp")

app = Server("teraterm-mcp")
API_BASE_URL = "http://127.0.0.1:8000"

@app.list_tools()
async def list_tools() -> list[Tool]:
    """List available Tera Term tools."""
    return [
        Tool(
            name="teraterm_connect",
            description="Connect to a target device (serial COM port or SSH) via Tera Term. Always call this first before sending commands.",
            inputSchema={
                "type": "object",
                "properties": {
                    "target": {
                        "type": "string",
                        "description": "The target to connect to (e.g., 'COM3', '192.168.1.1', 'yamaha_router')"
                    },
                    "connection_type": {
                        "type": "string",
                        "enum": ["serial", "ssh"],
                        "description": "The type of connection ('serial' or 'ssh')"
                    },
                    "baud_rate": {
                        "type": "integer",
                        "description": "Baud rate for serial connections (default: 9600)"
                    },
                    "encoding": {
                        "type": "string",
                        "description": "Character encoding (e.g., 'UTF-8', 'SJIS')"
                    }
                },
                "required": ["target", "connection_type"]
            }
        ),
        Tool(
            name="teraterm_read_buffer",
            description="Read the current output buffer from Tera Term. Use this to monitor the terminal output and check the prompt state before sending commands.",
            inputSchema={
                "type": "object",
                "properties": {}
            }
        ),
        Tool(
            name="teraterm_send_command",
            description="Inject a command into the Tera Term session. Wait for the command to complete and return the output.",
            inputSchema={
                "type": "object",
                "properties": {
                    "command": {
                        "type": "string",
                        "description": "The command to execute"
                    },
                    "wait_for_prompt": {
                        "type": "boolean",
                        "description": "Whether to wait for the command prompt to return (default: true)"
                    }
                },
                "required": ["command"]
            }
        ),
        Tool(
            name="teraterm_disconnect",
            description="Disconnect the current Tera Term session.",
            inputSchema={
                "type": "object",
                "properties": {}
            }
        )
    ]

@app.call_tool()
async def call_tool(name: str, arguments: dict) -> list[TextContent]:
    """Handle tool execution requests."""
    try:
        async with httpx.AsyncClient() as client:
            if name == "teraterm_connect":
                response = await client.post(f"{API_BASE_URL}/connect", json=arguments)
                response.raise_for_status()
                result = response.json()
                return [TextContent(type="text", text=result.get("message", str(result)))]

            elif name == "teraterm_read_buffer":
                response = await client.get(f"{API_BASE_URL}/read")
                response.raise_for_status()
                result = response.json()
                output = result.get("output", "")
                if not output:
                    output = "[Buffer empty]"
                return [TextContent(type="text", text=output)]

            elif name == "teraterm_send_command":
                response = await client.post(f"{API_BASE_URL}/send", json=arguments)
                response.raise_for_status()
                result = response.json()
                return [TextContent(type="text", text=result.get("output", str(result)))]

            elif name == "teraterm_disconnect":
                response = await client.post(f"{API_BASE_URL}/disconnect")
                response.raise_for_status()
                result = response.json()
                return [TextContent(type="text", text=result.get("message", str(result)))]

            else:
                raise ValueError(f"Unknown tool: {name}")

    except httpx.ConnectError:
        error_msg = "Error: Could not connect to the Tera Term API server. Ensure it is running on port 8000."
        logger.error(error_msg)
        return [TextContent(type="text", text=error_msg)]
    except httpx.HTTPStatusError as e:
        error_msg = f"API returned an error: {e.response.status_code} - {e.response.text}"
        logger.error(error_msg)
        return [TextContent(type="text", text=error_msg)]
    except Exception as e:
        error_msg = f"Error executing tool {name}: {str(e)}"
        logger.error(error_msg)
        return [TextContent(type="text", text=error_msg)]

async def main_async():
    """Run the MCP server."""
    logger.info("Starting Tera Term MCP Server over stdio")
    async with stdio_server() as (read_stream, write_stream):
        await app.run(
            read_stream,
            write_stream,
            app.create_initialization_options()
        )

def main():
    asyncio.run(main_async())

if __name__ == "__main__":
    main()
