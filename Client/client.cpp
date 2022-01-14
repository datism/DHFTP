#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include "Envar.h"
#include "Session.h"
#include "Service.h"
#include "Io.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

void parseReply(char *reply, char *header, char *p1, char *p2);

int main(int argc, char* argv[]) {
	// Validate the parameters
	/*if (argc != 3)
	{
		printf("Usage: %s <ServerIpAddressss> <ServerPortNumber>\n", argv[0]);
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

	LpSession session;
	int bytes;
	bool cont;
	char *pos = NULL,
		*reply = NULL,
		buff[BUFFSIZE];

	session = getSession();
	session->sock = cmdSock;

	printf("1.LOGIN\n");
	printf("2.LOGOUT\n");
	printf("3.REGISTER\n");
	printf("4.STORE FILE\n");
	printf("5.RETRIEVE FILE\n");
	printf("6.RENAME FILE\n");
	printf("7.DELETE FILE\n");
	printf("8.MAKE DIR\n");
	printf("9.REMOVE DIR\n");
	printf("10.CHANGE WROKING DIR\n");
	printf("11.PRINT WORKING DIR\n");
	printf("12.LIST DIR\n\n");


	while (1) {
		chooseService(session, buff);
		blockSend(session->sock, buff);

		strcpy_s(buff, BUFFSIZE, "");
		
		do {
			bytes = blockRecv(session->sock, buff, BUFFSIZE);
			if (!bytes)
				break;

			buff[bytes] = 0;
			reply = buff;

			while ((pos = strstr(reply, ENDING_DELIMITER)) != NULL) {
				*pos = 0;
				cont = handleReply(session, reply);
				reply = pos + strlen(ENDING_DELIMITER);
			}

			strcpy_s(buff, BUFFSIZE, reply);
		} while (strlen(buff) != 0 || cont == TRUE);
	}

	FreeSession(session);
	WSACleanup();

	return 0;
}