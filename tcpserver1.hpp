#ifndef  _EASYTCPSERVER_HPP_

#define  _EASYTCPSERVER_HPP_
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<windows.h>
#include<WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include<unistd.h> //uni std
#include<arpa/inet.h>
#include<string.h>

#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif  //end define _win32
#include <stdio.h>
#include <vector>
#include "MessageHeader.hpp"

class EasyTcpServer
{
	SOCKET _sock;
	std::vector<SOCKET> g_clients;
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTcpServer()
	{
		Close();
	}
	//初始化socket
	void InitSocket()
	{
#ifdef _WIN32
		//启动Windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		//初始化_sock 如果含有旧链接，关闭旧链接 之后再建立socket
		if (INVALID_SOCKET != _sock)
		{
			printf("<socket=%d>关闭旧连接...\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("错误，建立Socket失败...\n");
		}
		else {
			printf("建立Socket成功...\n");
		}
	}



	void bindPort(short port)
	{
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);//主机转网络字节地址
		_sin.sin_addr.S_un.S_addr = INADDR_ANY; //接收
		if (bind(_sock, (sockaddr*)&_sin, sizeof(_sin)) == SOCKET_ERROR)
		{
			printf("绑定网络端口失败！\n");
		}
		else {
			printf("绑定网络端口成功！\n");
		}
	}

	void listenPort()
	{
		if (SOCKET_ERROR == listen(_sock, 5))
		{
			printf("监听失败！\n");
		}
		else {
			printf("监听成功！\n");
		}

	}
	//关闭套节字closesocket
	void Close()
	{
		//如果是有效的_sock
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (int n = (int)g_clients.size(); n >= 0; n--)
			{
				closesocket(g_clients[n]);
			}
			closesocket(_sock);
			//清除Windows socket环境
			WSACleanup();
#else
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
	}

	int processor(SOCKET _cSock)
	{
		char szRecv[4096] = {};
		printf("当前<服务端>套接字%d，processor start.... \n", (int)_cSock);
		int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)szRecv;
		if (nLen <= 0)
		{
			printf("当前<服务端>套接字%d，processor end.... ,客户端退出!\n", (int)_cSock);
			return -1;
		}
		//处理请求
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			printf("命令 CMD_LOGIN,客户端!!登录!\n");
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLth - sizeof(DataHeader), 0);
			Login* login = (Login*)szRecv;
			printf("用户姓名：%s 用户密码：%s\n", login->usrName, login->passWord);
			LoginRst inRst;
			inRst.rst = 1000;
			send(_cSock, (char*)&inRst, sizeof(inRst), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			printf("命令 CMD_LOGOUT,客户端登出!!!\n");
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLth - sizeof(DataHeader), 0);
			Logout* logout = (Logout*)szRecv;
			printf("用户姓名：%s", logout->usrName);
			LogoutRst outRst;
			outRst.rst = -1000;
			send(_cSock, (char*)&outRst, sizeof(outRst), 0);
		}
		break;
		}
		printf("当前<服务端>套接字%d，processor end.... \n", (int)_cSock);
		return 0;
	}

	//处理网络消息
	bool OnRun()
	{
		if (isRun())
		{
			fd_set fdsRead;
			fd_set fdsWrite;
			fd_set fdsExp;

			FD_ZERO(&fdsRead);
			FD_ZERO(&fdsWrite);
			FD_ZERO(&fdsExp);

			FD_SET(_sock, &fdsRead);
			FD_SET(_sock, &fdsWrite);
			FD_SET(_sock, &fdsExp);//创建3种的fds用来检测是否有链接

								   //如果g_clients中含有套接字的话，将他添加进来
			for (int n = 0; n < (int)g_clients.size(); n++)
			{
				FD_SET(g_clients[n], &fdsRead);
			}

			timeval t = { 10,0 };

			int ret = select(_sock + 1, &fdsRead, &fdsWrite, &fdsExp, &t);

			if (ret < 0)
			{
				printf("select模型中没用可用套接字,客户端已退出!!!\n");
				return false;
			}

			if (FD_ISSET(_sock, &fdsRead))
			{
				FD_CLR(_sock, &fdsRead);
				//等待socket
				sockaddr_in _clientAddr = {};
				int nAddrLen = sizeof(sockaddr_in);
				SOCKET _cSock = INVALID_SOCKET;
				_cSock = accept(_sock, (sockaddr*)&_clientAddr, &nAddrLen);
				if (_cSock == INVALID_SOCKET)
				{
					printf("当前是无效客户端套接字!!!\n");
				}
				for (int n = (int)g_clients.size() - 1; n >= 0; n--)
				{
					NewUserJoin userjoin;
					userjoin.sock = (int)g_clients[n];
					printf("<服务端向客户端发送newuserjoin = %d>\n", userjoin.sock);
					send(g_clients[n], (char*)&userjoin, sizeof(userjoin), 0);
				}
				g_clients.push_back(_cSock);
				printf("新的客户端：socket = %d,IP = %s \n", int(_cSock), inet_ntoa(_clientAddr.sin_addr));
			}

			for (int n = 0; n <= (int)(fdsRead.fd_count - 1); n++)
			{
				if (processor(fdsRead.fd_array[n]) == -1)
				{
					auto iter = find(g_clients.begin(), g_clients.end(), fdsRead.fd_array[n]);
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

	//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}


private:

};


#endif//endif _EASYTCPSERVER_HPP_