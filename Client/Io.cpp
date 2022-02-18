#include "Io.h"
#include <WinSock2.h>
#include <MSWSock.h>
#include <stdio.h>
#include "Envar.h"

bool blockSend(SOCKET sock, char * sendBuff, DWORD len) {
	DWORD sentBytes = 0;
	WSABUF dataBuff;

	dataBuff.buf = sendBuff;
	dataBuff.len = len;

	//Send all buffer
	while (dataBuff.len > 0) {
		if (WSASend(sock, &dataBuff, 1, &sentBytes, 0, NULL, NULL) == SOCKET_ERROR) {
			printf("WSASend() failed with error %d\n", WSAGetLastError());
			return FALSE;
		}

		dataBuff.len -= sentBytes;
		dataBuff.buf = sendBuff + len - dataBuff.len;
	}

	return TRUE;
}

int blockRecv(SOCKET sock, char *recvBuff, DWORD length) {
	DWORD receivedBytes = 0, flag = 0;
	WSABUF dataBuff;

	//set receive buffer if  receive buffer not empty
	if (strlen(recvBuff) != 0)
		dataBuff.buf = recvBuff + strlen(recvBuff);
	else 
		dataBuff.buf = recvBuff;
	dataBuff.len = length;

	if (WSARecv(sock, &dataBuff, 1, &receivedBytes, &flag, NULL, NULL) == SOCKET_ERROR) {
		printf("WSARecv() failed with error %d\n", WSAGetLastError());
		return 0;
	}

	return receivedBytes;
}

bool sendFile(SOCKET sock, HANDLE hfile, LONG64 size) {
	WSAOVERLAPPED ol;
	LONG64 offset = 0;
	DWORD transBytes;

	ZeroMemory(&ol, sizeof(WSAOVERLAPPED));

	while (offset < size) {
		char buf[BUFFSIZE] = "";

		//readfile
		if (!ReadFile(hfile, buf, BUFFSIZE, &transBytes, &ol)) {
			printf("ReadFile() failed with error %d", GetLastError());
			return FALSE;
		}

		//sendfile
		if (!blockSend(sock, buf, transBytes)) {
			return FALSE;
		}


		offset += transBytes;
		ol.Offset = offset & 0xFFFF'FFFF;
		ol.OffsetHigh = (offset >> 32) & 0xFFFF'FFFF;
	}
	return TRUE;
}

bool recvFile(SOCKET sock, HANDLE hfile, LONG64 size) {
	WSAOVERLAPPED ol;
	LONG64 offset = 0;
	DWORD transBytes;

	ZeroMemory(&ol, sizeof(WSAOVERLAPPED));

	while (offset < size) {
		char buf[BUFFSIZE] = "";

		//receive file
		transBytes = blockRecv(sock, buf, BUFFSIZE);
		if (transBytes <= 0) {
			return FALSE;
		}

		ol.Offset = offset & 0xFFFF'FFFF;
		ol.OffsetHigh = (offset >> 32) & 0xFFFF'FFFF;

		//write file
		if (!WriteFile(hfile, buf, transBytes, &transBytes, &ol)) {
			printf("WriteFile() failed with error %d", GetLastError());
			return FALSE;
		}

		offset += transBytes;
	}
	
	return TRUE;
}
