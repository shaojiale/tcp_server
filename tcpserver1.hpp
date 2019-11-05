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
	//��ʼ��socket
	void InitSocket()
	{
#ifdef _WIN32
		//����Windows socket 2.x����
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		//��ʼ��_sock ������о����ӣ��رվ����� ֮���ٽ���socket
		if (INVALID_SOCKET != _sock)
		{
			printf("<socket=%d>�رվ�����...\n", _sock);
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("���󣬽���Socketʧ��...\n");
		}
		else {
			printf("����Socket�ɹ�...\n");
		}
	}



	void bindPort(short port)
	{
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);//����ת�����ֽڵ�ַ
		_sin.sin_addr.S_un.S_addr = INADDR_ANY; //����
		if (bind(_sock, (sockaddr*)&_sin, sizeof(_sin)) == SOCKET_ERROR)
		{
			printf("������˿�ʧ�ܣ�\n");
		}
		else {
			printf("������˿ڳɹ���\n");
		}
	}

	void listenPort()
	{
		if (SOCKET_ERROR == listen(_sock, 5))
		{
			printf("����ʧ�ܣ�\n");
		}
		else {
			printf("�����ɹ���\n");
		}

	}
	//�ر��׽���closesocket
	void Close()
	{
		//�������Ч��_sock
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (int n = (int)g_clients.size(); n >= 0; n--)
			{
				closesocket(g_clients[n]);
			}
			closesocket(_sock);
			//���Windows socket����
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
		printf("��ǰ<�����>�׽���%d��processor start.... \n", (int)_cSock);
		int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)szRecv;
		if (nLen <= 0)
		{
			printf("��ǰ<�����>�׽���%d��processor end.... ,�ͻ����˳�!\n", (int)_cSock);
			return -1;
		}
		//��������
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			printf("���� CMD_LOGIN,�ͻ���!!��¼!\n");
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLth - sizeof(DataHeader), 0);
			Login* login = (Login*)szRecv;
			printf("�û�������%s �û����룺%s\n", login->usrName, login->passWord);
			LoginRst inRst;
			inRst.rst = 1000;
			send(_cSock, (char*)&inRst, sizeof(inRst), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			printf("���� CMD_LOGOUT,�ͻ��˵ǳ�!!!\n");
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLth - sizeof(DataHeader), 0);
			Logout* logout = (Logout*)szRecv;
			printf("�û�������%s", logout->usrName);
			LogoutRst outRst;
			outRst.rst = -1000;
			send(_cSock, (char*)&outRst, sizeof(outRst), 0);
		}
		break;
		}
		printf("��ǰ<�����>�׽���%d��processor end.... \n", (int)_cSock);
		return 0;
	}

	//����������Ϣ
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
			FD_SET(_sock, &fdsExp);//����3�ֵ�fds��������Ƿ�������

								   //���g_clients�к����׽��ֵĻ���������ӽ���
			for (int n = 0; n < (int)g_clients.size(); n++)
			{
				FD_SET(g_clients[n], &fdsRead);
			}

			timeval t = { 10,0 };

			int ret = select(_sock + 1, &fdsRead, &fdsWrite, &fdsExp, &t);

			if (ret < 0)
			{
				printf("selectģ����û�ÿ����׽���,�ͻ������˳�!!!\n");
				return false;
			}

			if (FD_ISSET(_sock, &fdsRead))
			{
				FD_CLR(_sock, &fdsRead);
				//�ȴ�socket
				sockaddr_in _clientAddr = {};
				int nAddrLen = sizeof(sockaddr_in);
				SOCKET _cSock = INVALID_SOCKET;
				_cSock = accept(_sock, (sockaddr*)&_clientAddr, &nAddrLen);
				if (_cSock == INVALID_SOCKET)
				{
					printf("��ǰ����Ч�ͻ����׽���!!!\n");
				}
				for (int n = (int)g_clients.size() - 1; n >= 0; n--)
				{
					NewUserJoin userjoin;
					userjoin.sock = (int)g_clients[n];
					printf("<�������ͻ��˷���newuserjoin = %d>\n", userjoin.sock);
					send(g_clients[n], (char*)&userjoin, sizeof(userjoin), 0);
				}
				g_clients.push_back(_cSock);
				printf("�µĿͻ��ˣ�socket = %d,IP = %s \n", int(_cSock), inet_ntoa(_clientAddr.sin_addr));
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

	//�Ƿ�����
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}


private:

};


#endif//endif _EASYTCPSERVER_HPP_