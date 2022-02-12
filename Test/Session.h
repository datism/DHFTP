#pragma once
#include <WinSock2.h>
#include "FileObj.h"

typedef struct Session {
	SOCKET cmdSock;
	LPFILEOBJ fileobj;

	void closeFile(BOOL deleteFile);
} Session, *LpSession;

LpSession getSession();
void FreeSession(LpSession session);
#pragma once
