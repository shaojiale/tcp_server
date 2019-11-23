#include "tcpserver.hpp"


class MyServer : public TcpServer
{
public:
	//�ͻ��˼����¼�
	virtual void OnNetJoin(TcpClient* pClient)
	{
		_clientCount++;
		printf("client<%d> join\n", (int)pClient->sockfd());
	}
	//�ͻ����뿪�¼�
	virtual void OnNetLeave(TcpClient* pClient)
	{
		_clientCount++;
		printf("client<%d> leave\n", (int)pClient->sockfd());
	}
	//��Ӧ������Ϣ
	virtual void OnNetMsg(TcpClient* pClient, Dataheader* header)
	{
		_recvCount++;
		//������������
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			//printf("�յ�Socket<%d>���CMD_LOGIN ���ݳ��ȣ�%d\n", (int)csock, header->dataLength);
			//Login *login = (Login *)header;
			//�ж��û������Ƿ���ȷ�Ĺ���
			//printf("��¼�û�����%s ��¼�û����룺%s\n", login->userName, login->passWord);
			//LoginResult ret;
			//pClient->SendData(&ret);
			break;
		}
		case CMD_LOGOUT:
		{
			//printf("�յ�Socket<%d>���CMD_LOGOUT ���ݳ��ȣ�%d\n", (int)csock, header->dataLength);
			//Logout *logout = (Logout *)header;
			//printf("�ǳ��û�����%s \n", logout->userName);
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
private:

};


int main()
{  
	MyServer server;
	server.InitSocket();
	server.Bind(nullptr, 1245);
	server.Listen(20);
	server.Start(6);
	while (true)
	{
		server.OnRun();
	}
	server.Close();
	return 0;
}