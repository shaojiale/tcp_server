#ifndef _TCPSERVER_HPP
#define _TCPSERVER_HPP
#include <stdio.h>
#include <WinSock2.h>
#include <vector>
#include "MessageHeader.hpp"
#pragma comment(lib,"Ws2_32.lib ")

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
	//��ʼ��Socket
	SOCKET InitSocket()
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
		return _sock;
	}
	//�󶨶˿ں�
	int  Bind(const char* ip, unsigned short port)
	{
		/* ����IP��ַ */
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		//��������IP
		if (ip)
		{
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else
		{
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
		/*_sin.sin_addr.s_addr = inet_addr(ip);*/
		//_sin.sin_addr.s_addr = htonl(ip); //��������IP
		/* ���ö˿� */
		_sin.sin_port = htons(port); //�˿�Ϊ3000
		/* �󶨷������׽��� */
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr));
		if (SOCKET_ERROR == ret)
		{
			printf("ERROR:�󶨶˿�<%d>ʧ��......\n",port);
		}
		else
		{
			printf("�󶨶˿�<%d>�ɹ���\n", port);
		}
		return ret;
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
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{

				closesocket(g_clients[n]);
			}
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
		int len = sizeof(cliAddr);
		SOCKET _csock;
		_csock = accept(_sock, (sockaddr*)&cliAddr, &len);
		if (INVALID_SOCKET == _csock)
		{
			printf("ERROR:accept<Socket:%d> failed��\n", (int)_csock);
		}
		else
		{
			printf("Connected��Socket<%d> IP:%s Port��%d Connected success������\r\n", \
				(int)_csock, inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
			g_clients.push_back(_csock);
			NewUserJoin userjoin;
			SendDataToAll(&userjoin);
		}
		return _csock;
	}
	//�������� ����ճ�����
	int RecvData(SOCKET _csock)
	{
		//���ܻ���
		char szRecv[4096] = {};
		int cnt = recv(_csock, (char*)szRecv, sizeof(Dataheader), 0);
		Dataheader* header = (Dataheader*)szRecv;
		if (cnt <= 0)
		{
			//if ((cnt < 0) && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
			//{
			//	continue;//������������
			//}
			/*printf("�ͻ���<Socket%d>�Ѿ��˳����������\n", (int)_csock);*/
			return -1;//��������ѭ��
		}
		recv(_csock, szRecv + sizeof(header), header->dataLength - sizeof(header), 0);
		OnNetMsg(_csock, header);
	}
	//��Ӧ������Ϣ
	virtual void OnNetMsg(SOCKET _csock, Dataheader* header)
	{
		//������������
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			printf("�յ�Socket<%d>���CMD_LOGIN ���ݳ��ȣ�%d\n", (int)_sock, header->dataLength);
			Login* login = (Login*)header;
			//�ж��û������Ƿ���ȷ�Ĺ���
			printf("��¼�û�����%s ��¼�û����룺%s\n", login->userName, login->passWord);
			LoginResult ret;
			SendData(_csock, &ret);
			break;
		}
		case CMD_LOGOUT:
		{
			printf("�յ�Socket<%d>���CMD_LOGOUT ���ݳ��ȣ�%d\n", (int)_sock, header->dataLength);
			Logout* logout = (Logout*)header;
			printf("�ǳ��û�����%s \n", logout->userName);
			LogoutResult ret;
			SendData(_csock, &ret);
			break;
		}
		default:
		{
			printf("�յ�Socket<%d>��������...\n", (int)_sock);
			header->cmd = CMD_ERROR;
			header->dataLength = 0;
			SendData(_csock, header);
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
			SOCKET maxsock = _sock;
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				FD_SET(g_clients[n], &fdRead);
				if (maxsock < g_clients[n])
				{
					maxsock = g_clients[n];
				}
			}
			//selectģ��
			//��һ������ndfs��һ����������fd_set����������������(socket)�ķ�Χ������������
			//������(socket��һ������)
			//windows�п���д0
			//�����������ʱʱ�䣺NULLΪ����ģʽ��ʱ��Ϊ���ʱ��
			timeval timeout = { 0,0 };
			int ret = select(maxsock + 1, &fdRead, &fdWrite, &fdExp, &timeout);
			if (ret < 0)
			{
				printf("select�����Ѿ��˳����������\n");
				Close();
				return false;
			}
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				//�ȴ����ܿͻ�������
				Accept();
			}
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
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
	//socket��Ч
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//��������
	int SendData(SOCKET _csock, Dataheader* header)
	{
		if (isRun() && header)
		{
			return send(_csock, (char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	void SendDataToAll(Dataheader* header)
	{
		for (int n = (int)g_clients.size() - 1; n >= 0; n--)
		{
			SendData(g_clients[n], header);
		}
	}
private:

};


#endif // !_TCPSERVER_HPP