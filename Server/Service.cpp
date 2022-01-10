#include <WinSock2.h>
#include <MSWSock.h>
#include <stdio.h>
#include <string>
#include "Service.h"
#include "EnvVar.h"
#include "IoObj.h"
#include "FileObj.h"

void handleMess(LPSESSION session, char *mess, char *reply) {
    char cmd[BUFFSIZE], p1[BUFFSIZE], p2[BUFFSIZE];
	char res[BUFFSIZE];

    //Parse message
	parseMess(mess, cmd, p1, p2);

	if (!strcmp(cmd, STORE)) {
		LONG64 size = _atoi64(p2);
		if (strlen(p1) == 0 || strlen(p2) == 0 || size == 0)
			sprintf_s(res, BUFFSIZE, "%d%s%s", WRONG_SYNTAX, PARA_DELIMITER, "wrong parameter");
		else handleSTORE(session, p1, size, res);
	}
	else if (!strcmp(cmd, RETRIEVE)) {
		handleRETRIVE(session, p1, res);
	}
	else if (!strcmp(cmd, RECEIVE)) {
		handleRECEIVE(session, res);
	}
		
	else sprintf_s(res, BUFFSIZE, "%d%s%s", WRONG_SYNTAX, PARA_DELIMITER, "wrong header");

	sprintf_s(reply, BUFFSIZE, "%s%s%s%s", RESPONE, HEADER_DELIMITER, res, ENDING_DELIMITER);
}

void handleLOGIN(LPSESSION session, char *username, char *password, char *reply) {

}

void handleLOGOUT(LPSESSION session, char *reply) {

}

void handleREGISTER(char *username, char *password, char* reply) {

}

void handleRETRIVE(LPSESSION session, char *filename, char *reply) {
	LPFILEOBJ fileobj;
	HANDLE hFile;
	LARGE_INTEGER fileSize;


	//Open existing file
	hFile = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		printf("CreateFile failed with error %d\n", GetLastError());
		if (error == ERROR_FILE_NOT_FOUND)
			sprintf_s(reply, BUFFSIZE, "%d%s%s", FILE_ALREADY_EXIST, PARA_DELIMITER, "File not found");
		return;
	}

	GetFileSizeEx(hFile, &fileSize);
	fileobj = GetFileObj(hFile, fileSize.QuadPart);

	if (session->fileobj != NULL)
		session->closeFile();

	session->fileobj = fileobj;

	sprintf_s(reply, BUFFSIZE, "%d%s%lld", RETRIEVE_SUCCESS, PARA_DELIMITER, fileSize.QuadPart);
}

void handleRECEIVE(LPSESSION session, char * reply) {
	LPIO_OBJ sendFObj = getIoObject(IO_OBJ::SEND_F, NULL, 0);
	if (session->fileobj == NULL) {
		sprintf_s(reply, BUFFSIZE, "%d%s%s", SERVER_FAIL, PARA_DELIMITER, "Session fileobj null");
		return;
	}

	//Send file
	if (!TransmitFile(session->cmdSock, session->fileobj->file, session->fileobj->size, BUFFSIZE, &(sendFObj->overlapped), NULL, 0)) {
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
			printf("TransmitFile failed with error %d\n", error);
	}

	std::string resCode = std::to_string(FINISH_SEND);
	initParam(reply, resCode.c_str(), "Send file sucessful");
}

void handleSTORE(LPSESSION session, char * filename, LONG64 fileSize, char *reply) {
	char newfile[BUFFSIZE];
	LPFILEOBJ fileobj;

	//Create new file
	sprintf_s(newfile, BUFFSIZE, "%s-%d.txt", filename, session->cmdSock);
	HANDLE hFile = CreateFileA(newfile, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_FLAG_OVERLAPPED, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		printf("CreateFile failed with error %d\n", GetLastError());
		if (error == ERROR_FILE_EXISTS)
			sprintf_s(reply, BUFFSIZE, "%d%s%s", FILE_ALREADY_EXIST, PARA_DELIMITER, "file already exsit");
		return;
	}

	//Associates the file hanlde for writing
	if (CreateIoCompletionPort(hFile, completionPort, (ULONG_PTR)&(session->cmdSock), 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		FreeFileObj(fileobj);
		sprintf_s(reply, BUFFSIZE, "%d%s%s", SERVER_FAIL, PARA_DELIMITER, "CreateIoCompletionPort() failed");
		return;
	}

	fileobj = GetFileObj(hFile, fileSize);
	session->fileobj = fileobj;

	//Number of recveive file
	DWORD n = fileSize / BUFFSIZE;
	DWORD remain = fileSize % BUFFSIZE;

	for (int i = 0; i < n; ++i) {
		LPIO_OBJ recvFObj = getIoObject(IO_OBJ::RECV_F, NULL, BUFFSIZE);
		recvFObj->sequence = i;

		if (!PostRecv(session->cmdSock, recvFObj)) {
			sprintf_s(reply, BUFFSIZE, "%d%s%s", TRANSMIT_FAIL, PARA_DELIMITER, "Receive file fail");
			session->closeFile();
			return;
		}

		//InterlockedIncrement(&(fileobj->pending));
	}

	if (remain != 0) {
		LPIO_OBJ recvFObj = getIoObject(IO_OBJ::RECV_F, NULL, remain);
		recvFObj->sequence = n;

		if (!PostRecv(session->cmdSock, recvFObj)) {
			sprintf_s(reply, BUFFSIZE, "%d%s%s", TRANSMIT_FAIL, PARA_DELIMITER, "Receive file fail");
			session->closeFile();
			return;
		}

		//InterlockedIncrement(&(fileobj->pending));
	}

	sprintf_s(reply, BUFFSIZE, "%d%s%s", STORE_SUCCESS, PARA_DELIMITER, "Can start sending file");
}

void handleRENAME(LPSESSION session, char *oldname, char *newname, char *reply) {

}

void handleDELETE(LPSESSION session, char *pathname, char *reply) {

}

void handleMAKEDIR(LPSESSION session, char *pathname, char *reply) {

}

void handleREMOVEDIR(LPSESSION session, char *pathname, char *reply) {

}

void handleCHANGEWDIR(LPSESSION session, char *pathname, char *reply) {

}

void handlePRINTWDIR(LPSESSION session, char *reply) {

}

void handleLISTDIR(LPSESSION session, char *pathname, char *reply) {

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

void parseMess(const char *mess, char *cmd, char *p1, char *p2) {
	std::string strMess = mess;
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

