#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include "Envar.h"
#include "Session.h"
#include "Service.h"
#include "Io.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
 
sockaddr_in gCmdAddr;
sockaddr_in gFileAddr;

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
	char *serverIp = SERVER_ADDR;


	gCmdAddr.sin_family = AF_INET;
	int cmdPort = CMD_PORT;
	gCmdAddr.sin_port = htons(cmdPort);
	inet_pton(AF_INET, serverIp, &gCmdAddr.sin_addr);

	gFileAddr.sin_family = AF_INET;
	int filePort = FILE_PORT;
	gFileAddr.sin_port = htons(filePort);
	inet_pton(AF_INET, serverIp, &gFileAddr.sin_addr);

	SOCKET cmdSock;
	cmdSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect(cmdSock, (sockaddr *)&gCmdAddr, sizeof(gCmdAddr))) {
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
	session->cmdSock = cmdSock;

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
		blockSend(session->cmdSock, buff);

		strcpy_s(buff, BUFFSIZE, "");
		
		do {
			bytes = blockRecv(session->cmdSock, buff, BUFFSIZE);
			if (!bytes)
				break;

			buff[bytes] = 0;
			reply = buff;

			while ((pos = strstr(reply, ENDING_DELIMITER)) != NULL) {
				*pos = 0;
				handleReply(session, reply);
				reply = pos + strlen(ENDING_DELIMITER);
			}

			strcpy_s(buff, BUFFSIZE, reply);
		} while (strlen(buff) != 0);
	}

	FreeSession(session);
	WSACleanup();

	return 0;
}