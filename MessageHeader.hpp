enum CMD
{
	CMD_SIGNUP,
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};
//��Ϣͷ
struct Dataheader {
	Dataheader()
	{
		dataLength = sizeof(Dataheader);
		cmd = CMD_ERROR;
	}
	short cmd;
	int dataLength;
};

struct Signup :public Dataheader {
	Signup()
	{
		cmd = CMD_SIGNUP;
		dataLength = sizeof(Signup);
	}
	char userName[32];
	char passWord[32];
	char data[28];
};
struct Login :public Dataheader {
	Login()
	{
		cmd = CMD_LOGIN;
		dataLength = sizeof(Login);
	}
	char userName[32];
	char passWord[32];
	char data[28];
};

struct LoginResult :public Dataheader {
	LoginResult()
	{
		cmd = CMD_LOGIN_RESULT;
		dataLength = sizeof(LoginResult);
		result = 0;
	}
	int result;
	char data[88];
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
		clientSocket = 0;
	}
	int clientSocket;
};
