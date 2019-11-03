
#include <stdio.h>
#include <WinSock2.h>
#include <vector>
#include <ctime>
#pragma comment(lib,"Ws2_32.lib ")

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};
//��Ϣͷ
struct Dataheader{
	CMD cmd;
	int dataLength;
};

struct Login :public Dataheader{
	Login()
	{
		cmd = CMD_LOGIN;
		dataLength = sizeof(Login);
	}
	char userName[32];
	char passWord[32];
};

struct LoginResult :public Dataheader {
	LoginResult()
	{
		cmd = CMD_LOGIN_RESULT;
		dataLength = sizeof(LoginResult);
		result = 0;
	}
	int result;
};

struct Logout :public Dataheader {
	Logout()
	{
		cmd = CMD_LOGOUT;
		dataLength = sizeof(Logout);
	}
	char userName[32];
};

struct LogoutResult :public Dataheader {
	LogoutResult()
	{
		cmd = CMD_LOGOUT_RESULT;
		dataLength = sizeof(LogoutResult);
		result = 0;
	}
	int result;
};
struct NewUserJoin :public Dataheader {
	NewUserJoin()
	{
		cmd = CMD_NEW_USER_JOIN;
		dataLength = sizeof(NewUserJoin);
		sock = 0;
	}
	int sock;
};

std::vector<SOCKET> g_client;

int Processor(SOCKET clientSocket)
{
	//���ܻ���
	char szRecv[4096] = {};
	int cnt = recv(clientSocket, (char*)&szRecv, sizeof(Dataheader), 0);
	Dataheader* header = (Dataheader*)szRecv;
	if (cnt <= 0)
	{
		//if ((cnt < 0) && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
		//{
		//	continue;//������������
		//}
		printf("�ͻ���<Socket%d>�Ѿ��˳����������\n", (int)clientSocket);
		return -1;//��������ѭ��
	}
	//������������
	switch (header->cmd)
	{
	case CMD_LOGIN:
	{
		printf("�յ�Socket<%d>���CMD_LOGIN ���ݳ��ȣ�%d\n", (int)clientSocket,header->dataLength);
		recv(clientSocket, szRecv + sizeof(header), header->dataLength - sizeof(header), 0);
		Login *login = (Login*)szRecv;
		//�ж��û������Ƿ���ȷ�Ĺ���
		printf("��¼�û�����%s ��¼�û����룺%s\n", login->userName, login->passWord);
		LoginResult ret;
		send(clientSocket, (char*)&ret, sizeof(ret), 0);
		break;
	}
	case CMD_LOGOUT:
	{
		printf("�յ�Socket<%d>���CMD_LOGOUT ���ݳ��ȣ�%d\n", (int)clientSocket,header->dataLength);
		recv(clientSocket, szRecv + sizeof(header), header->dataLength - sizeof(header), 0);
		Logout* logout = (Logout*)szRecv;
		printf("�ǳ��û�����%s \n", logout-> userName);
		LogoutResult ret;
		send(clientSocket, (char*)&ret, sizeof(ret), 0);
		break;
	}
	default:
	{
		header->cmd = CMD_ERROR;
		header->dataLength = 0;
		send(clientSocket, (char*)&header, sizeof(header), 0);
		break;
	}
	}
	
}
double timethis;
double timelast = timethis;
int main()
{  
	//����winsocket2.2
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		printf("Error:RequestWindowsSocketLibrary failed!\n");
		return 0;
	}
	/* ����IP��ַ */
	sockaddr_in servAddr;
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

	while (true)
	{
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;

		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		FD_SET(serverSocket,&fdRead);
		FD_SET(serverSocket, &fdWrite);
		FD_SET(serverSocket, &fdExp);

		for (int n = (int)g_client.size() -1; n >=0; n--)
		{
			FD_SET(g_client[n], &fdRead);
		}
		//selectģ��
		//��һ������ndfs��һ����������fd_set����������������(socket)�ķ�Χ������������
		//������(socket��һ������)
		//windows�п���д0
		//�����������ʱʱ�䣺NULLΪ����ģʽ��ʱ��Ϊ���ʱ��
		timeval timeout = { 1,0 };
		int ret = select(serverSocket +1,&fdRead, &fdWrite, &fdExp, &timeout);
		if (ret < 0)
		{
			printf("select�����Ѿ��˳����������\n");
			break;
		}
		if (FD_ISSET(serverSocket, &fdRead))
		{
			FD_CLR(serverSocket, &fdRead);
			//�ȴ����ܿͻ�������
			sockaddr_in cliAddr;
			int len = sizeof(cliAddr);
			SOCKET clientSocket;
			clientSocket = accept(serverSocket, (sockaddr*)&cliAddr, &len);
			if (clientSocket == INVALID_SOCKET)
			{
				printf("ERROR:accept failed��\n");
			}
			printf("Connected��Socket<%d> IP:%s Port��%d Connected success������\r\n",\
				(int)clientSocket, inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
			for (int n = (int)g_client.size() - 1; n >= 0; n--)
			{
				NewUserJoin userjoin;
				send(g_client[n], (const char*)&userjoin, sizeof(userjoin), 0);
			}
			g_client.push_back(clientSocket);
		}
		for (size_t n = 0;n< fdRead.fd_count; n++)
		{
			if (-1 == Processor(fdRead.fd_array[n]))
			{
				auto iter = find(g_client.begin(), g_client.end(), fdRead.fd_array[n]);
				if (iter != g_client.end())
				{
					g_client.erase(iter);
				}
			}
		}
		timelast = timethis;
		timethis = clock(); 
		if ((double)(timethis - timelast) / CLOCKS_PER_SEC >= 1)
		{
			printf("time: %f Do something else!!!!!!!!��\n", (double)(timethis - timelast) / CLOCKS_PER_SEC);

		}
	}
	for (int n = (int)g_client.size() - 1; n >= 0; n--)
	{

		closesocket(g_client[n]);
	}
	//closesocket(clientSocket);
	WSACleanup();
	return 1;
}