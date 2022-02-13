#pragma once
#include <WinSock2.h>
#include <mswsock.h>

typedef struct {
	//listen socket
	SOCKET sock;

	//count depend on filelisten or cmdlisten
	LONG count;
	//accept event
	WSAEVENT acceptEvent;

	// Pointers to Microsoft specific extensions.
	LPFN_ACCEPTEX lpfnAcceptEx;
} LISTEN_OBJ, *LPLISTEN_OBJ;

_Ret_maybenull_ LPLISTEN_OBJ getListenObj(_In_ WORD port);
void FreeListenObj(_In_ LPLISTEN_OBJ listenobj);