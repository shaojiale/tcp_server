#ifndef _TCPSERVER_HPP
#define _TCPSERVER_HPP
#ifdef _WIN32
#define FD_SETSIZE 10024
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
#include "CELLTimestamp.hpp"
//缓冲区最小单元大小
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

class TcpClient
{
private:
	SOCKET _sockfd;
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	int _lastPos = 0;
public:
	TcpClient(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}
	~TcpClient()
	{

	}
	SOCKET sockfd()
	{
		return _sockfd;
	}
	char* msgBuf()
	{
		return _szMsgBuf;
	}
	int  getLastPos()
	{
		return _lastPos;
	}
	void  setLastPos(int n)
	{
		_lastPos = n;
	}
};


class TcpServer
{
private:
	SOCKET _sock;
	std::vector<TcpClient*> _clients;
	CELLTimestamp _timer;
	int _recvCount;
public:
	TcpServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
	}
	virtual ~TcpServer()
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
			for (int n = (int)_clients.size()-1; n >= 0; n--)
			{

				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			closesocket(_sock);
			WSACleanup();
			_clients.clear(); 
		}
		_sock = INVALID_SOCKET;
	}
	//接受客户端连接
	int Accept()
	{
		//等待接受客户端连接
		sockaddr_in cliAddr = {};
		int len = sizeof(sockaddr_in);
		SOCKET csock = INVALID_SOCKET;
		csock = accept(_sock, (sockaddr *)&cliAddr, &len);
		if (INVALID_SOCKET == csock)
		{
			printf("ERROR:accept<Socket:%d> failed！\n", (int)csock);
		}
		_clients.push_back(new TcpClient(csock));
		//printf("Connected：<%d>Socket<%d> IP:%s Port：%d Connected success！！！\r\n",
		//	   (int)_clients.size(),(int)csock, inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
		return csock;
	}
	//接受数据 处理粘包拆包
	char _szRecv[RECV_BUFF_SIZE] = {};
	int RecvData(TcpClient* pClient)
	{
		//接受缓存
		int nLen = recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			printf("服务器已经退出，任务结束\n");
			return -1;//跳出接收循环
		}
		//将收取到的数据拷贝到消息缓冲区
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		pClient->setLastPos(pClient->getLastPos() + nLen);
		while (pClient->getLastPos() >= sizeof(Dataheader))
		{
			//这时候就知道消息的长度
			Dataheader* header = (Dataheader*)pClient->msgBuf();
			if (pClient->getLastPos() >= header->dataLength)
			{
				//剩余未处理消息缓冲长度
				int nSize = pClient->getLastPos() - header->dataLength;
				//处理消息
				OnNetMsg(pClient->sockfd(),header);
				//将未处理消息前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				//新的未处理消息尾部
				pClient->setLastPos(nSize);
			}
			else
			{
				//没有完整消息了
				break;
			}
		}
		return 0;
	}
	//响应网络消息
	virtual void OnNetMsg(SOCKET csock, Dataheader *header)
	{
		_recvCount++;
		auto _timenow = _timer.getElapsedSec();
		if (_timenow >= 1.0)
		{
			printf("时间: %lf 收到数据包数量: %d\n", _timenow,_recvCount);
			_timer.update();
			_recvCount = 0;
		}
		//正常处理数据
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			//printf("收到Socket<%d>命令：CMD_LOGIN 数据长度：%d\n", (int)csock, header->dataLength);
			//Login *login = (Login *)header;
			//判断用户密码是否正确的过程
			//printf("登录用户名：%s 登录用户密码：%s\n", login->userName, login->passWord);
			LoginResult ret;
			ret.result = 1;
			SendData(csock, &ret);
			break;
		}
		case CMD_LOGOUT:
		{
			//printf("收到Socket<%d>命令：CMD_LOGOUT 数据长度：%d\n", (int)csock, header->dataLength);
			//Logout *logout = (Logout *)header;
			//printf("登出用户名：%s \n", logout->userName);
			//LogoutResult ret;
			//ret.result = 2;
			//SendData(csock, &ret);
			break;
		}
		case CMD_ERROR:
		{
			printf("收到<客户端%d>错误消息，数据长度：%d\n", (int)csock, header->dataLength);
			break;
		}
		default:
		{
			printf("收到<客户端%d>未知消息，数据长度：%d\n", (int)csock, header->dataLength);
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
			SOCKET maxSock = _sock;
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				FD_SET(_clients[n]->sockfd(), &fdRead);
				if (maxSock  < _clients[n]->sockfd())
				{
					maxSock = _clients[n]->sockfd();
				}
			}
			//select模型
			//第一个参数ndfs是一个整数，是fd_set集合中所有描述符(socket)的范围，而不是数量
			//描述符(socket是一个整数)
			//windows中可以写0
			//第五个参数超时时间：NULL为阻塞模式，时间为最大时间
			timeval timeout = {0, 10};
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &timeout);
			if (ret < 0)
			{
				printf("select任务已经退出，任务结束\n");
				return false;
			}
			//服务端readIO事件
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				//等待接受客户端连接
				Accept();
				return true;
			}
			for (int n = (int)(_clients.size() - 1); n >= 0; n--)
			{
				if (FD_ISSET(_clients[n]->sockfd(),&fdRead))
				{
					if (-1 == RecvData(_clients[n]))
					{
						auto iter = _clients.begin() + n;
						if (iter != _clients.end())
						{
							delete _clients[n];
							_clients.erase(iter);
						}
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
	int SendData(SOCKET csock, Dataheader *header)
	{
		if (isRun() && header)
		{
			return send(csock, (char *)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	void SendDataToAll(Dataheader *header)
	{
		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			SendData(_clients[n]->sockfd(), header);
		}
	}

private:
};

#endif // !_TCPSERVER_HPP
