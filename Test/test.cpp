
// TCPClient.cpp : Defines the entry point for the console application.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "process.h"
#include "Envar.h"
#include "IoObj.h"
#include "Session.h"
#include "FileObj.h"
#include "Io.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

HANDLE gCompletionPort;
sockaddr_in gCmdAddr;
sockaddr_in gFileAddr;

int serverPort;
unsigned __stdcall thread(void *param);
unsigned __stdcall thread1(void *param);
unsigned __stdcall thread2(void *param);
unsigned __stdcall completetionTheard(LPVOID completionPortID);
unsigned __stdcall fileTest(void *param);

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

template <typename... Args>
void initMessage(char *mess, const char *header, Args... paras) {
	char param[BUFFSIZE] = "";

	initParam(param, paras...);

	if (strlen(param) == 0)
		sprintf_s(mess, BUFFSIZE, "%s%s", header, ENDING_DELIMITER);
	else
		sprintf_s(mess, BUFFSIZE, "%s%s%s%s", header, HEADER_DELIMITER, param, ENDING_DELIMITER);
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

int main(int argc, char* argv[])
{
	//Step 1: Inittiate WinSock
	WSADATA wsaData;

	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	//Step 2: Construct socket	
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Specify server address

	//sockaddr_in serverAddr;
	//serverAddr.sin_family = AF_INET;
	//serverAddr.sin_port = htons(CMD_PORT);
	//serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	gCmdAddr.sin_family = AF_INET;
	gCmdAddr.sin_port = htons(CMD_PORT);
	gCmdAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	gFileAddr.sin_family = AF_INET;
	gFileAddr.sin_port = htons(FILE_PORT);
	gFileAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	////Step 4: Request to connect server
	//if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
	//	printf("Error! Cannot connect server. %d", WSAGetLastError());
	//	_getch();
	//	return 0;
	//}
	//printf("Connected server!\n");
	//int tv = 100; //Time-out interval: 100ms
	//setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));
	//Step 5: Communicate with server
	char sBuff[BUFFSIZE], rBuff[BUFFSIZE];
	int ret;

	////Sequence testing
	//strcpy(sBuff, "POST Hello\r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//if (ret < 0)
	//	printf("Sequence test fail!\n");
	//else {
	//	rBuff[ret] = 0;
	//	printf("Main: %s-->%s\n", sBuff, rBuff);
	//	if (strstr(rBuff, SUCCESS_POST) != 0)
	//		printf("Sequence test fail!\n");
	//}

	////Function testing
	//strcpy(sBuff, "USER ductq\r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//rBuff[ret] = 0;
	//printf("Main: %s-->%s\n\n", sBuff, rBuff);
	//if (strstr(rBuff, SUCCESS_LOGIN) != 0)
	//	printf("Login test fail!\n");

	//strcpy(sBuff, "USER admin\r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//rBuff[ret] = 0;
	//printf("Main: %s-->%s\n", sBuff, rBuff);
	//if (strstr(rBuff, SUCCESS_LOGIN) == 0)
	//	printf("Login test fail!\n");

	//strcpy(sBuff, "USER tungbt\r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//rBuff[ret] = 0;
	//printf("Main: %s-->%s\n", sBuff, rBuff);
	//if (strstr(rBuff, SUCCESS_LOGIN) != 0)
	//	printf("Login test fail!\n");

	//strcpy(sBuff, "POST Hello\r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//rBuff[ret] = 0;
	//printf("Main: %s-->%s\n", sBuff, rBuff);
	//if (strstr(rBuff, SUCCESS_POST) == 0)
	//	printf("Post message test fail!\n");

	//strcpy(sBuff, "QUIT \r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//rBuff[ret] = 0;
	//printf("Main: %s-->%s\n", sBuff, rBuff);
	//if (strstr(rBuff, SUCCESS_LOGOUT) == 0)
	//	printf("Logout test fail!\n");

	////Syntax testing
	//strcpy(sBuff, "USER \r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//if (ret < 0)
	//	printf("Syntax test fail!\n");
	//else {
	//	rBuff[ret] = 0;
	//	printf("Main: %s-->%s\n", sBuff, rBuff);
	//	if (strstr(rBuff, SUCCESS_LOGIN) != 0)
	//		printf("Login test fail!\n");
	//}

	//strcpy(sBuff, "foo\r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//if (ret < 0)
	//	printf("Syntax test fail!\n");
	//else {
	//	rBuff[ret] = 0;
	//	printf("Main: %s-->%s\n", sBuff, rBuff);
	//}

	//strcpy(sBuff, "USER tungbt\r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//rBuff[ret] = 0;
	//printf("Main: %s-->%s\n", sBuff, rBuff);

	////Stream testing
	//strcpy(sBuff, "USER admin\r\nPOST Hello world\r\nPOST Test stream\r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//if (ret < 0)
	//	printf("Stream test 1 fail!\n");
	//else {
	//	rBuff[ret] = 0;
	//	printf("Main: %s-->%s\n", sBuff, rBuff);
	//}

	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//if (ret < 0)
	//	printf("Stream test 1 fail!\n");
	//else {
	//	rBuff[ret] = 0;
	//	printf("Main: %s-->%s\n", sBuff, rBuff);
	//}

	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//if (ret < 0)
	//	printf("Stream test 1 fail!\n");
	//else {
	//	rBuff[ret] = 0;
	//	printf("Main: %s-->%s\n", sBuff, rBuff);
	//}

	//strcpy(sBuff, "POST I am tungbt");
	//ret = send(client, sBuff, strlen(sBuff), 0);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//if (ret > 0)
	//	printf("Stream test 2 fail!\n");

	//strcpy(sBuff, "\r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0);
	//ret = recv(client, rBuff, BUFF_SIZE, 0);
	//if (ret < 0)
	//	printf("Stream test 2 fail!\n");
	//else {
	//	rBuff[ret] = 0;
	//	printf("Main: %s-->%s\n", sBuff, rBuff);
	//}

	//_getch();

	////Concurency test 1
	//_beginthreadex(0, 0, thread1, 0, 0, 0); //start thread
	//_beginthreadex(0, 0, thread2, 0, 0, 0); //start thread
	//Sleep(2000);

	////Concurency test 2
	//int numConn = 0;
	//printf("Concurent connections: ");
	//scanf_s("%d", &numConn);
	//if (numConn > 0) {

	//	char buff[BUFFSIZE];
	//	int numSession = 0, numConnected = 0;
	//	SOCKET clients[MAX_CLIENT];
	//	for (int i = 0; i < numConn; i++) {
	//		clients[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//		if (connect(clients[i], (sockaddr *)&gCmdAddr, sizeof(gCmdAddr))) {
	//			printf("\nError: %d", WSAGetLastError());
	//			break;
	//		}
	//		numConnected++;
	//		int tv2 = 20;
	//		setsockopt(clients[i], SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv2), sizeof(int));

	//		strcpy_s(sBuff,BUFFSIZE, "USER admin\r\n\r\n");
	//		ret = send(clients[i], sBuff, strlen(sBuff), 0);
	//		if (ret < 0) {
	//			printf("send() fail %d.\n", WSAGetLastError());
	//			closesocket(clients[i]);
	//		}
	//		else {

	//			ret = recv(clients[i], rBuff, BUFFSIZE, 0);

	//			if (ret < 0) {
	//				printf("recv() fail %d.\n", WSAGetLastError());
	//			}
	//			else {
	//				rBuff[ret] = 0;
	//				printf("Concurent test: %s\n", rBuff);
	//				numSession++;
	//			}
	//		}
	//	}

	//	printf("\nNumber of success connection: %d", numConnected);
	//	printf("\nNumber of success session: %d\n", numSession);

	//	for (int i = 0; i < numConn; i++)
	//		closesocket(clients[i]);
	//}

	HANDLE theards[WSA_MAXIMUM_WAIT_EVENTS];

	////Concurency test 3
	//int numThread;
	//printf("Number of threads:");
	//scanf_s("%d", &numThread);
	//for (int i = 0; i < numThread; i++)
	//	theards[i] = (HANDLE) _beginthreadex(0, 0, thread, 0, 0, 0);

	//Concurency test 4
	int numThread;
	printf("Number of threads:");
	scanf_s("%d", &numThread);
	for (int i = 0; i < numThread; i++) {
		theards[i] = (HANDLE)_beginthreadex(0, 0, fileTest, (void *)i, 0, 0);
	}


	getchar();
	getchar();

	//Step 6: Close socket
	//closesocket(client);

	//Step 7: Terminate Winsock
	WSACleanup();

	return 0;
}

unsigned __stdcall thread(void *param)
{
	int ret = 0, numConn = 100;
	char buff[BUFFSIZE];
	int numSession = 0, numConnected = 0;
	SOCKET clients[MAX_CLIENT];

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(CMD_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	char sBuff[BUFFSIZE], rBuff[BUFFSIZE];
	for (int i = 0; i < numConn; i++) {
		clients[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (connect(clients[i], (sockaddr *)&serverAddr, sizeof(serverAddr))) {
			printf("\nError: %d", WSAGetLastError());
			break;
		}
		numConnected++;
		int tv2 = 20;
		setsockopt(clients[i], SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv2), sizeof(int));

		strcpy(sBuff, "USER admin\r\n\r\n");
		ret = send(clients[i], sBuff, strlen(sBuff), 0);
		if (ret < 0)
			printf("recv() fail %d.\n", WSAGetLastError());
		Sleep(100);
		ret = recv(clients[i], rBuff, BUFFSIZE, 0);

		if (ret < 0)
			printf("recv() fail %d\n", WSAGetLastError());
		else {
			rBuff[ret] = 0;
			printf("Concurent test: %s\n", rBuff);
			numSession++;
		}

	}

	for (int i = 0; i < numConn; i++) {
		int ok = 0;
		for (int k = 0; k < 5; k++) {
			strcpy(sBuff, "POST Hello. I am admin\r\n\r\n");
			ret = send(clients[i], sBuff, strlen(sBuff), 0);
			ret = recv(clients[i], rBuff, BUFFSIZE, 0);
			if (ret < 0)
				printf("recv() %d fail.\n %d", k, WSAGetLastError());
			else {
				ok++;
			}
		}
		if (ok < 5) printf("Concurency test 2 failed\n");
	}

	printf("\nNumber of success connection: %d", numConnected);
	printf("\nNumber of success session: %d\n", numSession);



	for (int i = 0; i < numConn; i++)
		closesocket(clients[i]);
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

		p2 = para[1].c_str();
		p3 = para[2].c_str();

		initMessage(request, CONNECT, p2);
		session->fileobj->size = _atoi64(p3);

		session->fileobj->fileSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		printf("Connecting to file port.....\n");
		if (connect(session->fileobj->fileSock, (sockaddr *)&gFileAddr, sizeof(gFileAddr))) {
			printf("connect() failed with error %d", WSAGetLastError());
			session->closeFile(TRUE);
			break;
		}
		blockSend(session->fileobj->fileSock, request);

		printf("Receiving file.....\n");
		if (recvFile(session->fileobj->fileSock, session->fileobj->file, session->fileobj->size))
			printf("Recieve file sucessful\n");
		session->closeFile(FALSE);
		break;
	case STORE_SUCCESS:
		if (para.size() != 2) {
			session->closeFile(FALSE);
			return;
		}

		p2 = para[1].c_str();
		initMessage(request, CONNECT, p2);

		session->fileobj->fileSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		printf("Connecting to file port.....\n");
		if (connect(session->fileobj->fileSock, (sockaddr *)&gFileAddr, sizeof(gFileAddr))) {
			printf("connect() failed with error %d", WSAGetLastError());
			session->closeFile(FALSE);
			break;
		}
		blockSend(session->fileobj->fileSock, request);

		printf("Sending file.....\n");
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

unsigned __stdcall fileTest(void *param) {
	int numcon = 10;
	HANDLE hfile;
	char *fileName = "testmid.rar";
	LARGE_INTEGER filesize;
	LpSession session;
	int bytes;
	char *pos = NULL;
	char sBuf[BUFFSIZE] = "";
	char rBuf[BUFFSIZE] = "";
	DWORD t = (int)param * 10;

	for (int i = 0; i < numcon; i++) {
		
		session = getSession();
		session->cmdSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (connect(session->cmdSock, (sockaddr *)&gCmdAddr, sizeof(gCmdAddr))) {
			printf("\nError: %d", WSAGetLastError());
			break;
		}

		hfile = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hfile == INVALID_HANDLE_VALUE) {
			printf("CreateFileA() failed with error %d\n", GetLastError());
			return -1;
		}

		if (!GetFileSizeEx(hfile, &filesize)) {
			printf("GetFileSizeEx() failed with error %d\n", GetLastError());
			return -1;
		}

		session->fileobj = GetFileObj(hfile, filesize.QuadPart, FILEOBJ::STOR);
		initMessage(sBuf, STORE, t + i, session->fileobj->size);
		blockSend(session->cmdSock, sBuf);

		bytes = blockRecv(session->cmdSock, rBuf, BUFFSIZE);
		if (!bytes)
			break;

		pos = strstr(rBuf, ENDING_DELIMITER);
		*pos = 0;
		handleReply(session, rBuf);

		strcpy_s(rBuf, BUFFSIZE, "");
	}
}

unsigned __stdcall completetionTheard(LPVOID completionPortID) {
	HANDLE completionPort = (HANDLE)completionPortID;
	DWORD transferredBytes;
	ULONG_PTR key = NULL;
	LPIO_OBJ ioobj = NULL;
	int rc;

	while (true) {
		rc = GetQueuedCompletionStatus(completionPort, &transferredBytes, (PULONG_PTR)&key, (LPOVERLAPPED *)&ioobj, INFINITE);
	}
}

unsigned __stdcall thread1(void *param)
{
	//Step 2: Construct socket	
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(CMD_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	//Step 4: Request to connect server
	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error! Cannot connect server. %d", WSAGetLastError());
		_getch();
		return 0;
	}
	printf("Connected server!\n");

	//Step 5: Communicate with server
	char sBuff[BUFFSIZE], rBuff[BUFFSIZE];
	int ret;
	strcpy(sBuff, "USER tungbt\r\n");
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	rBuff[ret] = 0;
	printf("Thread 1:  %s-->%s\n", sBuff, rBuff);

	for (int i = 0; i < 5; i++) {
		Sleep(10);
		strcpy(sBuff, "POST Hello. I am tungbt\r\n");
		ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
		ret = recv(client, rBuff, BUFFSIZE, 0);
		rBuff[ret] = 0;
		printf("Thread 1:  %s-->%s\n", sBuff, rBuff);
	}

	strcpy(sBuff, "QUIT \r\n");
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	rBuff[ret] = 0;
	printf("Thread 1:  %s-->%s\n", sBuff, rBuff);

	strcpy(sBuff, "USER test\r\n");
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	rBuff[ret] = 0;
	printf("Thread 1:  %s-->%s\n", sBuff, rBuff);

	for (int i = 0; i < 5; i++) {
		Sleep(10);
		strcpy(sBuff, "POST Hello. I am test\r\n");
		ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
		ret = recv(client, rBuff, BUFFSIZE, 0);
		rBuff[ret] = 0;
		printf("Thread 1:  %s-->%s\n", sBuff, rBuff);
	}

	//Step 6: Close socket
	closesocket(client);
	printf("Thread 1 end.\n");
}

unsigned __stdcall thread2(void *param)
{
	Sleep(10);
	//Step 2: Construct socket	
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(CMD_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	//Step 4: Request to connect server
	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error! Cannot connect server. %d", WSAGetLastError());
		_getch();
		return 0;
	}
	printf("Connected server!\n");

	//Step 5: Communicate with server
	char sBuff[BUFFSIZE], rBuff[BUFFSIZE];
	int ret;
	strcpy(sBuff, "USER admin\r\n");
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	rBuff[ret] = 0;
	printf("Thread 2:  %s-->%s\n", sBuff, rBuff);
	for (int i = 0; i < 10; i++) {
		Sleep(10);
		strcpy(sBuff, "POST Hello. I am admin\r\n");
		ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
		ret = recv(client, rBuff, BUFFSIZE, 0);
		rBuff[ret] = 0;
		printf("Thread 2:  %s-->%s\n", sBuff, rBuff);
	}

	strcpy(sBuff, "QUIT \r\n");
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	rBuff[ret] = 0;
	printf("Thread 1:  %s-->%s\n", sBuff, rBuff);

	strcpy(sBuff, "USER ductq\r\n");
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	rBuff[ret] = 0;
	printf("Thread 2:  %s-->%s\n", sBuff, rBuff);

	//Step 6: Close socket
	closesocket(client);
	printf("Thread 2 end.\n");
}
