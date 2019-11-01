#include <stdlib.h>
#include <stdio.h>
#include <WinSock2.h>
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
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
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
	printf("Connected��%s \r\n", inet_ntoa(cliAddr.sin_addr));

	/* ������Ϣ */
	char recvBuf[100] = { 0 };
	recv(clientSocket, recvBuf, sizeof(recvBuf), 0);

	/* ������Ϣ */
	send(clientSocket, recvBuf, 100, NULL);

	closesocket(clientSocket);
	WSACleanup();
	system("pause");
	return 1;
}