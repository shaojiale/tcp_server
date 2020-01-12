#ifndef _TCPSERVER_HPP
#define _TCPSERVER_HPP

#ifdef _WIN32
#define FD_SETSIZE 2506
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib ")
#endif //end define _win32

#include <stdio.h>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"
#include "CELLTask.hpp"
//��������С��Ԫ��С
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240 * 5
#define SEND_BUFF_SIZE 700
#endif // !RECV_BUFF_SIZE

typedef std::shared_ptr<Dataheader> DataHeaderPtr;
typedef std::shared_ptr<LoginResult> LoginResultPtr;
//�ͻ�������
class TcpClient
{
private:
	SOCKET _sockfd;
	char _szMsgBuf[RECV_BUFF_SIZE];
	int _lastPos = 0;
	char _szSendBuf[SEND_BUFF_SIZE];
	int _lastSendPos = 0;

public:
	TcpClient(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
		memset(_szSendBuf, 0, sizeof(_szSendBuf));
		_lastSendPos = 0;
	}
	SOCKET sockfd()
	{
		return _sockfd;
	}
	char *msgBuf()
	{
		return _szMsgBuf;
	}
	int getLastPos()
	{
		return _lastPos;
	}
	void setLastPos(int n)
	{
		_lastPos = n;
	}
	//��������
	int SendData(DataHeaderPtr &header)
	{
		int ret = SOCKET_ERROR;
		int nSendLen = header->dataLength;
		//ָ��Ҫ���͵���Ϣ
		const char *pSendData = (const char *)header.get();
		while (true)
		{
			if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
			{
				//��������
				int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
				memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
				//ʣ��δ����Ϣ
				pSendData += nCopyLen;
				//ʣ��δ����Ϣ����
				nSendLen -= nCopyLen;
				ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
				//���ͻ�����ƫ��
				_lastSendPos = 0;
				if (SOCKET_ERROR == ret)
				{
					return ret;
				}
			}
			else
			{
				memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
				_lastSendPos += nSendLen;
				break;
			}
		}
		return ret;
	}
};
typedef std::shared_ptr<TcpClient> ClientSocketPtr;
class CellServer;
//�����¼��ӿ�
class INetEvent
{
public:
	//�ͻ��˼����¼�
	virtual void OnNetJoin(ClientSocketPtr &pClient) = 0;
	//�ͻ����뿪�¼�
	virtual void OnNetLeave(ClientSocketPtr &pClient) = 0;
	//��Ӧ������Ϣ
	virtual void OnNetMsg(CellServer *pCellServer, ClientSocketPtr &pClient, Dataheader *header) = 0;
	//Recv�¼�
	virtual void OnNetRecv(ClientSocketPtr &pClient) = 0;

private:
};

class CellS2CTask : public CellTask
{

public:
	CellS2CTask(ClientSocketPtr& pClient, DataHeaderPtr& pHeader)
	{
		_pClient = pClient;
		_pHeader = pHeader;
	}
	~CellS2CTask()
	{
	}
	void doTask()
	{
		_pClient->SendData(_pHeader);
	}

private:
	ClientSocketPtr _pClient;
	DataHeaderPtr _pHeader;
};
typedef std::shared_ptr<CellS2CTask> CellS2CTaskPtr;

//��Ϣ�����߳�
class CellServer
{
private:
	SOCKET _sock;
	//��ʽ�ͻ�����
	std::map<SOCKET, ClientSocketPtr> _clients;
	//����ͻ�����
	std::vector<ClientSocketPtr> _clientsBuff;
	//������е���
	std::mutex _mutex;
	std::thread _thread;
	//�����¼�����
	INetEvent *_pINetEvent;
	//�ͻ��б���
	fd_set _fdRead_bak;
	//�ͻ��б�仯
	bool _clients_change;
	SOCKET _maxSock;
	CellTaskServer _taskServer;

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

	void setEventObj(INetEvent *event)
	{
		_pINetEvent = event;
	}

