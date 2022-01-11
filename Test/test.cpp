#include <Windows.h>
#include <fileapi.h>
#include <string.h>
#include <string>
#include <sstream>
#include <stdio.h>


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

//char *dat = "dat";
//char *hiep = "hiep";


int main() {
	char mess1[BUFFSIZE], mess2[BUFFSIZE], mess3[BUFFSIZE], reply[BUFFSIZE];
	initMessage(mess1, "abc", "123", "456");
	printf("Mess1 : %s\n", mess1);
	initMessage(mess2, "abc", "123", NULL);
	printf("Mess2 : %s\n", mess2);
	initMessage(mess3, "abc", NULL, NULL);
	printf("Mess3 : %s\n", mess3);

	//std::string res = std::to_string(LOGIN_SUCCESS);
	initMessage(reply, RESPONE, LOGIN_SUCCESS, NULL);
	printf("RES : %s\n", reply);
	
	/*char *datDir = "\\dat";*/
	/*if (SetCurrentDirectory(datDir)) {
		printf("SetCurrentDirectory() failed with error %d\n", GetLastError());
		return 1;
	}*/

	//char full_path[MAX_PATH];
	//char *filepath = "\\hiep\\hiepTest.txt";
	//char *fileName[MAX_PATH];
	//DWORD length = GetFullPathName(datDir, MAX_PATH, full_path, fileName);

	//if (length == 0) {
	//	printf("GetFullPathName() failed with error %d\n", GetLastError());
	//}
	//else {
	//	printf("full path: %s\n", full_path);
	//	printf("file name: %s\n", *fileName);
	//}

	/*char longPath[MAX_PATH];
	length = GetLongPathNameA(full_path, longPath, MAX_PATH);
	if (length == 0) {
		printf("GetLongPathNameA() failed with error %d\n", GetLastError());
	}
	else {
		printf("long path: %s\n", longPath);
	}*/

	//CreateDirectory(userName1, NULL);
}