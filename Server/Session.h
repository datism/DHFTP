#pragma once
#include <WinSock2.h>
#include <list>
#include "FileObj.h"
#include "EnvVar.h"
#include "IoObj.h"

typedef struct SESSION {
	//cmd socket
	SOCKET cmdSock;

	//username and root folder
	char username[MAX_PATH];
	//woring directory
	char workingDir[MAX_PATH];

	CRITICAL_SECTION cs;
	//file obj use for transmitting file
	LPFILEOBJ fileobj;

	//for control maximum oustanding operation
	volatile LONG outstandingSend;

	//for shutdown session
	volatile LONG oustandingOp;
	volatile LONG bclosing;

	//for control order of operation
	std::list<LPIO_OBJ> *pending;

	void setUsername(const char *iUsername);
	void setWorkingDir(const char *iWorkingDir);
	void EnListPendingOperation(_In_ LPIO_OBJ ioObj);

	//close file and close file connection
	void closeFile(BOOL deletefile);
} SESSION, *LPSESSION;

/**
 * @brief Get the session obj
 * 
 * @return new session obj or NULL if out of memory
 */
_Ret_maybenull_ LPSESSION getSession();

/**
 * @brief free session obj
 * 
 * @param LPSESSION session
 */
void freeSession(_In_ LPSESSION session);

