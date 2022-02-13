#pragma once
#include <WinSock2.h>
#include "FileObj.h"

typedef struct Session{
	//cmd socket
	SOCKET cmdSock;
	//file obj use for transmitting file
	LPFILEOBJ fileobj;
	
	void closeFile(BOOL deleteFile);
} Session, *LpSession;

/**
 * @brief Get the session obj
 * 
 * @return new session obj or NULL if out of memory
 */
LpSession getSession();

/**
 * @brief free session obj
 * 
 * @param LPSESSION session
 */
void FreeSession(LpSession session);
