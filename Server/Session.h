#pragma once
#include <WinSock2.h>

typedef struct FILEOBJ *LPFILEOBJ;

typedef struct SESSION {
	SOCKET cmdSock;
	char *username;

	FILEOBJ *fileobj;

	void closeFile();
} SESSION, *LPSESSION;

_Ret_maybenull_ LPSESSION getSession();
void freeSession(_In_ LPSESSION);