	int RecvData(ClientSocketPtr &pClient)
	{
		//���ܻ���
		char *_szRecv = pClient->msgBuf() + pClient->getLastPos();
		int nLen = recv(pClient->sockfd(), _szRecv, (RECV_BUFF_SIZE)-pClient->getLastPos(), 0);
		_pINetEvent->OnNetRecv(pClient);
		if (nLen <= 0)
		{
			printf("�������Ѿ��˳����������\n");
			return -1; //��������ѭ��
		}
		//����ȡ�������ݿ�������Ϣ������
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		pClient->setLastPos(pClient->getLastPos() + nLen);
		while (pClient->getLastPos() >= sizeof(Dataheader))
		{
			//��ʱ���֪����Ϣ�ĳ���
			Dataheader *header = (Dataheader *)pClient->msgBuf();
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
	virtual void OnNetMsg(ClientSocketPtr &pClient, Dataheader *header)
	{
		_pINetEvent->OnNetMsg(this, pClient, header);
	}
	//����������Ϣ
	bool OnRun()
	{
		_clients_change = true;
		while (isRun())
		{
			//�ӻ������ȡ���ͻ�����
			if (!_clientsBuff.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients[pClient->sockfd()] = pClient;
				}
				_clientsBuff.clear();
				_clients_change = true;
			}
			if (_clients.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			fd_set fdRead;
			FD_ZERO(&fdRead);
			if (_clients_change)
			{
				_clients_change = false;
				_maxSock = _clients.begin()->second->sockfd();
				for (auto iter : _clients)
				{
					FD_SET(iter.second->sockfd(), &fdRead);
					if (_maxSock < iter.second->sockfd())
					{
						_maxSock = iter.second->sockfd();
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else
			{
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}
			//selectģ��

			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0)
			{
				printf("select�����Ѿ��˳����������\n");
				Close();
				return false;
			}
			else if (ret == 0)
			{
				continue;
			}
#ifdef _WIN32
			for (int n = 0; n < fdRead.fd_count; n++)
			{
				auto iter = _clients.find(fdRead.fd_array[n]);
				if (iter != _clients.end())
				{
					if (-1 == RecvData(iter->second))
					{
						if (_pINetEvent)
							_pINetEvent->OnNetLeave(iter->second);
						_clients_change = true;
						_clients.erase(iter->first);
					}
				}
				else
				{
					printf("error. if (iter != _clients.end())\n");
				}
			}
#endif // _WIN32
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
			for (auto iter : _clients)
			{
				closesocket(iter.second->sockfd());
			}
			closesocket(_sock);
			_clients.clear();
		}
		// _sock = INVALID_SOCKET;
	}
	//��ӿͻ���
	void addClients(ClientSocketPtr& pClient)
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
		_taskServer.Start();
	}

	void addSendTask(ClientSocketPtr& pClient, DataHeaderPtr& header)
	{
		auto task = std::make_shared<CellS2CTask>(pClient, header);
		_taskServer.addTask((CellTaskPtr&)task);
	}
	//���ؿͻ�����
	size_t getClientCount()
	{
		return _clients.size() + _clientsBuff.size();
	}
};
typedef std::shared_ptr<CellServer> CellServerPtr;
//���߳�
class TcpServer : public INetEvent
{
private:
	SOCKET _sock;
	//��Ϣ�����̶߳���
	std::vector<CellServerPtr> _cellServers;
	//����ʱ�������
	CELLTimestamp _timer;

protected:
	std::atomic_int _recvCount;
	std::atomic_int _msgCount;
	//�ͻ��˼���Ͽ����Ӽ���
	std::atomic_int _clientCount;

public:
	TcpServer()
	{
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_msgCount = 0;
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
		else
		{
			addClientToCellServer((ClientSocketPtr&)std::make_shared<TcpClient>(csock));
		}
		return csock;
	}
	//�ͻ��˷���
	void addClientToCellServer(ClientSocketPtr& pClient)
	{
		//�������ٿͻ��˵�cell��Ϣ�������
		auto pMinServer = _cellServers[0];
		for (auto pCellServer : _cellServers)
		{
			if (pMinServer->getClientCount() > pCellServer->getClientCount())
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
			auto ser = std::make_shared<CellServer>(_sock);
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
			printf("Thread��%d Clients��%d Time: %lf recv: %f msg: %f\n",
				   (int)_cellServers.size(), (int)_clientCount, t1, (float)(_recvCount / t1), (float)(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_timer.update();
		}
	}

	bool OnRun()
	{
		if (isRun())
		{
			time4msg();
			fd_set fdRead;
			FD_ZERO(&fdRead);
			FD_SET(_sock, &fdRead);
			//selectģ��

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

	virtual void OnNetJoin(ClientSocketPtr &pClient)
	{
		_clientCount++;
	}
	virtual void OnNetLeave(ClientSocketPtr &pClient)
	{
		_clientCount--;
	}
	virtual void OnNetRecv(ClientSocketPtr& pClient)
	{
		_recvCount++;
		//printf("client<%d> leave\n", pClient->sockfd());
	}
	//��Ӧ������Ϣ
	virtual void OnNetMsg(CellServer *pCellServer, ClientSocketPtr &pClient, Dataheader *header)
	{
		_msgCount++;
	}

};

#endif // !_TCPSERVER_HPP
