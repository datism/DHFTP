#pragma once

#include <WinSock2.h>

typedef struct {
	SOCKET cmdSock;
	SOCKET fileSock;
	char *username;
} SESSION, *LPSESSION;

void freeSession(_In_ LPSESSION);