#include "Service.h"
#include <stdio.h>
#include <sstream>
#include <string>
#include "Envar.h"
#include "FileObj.h"
#include "Io.h"

void LoginRequest(char *sendBuff, const char *username, const char *password) {
	initMessage(sendBuff, LOGIN, username, password);
}

void LogoutRequest(char *sendBuff) {
	initMessage(sendBuff, LOGOUT);
}

void RegisterRequest(char *sendBuff, const char *username, const char *password) {
	initMessage(sendBuff, REGISTER, username, password);
}

bool StoreRequest(LpSession session, char *sendBuff, const char *fileName) {
	HANDLE hfile;
	LARGE_INTEGER fileSize;

	if (session->fileobj != NULL)
		session->closeFile(FALSE);

	//Open existing file
	hfile = CreateFileA(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) {
		printf("CreateFileA() failed with error %d\n", GetLastError());
		return false;
	}
	else {
		//get file size
		if (!GetFileSizeEx(hfile, &fileSize)) {
			printf("GetFileSizeEx() failed with error %d\n", GetLastError());
			return false;
		}

		session->fileobj = GetFileObj(hfile, fileSize.QuadPart, FILEOBJ::STOR);
		initMessage(sendBuff, STORE, fileName, session->fileobj->size);

		return true;
	}
}

bool RetrieveRequest(LpSession session, char * sendBuff, const char * fileName) {
	HANDLE hfile;

	if (session->fileobj != NULL)
		session->closeFile(TRUE);

	//open new file
	hfile = CreateFileA(fileName, GENERIC_WRITE | DELETE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE) {
		printf("CreateFileA() failed with error %d\n", GetLastError());
		return false;
	}
	else {
		session->fileobj = GetFileObj(hfile, 0, FILEOBJ::RETR);
		initMessage(sendBuff, RETRIEVE, fileName);

		return true;
	}
}

void RenameRequest(char *sendBuf, const char *serverFile, const char *newname) {
	initMessage(sendBuf, RENAME, serverFile, newname);
}

void DeleteRequest(char *sendBuf, const char *serverFile) {
	initMessage(sendBuf, DELETEFILE, serverFile);
}

void MakeDirRequest(char *sendBuf, const char *dirPath) {
	initMessage(sendBuf, MAKEDIR, dirPath);
}

void RemoveDirRequest(char *sendBuf, const char *dirPath) {
	initMessage(sendBuf, REMOVEDIR, dirPath);
}

void ChangeWDirRequest(char *sendBuf, const char *dirPath) {
	initMessage(sendBuf, CHANGEWDIR, dirPath);
}

void PrintWDirRequest(char *sendBuf) {
	initMessage(sendBuf, PRINTWDIR);
}

void ListDirRequest(char *sendBuf, const char *dirPath) {
	initMessage(sendBuf, LISTDIR, dirPath);
}

void chooseService(_Inout_ LpSession session, _Out_ char *sendBuff) {
	strcpy_s(sendBuff, BUFFSIZE, "");
	int choice;
	char p1[BUFFSIZE], p2[BUFFSIZE];

	while (1) {
		printf("\nChoose service: ");
		if (scanf_s("%d", &choice) == 0)
			choice = 0;
		printf("\n");

		//Clear input buffer
		int c;
		while ((c = getchar()) != '\n' && c != EOF) {}

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
				/*strcpy_s(p1, BUFFSIZE, "testbig.rar");*/
				if (!StoreRequest(session, sendBuff, p1))
					break;
				return;
			//Retrieve
			case 5:
				printf("Enter file name: ");
				gets_s(p1, BUFFSIZE);
				/*strcpy_s(p1, BUFFSIZE, "testmid.rar");*/
				if (!RetrieveRequest(session, sendBuff, p1))
					break;
				return;
			case 6:
				printf("Enter server file path: ");
				gets_s(p1, BUFFSIZE);
				printf("Enter new name: ");
				gets_s(p2, BUFFSIZE);
				RenameRequest(sendBuff, p1, p2);
				return;
			case 7:
				printf("Enter server file path: ");
				gets_s(p1, BUFFSIZE);
				DeleteRequest(sendBuff, p1);
				return;
			case 8:
				printf("Enter directory path: ");
				gets_s(p1, BUFFSIZE);
				MakeDirRequest(sendBuff, p1);
				return;
			case 9:
				printf("Enter directory path: ");
				gets_s(p1, BUFFSIZE);
				RemoveDirRequest(sendBuff, p1);
				return;
			case 10:
				printf("Enter directory path: ");
				gets_s(p1, BUFFSIZE);
				ChangeWDirRequest(sendBuff, p1);
				return;
			case 11:
				PrintWDirRequest(sendBuff);
				return;
			case 12:
				printf("Enter directory path: ");
				gets_s(p1, BUFFSIZE);
				ListDirRequest(sendBuff, p1);
				return;
			//Invalid input
			default: printf("Invalid input, choose again.\n"); 
		}
	}

}

