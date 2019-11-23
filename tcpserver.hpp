#ifndef _TCPSERVER_HPP
#define _TCPSERVER_HPP

#ifdef _WIN32
	#define FD_SETSIZE 2506
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#include<windows.h>
	#include <WinSock2.h>
	#pragma comment(lib, "Ws2_32.lib ")
#endif  //end define _win32

#include <stdio.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include  <functional>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"

//缓冲区最小单元大小
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

//客户端类型
class TcpClient
{
private:
	SOCKET _sockfd;
	char _szMsgBuf[RECV_BUFF_SIZE * 5];
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
	//发送数据
	int SendData(Dataheader* header)
	{
		if (header)
		{
			return send(_sockfd, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
};
//网络事件接口
class INetEvent
{
public:
	//客户端加入事件
	virtual void OnNetJoin(TcpClient* pClient) = 0;
	//客户端离开事件
	virtual void OnNetLeave(TcpClient* pClient) = 0;	
	//响应网络消息
	virtual void OnNetMsg(TcpClient* pClient, Dataheader* header) = 0;
private:

};
//消息处理线程
class CellServer
{
private:
	SOCKET _sock;
	//正式客户队列
	std::vector<TcpClient*> _clients;
	//缓冲客户队列
	std::vector<TcpClient*> _clientsBuff;
	//客户消息缓冲区
	char _szRecv[RECV_BUFF_SIZE*5] = {};
	//缓冲队列的锁
	std::mutex _mutex;
	std::thread _thread;
	//网络事件对象
	INetEvent* _pINetEvent;
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_pINetEvent = nullptr;
	}
	~CellServer()
	{
		Close();
		_sock = INVALID_SOCKET;
	}

	void setEventObj(INetEvent* event)
	{
		_pINetEvent = event;
	}

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
				OnNetMsg(pClient, header);
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
	virtual void OnNetMsg(TcpClient* pClient, Dataheader* header)
	{
		_pINetEvent->OnNetMsg(pClient, header);
	}
	//发送数据
	int SendData(SOCKET csock, Dataheader* header)
	{
		if (isRun() && header)
		{
			return send(csock, (char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	void SendDataToAll(Dataheader* header)
	{
		for (int n = (int)_clients.size() - 1; n >= 0; n--)
		{
			SendData(_clients[n]->sockfd(), header);
		}
	}
	//处理网络消息
	//运用到非阻塞模型
	//多路复用IO
	bool OnRun()
	{
		while (isRun())
		{
			//从缓冲队列取出客户队列
			if (_clientsBuff.size() > 0)
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients.push_back(pClient);
				}
				_clientsBuff.clear();
			}
			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			fd_set fdRead;
			FD_ZERO(&fdRead);
			SOCKET maxSock = _clients[0]->sockfd();
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{
				FD_SET(_clients[n]->sockfd(), &fdRead);
				if (maxSock < _clients[n]->sockfd())
				{
					maxSock = _clients[n]->sockfd();
				}
			}
			//select模型
			//第一个参数ndfs是一个整数，是fd_set集合中所有描述符(socket)的范围，而不是数量
			//描述符(socket是一个整数)
			//windows中可以写0
			//第五个参数超时时间：NULL为阻塞模式，时间为最大时间
			int ret = select(maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0)
			{
				printf("select任务已经退出，任务结束\n");
				Close();
				return false;
			}
			for (int n = (int)(_clients.size() - 1); n >= 0; n--)
			{
				if (FD_ISSET(_clients[n]->sockfd(), &fdRead))
				{
					if (-1 == RecvData(_clients[n]))
					{
						auto iter = _clients.begin() + n;
						if (iter != _clients.end())
						{
							if(_pINetEvent)
								_pINetEvent->OnNetLeave(_clients[n]);
							delete _clients[n];
							_clients.erase(iter);
						}
					}
				}
			}
		}
	}
	//socket有效
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	//关闭Socket
	void Close()
	{
		if (INVALID_SOCKET != _sock)
		{
			for (int n = (int)_clients.size() - 1; n >= 0; n--)
			{

				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			closesocket(_sock);
			_clients.clear();
		}
		_sock = INVALID_SOCKET;
	}
	//添加客户端
	void addClients(TcpClient* pClient)
	{
		//自解锁
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}
	//启动消费者线程
	void Start()
	{
		//线程启动函数和默认成员指针参数
		_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
	}
	//返回客户数量
	size_t ClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}
};
//主线程
class TcpServer : public INetEvent
{
private:
	SOCKET _sock;
	//消息处理线程对象
	std::vector<CellServer*> _cellServers;
protected:
	//计数时间和数量
	CELLTimestamp _timer;
	std::atomic_int _recvCount;
	//客户端加入断开连接计数
	std::atomic_int _clientCount;
public:
	TcpServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_clientCount = 0;
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
		SOCKET csock = INVALID_SOCKET;
		csock = accept(_sock, (sockaddr *)&cliAddr, &len);
		if (INVALID_SOCKET == csock)
		{
			printf("ERROR:accept<Socket:%d> failed！\n", (int)csock);
		}
		addClientToCellServer(new TcpClient(csock));
		return csock;
	}
	//客户端分配
	void addClientToCellServer(TcpClient* pClient)
	{
		//查找最少客户端的cell消息处理对象
		auto pMinServer = _cellServers[0];
		for (auto pCellServer : _cellServers)
		{
			if (pMinServer->ClientCount() < pCellServer->ClientCount())
			{
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClients(pClient);
		OnNetJoin(pClient);
	}
	//启动消费者线程
	void Start(int nCell)
	{
		for (int n = 0; n < nCell; n++)
		{
			auto ser = new CellServer(_sock);
			_cellServers.push_back(ser);
			//注册网络事件接受对象
			ser->setEventObj(this);
			//启动服务线程
			ser->Start();
		}
	}
	//消息计数
	void time4msg()
	{
		auto t1 = _timer.getElapsedSec();
		if (t1 >= 1.0)
		{
			printf("线程数：%d客户端数量：%d时间: %lf 收到数据包数量: %d\n", (int)_cellServers.size(),(int)_clientCount, t1, (int)(_recvCount / t1));
			_recvCount = 0;
			_timer.update();
		}
	}

	bool OnRun()
	{
		if (isRun())
		{
			time4msg();
			fd_set fdRead;
			//fd_set fdWrite;
			//fd_set fdExp;

			FD_ZERO(&fdRead);
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExp);

			FD_SET(_sock, &fdRead);
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExp);
			//select模型
			//第一个参数ndfs是一个整数，是fd_set集合中所有描述符(socket)的范围，而不是数量
			//描述符(socket是一个整数)
			//windows中可以写0
			//第五个参数超时时间：NULL为阻塞模式，时间为最大时间
			timeval timeout = {0, 10};
			int ret = select(_sock + 1, &fdRead, 0, 0, &timeout);
			if (ret < 0)
			{
				printf("select任务已经退出，任务结束\n");
				Close();
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
			return true;
		}
		return false;
	}
	//socket有效
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	virtual void OnNetJoin(TcpClient* pClient)
	{
		_clientCount++;
	}
	virtual void OnNetLeave(TcpClient* pClient)
	{
		_clientCount--;
	}

	//响应网络消息
	virtual void OnNetMsg(TcpClient* pClient, Dataheader* header)
	{
		_recvCount++;
	}
};

#endif // !_TCPSERVER_HPP
