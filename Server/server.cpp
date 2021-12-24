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
	if ((listenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
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

	printf("Server started\n");

	SOCKET acceptSock;
	LPSESSION session;
	LPIO_OBJ receiveObj;
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
			continue;
		}

		// Step 7: Associate the accepted socket with the original completion port
		printf("Socket number %d got connected...\n", acceptSock);
		session->cmdSock = acceptSock;
		if (CreateIoCompletionPort((HANDLE)acceptSock, completionPort, (ULONG_PTR)&(session->cmdSock), 0) == NULL) {
			printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
			return 1;
		}

		receiveObj = getIoObject(IO_OBJ::RECV_C);
		if (receiveObj == NULL) {
			freeSession(session);
			continue;
		}

		flags = 0;

		if (WSARecv(acceptSock, &(receiveObj->dataBuff), 1, &transferredBytes, &flags, &(receiveObj->overlapped), NULL) == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				printf("WSARecv() failed with error %d\n", WSAGetLastError());
				return 1;
			}
		}
	}

	return 0;
}



void handleRecieve(_Inout_ LPIO_OBJ recieveObj,_Inout_ LPSESSION session,_In_range_(1, BUFFSIZE) DWORD transferredBytes) {
	recieveObj->buffer[transferredBytes] = 0;
	LPIO_OBJ replyObj;
	char *mess = recieveObj->buffer,
		* pos = NULL,
		reply[BUFFSIZE];
	DWORD flags = 0;

	//Split string by ending delimiter
	while ((pos = strstr(mess, ENDING_DELIMITER)) != NULL){
		*pos = 0;
		handleMess(session, mess, reply);
		
		//Creat new overlapped object to send
		replyObj = getIoObject(IO_OBJ::SEND_C);
		if (replyObj == NULL)
			continue;

		replyObj->setBufferSend(reply);

		WSASend(session->cmdSock, &(replyObj->dataBuff), 1, NULL, 0, &(replyObj->overlapped), NULL);
		mess = pos + strlen(ENDING_DELIMITER);
	}

	//The remaining buffer which doesnt end with ending delimiter
	//mess contains garbage after this call
	recieveObj->setBufferRecv(mess);

	WSARecv(session->cmdSock, &(recieveObj->dataBuff), 1, NULL, &flags, &(recieveObj->overlapped), NULL);
}

void handleSend(_Inout_ LPIO_OBJ sendObj,_Inout_ LPSESSION session, _In_range_(1, BUFFSIZE) DWORD transferredBytes) {

	if (transferredBytes < sendObj->dataBuff.len) {
		sendObj->dataBuff.len -= transferredBytes;
		sendObj->dataBuff.buf = sendObj->buffer + strlen(sendObj->buffer) - sendObj->dataBuff.len;

		WSASend(session->cmdSock, &(sendObj->dataBuff), 1, NULL, 0, &(sendObj->overlapped), NULL);
	}
	else
		freeIoObject(sendObj);
}

unsigned __stdcall serverWorkerThread(LPVOID completionPortID) {
	HANDLE completionPort = (HANDLE)completionPortID;
	DWORD transferredBytes;
	LPSESSION session = NULL;
	LPIO_OBJ ioobj = NULL;
	ULONG_PTR key = NULL;

	while (true) {
		if (GetQueuedCompletionStatus(completionPort, &transferredBytes, (PULONG_PTR) &key, (LPOVERLAPPED *)&ioobj, INFINITE) == 0) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
			return 0;
		}

		/*if (buffer->operation == BUFFER::RECV_F)
			session = CONTAINING_RECORD(key, SESSION, fileSock);
		else*/
			session = CONTAINING_RECORD(key, SESSION, cmdSock);
			
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

