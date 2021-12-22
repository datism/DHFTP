#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <conio.h>
#include "EnvVar.h"
#include "IoObj.h"
#include "Service.h"

#pragma comment(lib, "Ws2_32.lib")

void CALLBACK workerRoutine(DWORD error, DWORD transferredBytes, LPWSAOVERLAPPED lpOverlapped, DWORD inFlags);
void chooseService(LPIO_OBJ sendObj);
void blockSend(SOCKET sock, LPIO_OBJ sendObj);
void blockReceive(SOCKET sock, LPIO_OBJ receiveObj);

int main(int argc, char* argv[]) {
	// Validate the parameters
	/*if (argc != 3)
	{
		printf("Usage: %s <ServerIpAddress> <ServerPortNumber>\n", argv[0]);
		return 1;
	}
*/
	//Inittiate Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported \n");

	//Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	char *serverIp = SERVER_ADDR;
	int serverPort = CMD_PORT;
	serverAddr.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

	SOCKET cmdSock;
	cmdSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect(cmdSock, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("\nError: %d", WSAGetLastError());
		closesocket(cmdSock);
		return 0;
	}
	else printf("Connected\n");

	LPIO_OBJ sendObj = getIoObject(IO_OBJ::SEND_C);
	LPIO_OBJ recvObj = getIoObject(IO_OBJ::RECV_C);

	while (1) {
		chooseService(sendObj);
		blockSend(cmdSock, sendObj);
		blockReceive(cmdSock, recvObj);
	}

	closesocket(cmdSock);
	WSACleanup();

	return 0;
}

void chooseService(LPIO_OBJ sendObj) {
	printf("Choose service\n");
	_getch();
	sendObj->setBufferSend("LOGIN");
}

void blockSend(SOCKET sock, LPIO_OBJ sendObj) {
	DWORD sentBytes = 0,
		bufferLen = strlen(sendObj->buffer);
	//Send all buffer
	while (sendObj->dataBuff.len > 0) {
		if (WSASend(sock, &sendObj->dataBuff, 1, &sentBytes, 0, NULL, NULL) == SOCKET_ERROR) {
			printf("WSASend() failed with error %d\n", WSAGetLastError());
			return;
		}

		sendObj->dataBuff.len -= sentBytes;
		sendObj->dataBuff.buf = sendObj->buffer + bufferLen - sendObj->dataBuff.len;
	}
}

void blockReceive(SOCKET sock, LPIO_OBJ receiveObj) {
	DWORD receivedBytes = 0;
	char *reply = NULL,
		*pos = NULL;

	do {
		//Blocking receive
		if (WSARecv(sock, &receiveObj->dataBuff, 1, &receivedBytes, 0, NULL, NULL) == SOCKET_ERROR) {
			printf("WSARecv() failed with error %d\n", WSAGetLastError());
			return;
		}

		receiveObj->buffer[receivedBytes] = 0;
		reply = receiveObj->buffer;

		//Split string by ending delimiter
		while ((pos = strstr(reply, ENDING_DELIMITER)) != NULL) {
			*pos = 0;
			handleReply(reply);
			reply = pos + strlen(ENDING_DELIMITER);
		}

		//Set the remaining buffer to receive
		receiveObj->setBufferRecv(reply);

	} while (strlen(reply) != 0); //Until buffer end with ending delimiter
}

void CALLBACK workerRoutine(DWORD error, DWORD transferredBytes, LPWSAOVERLAPPED lpOverlapped, DWORD inFlags) {

}