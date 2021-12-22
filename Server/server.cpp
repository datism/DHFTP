#include <WinSock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include <stdio.h>
#include "Session.h"
#include "IoObj.h"
#include "Service.h"

#pragma comment(lib, "Ws2_32.lib")


unsigned __stdcall serverWorkerThread(LPVOID completionPortID);

int main(int argc, char *argv[]) {
	WSADATA wsaData;
	if (WSAStartup((2, 2), &wsaData) != 0) {
		printf("WSAStartup() failed with error %d\n", GetLastError());
		return 1;
	}

	// Step 1: Setup an I/O completion port
	HANDLE completionPort;
	if ((completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return 1;
	}

	// Step 2: Determine how many processors are on the system
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	// Step 3: Create worker threads based on the number of processors available on the
	// system. Create two worker threads for each processor	
	for (int i = 0; i < (int)systemInfo.dwNumberOfProcessors * 2; i++) {
		// Create a server worker thread and pass the completion port to the thread
		if (_beginthreadex(0, 0, serverWorkerThread, (void*)completionPort, 0, 0) == 0) {
			printf("Create thread failed with error %d\n", GetLastError());
			return 1;
		}
	}

	// Step 4: Create a listening socket
	SOCKET listenSock;
	SOCKADDR_IN serverAddr;
	if ((listenSock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		printf("WSASocket() failed with error %d\n", WSAGetLastError());
		return 1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(CMD_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);
	if (bind(listenSock, (PSOCKADDR)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		printf("bind() failed with error %d\n", WSAGetLastError());
		return 1;
	}

	// Prepare socket for listening
	if (listen(listenSock, 20) == SOCKET_ERROR) {
		printf("listen() failed with error %d\n", WSAGetLastError());
		return 1;
	}

	SOCKET acceptSock;
	LPSESSION session;
	LPIO_OBJ ioobj;
	DWORD flags, transferredBytes;
	while (true) {
		// Step 5: Accept connections
		if ((acceptSock = WSAAccept(listenSock, NULL, NULL, NULL, 0)) == SOCKET_ERROR) {
			printf("WSAAccept() failed with error %d\n", WSAGetLastError());
			return 1;
		}

		// Step 6: Create a socket information structure to associate with the socket
		if ((session = (LPSESSION)GlobalAlloc(GPTR, sizeof(SESSION))) == NULL) {
			printf("GlobalAlloc() failed with error %d\n", GetLastError());
			return 1;
		}

		// Step 7: Associate the accepted socket with the original completion port
		printf("Socket number %d got connected...\n", acceptSock);
		session->cmdSock = acceptSock;
		if (CreateIoCompletionPort((HANDLE)acceptSock, completionPort, (DWORD)session, 0) == NULL) {
			printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
			return 1;
		}

		ioobj = getIoObject(IO_OBJ::RECV_C);
		flags = 0;

		if (WSARecv(acceptSock, &(ioobj->dataBuff), 1, &transferredBytes, &flags, &(ioobj->overlapped), NULL) == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				printf("WSARecv() failed with error %d\n", WSAGetLastError());
				return 1;
			}
		}
	}

	return 0;
}



void handleRecieve(LPIO_OBJ ioobj, LPSESSION session, DWORD transferredBytes) {
	ioobj->buffer[transferredBytes] = 0;
	char *pos = NULL;
	char *mess = ioobj->buffer;
	LPIO_OBJ reply;
	char reply[BUFFSIZE];

	while ((pos = strstr(mess, ENDING_DELIMITER)) != NULL){
		reply = getIoObject(IO_OBJ::SEND_C);
		handleMess(session, mess, reply->buffer);
		reply->length = strlen(reply->buffer);
		WSASend(session->cmdSock, &(reply->dataBuff), 1, NULL, 0, &(reply->overlapped), NULL);
		mess = pos + strlen(ENDING_DELIMITER);
	}

	if (strlen(mess) != 0) {
		strcpy_s(ioobj->buffer, BUFFSIZE, mess);
		ioobj->dataBuff.buf = ioobj->buffer + strlen(mess) + 1;
	}

	WSARecv(session->cmdSock, &(ioobj->dataBuff), 1, NULL, 0, &(ioobj->overlapped), NULL);
}

void handleSend(LPIO_OBJ ioobj, LPSESSION session, DWORD transferredBytes) {
	if (transferredBytes < ioobj->length) {
		int len = ioobj->length - transferredBytes;
		ioobj->dataBuff.buf = ioobj->buffer + strlen(ioobj->buffer) - len;
		ioobj->length = len;
		WSASend(session->cmdSock, &(ioobj->dataBuff), 1, NULL, 0, &(ioobj->overlapped), NULL);
	}
	else
		free(ioobj);
}

unsigned __stdcall serverWorkerThread(LPVOID completionPortID) {
	HANDLE completionPort = (HANDLE)completionPortID;
	DWORD transferredBytes;
	LPSESSION session = NULL;
	LPIO_OBJ ioobj = NULL;
	ULONG_PTR key;
	DWORD flag;

	while (true) {
		if (GetQueuedCompletionStatus(completionPort, &transferredBytes, (PULONG_PTR) key, (LPOVERLAPPED *)&ioobj, INFINITE) == 0) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
			return 0;
		}

		/*if (buffer->operation == BUFFER::RECV_F)
			session = CONTAINING_RECORD(key, SESSION, fileSock);
		else*/
			session == CONTAINING_RECORD(key, SESSION, cmdSock);

		// Check to see if an error has occurred on the socket and if so
		// then close the socket and cleanup the SOCKET_INFORMATION structure
		// associated with the socket
		if (transferredBytes == 0) {
			freeSession(session);
			GlobalFree(ioobj);
			continue;
		}

		if (ioobj->operation == IO_OBJ::RECV_C)
			handleRecieve(ioobj, session, transferredBytes);
		else if (ioobj->operation == IO_OBJ::SEND_C)
			handleSend(ioobj, session, transferredBytes);
	}

}

