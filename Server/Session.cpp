#include "Session.h"
#include <stdio.h>

void freeSession(LPSESSION session) {
	printf("Closing socket %d\n", session->cmdSock);
	if (closesocket(session->cmdSock) == SOCKET_ERROR) {
		printf("close comand socket failed with error %d\n", WSAGetLastError());
	}

	/*if (session->fileSock != 0) {
	printf("Closing socket %d\n", session->fileSock);
	if (closesocket(session->fileSock) == SOCKET_ERROR) {
	printf("close file socket failed with error %d\n", WSAGetLastError());
	}
	}*/

	GlobalFree(session);
}
