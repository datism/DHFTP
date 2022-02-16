
// TCPClient.cpp : Defines the entry point for the console application.
//
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
	inet_pton(AF_INET, SERVER_ADDR, &gCmdAddr.sin_addr);

	gFileAddr.sin_family = AF_INET;
	gFileAddr.sin_port = htons(FILE_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &gFileAddr.sin_addr);

	//Step 4: Request to connect server
	if (connect(client, (sockaddr *)&gCmdAddr, sizeof(gCmdAddr))) {
		printf("Error! Cannot connect server. %d", WSAGetLastError());
		_getch();
		return 0;
	}
	printf("Connected server!\n");
	int tv = 100; //Time-out interval: 100ms
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));
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
	//strcpy_s(sBuff, BUFFSIZE, "USER admin\r\n\r\nPOST Hello world\r\n\r\nPOST Test stream\r\n\r\n");
	//ret = send(client, sBuff, strlen(sBuff), 0);
	//ret = recv(client, rBuff, BUFFSIZE, 0);
	//if (ret < 0)
	//	printf("Stream test 1 fail!\n");
	//else {
	//	rBuff[ret] = 0;
	//	printf("Main: %s-->%s\n", sBuff, rBuff);
	//}

	//ret = recv(client, rBuff, BUFFSIZE, 0);
	//if (ret < 0)
	//	printf("Stream test 1 fail!\n");
	//else {
	//	rBuff[ret] = 0;
	//	printf("Main: %s-->%s\n", sBuff, rBuff);
	//}

	//ret = recv(client, rBuff, BUFFSIZE, 0);
	//if (ret < 0)
	//	printf("Stream test 1 fail!\n");
	//else {
	//	rBuff[ret] = 0;
	//	printf("Main: %s-->%s\n", sBuff, rBuff);
	//}

	strcpy_s(sBuff, BUFFSIZE, "POST I am tungbt");
	ret = send(client, sBuff, strlen(sBuff), 0);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	if (ret > 0)
		printf("Stream test 2 fail!\n");

	strcpy_s(sBuff, BUFFSIZE, "\r\n\r\n");
	ret = send(client, sBuff, strlen(sBuff), 0);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	if (ret < 0)
		printf("Stream test 2 fail!\n");
	else {
		rBuff[ret] = 0;
		printf("Main: %s-->%s\n", sBuff, rBuff);
	}

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

	//HANDLE theards[WSA_MAXIMUM_WAIT_EVENTS];

	////Concurency test 3
	//int numThread;
	//printf("Number of threads:");
	//scanf_s("%d", &numThread);
	//for (int i = 0; i < numThread; i++)
	//	theards[i] = (HANDLE) _beginthreadex(0, 0, thread, 0, 0, 0);

	getchar();
	getchar();

	if ((gCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return 1;
	}

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	for (int waitCount = 0; waitCount < (int)systemInfo.dwNumberOfProcessors * 2; waitCount++) {
		// Create a server worker thread and pass the completion port to the thread
		_beginthreadex(0, 0, completetionTheard, (void*)gCompletionPort, 0, 0);
	}

	//Concurency test 4
	int numThread1;
	printf("Number of threads:");
	scanf_s("%d", &numThread1);
	for (int i = 0; i < numThread1; i++) {
		_beginthreadex(0, 0, fileTest, (void *)i, 0, 0);
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

	char sBuff[BUFFSIZE], rBuff[BUFFSIZE];
	for (int i = 0; i < numConn; i++) {
		clients[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (connect(clients[i], (sockaddr *)&gCmdAddr, sizeof(gCmdAddr))) {
			printf("\nError: %d", WSAGetLastError());
			break;
		}
		numConnected++;
		int tv2 = 20;
		setsockopt(clients[i], SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv2), sizeof(int));

		strcpy_s(sBuff,BUFFSIZE, "USER admin\r\n\r\n");
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
			strcpy_s(sBuff, BUFFSIZE, "POST Hello. I am admin\r\n\r\n");
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
	case STORE_SUCCESS: {
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

		if (CreateIoCompletionPort((HANDLE)session->fileobj->fileSock, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
			printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		}

		LPIO_OBJ sendobj = getIoObject(IO_OBJ::ACPT_F, request, strlen(request));
		PostSend(session->fileobj->fileSock, sendobj);
		break;
	}
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
	char *fileName = "Ignition.pdf";
	LARGE_INTEGER filesize;
	LpSession session;
	int bytes;
	char sBuf[BUFFSIZE] = "";
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


		if (CreateIoCompletionPort((HANDLE)session->cmdSock, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
			printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		}

		session->fileobj = GetFileObj(hfile, filesize.QuadPart, FILEOBJ::STOR);
		initMessage(sBuf, STORE, t + i, session->fileobj->size);

		printf("FILE: %d\n", t + i);
		LPIO_OBJ sendobj = getIoObject(IO_OBJ::SEND_C, sBuf, strlen(sBuf));
		PostSend(session->cmdSock, sendobj);
	}
}

unsigned __stdcall completetionTheard(LPVOID completionPortID) {
	HANDLE completionPort = (HANDLE)completionPortID;

	ULONG_PTR key = NULL;
	LPIO_OBJ ioobj = NULL;
	LpSession session = NULL;
	DWORD transferredBytes;
	int rc;

	while (true) {
		rc = GetQueuedCompletionStatus(completionPort, &transferredBytes, (PULONG_PTR)&key, (LPOVERLAPPED *)&ioobj, INFINITE);
		if (rc == FALSE) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
			freeIoObject(ioobj);
			FreeSession((LpSession) key);
			continue;
		}

		session = (LpSession)key;

		switch (ioobj->operation) {
		case IO_OBJ::SEND_C: {
			LPIO_OBJ recvobj = getIoObject(IO_OBJ::RECV_C, NULL, BUFFSIZE);
			PostRecv(session->cmdSock, recvobj);
			freeIoObject(ioobj);
			break;
		}
		case IO_OBJ::RECV_C: {
			ioobj->buffer[transferredBytes] = 0;
			char *pos = strstr(ioobj->buffer, ENDING_DELIMITER);
			*pos = 0;
			handleReply(session, ioobj->buffer);
			freeIoObject(ioobj);
			break;
		}
		case IO_OBJ::ACPT_F: {
			printf("Sending file.....\n");
			LPIO_OBJ sendfobj = getIoObject(IO_OBJ::SEND_F, NULL, 0);
			sendfobj->dataBuff.len = min(session->fileobj->size, TRANSMITFILE_MAX);
			PostSendFile(session->fileobj->fileSock, session->fileobj->file, sendfobj);
			freeIoObject(ioobj);
			break;
		}
		case IO_OBJ::SEND_F: {
			session->fileobj->bytesSended += transferredBytes;
			LONG64 remain = session->fileobj->size - session->fileobj->bytesSended;
			if (remain <= 0) {
				printf("Finish sending file.\n");
				freeIoObject(ioobj);
				session->closeFile(FALSE);
				break;
			}
			else {
				ioobj->setFileOffset(session->fileobj->bytesSended);
				ioobj->dataBuff.len = min(remain, TRANSMITFILE_MAX);
				PostSendFile(session->fileobj->fileSock, session->fileobj->file, ioobj);
				break;
			}
		}
		default:
			break;
		}
	}
}