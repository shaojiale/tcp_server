#include "tcpserver.hpp"


class MyServer : public TcpServer
{
public:
	//客户端加入事件
	virtual void OnNetJoin(TcpClient* pClient)
	{
		_clientCount++;
		printf("client<%d> join\n", (int)pClient->sockfd());
	}
	//客户端离开事件
	virtual void OnNetLeave(TcpClient* pClient)
	{
		_clientCount++;
		printf("client<%d> leave\n", (int)pClient->sockfd());
	}
	//响应网络消息
	virtual void OnNetMsg(TcpClient* pClient, Dataheader* header)
	{
		_recvCount++;
		//正常处理数据
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			//printf("收到Socket<%d>命令：CMD_LOGIN 数据长度：%d\n", (int)csock, header->dataLength);
			//Login *login = (Login *)header;
			//判断用户密码是否正确的过程
			//printf("登录用户名：%s 登录用户密码：%s\n", login->userName, login->passWord);
			//LoginResult ret;
			//pClient->SendData(&ret);
			break;
		}
		case CMD_LOGOUT:
		{
			//printf("收到Socket<%d>命令：CMD_LOGOUT 数据长度：%d\n", (int)csock, header->dataLength);
			//Logout *logout = (Logout *)header;
			//printf("登出用户名：%s \n", logout->userName);
			//LogoutResult ret;
			//pClient->SendData(&ret);
			break;
		}
		case CMD_ERROR:
		{
			printf("收到<客户端%d>错误消息，数据长度：%d\n", (int)pClient->sockfd(), header->dataLength);
			break;
		}
		default:
		{
			printf("收到<客户端%d>未知消息，数据长度：%d\n", (int)pClient->sockfd(), header->dataLength);
			break;
		}
		}
	}
private:

};


int main()
{  
	MyServer server;
	server.InitSocket();
	server.Bind(nullptr, 1245);
	server.Listen(20);
	server.Start(6);
	while (true)
	{
		server.OnRun();
	}
	server.Close();
	return 0;
}