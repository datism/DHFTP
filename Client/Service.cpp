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

bool StoreRequest(LpSession session, char *sendBuff, const char *serverFile, const char *localFile) {
	LARGE_INTEGER fileSize;

	if (session->hfile != INVALID_HANDLE_VALUE)
		session->closeFile();

	session->hfile = CreateFileA(localFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (session->hfile == INVALID_HANDLE_VALUE) {
		printf("CreateFileA() failed with error %d\n", GetLastError());
		return false;
	}

	if (!GetFileSizeEx(session->hfile, &fileSize)) {
		printf("GetFileSizeEx() failed with error %d\n", GetLastError());
		return false;
	}

	session->fileSize = fileSize.QuadPart;

	initMessage(sendBuff, STORE, serverFile, session->fileSize);
}

bool RetrieveRequest(LpSession session, char *sendBuff, const char *serverFile, const char *localFile) {
	if (session->hfile != INVALID_HANDLE_VALUE)
		session->closeFile();
	session->hfile = CreateFileA(localFile, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (session->hfile == INVALID_HANDLE_VALUE) {
		printf("CreateFileA() failed with error %d\n", GetLastError());
		return false;
	}

	initMessage(sendBuff, RETRIEVE, serverFile, NULL);
	return true;
}

void chooseService(_Inout_ LpSession session, _Out_ char *sendBuff) {
	strcpy_s(sendBuff, BUFFSIZE, "");

	printf("\nChoose service\n");
	printf("1.LOGIN\n");
	printf("2.LOGOUT\n");
	printf("3.REGISTER\n");
	printf("4.STORE FILE\n");
	printf("5.RETRIEVE FILE\n");

	int choice;
	char p1[BUFFSIZE], p2[BUFFSIZE];
	while (1) {
		scanf_s("%d", &choice);

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
			printf("Enter server file path: ");
			gets_s(p1, BUFFSIZE);
			printf("Enter local file path: ");
			gets_s(p2, BUFFSIZE);
			if (!StoreRequest(session, sendBuff, p1, p2))
				break;
			return;
			//Retrieve
		case 5:
			printf("Enter server file path: ");
			gets_s(p1, BUFFSIZE);
			printf("Enter local file path: ");
			gets_s(p2, BUFFSIZE);
			if (!RetrieveRequest(session, sendBuff, p1, p2))
				break;
			return;
			//Invalid input
		default: {printf("Invalid input, choose again.\n"); continue; }
		}
	}

}

bool handleReply(LpSession session, const char *reply) {
	char header[BUFFSIZE], p1[BUFFSIZE], p2[BUFFSIZE];
	parseReply(reply, header, p1, p2);

	if (strcmp(header, "RES") || strlen(p1) == 0)
		return FALSE;

	int res = atoi(p1);

	switch (res) {
	case RETRIEVE_SUCCESS:
		session->fileSize = _atoi64(p2);
		return recvFile(session);
	case 221:return sendFile(session);
	case 120:
		session->closeFile();
		break;
	default:
		if (strlen(p1) != 0)
			printf("%d %s\n", res, p2);
		break;
	}

	return FALSE;
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