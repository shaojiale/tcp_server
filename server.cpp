#include <stdlib.h>
#include <stdio.h>
#include <WinSock2.h>
#include <iostream>
#pragma comment(lib,"Ws2_32.lib ")

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
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); //�󶨱���IP
	//servAddr.sin_addr.s_addr = inet_addr("192.168.1.53");

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
	printf("Connected��%s %dsuccess������\r\n", inet_ntoa(cliAddr.sin_addr));

	while (true)
	{
		char recvBuf[100] = { 0 };
		int cnt = recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
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
			//������������
			if (0 == strcmp(recvBuf, "XXX"))
			{
			}
			else if (0 == strcmp(recvBuf, "exit"))
			{
				std::cout << recvBuf << std::endl;
				std::cout << "�ͻ������˳�" << std::endl;
				break;
			}
			else
			{
				std::cout << recvBuf << std::endl;
				send(clientSocket, recvBuf, sizeof(recvBuf) + 1, 0);//����msgbuf�е����ݡ��������ȷ����Ӧ��ÿ�ζ����¼���
				recv(clientSocket, recvBuf, 100, 0);
			}
		}
	}
	closesocket(clientSocket);
	WSACleanup();
	return 1;
}