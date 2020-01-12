

#include "Alloc.h"
#include "tcpserver.hpp"
#include <mysql.h>
bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			printf("�˳�cmdThread�߳�\n");
			break;
		}
		else {
			printf("��֧�ֵ����\n");
		}
	}
}
MYSQL mysql;    //һ�����ݿ�ṹ��
class MyServer : public TcpServer
{
public:
	//�ͻ��˼����¼�
	virtual void OnNetJoin(ClientSocketPtr& pClient)
	{
		TcpServer::OnNetJoin(pClient);
		//printf("client<%d> join\n", (int)pClient->sockfd());
	}
	//�ͻ����뿪�¼�
	virtual void OnNetLeave(ClientSocketPtr& pClient)
	{
		TcpServer::OnNetLeave(pClient);
		printf("client<%d> leave\n", (int)pClient->sockfd());
	}
	//��Ӧ������Ϣ
	virtual void OnNetMsg(CellServer* pCellServer, ClientSocketPtr& pClient, Dataheader* header)
	{
		TcpServer::OnNetMsg(pCellServer, pClient, header);
		//������������
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			printf("�յ��ͻ���<Socket=%d>����CMD_LOGIN,���ݳ��ȣ�%d,userName=%s PassWord=%s\n",\
				pClient->sockfd(), login->dataLength, login->userName, login->passWord);
			char insertsql[1024];
			int len = sprintf_s(insertsql, 256, "INSERT INTO userinformation values ('%s','%s');", \
				login->userName, login->passWord);
			mysql_query(&mysql, insertsql);

			auto ret = std::make_shared<LoginResult>();
			pCellServer->addSendTask(pClient,(DataHeaderPtr&)ret);
			//pClient->SendData(ret);
			break;
		}
		case CMD_LOGOUT:
		{
			//LogoutResult ret;
			//pClient->SendData(&ret);
			break;
		}
		case CMD_ERROR:
		{
			printf("�յ�<�ͻ���%d>������Ϣ�����ݳ��ȣ�%d\n", (int)pClient->sockfd(), header->dataLength);
			break;
		}
		default:
		{
			printf("�յ�<�ͻ���%d>δ֪��Ϣ�����ݳ��ȣ�%d\n", (int)pClient->sockfd(), header->dataLength);
			break;
		}
		}

	}
	virtual void OnNetRecv(TcpClient * pClient)
	{
		_recvCount++;
	}
private:

};


int main()
{  

	mysql_init(&mysql);
	//���ñ��뷽ʽ
	mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "gbk");
	//�������ݿ�
	//�ж��������ʧ�ܾ��������ʧ�ܡ�
	if (mysql_real_connect(&mysql, "localhost", "root", "sjl@142010", "test", 3306, NULL, 0) == NULL)
		printf("����ʧ�ܣ�\\n");
	mysql_query(&mysql, "use users");
	MyServer server;
	server.InitSocket();
	server.Bind(nullptr, 1245);
	server.Listen(20);
	server.Start(6);

	//����UI�߳�
	std::thread t1(cmdThread);
	t1.detach();

	while (g_bRun)
	{
		server.OnRun();
	}
	server.Close();
	printf("���˳���\n");
	getchar();
	return 0;
}