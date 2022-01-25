#include "ListenObj.h"
#include <WS2tcpip.h>
#include <stdio.h>
#include "EnvVar.h"

LPLISTEN_OBJ getListenObj(WORD port) {
	LPLISTEN_OBJ newObj = NULL;
	GUID guidAcceptEx = WSAID_ACCEPTEX, 
		guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	SOCKADDR_IN addr;
	int rc;
	DWORD bytes;

	if ((newObj = (LPLISTEN_OBJ)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LISTEN_OBJ))) == NULL) {
		printf("HeapAlloc() failed with error %d", GetLastError());
		return NULL;
	}

	if ((newObj->sock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		printf("WSASocket() failed with error %d\n", WSAGetLastError());
		return NULL;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, SERVER_ADDR, &addr.sin_addr);
	if (bind(newObj->sock, (PSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR) {
		printf("bind() failed with error %d\n", WSAGetLastError());
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

	//Load GetAcceptExSockaddrs
	rc = WSAIoctl(
		newObj->sock,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidGetAcceptExSockaddrs,
		sizeof(guidGetAcceptExSockaddrs),
		&newObj->lpfnGetAcceptExSockaddrs,
		sizeof(newObj->lpfnGetAcceptExSockaddrs),
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
