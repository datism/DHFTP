#include "Service.h"
#include <stdio.h>
#include <sstream>
#include <string>
#include "Envar.h"
#include "Io.h"

void LoginRequest(char *sendBuff, const char *username, const char *password) {
	initMessage(sendBuff, LOGIN, username, password);
}

void LogoutRequest(char *sendBuff) {
	initMessage(sendBuff, LOGOUT, NULL, NULL);
}

void RegisterRequest(char *sendBuff, const char *username, const char *password) {
	initMessage(sendBuff, REGISTER, username, password);
}

bool StoreRequest(LpSession session, char *sendBuff, const char *fileName) {
	LARGE_INTEGER fileSize;

	if (session->hfile != INVALID_HANDLE_VALUE)
		session->closeFile();

	session->hfile = CreateFileA(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (session->hfile == INVALID_HANDLE_VALUE) {
		printf("CreateFileA() failed with error %d\n", GetLastError());
		return false;
	}

	if (!GetFileSizeEx(session->hfile, &fileSize)) {
		printf("GetFileSizeEx() failed with error %d\n", GetLastError());
		return false;
	}

	session->fileSize = fileSize.QuadPart;

	initMessage(sendBuff, STORE, fileName, session->fileSize);
}


bool RetrieveRequest(LpSession session, char * sendBuff, const char * fileName) {
	if (session->hfile != INVALID_HANDLE_VALUE)
		session->closeFile();
	session->hfile = CreateFileA(fileName, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (session->hfile == INVALID_HANDLE_VALUE) {
		printf("CreateFileA() failed with error %d\n", GetLastError());
		return false;
	}

	initMessage(sendBuff, RETRIEVE, fileName, NULL);
	return true;
}

void RenameRequest(char *sendBuf, const char *serverFile, const char *newname) {
	initMessage(sendBuf, RENAME, serverFile, newname);
}

void DeleteRequest(char *sendBuf, const char *serverFile) {
	initMessage(sendBuf, DELETEFILE, serverFile, NULL);
}

void MakeDirRequest(char *sendBuf, const char *dirPath) {
	initMessage(sendBuf, MAKEDIR, dirPath, NULL);
}

void RemoveDirRequest(char *sendBuf, const char *dirPath) {
	initMessage(sendBuf, REMOVEDIR, dirPath, NULL);
}

void ChangeWDirRequest(char *sendBuf, const char *dirPath) {
	initMessage(sendBuf, CHANGEWDIR, dirPath, NULL);
}

void PrintWDirRequest(char *sendBuf) {
	initMessage(sendBuf, PRINTWDIR);
}

void ListDirRequest(char *sendBuf, const char *dirPath) {
	initMessage(sendBuf, LISTDIR, dirPath, NULL);
}

void chooseService(_Inout_ LpSession session, _Out_ char *sendBuff) {
	strcpy_s(sendBuff, BUFFSIZE, "");
	int choice;
	char p1[BUFFSIZE], p2[BUFFSIZE];

	while (1) {
		printf("\nChoose service: ");
		scanf_s("%d", &choice);
		printf("\n");

		//Clear input buffer
		int c;
		while ((c = getchar()) != '\n');

		switch (choice) {
			//Login
		case 1:
			printf("Enter username: ");
			gets_s(p1, BUFFSIZE);
			printf("Enter password: ");
			gets_s(p2, BUFFSIZE);
			LoginRequest(sendBuff, p1, p2);
			return;
			//Logout
		case 2:
			LogoutRequest(sendBuff);
			return;
			//Register
		case 3:
			printf("Enter username: ");
			gets_s(p1, BUFFSIZE);
			printf("Enter password: ");
			gets_s(p2, BUFFSIZE);
			RegisterRequest(sendBuff, p1, p2);
			return;
			//Store file
		case 4:
			printf("Enter file name: ");
			gets_s(p1, BUFFSIZE);
			if (!StoreRequest(session, sendBuff, p1))
				break;
			return;
			//Retrieve
		case 5:
			printf("Enter file name: ");
			gets_s(p1, BUFFSIZE);
			if (!RetrieveRequest(session, sendBuff, p1))
				break;
			return;
		case 6:
			printf("Enter server file path: ");
			gets_s(p1, BUFFSIZE);
			printf("Enter new name: ");
			gets_s(p2, BUFFSIZE);
			RenameRequest(sendBuff, p1, p2);
			break;
		case 7:
			printf("Enter server file path: ");
			gets_s(p1, BUFFSIZE);
			DeleteRequest(sendBuff, p1);
			break;
		case 8:
			printf("Enter directory path: ");
			gets_s(p1, BUFFSIZE);
			MakeDirRequest(sendBuff, p1);
			break;
		case 9:
			printf("Enter directory path: ");
			gets_s(p1, BUFFSIZE);
			RemoveDirRequest(sendBuff, p1);
			break;
		case 10:
			printf("Enter directory path: ");
			gets_s(p1, BUFFSIZE);
			ChangeWDirRequest(sendBuff, p1);
			break;
		case 11:
			printf("Enter directory path: ");
			gets_s(p1, BUFFSIZE);
			PrintWDirRequest(sendBuff);
			break;
		case 12:
			printf("Enter directory path: ");
			gets_s(p1, BUFFSIZE);
			ListDirRequest(sendBuff, p1);
			break;
			//Invalid input
		default: {printf("Invalid input, choose again.\n"); continue; }
		}
	}

}

void handleReply(LpSession session, const char *reply) {
	char header[BUFFSIZE], p1[BUFFSIZE], p2[BUFFSIZE];
	parseReply(reply, header, p1, p2);

	if (strcmp(header, "RES") || strlen(p1) == 0)
		return;

	int res = atoi(p1);

	switch (res) {
	case RETRIEVE_SUCCESS:
		session->fileSize = _atoi64(p2);

		session->fileSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		printf("Connecting to file port.....\n");
		if (connect(session->fileSock, (sockaddr *)&gFileAddr, sizeof(gFileAddr))) {
			printf("connect() failed with error %d", WSAGetLastError());
			closesocket(session->fileSock);
			break;
		}
		printf("Receiving file.....\n");
		if (recvFile(session))
			printf("Recieve file sucessful\n");
		session->closeFile();
		closesocket(session->fileSock);
		break;
	case STORE_SUCCESS:
		session->fileSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		printf("Connecting to file port.....\n");
		if (connect(session->fileSock, (sockaddr *)&gFileAddr, sizeof(gFileAddr))) {
			printf("connect() failed with error %d", WSAGetLastError());
			closesocket(session->fileSock);
			break;
		}

		printf("Sending file.....\n");
		if(sendFile(session)) 
			printf("Send file sucessful\n");
		session->closeFile();
		closesocket(session->fileSock);
		break;
	case FINISH_SEND:
		session->closeFile();
		closesocket(session->fileSock);
		break;
	default:
		if (strlen(p2) != 0) {
			printf("%s\n", p2);
		}
		break;
	}
}

void parseReply(const char *reply, char *cmd, char *p1, char *p2) {
	std::string strMess = reply;
	std::string strCmd, strP1, strP2;
	int lenStr = strMess.length(), crPos = strMess.find(HEADER_DELIMITER), spPos = strMess.find(PARA_DELIMITER);
	if (crPos == -1) {
		std::string strCmd = strMess.substr(0, lenStr);
		strcpy_s(cmd, BUFFSIZE, strCmd.c_str());
	}
	else {
		strCmd = strMess.substr(0, crPos);
		strcpy_s(cmd, BUFFSIZE, strCmd.c_str());
		if (spPos == -1) {
			strP1 = strMess.substr(crPos + 1, lenStr - crPos - 1);
			strcpy_s(p1, BUFFSIZE, strP1.c_str());
		}
		else {
			strP1 = strMess.substr(crPos + 1, spPos - crPos - 1);
			strcpy_s(p1, BUFFSIZE, strP1.c_str());
			strP2 = strMess.substr(spPos + 1, lenStr - spPos - 1);
			strcpy_s(p2, BUFFSIZE, strP2.c_str());
		}
	}
}

void usage() {
	printf("Command are:\n");
	printf("login <username> <password>\n");
	printf("logout <username>\n");
	printf("reg <username> <password>\n");
	printf("get <path-to-file> <save-as>\n");
	printf("put <path-to-file> <save-as>\n");
	printf("rn <path-to-file> <new-name>\n");
	printf("del <path-to-file>\n");
	printf("mkdr <path-to-dir>\n");
	printf("rmdr <path-to-dir>\n");
	printf("cd <path-to-dir>\n");
	printf("pwd <path-to-dir>\n");
	printf("ls <path-to-dir>\n");
}

void initParam(char *param) {}

template <typename P>
void initParam(char *param, P p) {
	std::ostringstream sstr;
	sstr << p;
	strcat_s(param, BUFFSIZE, sstr.str().c_str());
}

template <typename P, typename... Args>
void initParam(char *param, P p, Args... paras) {
	std::ostringstream sstr;

	sstr << p << PARA_DELIMITER;
	strcat_s(param, BUFFSIZE, sstr.str().c_str());

	initParam(param, paras...);
}
