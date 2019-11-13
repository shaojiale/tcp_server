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
//��������С��Ԫ��С
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
		_clients.push_back(new TcpClient(csock));
		//printf("Connected��<%d>Socket<%d> IP:%s Port��%d Connected success������\r\n",
		//	   (int)_clients.size(),(int)csock, inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
		return csock;
	}
	//�������� ����ճ�����
	char _szRecv[RECV_BUFF_SIZE] = {};
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
				OnNetMsg(pClient->sockfd(),header);
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
	virtual void OnNetMsg(SOCKET csock, Dataheader *header)
	{
		_recvCount++;
		auto _timenow = _timer.getElapsedSec();
		if (_timenow >= 1.0)
		{
			printf("ʱ��: %lf �յ����ݰ�����: %d\n", _timenow,_recvCount);
			_timer.update();
			_recvCount = 0;
		}
		//������������
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			//printf("�յ�Socket<%d>���CMD_LOGIN ���ݳ��ȣ�%d\n", (int)csock, header->dataLength);
			//Login *login = (Login *)header;
			//�ж��û������Ƿ���ȷ�Ĺ���
			//printf("��¼�û�����%s ��¼�û����룺%s\n", login->userName, login->passWord);
			LoginResult ret;
			ret.result = 1;
			SendData(csock, &ret);
			break;
		}
		case CMD_LOGOUT:
		{
			//printf("�յ�Socket<%d>���CMD_LOGOUT ���ݳ��ȣ�%d\n", (int)csock, header->dataLength);
			//Logout *logout = (Logout *)header;
			//printf("�ǳ��û�����%s \n", logout->userName);
			//LogoutResult ret;
			//ret.result = 2;
			//SendData(csock, &ret);
			break;
		}
		case CMD_ERROR:
		{
			printf("�յ�<�ͻ���%d>������Ϣ�����ݳ��ȣ�%d\n", (int)csock, header->dataLength);
			break;
		}
		default:
		{
			printf("�յ�<�ͻ���%d>δ֪��Ϣ�����ݳ��ȣ�%d\n", (int)csock, header->dataLength);
			break;
		}
		}
	}
	//����������Ϣ
	//���õ�������ģ��
	//��·����IO
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
			//selectģ��
			//��һ������ndfs��һ����������fd_set����������������(socket)�ķ�Χ������������
			//������(socket��һ������)
			//windows�п���д0
			//�����������ʱʱ�䣺NULLΪ����ģʽ��ʱ��Ϊ���ʱ��
			timeval timeout = {0, 10};
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &timeout);
			if (ret < 0)
			{
				printf("select�����Ѿ��˳����������\n");
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
	//socket��Ч
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//��������
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
