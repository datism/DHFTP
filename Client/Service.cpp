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

void ChangePasRequest(char *sendBuff, const char *oPassword, const char *nPassword) {
	initMessage(sendBuff, CHANGEPASS, oPassword, nPassword);
}

bool StoreRequest(LpSession session, char *sendBuff, const char *localFile, const char *remoteFile) {
	HANDLE hfile;
	LARGE_INTEGER fileSize;

	if (session->fileobj != NULL)
		session->closeFile(FALSE);

	//Open existing file
	hfile = CreateFileA(localFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hfile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND)
			printf("Cant find local file\n");
		else
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
		if (session->fileobj != NULL) {
			initMessage(sendBuff, STORE, remoteFile, session->fileobj->size);
			return true;
		}
		else
			return FALSE;
	}
}

bool RetrieveRequest(LpSession session, char * sendBuff, const char *localFile, const char *remoteFile) {
	HANDLE hfile;

	if (session->fileobj != NULL)
		session->closeFile(TRUE);

	//open new file
	hfile = CreateFileA(localFile, GENERIC_WRITE | DELETE, 0, NULL, CREATE_NEW, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hfile == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		if (error == ERROR_FILE_EXISTS)
			printf("Local file alredy exist\n");
		else 
			printf("CreateFileA() failed with error %d\n", GetLastError());
		return false;
	}
	else {
		session->fileobj = GetFileObj(hfile, 0, FILEOBJ::RETR);
		initMessage(sendBuff, RETRIEVE, remoteFile);

		return true;
	}
}

void MoveRequest(char *sendBuf, const char *oldPath, const char *newPath) {
	initMessage(sendBuf, MOVE, oldPath, newPath);
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

void ChooseService(_Inout_ LpSession session, _Out_ char *sendBuff) {
	strcpy_s(sendBuff, BUFFSIZE, "");
	int choice;
	char p1[BUFFSIZE] = "", p2[BUFFSIZE] = "";

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
			//Change pass
			case 4:
				printf("Enter old password: ");
				gets_s(p1, BUFFSIZE);
				printf("Enter new password: ");
				gets_s(p2, BUFFSIZE);
				ChangePasRequest(sendBuff, p1, p2);
				return;
			//Store file
			case 5:
				printf("Enter local file path: ");
				gets_s(p1, BUFFSIZE);
				printf("Enter remote file path: ");
				gets_s(p2, BUFFSIZE);
				/*strcpy_s(p1, BUFFSIZE, "testbig.rar");*/
				if (!StoreRequest(session, sendBuff, p1, p2))
					break;
				return;
			//Retrieve
			case 6:
				printf("Enter remote file path: ");
				gets_s(p1, BUFFSIZE);;
				printf("Enter local file path: ");
				gets_s(p2, BUFFSIZE);
				/*strcpy_s(p1, BUFFSIZE, "testmid.rar");*/
				if (!RetrieveRequest(session, sendBuff, p2, p1))
					break;
				return;
			//Move 
			case 7:
				printf("Enter old path: ");
				gets_s(p1, BUFFSIZE);
				printf("Enter new path: ");
				gets_s(p2, BUFFSIZE);
				MoveRequest(sendBuff, p1, p2);
				return;
			//Delete file
			case 8:
				printf("Enter server file path: ");
				gets_s(p1, BUFFSIZE);
				DeleteRequest(sendBuff, p1);
				return;
			//Make Directory
			case 9:
				printf("Enter directory path: ");
				gets_s(p1, BUFFSIZE);
				MakeDirRequest(sendBuff, p1);
				return;
			//Remove Directory
			case 10:
				printf("Enter directory path: ");
				gets_s(p1, BUFFSIZE);
				RemoveDirRequest(sendBuff, p1);
				return;
			//Change working directory
			case 11:
				printf("Enter directory path: ");
				gets_s(p1, BUFFSIZE);
				ChangeWDirRequest(sendBuff, p1);
				return;
			//Print working directory
			case 12:
				PrintWDirRequest(sendBuff);
				return;
			//List directory
			case 13:
				printf("Enter directory path: ");
				gets_s(p1, BUFFSIZE);
				ListDirRequest(sendBuff, p1);
				return;
			//Help
			case 14:
				Help();
				break;
			//Exit
			case 15:
				FreeSession(session);
				WSACleanup();
				ExitProcess(-1);
			//Invalid input
			default: printf("Invalid input, choose again.\n"); 
		}
	}

}

void HandleReply(LpSession session, const char *reply) {
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
		blockSend(session->fileobj->fileSock, request, strlen(request));

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
		blockSend(session->fileobj->fileSock, request, strlen(request));

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

void Help() {
	printf("1.LOGIN\n");
	printf("2.LOGOUT\n");
	printf("3.REGISTER\n");
	printf("4.CHANGE PASSWORD\n");
	printf("5.STORE FILE\n");
	printf("6.RETRIEVE FILE\n");
	printf("7.MOVE FILE/FOLDER\n");
	printf("8.DELETE FILE\n");
	printf("9.MAKE DIRECTORY\n");
	printf("10.REMOVE DIRECTORY\n");
	printf("11.CHANGE WORKING DIRECTORY\n");
	printf("12.PRINT WORKING DIRECTORY\n");
	printf("13.LIST DIRECTORY\n");
	printf("14.HELP\n");
	printf("15.EXIT\n\n");
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
