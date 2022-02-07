#include "FileObj.h"
#include "Session.h"
#include <stdio.h>

LPFILEOBJ GetFileObj(HANDLE hfile, LPSOCKADDR_IN addr, LONG64 size, FILEOBJ::OP op) {
	LPFILEOBJ newobj = NULL; 
	GUID guidConnectEx = WSAID_CONNECTEX;
	DWORD bytes;
	int rc;

	if ((newobj = (LPFILEOBJ)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FILEOBJ))) == NULL)
		printf("HeapAlloc() failed with error %d\n", GetLastError());

	if (newobj) {
		newobj->file = hfile;
		newobj->size = size;
		newobj->operation = op;

		memcpy_s(&newobj->clientAddr, sizeof(newobj->clientAddr), addr, sizeof(SOCKADDR_IN));

		if ((newobj->fileSock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
			printf("WSASocket() failed with error %d\n", WSAGetLastError());
			return NULL;
		}

		rc = WSAIoctl(
			newobj->fileSock,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidConnectEx,
			sizeof(guidConnectEx),
			&newobj->lpfnConnectEx,
			sizeof(newobj->lpfnConnectEx),
			&bytes,
			NULL,
			NULL
		);
		if (rc == SOCKET_ERROR)
		{
			printf("WSAIoctl: SIO_GET_EXTENSION_FUNCTION_POINTER failed with error %d\n", WSAGetLastError());
			return NULL;
		}
	}

	return newobj;
}

void FreeFileObj(LPFILEOBJ fileobj) {
	CloseHandle(fileobj->file);
	HeapFree(GetProcessHeap(), NULL, fileobj);
}


