
// TCPClient.cpp : Defines the entry point for the console application.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "process.h"

#define MAX_CLIENT 4096
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 5500
#define BUFFSIZE 4096
#define TRANSMITFILE_MAX ((2<<30) - 1)

#define ENDING_DELIMITER "\r\n"
#define HEADER_DELIMITER "\r"
#define PARA_DELIMITER " "

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

#define LOGIN_SUCCESS "110"
#define LOGOUT_SUCCESS "111"
#define REGISTER_SUCCESS "112"

#define RETRIEVE_SUCCESS "220"
#define STORE_SUCCESS "221"
#define RENAME_SUCCESS "121"
#define DELETE_SUCCESS "122"
#define MAKEDIR_SUCCESS "123"
#define REMOVEDIR_SUCCESS "124"
#define CHANGEWDIR_SUCCESS "125"
#define PRINTWDIR_SUCCESS "126"
#define LIST_SUCCESS "127"

#pragma comment (lib, "Ws2_32.lib")

void initParam(char *param);

template <typename P>
void initParam(char *param, P p);

template <typename P, typename... Args>
void initParam(char *param, P p, Args... paras);

template <typename... Args>
void initMessage(char *mess, const char *header, Args... paras);

int serverPort;
unsigned __stdcall thread(void *param);
unsigned __stdcall thread1(void *param);
unsigned __stdcall thread2(void *param);

