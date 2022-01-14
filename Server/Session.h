#pragma once
#include <WinSock2.h>
#include "EnvVar.h"

typedef struct FILEOBJ *LPFILEOBJ;

typedef struct SESSION {
	SOCKET cmdSock;
	char username[MAX_PATH];
	char workingDir[MAX_PATH];

	FILEOBJ *fileobj;

	void setUsername(const char *iUsername);
	void setWorkingDir(const char *iWorkingDir);
	void closeFile();
} SESSION, *LPSESSION;

_Ret_maybenull_ LPSESSION getSession();
void freeSession(_In_ LPSESSION);

