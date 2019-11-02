#include <stdlib.h>
#include <stdio.h>
#include <WinSock2.h>
#include <iostream>
#pragma comment(lib,"Ws2_32.lib ")

enum CMD
{
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_ERROR
};
//��Ϣͷ
struct Dataheader {
	CMD cmd;
	int dataLength;
};

struct Login {
	char userName[32];
	char passWord[32];
};

struct LoginResult
{
	int result;
};

struct Logout{
	char userName[32];
};

struct LogoutResult
{
	int result;
};
int main()
{
	WSADATA wsaData;
	struct sockaddr_in servAddr;
	struct sockaddr_in cliAddr;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		printf("Error:RequestWindowsSocketLibrary failed!\n");
		return 0;
	}
	/* ����IP��ַ */
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); //��������IP

	/* ���ö˿� */
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(3000); //�˿�Ϊ3000

	/* �����׽ӷ����� */
	SOCKET serverSocket;
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		printf("Error:CreatServerSocket failed!\n");
		return 0;
	}

	/* �󶨷������׽��� */
	if (bind(serverSocket, (sockaddr*)&servAddr, sizeof(sockaddr)) == SOCKET_ERROR)
	{
		printf("ERROR:Bind failed��\n");
		return 0;
	}

	/* �����˿� */
	if (listen(serverSocket, 20) == SOCKET_ERROR)
	{
		closesocket(serverSocket);
		WSACleanup();
		printf("ERROR:Listen failed��\n");
		return 0;
	}
	printf("linstening:%dport...\n", ntohs(servAddr.sin_port));

	/* ������ʽ�ȴ�accept */
	int len = sizeof(cliAddr);
	SOCKET clientSocket;
	clientSocket = accept(serverSocket, (sockaddr*)&cliAddr, &len);
	if (clientSocket == INVALID_SOCKET)
	{
		printf("ERROR:accept failed��\n");
	}
	printf("Connected��Socket<%d> IP:%s Connected success������\r\n", (int)clientSocket,inet_ntoa(cliAddr.sin_addr));
	while (true)
	{
		Dataheader header = {};
		int cnt = recv(clientSocket,(char *)&header, sizeof(header), 0);
		if(cnt <= 0)
		{
			if ((cnt < 0) && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
			{
				continue;//������������
			}
			std::cout << "�ͻ����Ѿ��˳����������" << std::endl;
			break;//��������ѭ��
		}
		else
		{
			printf("�յ����%d ���ݳ��ȣ�%d\n", header.cmd, header.dataLength);
			//������������
			switch (header.cmd)
			{
			case CMD_LOGIN:
			{
				Login login{};
				recv(clientSocket, (char*)&login, sizeof(Login), 0);
				//�ж��û������Ƿ���ȷ�Ĺ���
				printf("��¼�û�����%s ��¼�û����룺%s\n", login.userName, login.passWord);
				LoginResult ret = { 1 };
				send(clientSocket, (char*)&header, sizeof(header), 0);
				send(clientSocket, (char*)&ret, sizeof(ret), 0);
				break;
			}
			case CMD_LOGOUT:
			{
				Logout logout = {};
				recv(clientSocket, (char*)&logout, sizeof(Login), 0);
				//�ж��û������Ƿ���ȷ�Ĺ���
				printf("�ǳ��û�����%s \n", logout.userName);
				LogoutResult ret = { 1 };
				send(clientSocket, (char*)&header, sizeof(header), 0);
				send(clientSocket, (char*)&ret, sizeof(ret), 0);
				break;
			}
			default:
			{
				header.cmd = CMD_ERROR;
				header.dataLength = 0;
				send(clientSocket, (char*)&header, sizeof(header), 0);
				break;
			}
			}
		}
	}
	closesocket(clientSocket);
	WSACleanup();
	return 1;
}