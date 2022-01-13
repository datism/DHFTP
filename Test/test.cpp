#include <Windows.h>
#include <fileapi.h>
#include <string.h>
#include <string>
#include <sstream>
#include <stdio.h>

typedef struct SESSION {
	SOCKET cmdSock;
	char username[MAX_PATH];
	char workingDir[MAX_PATH];

	void setUsername(const char *iUsername);
	void setWorkingDir(const char *iWorkingDir);
	void closeFile();
} SESSION, *LPSESSION;

void SESSION::setUsername(const char * iUsername) {
	strcpy_s(this->username, MAX_PATH, iUsername);
}

void SESSION::setWorkingDir(const char * iWorkingDir) {
	strcpy_s(this->workingDir, MAX_PATH, iWorkingDir);
}

#define SERVER_ADDR "127.0.0.1"
#define CMD_PORT 5500
#define BUFFSIZE 4096
#define ENDING_DELIMITER "\"End\""
#define HEADER_DELIMITER "\"Head\""
#define PARA_DELIMITER "\"Para\""

#define LOGIN "LOGI"
#define LOGOUT "LOGO"
#define REGISTER "REG"
#define RETRIEVE "RETR"
#define STORE "STOR"
#define RENAME "RN"
#define DELETEFILE "DEL"
#define MAKEDIR "MKD"
#define REMOVEDIR "RMD"
#define CHANGEWDIR "CWD"
#define PRINTWDIR "PWD"
#define LISTDIR "LIST"
#define RESPONE "RES"
#define RECEIVE "RECV"

enum REPLY_CODE {
	LOGIN_SUCCESS = 110,
	LOGOUT_SUCCESS = 111,
	REGISTER_SUCCESS = 112,

	NOT_LOGIN = 310,
	ALREADY_LOGIN = 311,
	ID_NOT_EXIST = 312,
	ID_ALREADY_EXIST = 313,
	WRONG_PASSWORD = 314,

	RETRIEVE_SUCCESS = 220,
	STORE_SUCCESS = 221,
	FINISH_SEND = 120,
	RENAME_SUCCESS = 121,
	DELETE_SUCCESS = 122,
	MAKEDIR_SUCCESS = 123,
	REMOVEDIR_SUCCESS = 124,
	CHANGEWDIR_SUCCESS = 125,
	PRINTWDIR_SUCCESS = 126,
	LIST_SUCCESS = 127,

	NO_ACCESS = 320,
	FILE_NOT_EXIST = 321,
	FILE_ALREADY_EXIST = 322,
	NAME_WRONG_FORMAT = 323,
	TRANSMIT_FAIL = 324,

	WRONG_SYNTAX = 330,
	SERVER_FAIL = 331
};

template <typename T, typename X>
void initParam(char *param, const T p1, const X p2);

template <typename T, typename X>
void initMessage(char *mess, const char *header, const T p1, const X p2) {
	char param[BUFFSIZE];

	initParam(param, p1, p2);

	if (strlen(param) == 0)
		sprintf_s(mess, BUFFSIZE, "%s%s", header, ENDING_DELIMITER);
	else
		sprintf_s(mess, BUFFSIZE, "%s%s%s%s", header, HEADER_DELIMITER, param, ENDING_DELIMITER);
}

template <typename T, typename X>
void initParam(char *param, const T p1, const X p2) {
	strcpy_s(param, BUFFSIZE, "");
	std::ostringstream sstr;

	if (p1 == NULL)
		return;

	if (p2 == NULL)
		sstr << p1;
	else
		sstr << p1 << PARA_DELIMITER << p2;

	strcpy_s(param, BUFFSIZE, sstr.str().c_str());
}

bool checkAccess(LPSESSION session, char *path) {
	char rootPath[MAX_PATH];
	char fullPath[MAX_PATH];
	char temp[MAX_PATH];

	sprintf_s(temp, MAX_PATH, "%s%s%s", session->workingDir, "\\", path);

	DWORD rootLength = GetFullPathNameA(session->username, MAX_PATH, rootPath, NULL);
	DWORD pathLength = GetFullPathNameA(temp, MAX_PATH, fullPath, NULL);

	if (rootLength != 0 && pathLength != 0 && strstr(fullPath, rootPath) != NULL) {
		strcpy_s(path, MAX_PATH, fullPath);
		return TRUE;
	}

	strcpy_s(path, MAX_PATH, "");
	return FALSE;
}


int main() {
	//char mess1[BUFFSIZE], mess2[BUFFSIZE], mess3[BUFFSIZE], reply[BUFFSIZE];
	//initMessage(mess1, "abc", "123", "456");
	//printf("Mess1 : %s\n", mess1);
	//initMessage(mess2, "abc", "123", NULL);
	//printf("Mess2 : %s\n", mess2);
	//initMessage(mess3, "abc", NULL, NULL);
	//printf("Mess3 : %s\n", mess3);

	////std::string res = std::to_string(LOGIN_SUCCESS);
	//initMessage(reply, RESPONE, LOGIN_SUCCESS, NULL);
	//printf("RES : %s\n", reply);
	//



	SESSION session;
	session.setUsername("dat");
	session.setWorkingDir("dat");
	char path[MAX_PATH] = "smth";

	if (checkAccess(&session, path))
		printf("%s\n", path);
	if (!CreateDirectoryA(path, NULL))
		printf("CreateDirectoryA() failed with error %d\n", GetLastError());
	strcpy_s(path, MAX_PATH, "..\\..\\smth");
	if (!checkAccess(&session, path))
		printf("CHILL\n");
}