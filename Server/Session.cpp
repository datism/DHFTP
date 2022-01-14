#include "Session.h"
#include "FileObj.h"
#include <stdio.h>

_Ret_maybenull_ LPSESSION getSession() {
	LPSESSION session;

	if ((session = (LPSESSION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SESSION))) == NULL)
		printf("HeapAlloc() failed with error %d\n", GetLastError());

	return session;
}

void freeSession(_In_ LPSESSION session) {
	printf("Closing socket %d\n", session->cmdSock);
	if (closesocket(session->cmdSock) == SOCKET_ERROR) {
		printf("close comand socket failed with error %d\n", WSAGetLastError());
	}
	
	session->closeFile();

	HeapFree(GetProcessHeap(), NULL, session);
}

void SESSION::setUsername(const char * iUsername) {
	strcpy_s(this->username, MAX_PATH, iUsername);
}

void SESSION::setWorkingDir(const char * iWorkingDir) {
	strcpy_s(this->workingDir, MAX_PATH, iWorkingDir);
}

void SESSION::closeFile() {
	if (this->fileobj != NULL)
		FreeFileObj(this->fileobj);
	fileobj = NULL;
}


