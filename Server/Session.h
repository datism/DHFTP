#pragma once

#include <WinSock2.h>

typedef struct {
	SOCKET cmdSock;
	char *username;
} SESSION, *LPSESSION;