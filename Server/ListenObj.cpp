#include "ListenObj.h"
#include <WS2tcpip.h>
#include <stdio.h>
#include "EnvVar.h"

_Ret_maybenull_ LPLISTEN_OBJ GetListenObj(_In_ char *ipAddr, _In_ WORD port) {
	LPLISTEN_OBJ newObj = NULL;
	GUID guidAcceptEx = WSAID_ACCEPTEX, 
		guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	SOCKADDR_IN addr;
	int rc;
	DWORD bytes, dontlinger;

	if ((newObj = (LPLISTEN_OBJ)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LISTEN_OBJ))) == NULL) {
		printf("HeapAlloc() failed with error %d", GetLastError());
		return NULL;
	}

	//creat accept event
	newObj->acceptEvent = WSACreateEvent();

	//creat new socket
	if ((newObj->sock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		printf("WSASocket() failed with error %d\n", WSAGetLastError());
		return NULL;
	}

	//bind
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ipAddr, &addr.sin_addr);
	if (bind(newObj->sock, (PSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR) {
		printf("bind() failed with error %d\n", WSAGetLastError());
		return NULL;
	}

	//Graceful close
	dontlinger = 0;
	if (setsockopt(newObj->sock, SOL_SOCKET, SO_DONTLINGER,(const char *)&dontlinger, sizeof(DWORD)) == SOCKET_ERROR) {
		printf("setsockopt failed with error %d\n", WSAGetLastError());
		return NULL;
	}

	//Load Acceptex
	rc = WSAIoctl(
		newObj->sock,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx,
		sizeof(guidAcceptEx),
		&newObj->lpfnAcceptEx,
		sizeof(newObj->lpfnAcceptEx),
		&bytes,
		NULL,
		NULL
	);
	if (rc == SOCKET_ERROR)
	{
		printf("WSAIoctl: SIO_GET_EXTENSION_FUNCTION_POINTER failed with error %d\n", WSAGetLastError());
		return NULL;
	}

	//Listen
	rc = listen(newObj->sock, 200);
	if (rc == SOCKET_ERROR)
	{
		printf("listen failed with error %d\n", WSAGetLastError());
		return NULL;
	}

	return newObj;
}

void FreeListenObj(_In_ LPLISTEN_OBJ listenobj) {
	printf("Close listen socket %d\n", listenobj->sock);
	if (closesocket(listenobj->sock) == SOCKET_ERROR) {
		printf("closesocket failed with error %d\n", WSAGetLastError());
	}
	HeapFree(GetProcessHeap(), NULL, listenobj);
}
