#ifndef _TCPSERVER_HPP
#define _TCPSERVER_HPP
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<windows.h>
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib ")
#endif  //end define _win32

#include <stdio.h>
#include <vector>
#include "MessageHeader.hpp"


class tcpserver
{
private:
	SOCKET _sock;
	std::vector<SOCKET> g_clients;

public:
	tcpserver()
	{
		_sock = INVALID_SOCKET;
	}
	virtual ~tcpserver()
	{
		Close();
	}
	//初始化Socket
	void InitSocket()
	{
		//启动winsocket2.2
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData))
		{
			printf("Error:启动环境失败 failed!\n");
		}
		if (INVALID_SOCKET != _sock)
		{
			printf("<Socket:%d>关闭旧连接......\n", (int)_sock);
			Close();
		}
		/* 创建套接服务字 */
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("Error:建立Socket失败......\n");
		}
		else
		{
			printf("<Socket:%d>建立成功......\n", (int)_sock);
		}
	}
	//绑定端口号
	void Bind(const char *ip, unsigned short port)
	{
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port); //端口为3000
		//接受任意IP
		if (ip)
		{
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
		/* 绑定服务器套接字 */
		int ret = bind(_sock, (sockaddr *)&_sin, sizeof(sockaddr));
		if (SOCKET_ERROR == ret)
		{
			printf("ERROR:绑定端口<%d>失败......\n", port);
		}
		else
		{
			printf("绑定端口<%d>成功！\n", port);
		}
	}
	//监听端口号
	void Listen(int n)
	{
		/* 监听端口 */

		int ret = listen(_sock, n);
		if (SOCKET_ERROR == n)
		{
			printf("ERROR:Listen failed！\n");
		}
		printf("linstening:%dport...\n", n);
	}
	//关闭Socket
	void Close()
	{
		if (INVALID_SOCKET != _sock)
		{
			for (int n = (int)g_clients.size(); n >= 0; n--)
			{

				closesocket(g_clients[n]);
			}
			closesocket(_sock);
			WSACleanup();
		}
		_sock = INVALID_SOCKET;
	}
	//接受客户端连接
	int Accept()
	{
		//等待接受客户端连接
		sockaddr_in cliAddr = {};
		int len = sizeof(sockaddr_in);
		SOCKET _csock = INVALID_SOCKET;
		_csock = accept(_sock, (sockaddr *)&cliAddr, &len);
		if (INVALID_SOCKET == _csock)
		{
			printf("ERROR:accept<Socket:%d> failed！\n", (int)_csock);
		}
		g_clients.push_back(_csock);
		printf("Connected：Socket<%d> IP:%s Port：%d Connected success！！！\r\n",
			   (int)_csock, inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
		NewUserJoin userjoin;
		SendDataToAll(&userjoin);
		return _csock;
	}
	//接受数据 处理粘包拆包
	int RecvData(SOCKET _csock)
	{
		//接受缓存
		char szRecv[4096] = {};
		int cnt = recv(_csock, szRecv, sizeof(Dataheader), 0);
		Dataheader *header = (Dataheader *)szRecv;
		if (cnt <= 0)
		{
			printf("errno is: %d\n", errno);
			printf("客户端<Socket%d>已经退出，任务结束\n", (int)_csock);
			return -1; //跳出接收循环
		}
		recv(_csock, szRecv + sizeof(header), header->dataLength - sizeof(header), 0);
		OnNetMsg(_csock, header);
		return 0;
	}
	//响应网络消息
	virtual void OnNetMsg(SOCKET _csock, Dataheader *header)
	{
		//正常处理数据
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			printf("收到Socket<%d>命令：CMD_LOGIN 数据长度：%d\n", (int)_csock, header->dataLength);
			Login *login = (Login *)header;
			//判断用户密码是否正确的过程
			printf("登录用户名：%s 登录用户密码：%s\n", login->userName, login->passWord);
			LoginResult ret;
			ret.result = 1;
			SendData(_csock, &ret);
			break;
		}
		case CMD_LOGOUT:
		{
			printf("收到Socket<%d>命令：CMD_LOGOUT 数据长度：%d\n", (int)_csock, header->dataLength);
			Logout *logout = (Logout *)header;
			printf("登出用户名：%s \n", logout->userName);
			LogoutResult ret;
			ret.result = 2;
			SendData(_csock, &ret);
			break;
		}
		default:
		{
			printf("收到Socket<%d>错误命令...\n", (int)_csock);
			header->cmd = CMD_ERROR;
			header->dataLength = 0;
			SendData(_csock, header);
			break;
		}
		}
	}
	//处理网络消息
	//运用到非阻塞模型
	//多路复用IO
	bool OnRun()
	{
		if (isRun())
		{
			fd_set fdRead;
			fd_set fdWrite;
			fd_set fdExp;

			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);

			FD_SET(_sock, &fdRead);
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExp);

			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				FD_SET(g_clients[n], &fdRead);
			}
			//select模型
			//第一个参数ndfs是一个整数，是fd_set集合中所有描述符(socket)的范围，而不是数量
			//描述符(socket是一个整数)
			//windows中可以写0
			//第五个参数超时时间：NULL为阻塞模式，时间为最大时间
			timeval timeout = {1, 0};
			int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &timeout);
			if (ret < 0)
			{
				printf("select任务已经退出，任务结束\n");
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				//等待接受客户端连接
				Accept();
			}
			for (int n = (int)fdRead.fd_count - 1; n >= 0; n--)
			{

				if (-1 == RecvData(fdRead.fd_array[n]))
				{
					auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[n]);
					if (iter != g_clients.end())
					{
						g_clients.erase(iter);
					}
				}
			}
			return true;
		}
		return false;
	}
	//socket有效
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//发送数据
	int SendData(SOCKET _csock, Dataheader *header)
	{
		if (isRun() && header)
		{
			return send(_csock, (char *)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	void SendDataToAll(Dataheader *header)
	{
		for (int n = (int)g_clients.size() - 1; n >= 0; n--)
		{
			SendData(g_clients[n], header);
		}
	}

private:
};

#endif // !_TCPSERVER_HPP