void handleReply(LpSession session, const char *reply) {
	char header[BUFFSIZE] = "", request[BUFFSIZE] = "";
	const char *p1, *p2, *p3;
	std::vector<std::string> para;

	newParseReply(reply, header, para);

	if (strcmp(header, "RES") || para.size() < 1)
		return;

	p1 = para[0].c_str();

	int res = atoi(p1);

	switch (res) {
	case RETRIEVE_SUCCESS:
		if (para.size() != 3) {
			session->closeFile(TRUE);
			return;
		}

		//session key
		p2 = para[1].c_str();
		//file size
		p3 = para[2].c_str();

		initMessage(request, CONNECT, p2);
		session->fileobj->size = _atoi64(p3);

		//open file connection
		session->fileobj->fileSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		printf("Connecting to file port.....\n");
		if (connect(session->fileobj->fileSock, (sockaddr *)&gFileAddr, sizeof(gFileAddr))) {
			printf("connect() failed with error %d", WSAGetLastError());
			session->closeFile(TRUE);
			break;
		}
		//send session key
		blockSend(session->fileobj->fileSock, request);

		printf("Receiving file.....\n");
		//receive file
		if (recvFile(session->fileobj->fileSock, session->fileobj->file, session->fileobj->size))
			printf("Recieve file sucessful\n");

		session->closeFile(FALSE);
		break;
	case STORE_SUCCESS:
		if (para.size() != 2) {
			session->closeFile(FALSE);
			return;
		}

		//session key
		p2 = para[1].c_str();
		initMessage(request, CONNECT, p2);

		//open file connection
		session->fileobj->fileSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		printf("Connecting to file port.....\n");
		if (connect(session->fileobj->fileSock, (sockaddr *)&gFileAddr, sizeof(gFileAddr))) {
			printf("connect() failed with error %d", WSAGetLastError());
			session->closeFile(FALSE);
			break;
		}
		//send session key
		blockSend(session->fileobj->fileSock, request);

		printf("Sending file.....\n");
		//send file
		if (sendFile(session->fileobj->fileSock, session->fileobj->file, session->fileobj->size))
			printf("Send file sucessful\n");
		session->closeFile(FALSE);
		break;
	default:
		if (session->fileobj != NULL)
			session->closeFile(TRUE);

		if (para.size() > 1) {
			p2 = para[1].c_str();
			printf("%s\n", p2);
		}
		break;
	}
}

void newParseReply(const char *reply, char *cmd, std::vector<std::string> &para) {
	std::string strMess = reply;
	std::string strCmd, strP;
	int crPos, spPos, lenStr;

	lenStr = strMess.length();

	crPos = strMess.find(HEADER_DELIMITER);

	if (crPos == -1) {
		strcpy_s(cmd, BUFFSIZE, strMess.c_str());
	}
	else {
		strCmd = strMess.substr(0, crPos);
		strcpy_s(cmd, BUFFSIZE, strCmd.c_str());

		strP = strMess.substr(crPos + strlen(HEADER_DELIMITER), lenStr - crPos - strlen(HEADER_DELIMITER));
		spPos = strP.find(PARA_DELIMITER);

		while (1) {
			spPos = strP.find(PARA_DELIMITER);

			if (spPos == -1) {
				para.push_back(strP);
				break;
			}
			else {
				para.push_back(strP.substr(0, spPos));
				strP = strP.substr(spPos + strlen(PARA_DELIMITER), strP.length() - spPos - strlen(PARA_DELIMITER));
			}
		}
	}
}

void initParam(char *param) {}

template <typename P>
void initParam(char *param, P p) {
	std::ostringstream sstr;

	//param + para
	sstr << p;
	strcat_s(param, BUFFSIZE, sstr.str().c_str());
}

template <typename P, typename... Args>
void initParam(char *param, P p, Args... paras) {
	std::ostringstream sstr;

	//param + para + para delimiter
	sstr << p << PARA_DELIMITER;
	strcat_s(param, BUFFSIZE, sstr.str().c_str());

	//recursion
	initParam(param, paras...);
}
