#include "Session.h"
#include <stdio.h>

void Session::closeFile() {
	if (hfile != INVALID_HANDLE_VALUE)
		CloseHandle(hfile);
	hfile = INVALID_HANDLE_VALUE;
}

LpSession getSession() {
	LpSession newobj = NULL;

	if ((newobj = (LpSession)GlobalAlloc(GPTR, sizeof(Session))) == NULL)
		printf("GlobalAlloc() failed with error %d\n", GetLastError());

	if (newobj)
		newobj->hfile = INVALID_HANDLE_VALUE;

	return newobj;
}

void FreeSession(LpSession session) {
	session->closeFile();
	closesocket(session->cmdSock);
	closesocket(session->fileSock);
	GlobalFree(session);
}
