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
	/* 设置IP地址 */
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); //绑定本机IP
	//servAddr.sin_addr.s_addr = inet_addr("192.168.1.53");

	/* 设置端口 */
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(3000); //端口为3000

	/* 创建套接服务字 */
	SOCKET serverSocket;
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		printf("Error:CreatServerSocket failed!\n");
		return 0;
	}

	/* 绑定服务器套接字 */
	if (bind(serverSocket, (sockaddr*)&servAddr, sizeof(sockaddr)) == SOCKET_ERROR)
	{
		printf("ERROR:Bind failed！\n");
		return 0;
	}

	/* 监听端口 */
	if (listen(serverSocket, 20) == SOCKET_ERROR)
	{
		closesocket(serverSocket);
		WSACleanup();
		printf("ERROR:Listen failed！\n");
		return 0;
	}
	printf("linstening:%dport...\n", ntohs(servAddr.sin_port));

	/* 阻塞方式等待accept */
	int len = sizeof(cliAddr);
	SOCKET clientSocket;
	clientSocket = accept(serverSocket, (sockaddr*)&cliAddr, &len);
	printf("Connected：%s \r\n", inet_ntoa(cliAddr.sin_addr));

	/* 接受消息 */
	char recvBuf[100] = { 0 };
	recv(clientSocket, recvBuf, sizeof(recvBuf), 0);

	/* 发送消息 */
	send(clientSocket, recvBuf, 100, NULL);

	closesocket(clientSocket);
	WSACleanup();
	system("pause");
	return 1;
}