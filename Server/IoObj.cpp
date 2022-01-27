#include "IoObj.h"
#include <stdio.h>
#include <MSWSock.h>
#include "EnvVar.h"

LPIO_OBJ getIoObject(IO_OBJ::OP operation, LPSESSION session, char * buffer, DWORD length) {
	LPIO_OBJ newobj = NULL;

	if ((newobj = (LPIO_OBJ)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IO_OBJ) + sizeof(BYTE) * length)) == NULL)
		printf("HeapAlloc() failed with error %d\n", GetLastError());

	if (newobj) {
		newobj->operation = operation;

		newobj->buffer = (char *)(((char *)newobj) + sizeof(IO_OBJ));

		newobj->session = session;

		newobj->dataBuff.len = length;
		newobj->dataBuff.buf = newobj->buffer;

		if (buffer != NULL)
			strcpy_s(newobj->buffer, length, buffer);
	}


	return newobj;
}

void freeIoObject(_In_ LPIO_OBJ ioobj) {
	printf("OP: %d\n", ioobj->operation);
	HeapFree(GetProcessHeap(), NULL, ioobj);
}

void IO_OBJ::setBufferSend(_In_z_ char *i_buffer) {
	strcpy_s(this->buffer, BUFFSIZE, i_buffer);
	this->dataBuff.buf = this->buffer;
	this->dataBuff.len = strlen(this->buffer);
}

void IO_OBJ::setBufferRecv(_In_z_ char *i_buffer) {
	strcpy_s(this->buffer, BUFFSIZE, i_buffer);
	int length = strlen(buffer);
	this->dataBuff.buf = this->buffer + length;
	this->dataBuff.len = BUFFSIZE - length;
}

void IO_OBJ::setFileOffset(LONG64 fileOffset) {
	this->overlapped.Offset = fileOffset & 0xFFFF'FFFF;
	this->overlapped.OffsetHigh = (fileOffset >> 32) & 0xFFFF'FFFF;
}

bool PostSend(_In_ SOCKET sock, _In_ LPIO_OBJ sendObj) {
	if ((WSASend(sock, &(sendObj->dataBuff), 1, NULL, 0, &(sendObj->overlapped), NULL)) == SOCKET_ERROR) {
		DWORD error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			printf("WSASend failed with error %d\n", error);
			return FALSE;
		}
	}

	return TRUE;
}

bool PostRecv(_In_ SOCKET sock, _In_ LPIO_OBJ recvObj) {
	DWORD flags = 0;
	if ((WSARecv(sock, &(recvObj->dataBuff), 1, NULL, &flags, &(recvObj->overlapped), NULL)) == SOCKET_ERROR) {
		DWORD error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			printf("WSARecv failed with error %d\n", error);
			return FALSE;
		}
	}

	return TRUE;
}

bool PostWrite(_In_ HANDLE hfile, _In_ LPIO_OBJ writeObj) {
	if (!WriteFile(hfile, writeObj->buffer, writeObj->dataBuff.len, NULL, &(writeObj->overlapped))) {
		DWORD error = WSAGetLastError();
		if (error != ERROR_IO_PENDING) {
			printf("WriteFile failed with error %d\n", error);
			return FALSE;
		}
	}

	return TRUE;
}

bool PostSendFile(SOCKET sock, HANDLE hfile, LPIO_OBJ sendFObj) {
	if (!TransmitFile(sock, hfile, sendFObj->dataBuff.len, 0, &(sendFObj->overlapped), NULL, 0)) {
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			printf("TransmitFile failed with error %d\n", error);
			return FALSE;
		}
	}
	return TRUE;
}

bool PostAcceptEx(LPLISTEN_OBJ listen, LPIO_OBJ acceptobj) {
	DWORD bytes;
	int rc;
	
	acceptobj->acceptSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (acceptobj->acceptSock == INVALID_SOCKET) {
		printf("socket() failed with error %d\n", WSAGetLastError());
		return FALSE;
	}

	rc = listen->lpfnAcceptEx(
		listen->sock,
		acceptobj->acceptSock,
		acceptobj->buffer,
		0,
		SIZE_OF_ADDRESS,
		SIZE_OF_ADDRESS,
		&bytes,
		&acceptobj->overlapped
	);
	
	if (rc == FALSE) {
		if (WSAGetLastError() != WSA_IO_PENDING) {
			printf("AcceptEx() failed with error %d\n", WSAGetLastError());
			return FALSE;
		}
	}

	return TRUE;
}

