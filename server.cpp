#include "tcpserver.hpp"

int main()
{  
	TcpServer server;
	server.InitSocket();
	server.Bind(nullptr, 1245);
	server.Listen(20);
	while (server.isRun())
	{
		server.OnRun();
	}
	server.Close();
	return 0;
}