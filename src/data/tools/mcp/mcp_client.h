#include "mcp_client.h"


struct mcp_client
{
	// TODO: implement MCP handshake + tool discovery.
	void connect(const std::string & /*endpoint*/)
	{
	}

	void register_discovered_tools(tool_registry & /*registry*/)
	{
	}
};