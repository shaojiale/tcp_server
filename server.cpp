

#include "Alloc.h"
#include "tcpserver.hpp"
#include <mysql.h>
bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("退出cmdThread线程\n");
			break;
		}
		else {
			printf("不支持的命令。\n");
		}
	}
}
MYSQL mysql;    //一个数据库结构体
class MyServer : public TcpServer
{
public:
	//客户端加入事件
	virtual void OnNetJoin(ClientSocketPtr& pClient)
	{
		TcpServer::OnNetJoin(pClient);
		//printf("client<%d> join\n", (int)pClient->sockfd());
	}
	//客户端离开事件
	virtual void OnNetLeave(ClientSocketPtr& pClient)
	{
		TcpServer::OnNetLeave(pClient);
		printf("client<%d> leave\n", (int)pClient->sockfd());
	}
	//响应网络消息
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocketPtr& pClient, Dataheader* header)
	{
		TcpServer::OnNetMsg(pCellServer, pClient, header);
		//正常处理数据
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			printf("收到客户端<Socket=%d>请求：CMD_LOGIN,数据长度：%d,userName=%s PassWord=%s\n",\
				pClient->sockfd(), login->dataLength, login->userName, login->passWord);
			char insertsql[1024];
			int len = sprintf_s(insertsql, 256, "INSERT INTO userinformation values ('%s','%s');", \
				login->userName, login->passWord);
			mysql_query(&mysql, insertsql);

			auto ret = std::make_shared<LoginResult>();
			pCellServer->addSendTask(pClient,(DataHeaderPtr&)ret);
			//pClient->SendData(ret);
			break;
		}
		case CMD_LOGOUT:
		{
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
	virtual void OnNetRecv(TcpClient * pClient)
	{
		_recvCount++;
	}
private:

};


int main()
{  

	mysql_init(&mysql);
	//设置编码方式
	mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "gbk");
	//连接数据库
	//判断如果连接失败就输出连接失败。
	if (mysql_real_connect(&mysql, "localhost", "root", "sjl@142010", "test", 3306, NULL, 0) == NULL)
		printf("连接失败！\\n");
	mysql_query(&mysql, "use users");
	MyServer server;
	server.InitSocket();
	server.Bind(nullptr, 1245);
	server.Listen(20);
	server.Start(6);

	//启动UI线程
	std::thread t1(cmdThread);
	t1.detach();

	while (g_bRun)
	{
		server.OnRun();
	}
	server.Close();
	printf("已退出。\n");
	getchar();
	return 0;
}