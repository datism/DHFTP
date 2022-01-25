#pragma once
#include <WinSock2.h>

typedef struct Session{
	SOCKET cmdSock;
	SOCKET fileSock;

	HANDLE hfile;
	LONG64 fileSize;

	void closeFile();
} Session, *LpSession;

LpSession getSession();
void FreeSession(LpSession session);
