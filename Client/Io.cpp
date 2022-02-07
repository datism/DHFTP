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
	LARGE_INTEGER totalBytes;
	ZeroMemory(&totalBytes, sizeof(totalBytes));
	//Send file
	while (totalBytes.QuadPart < session->fileSize) {
		DWORD bytes = min(session->fileSize - totalBytes.QuadPart, TRANSMITFILE_MAX);

		if (!TransmitFile(session->sock, session->hfile, bytes, 0, NULL, NULL, 0)) {
			printf("TransmitFile() failed with error %d\n", WSAGetLastError());
			session->closeFile();
			return FALSE;
		}

		totalBytes.LowPart += bytes;
		SetFilePointerEx(session->hfile, totalBytes, NULL, FILE_BEGIN);
	}
	return TRUE;
}

bool recvFile(LpSession session) {
	WSAOVERLAPPED ol;
	LONG64 offset = 0;
	DWORD transBytes;

	ZeroMemory(&ol, sizeof(WSAOVERLAPPED));

	while (offset < session->fileSize) {
		char buf[BUFFSIZE] = "";

		transBytes = blockRecv(session->sock, buf, BUFFSIZE);
		if (transBytes <= 0) {
			session->closeFile();
			return FALSE;
		}

		ol.Offset = offset & 0xFFFF'FFFF;
		ol.OffsetHigh = (offset >> 32) & 0xFFFF'FFFF;

		if (!WriteFile(session->hfile, buf, transBytes, &transBytes, &ol)) {
			printf("WriteFile() failed with error %d", GetLastError());
			session->closeFile();
			return FALSE;
		}

		offset += transBytes;
	}
	

	return TRUE;
}
