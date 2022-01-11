#include <WinSock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include <stdio.h>
#include <sqltypes.h>
#include "EnvVar.h"
#include "Session.h"
#include "IoObj.h"
#include "Service.h"
#include "FileObj.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

HANDLE gCompletionPort;
SQLHANDLE gSqlStmtHandle;

unsigned __stdcall serverWorkerThread(LPVOID completionPortID);

int main(int argc, char *argv[]) {
	if (!connectSQL())
		return 1;

	WSADATA wsaData;
	if (WSAStartup((2, 2), &wsaData) != 0) {
		printf("WSAStartup() failed with error %d\n", GetLastError());
		return 1;
	}

	// Step 1: Setup an I/O completion port
	if ((gCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) == NULL) {
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
		if (_beginthreadex(0, 0, serverWorkerThread, (void*)gCompletionPort, 0, 0) == 0) {
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

	while (true) {
		// Step 5: Accept connections
		if ((acceptSock = WSAAccept(listenSock, NULL, NULL, NULL, 0)) == SOCKET_ERROR) {
			printf("WSAAccept() failed with error %d\n", WSAGetLastError());
			return 1;
		}

		// Step 6: Create a socket information structure to associate with the socket
		if ((session = getSession()) == NULL)
			continue;

		// Step 7: Associate the accepted socket with the original completion port
		printf("Socket number %d got connected...\n", acceptSock);
		session->cmdSock = acceptSock;
		if (CreateIoCompletionPort((HANDLE)acceptSock, gCompletionPort, (ULONG_PTR)&(session->cmdSock), 0) == NULL) {
			printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
			return 1;
		}

		receiveObj = getIoObject(IO_OBJ::RECV_C, NULL, BUFFSIZE);
		if (receiveObj == NULL) {
			freeSession(session);
			continue;
		}

		PostRecv(acceptSock, receiveObj);
	}

	return 0;
}

void handleRecieve(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ recieveObj, _In_ DWORD transferredBytes) {
	recieveObj->buffer[transferredBytes] = 0;
	LPIO_OBJ replyObj;
	char *mess = recieveObj->buffer,
		*pos = NULL,
		reply[BUFFSIZE];
	DWORD flags = 0;


	//Split string by ending delimiter
	for (; (pos = strstr(mess, ENDING_DELIMITER)) != NULL; mess = pos + strlen(ENDING_DELIMITER)) {
		*pos = 0;
		handleMess(session, mess, reply);

		if (strlen(reply) == 0)
			continue;

		replyObj = getIoObject(IO_OBJ::SEND_C, reply, strlen(reply) + 1);
		if (replyObj == NULL)
			continue;

		PostSend(session->cmdSock, replyObj);
	}

	//The remaining buffer which doesnt end with ending delimiter
	recieveObj->setBufferRecv(mess);

	if (!PostRecv(session->cmdSock, recieveObj))
		freeSession(session);
}

void handleSend(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ sendObj, _In_ DWORD transferredBytes) {
	if (transferredBytes < sendObj->dataBuff.len) {
		//Send the rest
		sendObj->dataBuff.len -= transferredBytes;
		sendObj->dataBuff.buf = sendObj->buffer + strlen(sendObj->buffer) - sendObj->dataBuff.len;

		PostSend(session->cmdSock, sendObj);
	}
	else
		freeIoObject(sendObj);
}

void handleRecvFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ recvObj, _In_ DWORD transferredBytes) {
	//Change operation to write file
	recvObj->operation = IO_OBJ::WRTE_F;

	//Offset based on sequecnce of receive request
	DWORD64 fileOffset = recvObj->sequence * BUFFSIZE;

	recvObj->overlapped.Offset = fileOffset & 0xFFFF'FFFF;
	recvObj->overlapped.OffsetHigh = (fileOffset >> 32) & 0xFFFF'FFFF;

	//Write
	if (!PostWrite(session->fileobj->file, recvObj)) {
		char reply[BUFFSIZE];
		sprintf_s(reply, BUFFSIZE, "%s%s%d%s%s%s", RESPONE, HEADER_DELIMITER,
			TRANSMIT_FAIL, PARA_DELIMITER, "Write file fail", ENDING_DELIMITER);

		LPIO_OBJ sendObj = getIoObject(IO_OBJ::SEND_C, reply, strlen(reply) + 1);
		PostSend(session->cmdSock, sendObj);

		session->closeFile();
	}
}

void handleWriteFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ writeObj, _In_ DWORD transferredBytes) {
	freeIoObject(writeObj);

	//fileSize - transferredBytes
	InterlockedAdd64(&(session->fileobj->size), -(LONG64)transferredBytes);

	//if(fileSize == 0)
	if (!InterlockedCompareExchange64(&(session->fileobj->size), -1, 0)) {
		char reply[BUFFSIZE];
		sprintf_s(reply, BUFFSIZE, "%s%s%d%s%s%s", RESPONE, HEADER_DELIMITER,
			FINISH_SEND, PARA_DELIMITER, "Received file", ENDING_DELIMITER);

		LPIO_OBJ sendObj = getIoObject(IO_OBJ::SEND_C, reply, strlen(reply) + 1);

		PostSend(session->cmdSock, sendObj);
		session->closeFile();
	}
}

void hanldeSendFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ sendObj, _In_ DWORD transferredBytes) {
	freeIoObject(sendObj);

	//fileSize - transferredBytes
	InterlockedAdd64(&(session->fileobj->size), -(LONG64)transferredBytes);

	//if(fileSize == 0)
	if (!InterlockedCompareExchange64(&(session->fileobj->size), -1, 0))
		session->closeFile();
}

unsigned __stdcall serverWorkerThread(LPVOID completionPortID) {
	HANDLE completionPort = (HANDLE)completionPortID;
	DWORD transferredBytes;
	LPSESSION session = NULL;
	LPIO_OBJ ioobj = NULL;
	ULONG_PTR key = NULL;

	while (true) {
		if (GetQueuedCompletionStatus(completionPort, &transferredBytes, (PULONG_PTR)&key, (LPOVERLAPPED *)&ioobj, INFINITE) == 0) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
			continue;
		}

		session = CONTAINING_RECORD(key, SESSION, cmdSock);

		// Check to see if an error has occurred on the socket and if so
		// then close the socket and cleanup the SOCKET_INFORMATION structure
		// associated with the socket
		if (transferredBytes == 0) {
			freeSession(session);
			freeIoObject(ioobj);
			continue;
		}

		switch (ioobj->operation)
		{
		case IO_OBJ::RECV_C: handleRecieve(session, ioobj, transferredBytes); break;
		case IO_OBJ::SEND_C: handleSend(session, ioobj, transferredBytes); break;
		case IO_OBJ::RECV_F: handleRecvFile(session, ioobj, transferredBytes); break;
		case IO_OBJ::SEND_F: hanldeSendFile(session, ioobj, transferredBytes); break;
		case IO_OBJ::WRTE_F: handleWriteFile(session, ioobj, transferredBytes); break;
		default:
			break;
		}
	}

}
