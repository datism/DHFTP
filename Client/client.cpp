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

int main(int argc, char* argv[]) {
	// Validate the parameters
	if (argc != 4) {
		printf("Usage: %s <ServerIpAddressss> <ServerCmdPort> <ServerFilePort>\n", argv[0]);
		return 1;
	}

	//Inittiate Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported \n");

	//Specify server address
	char *serverIp = argv[1];

	//cmd addr
	gCmdAddr.sin_family = AF_INET;
	int cmdPort = atoi(argv[2]);
	gCmdAddr.sin_port = htons(cmdPort);
	inet_pton(AF_INET, serverIp, &gCmdAddr.sin_addr);

	//file addr
	gFileAddr.sin_family = AF_INET;
	int filePort = atoi(argv[3]);
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
	char *pos = NULL,
		*reply = NULL,
		buff[BUFFSIZE] = "";

	session = getSession();
	session->cmdSock = cmdSock;

	Help();

	while (1) {
		ChooseService(session, buff);
		blockSend(session->cmdSock, buff, strlen(buff));

		strcpy_s(buff, BUFFSIZE, "");
		
		//continue receive until buffer end with ENDING_DELIMITER
		do {
			bytes = blockRecv(session->cmdSock, buff + strlen(buff), BUFFSIZE);
			if (!bytes)
				break;

			buff[bytes] = 0;
			reply = buff;

			while ((pos = strstr(reply, ENDING_DELIMITER)) != NULL) {
				*pos = 0;
				HandleReply(session, reply);
				reply = pos + strlen(ENDING_DELIMITER);
			}

			strcpy_s(buff, BUFFSIZE, reply);

		} while (strlen(buff) != 0);
	}

	return 0;
}