const char *bigTest = "testbig.rar";
const char *midTest = "testmid.rar";
const char *smallTest = "testsmall.rar";
HANDLE bigTestH;
HANDLE midTestH;
HANDLE smallTestH;
LARGE_INTEGER bigTestS;
LARGE_INTEGER midTestS;
LARGE_INTEGER smallTestS;



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
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	bigTestH = CreateFile(bigTest, GENERIC_READ, MAXDWORD, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	midTestH = CreateFile(midTest, GENERIC_READ, MAXDWORD, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	smallTestH = CreateFile(smallTest, GENERIC_READ, MAXDWORD, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (bigTestH == INVALID_HANDLE_VALUE || midTestH == INVALID_HANDLE_VALUE || smallTestH == INVALID_HANDLE_VALUE) {
		printf("CreateFileA() failed with error %d\n", GetLastError());
		return 1;
	}

	GetFileSizeEx(bigTestH, &bigTestS);
	GetFileSizeEx(midTestH, &midTestS);
	GetFileSizeEx(smallTestH, &smallTestS);

	//Step 4: Request to connect server
	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
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

	//Sequence testing
	initMessage(sBuff, LOGIN, "test", "1");
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	if (ret < 0)
		printf("Sequence test fail!\n");
	else {
		rBuff[ret] = 0;
		printf("Main: %s-->%s\n", sBuff, rBuff);
		if (strstr(rBuff, RESPONE) != 0)
			printf("Sequence test fail!\n");
	}

	//Function testing
	initMessage(sBuff, REGISTER, "test", "1");
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	rBuff[ret] = 0;
	printf("Main: %s-->%s\n\n", sBuff, rBuff);
	if (strstr(rBuff, REGISTER_SUCCESS) != 0)
		printf("REGISTER test fail!\n");

	initMessage(sBuff, LOGIN, "test", 1);
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	rBuff[ret] = 0;
	printf("Main: %s-->%s\n", sBuff, rBuff);
	if (strstr(rBuff, LOGIN_SUCCESS) == 0)
		printf("LOGIN test fail!\n");

	initMessage(sBuff, STORE, smallTest, smallTestS.QuadPart);
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	rBuff[ret] = 0;
	printf("Main: %s-->%s\n", sBuff, rBuff);
	if (strstr(rBuff, STORE_SUCCESS) != 0)
		printf("STORE test fail!\n");

	HANDLE smallRetrieve = CreateFile("smallRetrieve", GENERIC_WRITE | DELETE, MAXDWORD, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	initMessage(sBuff, RETRIEVE, smallTest);
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);




	//Syntax testing
	strcpy(sBuff, "USER \r\n");
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	if (ret < 0)
		printf("Syntax test fail!\n");
	else {
		rBuff[ret] = 0;
		printf("Main: %s-->%s\n", sBuff, rBuff);
		if (strstr(rBuff, SUCCESS_LOGIN) != 0)
			printf("Login test fail!\n");
	}

	strcpy(sBuff, "foo\r\n");
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	if (ret < 0)
		printf("Syntax test fail!\n");
	else {
		rBuff[ret] = 0;
		printf("Main: %s-->%s\n", sBuff, rBuff);
	}

	strcpy(sBuff, "USER tungbt\r\n");
	ret = send(client, sBuff, strlen(sBuff), 0); Sleep(100);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	rBuff[ret] = 0;
	printf("Main: %s-->%s\n", sBuff, rBuff);

	//Stream testing
	strcpy(sBuff, "USER admin\r\nPOST Hello world\r\nPOST Test stream\r\n");
	ret = send(client, sBuff, strlen(sBuff), 0);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	if (ret < 0)
		printf("Stream test 1 fail!\n");
	else {
		rBuff[ret] = 0;
		printf("Main: %s-->%s\n", sBuff, rBuff);
	}

	ret = recv(client, rBuff, BUFFSIZE, 0);
	if (ret < 0)
		printf("Stream test 1 fail!\n");
	else {
		rBuff[ret] = 0;
		printf("Main: %s-->%s\n", sBuff, rBuff);
	}

	ret = recv(client, rBuff, BUFFSIZE, 0);
	if (ret < 0)
		printf("Stream test 1 fail!\n");
	else {
		rBuff[ret] = 0;
		printf("Main: %s-->%s\n", sBuff, rBuff);
	}

	strcpy(sBuff, "POST I am tungbt");
	ret = send(client, sBuff, strlen(sBuff), 0);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	if (ret > 0)
		printf("Stream test 2 fail!\n");

	strcpy(sBuff, "\r\n");
	ret = send(client, sBuff, strlen(sBuff), 0);
	ret = recv(client, rBuff, BUFFSIZE, 0);
	if (ret < 0)
		printf("Stream test 2 fail!\n");
	else {
		rBuff[ret] = 0;
		printf("Main: %s-->%s\n", sBuff, rBuff);
	}

	_getch();

	//Concurency test 1
	_beginthreadex(0, 0, thread1, 0, 0, 0); //start thread
	_beginthreadex(0, 0, thread2, 0, 0, 0); //start thread
	Sleep(2000);

	//Concurency test 2
	int numConn = 0;
	printf("Concurent connections: ");
	scanf_s("%d", &numConn);
	if (numConn > 0) {

		char buff[BUFFSIZE];
		int numSession = 0, numConnected = 0;
		SOCKET clients[MAX_CLIENT];
		for (int i = 0; i < numConn; i++) {
			clients[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (connect(clients[i], (sockaddr *)&serverAddr, sizeof(serverAddr))) {
				printf("\nError: %d", WSAGetLastError());
				break;
			}
			numConnected++;
			int tv2 = 20;
			setsockopt(clients[i], SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv2), sizeof(int));

			strcpy(sBuff, "USER admin\r\n");
			ret = send(clients[i], sBuff, strlen(sBuff), 0);
			ret = recv(clients[i], rBuff, BUFFSIZE, 0);

			if (ret < 0)
				printf("recv() fail.\n");
			else {
				rBuff[ret] = 0;
				printf("Concurent test: %s\n", rBuff);
				numSession++;
			}
		}

		printf("\nNumber of success connection: %d", numConnected);
		printf("\nNumber of success session: %d\n", numSession);

		for (int i = 0; i < numConn; i++)
			closesocket(clients[i]);
	}

	//Concurency test 3
	int numThread;
	printf("Number of threads:");
	scanf_s("%d", &numThread);
	for (int i = 0; i < numThread; i++)
		_beginthreadex(0, 0, thread, 0, 0, 0); //start thread
											   //Step 6: Close socket
	closesocket(client);

	Sleep(5000);

	//Step 7: Terminate Winsock
	WSACleanup();

	return 0;
}

unsigned __stdcall thread(void *param)
{
	int ret = 0, numConn = 10;
	char buff[BUFFSIZE];
	int numSession = 0, numConnected = 0;
	SOCKET clients[MAX_CLIENT];

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	char sBuff[BUFFSIZE], rBuff[BUFFSIZE];
	for (int i = 0; i < numConn; i++) {
		clients[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (connect(clients[i], (sockaddr *)&serverAddr, sizeof(serverAddr))) {
			printf("\nError: %d", WSAGetLastError());
			break;
		}
		numConnected++;
		int tv2 = 100;
		setsockopt(clients[i], SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv2), sizeof(int));

		strcpy(sBuff, "USER admin\r\n");
		ret = send(clients[i], sBuff, strlen(sBuff), 0); Sleep(100);
		ret = recv(clients[i], rBuff, BUFFSIZE, 0);

		if (ret < 0)
			printf("recv() fail.\n");
		else {
			rBuff[ret] = 0;
			printf("Concurent test: %s\n", rBuff);
			numSession++;
		}

	}

	for (int i = 0; i < numConn; i++) {
		int ok = 0;
		for (int k = 0; k < 5; k++) {
			strcpy(sBuff, "POST Hello. I am admin\r\n");
			ret = send(clients[i], sBuff, strlen(sBuff), 0);
			ret = recv(clients[i], rBuff, BUFFSIZE, 0);
			if (ret < 0)
				printf("recv() %d fail.\n", k);
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
unsigned __stdcall thread1(void *param)
{
	//Step 2: Construct socket	
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
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
	serverAddr.sin_port = htons(SERVER_PORT);
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