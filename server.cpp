#include "tcpserver.hpp"

int main()
{  
	tcpserver server;
	server.InitSocket();
	server.Bind(nullptr,3000);
	server.Listen(20);
	while (server.isRun())
	{
		server.OnRun();
	}
	server.Close();
	return 0;
}