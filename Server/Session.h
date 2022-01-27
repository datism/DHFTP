#pragma once
#include <WinSock2.h>
#include <list>
#include <set>
#include "FileObj.h"
#include "EnvVar.h"
#include "IoObj.h"

typedef struct SESSION {
	SOCKET cmdSock;
	SOCKET fileSock;

	char username[MAX_PATH];
	char workingDir[MAX_PATH];

	CRITICAL_SECTION cs;
	FILEOBJ *fileobj;

	volatile LONG outstandingSend;
	volatile LONG oustandingOp;
	std::list<LPIO_OBJ> *pending;

	volatile LONG bclosing;

	void setUsername(const char *iUsername);
	void setWorkingDir(const char *iWorkingDir);
	void EnListPendingOperation(_In_ LPIO_OBJ ioObj);
	//void ProcessPendingOperations();
	void closeFile(BOOL deletefile);
} SESSION, *LPSESSION;

_Ret_maybenull_ LPSESSION getSession();
void freeSession(_In_ LPSESSION);

