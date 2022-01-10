#include "Service.h"
#include <stdio.h>
#include <string>
#include "Envar.h"
#include "Io.h"

void parseReply(const char *reply, char *cmd, char *p1, char *p2);

void chooseService(_Inout_ LpSession session, _Out_ char *sendBuff) {
	*sendBuff = 0;
	printf("Choose service\n");
	getchar();

	/*char *fileName = "test.txt";
	if (session->hfile == INVALID_HANDLE_VALUE) {
		session->hfile = CreateFileA(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (session->hfile == INVALID_HANDLE_VALUE) {
			printf("CreateFileA() failed with error %d\n", GetLastError());
			return;
		}
	}
	else {
		printf("Close previous file handle\n");
		return;
	}

	GetFileSizeEx(session->hfile, &session->fileSize);

	sprintf_s(sendBuff, BUFFSIZE, "STOR\r%s %lld\r\n", fileName, session->fileSize.QuadPart);*/

	char *fileName = "test.txt";
	char *newName = "test-client.txt";
	if (session->hfile != INVALID_HANDLE_VALUE)
		session->closeFile();
	session->hfile = CreateFileA(newName, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (session->hfile == INVALID_HANDLE_VALUE) {
		printf("CreateFileA() failed with error %d\n", GetLastError());
		return;
	}
	
	initMessage(sendBuff, RETRIEVE, fileName, NULL);
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

void initMessage(char *mess, const char *header, const char *p1, const char *p2) {
	char param[BUFFSIZE];

	initParam(param, p1, p2);

	if (strlen(param) == 0)
		sprintf_s(mess, BUFFSIZE, "%s%s", header, ENDING_DELIMITER);
	else
		sprintf_s(mess, BUFFSIZE, "%s%s%s%s", header, HEADER_DELIMITER, param, ENDING_DELIMITER);
}

void initParam(char *param, const char *p1, const char *p2) {
	*param = 0;

	if (p1 == NULL)
		return;

	if (p2 == NULL) {
		strcpy_s(param, BUFFSIZE, p1);
		return;
	}

	sprintf_s(param, BUFFSIZE, "%s%s%s", p1, PARA_DELIMITER, p2);
}