#include "Io.h"
#include <WinSock2.h>
#include <MSWSock.h>
#include <stdio.h>
#include "Envar.h"
#include "Service.h"

bool blockSend(SOCKET sock, char * sendBuff) {
	DWORD sentBytes = 0, bufferLen;
	WSABUF dataBuff;

	bufferLen = strlen(sendBuff);
	dataBuff.buf = sendBuff;
	dataBuff.len = bufferLen;

	//Send all buffer
	while (dataBuff.len > 0) {
		if (WSASend(sock, &dataBuff, 1, &sentBytes, 0, NULL, NULL) == SOCKET_ERROR) {
			printf("WSASend() failed with error %d\n", WSAGetLastError());
			return FALSE;
		}

		dataBuff.len -= sentBytes;
		dataBuff.buf = sendBuff + bufferLen - dataBuff.len;
	}

	return TRUE;
}

int blockRecv(SOCKET sock, char *recvBuff, DWORD length) {
	DWORD receivedBytes = 0, flag = 0;
	WSABUF dataBuff;

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

bool sendFile(LpSession session) {
	//Send file
	if (!TransmitFile(session->sock, session->hfile, session->fileSize, BUFFSIZE, NULL, NULL, 0)) {
		printf("TransmitFile() failed with error %d\n", WSAGetLastError());
		session->closeFile();
		return FALSE;
	}

	return TRUE;
}

bool recvFile(LpSession session) {
	WSAOVERLAPPED ol;
	char buf[BUFFSIZE];
	DWORD transBytes, n, remain;

	ZeroMemory(&ol, sizeof(WSAOVERLAPPED));

	initMessage(buf, RECEIVE, NULL, NULL);
	blockSend(session->sock, buf);

	*buf = 0;
	n = session->fileSize / BUFFSIZE;
	remain = session->fileSize % BUFFSIZE;
	
	for(int i = 0; i < n; ++i, *buf = 0) {
		if (blockRecv(session->sock, buf, BUFFSIZE) != BUFFSIZE) {
			session->closeFile();
			return FALSE;
		}

		ol.Offset = (i * BUFFSIZE) & 0xFFFF'FFFF;
		ol.OffsetHigh = ((i * BUFFSIZE) >> 32) & 0xFFFF'FFFF;

		if (!WriteFile(session->hfile, buf, BUFFSIZE, &transBytes, &ol)) {
			printf("WriteFile() failed with error %d", GetLastError());
			session->closeFile();
			return FALSE;
		}
	}

	if (remain != 0) {
		*buf = 0;
		
		if (blockRecv(session->sock, buf, remain) != remain) {
			session->closeFile();
			return FALSE;
		}

		ol.Offset = (n * BUFFSIZE) & 0xFFFF'FFFF;
		ol.OffsetHigh = ((n * BUFFSIZE) >> 32) & 0xFFFF'FFFF;

		if (!WriteFile(session->hfile, buf, remain, &transBytes, &ol)) {
			printf("WriteFile() failed with error %d", GetLastError());
			session->closeFile();
			return FALSE;
		}
	}

	return TRUE;
}
