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

//��������С��Ԫ��С
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

//�ͻ�������
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
	//��������
	int SendData(Dataheader* header)
	{
		if (header)
		{
			return send(_sockfd, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
};
//�����¼��ӿ�
class INetEvent
{
public:
	//�ͻ��˼����¼�
	virtual void OnNetJoin(TcpClient* pClient) = 0;
	//�ͻ����뿪�¼�
	virtual void OnNetLeave(TcpClient* pClient) = 0;	
	//��Ӧ������Ϣ
	virtual void OnNetMsg(TcpClient* pClient, Dataheader* header) = 0;
private:

};
//��Ϣ�����߳�
class CellServer
{
private:
	SOCKET _sock;
	//��ʽ�ͻ�����
	std::vector<TcpClient*> _clients;
	//����ͻ�����
	std::vector<TcpClient*> _clientsBuff;
	//�ͻ���Ϣ������
	char _szRecv[RECV_BUFF_SIZE*5] = {};
	//������е���
	std::mutex _mutex;
	std::thread _thread;
	//�����¼�����
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
		//���ܻ���
		int nLen = recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0)
		{
			printf("�������Ѿ��˳����������\n");
			return -1;//��������ѭ��
		}
		//����ȡ�������ݿ�������Ϣ������
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		pClient->setLastPos(pClient->getLastPos() + nLen);
		while (pClient->getLastPos() >= sizeof(Dataheader))
		{
			//��ʱ���֪����Ϣ�ĳ���
			Dataheader* header = (Dataheader*)pClient->msgBuf();
			if (pClient->getLastPos() >= header->dataLength)
			{
				//ʣ��δ������Ϣ���峤��
				int nSize = pClient->getLastPos() - header->dataLength;
				//������Ϣ
				OnNetMsg(pClient, header);
				//��δ������Ϣǰ��
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				//�µ�δ������Ϣβ��
				pClient->setLastPos(nSize);
			}
			else
			{
				//û��������Ϣ��
				break;
			}
		}
		return 0;
	}
	//��Ӧ������Ϣ
	virtual void OnNetMsg(TcpClient* pClient, Dataheader* header)
	{
		_pINetEvent->OnNetMsg(pClient, header);
	}
	//��������
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
	//����������Ϣ
	//���õ�������ģ��
	//��·����IO
	bool OnRun()
	{
		while (isRun())
		{
			//�ӻ������ȡ���ͻ�����
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
			//selectģ��
			//��һ������ndfs��һ����������fd_set����������������(socket)�ķ�Χ������������
			//������(socket��һ������)
			//windows�п���д0
			//�����������ʱʱ�䣺NULLΪ����ģʽ��ʱ��Ϊ���ʱ��
			int ret = select(maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0)
			{
				printf("select�����Ѿ��˳����������\n");
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
	//socket��Ч
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}
	//�ر�Socket
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
	//��ӿͻ���
	void addClients(TcpClient* pClient)
	{
		//�Խ���
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock();
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}
	//�����������߳�
	void Start()
	{
		//�߳�����������Ĭ�ϳ�Աָ�����
		_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
	}
	//���ؿͻ�����
	size_t ClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}
};
//���߳�
class TcpServer : public INetEvent
{
private:
	SOCKET _sock;
	//��Ϣ�����̶߳���
	std::vector<CellServer*> _cellServers;
protected:
	//����ʱ�������
	CELLTimestamp _timer;
	std::atomic_int _recvCount;
	//�ͻ��˼���Ͽ����Ӽ���
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
	//��ʼ��Socket
	void InitSocket()
	{
		//����winsocket2.2
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData))
		{
			printf("Error:��������ʧ�� failed!\n");
		}
		if (INVALID_SOCKET != _sock)
		{
			printf("<Socket:%d>�رվ�����......\n", (int)_sock);
			Close();
		}
		/* �����׽ӷ����� */
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("Error:����Socketʧ��......\n");
		}
		else
		{
			printf("<Socket:%d>�����ɹ�......\n", (int)_sock);
		}
	}
	//�󶨶˿ں�
	void Bind(const char *ip, unsigned short port)
	{
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port); //�˿�Ϊ3000
		//��������IP
		if (ip)
		{
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
		/* �󶨷������׽��� */
		int ret = bind(_sock, (sockaddr *)&_sin, sizeof(sockaddr));
		if (SOCKET_ERROR == ret)
		{
			printf("ERROR:�󶨶˿�<%d>ʧ��......\n", port);
		}
		else
		{
			printf("�󶨶˿�<%d>�ɹ���\n", port);
		}
	}
	//�����˿ں�
	void Listen(int n)
	{
		/* �����˿� */

		int ret = listen(_sock, n);
		if (SOCKET_ERROR == n)
		{
			printf("ERROR:Listen failed��\n");
		}
		printf("linstening:%dport...\n", n);
	}
	//�ر�Socket
	void Close()
	{
		if (INVALID_SOCKET != _sock)
		{
			closesocket(_sock);
			WSACleanup();
		}
		_sock = INVALID_SOCKET;
	}
	//���ܿͻ�������
	int Accept()
	{
		//�ȴ����ܿͻ�������
		sockaddr_in cliAddr = {};
		int len = sizeof(sockaddr_in);
		SOCKET csock = INVALID_SOCKET;
		csock = accept(_sock, (sockaddr *)&cliAddr, &len);
		if (INVALID_SOCKET == csock)
		{
			printf("ERROR:accept<Socket:%d> failed��\n", (int)csock);
		}
		addClientToCellServer(new TcpClient(csock));
		return csock;
	}
	//�ͻ��˷���
	void addClientToCellServer(TcpClient* pClient)
	{
		//�������ٿͻ��˵�cell��Ϣ�������
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
	//�����������߳�
	void Start(int nCell)
	{
		for (int n = 0; n < nCell; n++)
		{
			auto ser = new CellServer(_sock);
			_cellServers.push_back(ser);
			//ע�������¼����ܶ���
			ser->setEventObj(this);
			//���������߳�
			ser->Start();
		}
	}
	//��Ϣ����
	void time4msg()
	{
		auto t1 = _timer.getElapsedSec();
		if (t1 >= 1.0)
		{
			printf("�߳�����%d�ͻ���������%dʱ��: %lf �յ����ݰ�����: %d\n", (int)_cellServers.size(),(int)_clientCount, t1, (int)(_recvCount / t1));
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
			//selectģ��
			//��һ������ndfs��һ����������fd_set����������������(socket)�ķ�Χ������������
			//������(socket��һ������)
			//windows�п���д0
			//�����������ʱʱ�䣺NULLΪ����ģʽ��ʱ��Ϊ���ʱ��
			timeval timeout = {0, 10};
			int ret = select(_sock + 1, &fdRead, 0, 0, &timeout);
			if (ret < 0)
			{
				printf("select�����Ѿ��˳����������\n");
				Close();
				return false;
			}
			//�����readIO�¼�
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				//�ȴ����ܿͻ�������
				Accept();
				return true;
			}
			return true;
		}
		return false;
	}
	//socket��Ч
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

	//��Ӧ������Ϣ
	virtual void OnNetMsg(TcpClient* pClient, Dataheader* header)
	{
		_recvCount++;
	}
};

#endif // !_TCPSERVER_HPP
