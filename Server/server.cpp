#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>
#include <sqltypes.h>
#include <process.h>
#include <stdio.h>
#include "Service.h"
#include "EnvVar.h"
#include "Session.h"
#include "IoObj.h"
#include "FileObj.h"
#include "ListenObj.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

HANDLE gCompletionPort;
SQLHANDLE gSqlStmtHandle;
LPLISTEN_OBJ gCmdListen;
LPLISTEN_OBJ gFileListen;

unsigned __stdcall serverWorkerThread(LPVOID completionPortID);

int main(int argc, char *argv[]) {
	SYSTEM_INFO systemInfo;
	SOCKET acceptSock;
	LPSESSION session;
	LPIO_OBJ receiveObj;

	if (!connectSQL())
		return 1;

	WSADATA wsaData;
	if (WSAStartup((2, 2), &wsaData) != 0) {
		printf("WSAStartup() failed with error %d\n", GetLastError());
		return 1;
	}

	//Setup an I/O completion port
	if ((gCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return 1;
	}

	// Determine how many processors are on the system
	GetSystemInfo(&systemInfo);

	//Create worker threads based on the number of processors available on the
	//system. Create two worker threads for each processor	
	for (int i = 0; i < (int)systemInfo.dwNumberOfProcessors * 2; i++) {
		// Create a server worker thread and pass the completion port to the thread
		if (_beginthreadex(0, 0, serverWorkerThread, (void*)gCompletionPort, 0, 0) == 0) {
			printf("Create thread failed with error %d\n", GetLastError());
			return 1;
		}
	}

	gCmdListen = getListenObj(CMD_PORT);
	gFileListen = getListenObj(FILE_PORT);

	if (gCmdListen == NULL || gFileListen == NULL)
		return 1;

	if (CreateIoCompletionPort((HANDLE)gFileListen->sock, gCompletionPort, (ULONG_PTR)&gFileListen, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return 1;
	}

	printf("Server started\n");

	while (true) {
		//Accept connections
		if ((acceptSock = WSAAccept(gCmdListen->sock, NULL, NULL, NULL, 0)) == SOCKET_ERROR) {
			printf("WSAAccept() failed with error %d\n", WSAGetLastError());
			return 1;
		}

		//Create a socket information structure to associate with the socket
		if ((session = getSession()) == NULL)
			continue;

		//Associate the accepted socket with the original completion port
		printf("Socket number %d got connected...\n", acceptSock);
		session->cmdSock = acceptSock;
		if (CreateIoCompletionPort((HANDLE)acceptSock, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
			printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
			return 1;
		}

		receiveObj = getIoObject(IO_OBJ::RECV_C, session, NULL, BUFFSIZE);
		if (receiveObj == NULL) {
			freeSession(session);
			continue;
		}


		InterlockedIncrement(&session->oustandingOp);

		if (!PostRecv(acceptSock, receiveObj)) {
			freeIoObject(receiveObj);
			freeSession(session);
		}


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
	while (((pos = strstr(mess, ENDING_DELIMITER)) != NULL) && session->outstandingSend < MAX_SEND_PER_SESSION)   {
		*pos = 0;
		handleMess(session, mess, reply);

		if (strlen(reply) == 0)
			break;

		replyObj = getIoObject(IO_OBJ::SEND_C, session, reply, strlen(reply) + 1);
		if (replyObj == NULL)
			break;

		session->EnListPendingOperation(replyObj);
		InterlockedIncrement(&session->outstandingSend);

		mess = pos + strlen(ENDING_DELIMITER);
	}

	//The remaining buffer which doesnt end with ending delimiter
	recieveObj->setBufferRecv(mess);

	session->EnListPendingOperation(recieveObj);
}

void handleSend(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ sendObj, _In_ DWORD transferredBytes) {
	if (transferredBytes != sendObj->dataBuff.len)
		printf("Internal error?\n");
	freeIoObject(sendObj);
	InterlockedDecrement(&session->outstandingSend);
}

void handleRecvFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ recvObj, _In_ DWORD transferredBytes) {
	//Change operation to write file
	recvObj->operation = IO_OBJ::WRTE_F;
	recvObj->dataBuff.len = transferredBytes;
	session->EnListPendingOperation(recvObj);
}

void handleWriteFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ writeObj, _In_ DWORD transferredBytes) {
	EnterCriticalSection(&(session->cs));

	if (!session->fileobj) {
		freeIoObject(writeObj);
		LeaveCriticalSection(&(session->cs));
		return;
	}

	session->fileobj->bytesWritten += transferredBytes;

	if (session->fileobj->bytesWritten >= session->fileobj->size) {
		freeIoObject(writeObj);
		session->closeFile(FALSE);
	}
	else if (session->fileobj->bytesRecved >= session->fileobj->size)
		freeIoObject(writeObj);
	else {
		ZeroMemory(&(writeObj->overlapped), sizeof(OVERLAPPED));

		writeObj->operation = IO_OBJ::RECV_F;
		writeObj->dataBuff.len = BUFFSIZE;
		writeObj->setFileOffset(session->fileobj->bytesRecved);

		session->EnListPendingOperation(writeObj);
		session->fileobj->bytesRecved += writeObj->dataBuff.len;
	}

	LeaveCriticalSection(&(session->cs));
}

void hanldeSendFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ sendObj, _In_ DWORD transferredBytes) {
	LONG64 remain;

	if (!session->fileobj)
		return;

	session->fileobj->bytesSended += transferredBytes;
	remain = session->fileobj->size - session->fileobj->bytesSended;

	if (remain <= 0) {
		freeIoObject(sendObj);
		closesocket(session->fileSock);
		return;
	}

	sendObj->setFileOffset(session->fileobj->bytesSended);

	sendObj->dataBuff.len = min(remain, TRANSMITFILE_MAX);
	session->EnListPendingOperation(sendObj);
}

void handleAccpetFile(_In_ LPLISTEN_OBJ listenobj, _Inout_ LPSESSION session, _Inout_ LPIO_OBJ acceptObj) {
	SOCKADDR_STORAGE *LocalSockaddr = NULL, 
		*RemoteSockaddr = NULL;
	LPIO_OBJ replyObj = NULL;
	char reply[BUFFSIZE];
	int LocalSockaddrLen, RemoteSockaddrLen, rc;

	listenobj->lpfnGetAcceptExSockaddrs(
		acceptObj->buffer,
		0,
		SIZE_OF_ADDRESS,
		SIZE_OF_ADDRESS,
		(SOCKADDR **)&LocalSockaddr,
		&LocalSockaddrLen,
		(SOCKADDR **)&RemoteSockaddr,
		&RemoteSockaddrLen
	);

	session->fileSock = acceptObj->acceptSock;
	freeIoObject(acceptObj);

	if (setsockopt(session->fileSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		(char *)&gFileListen->sock, sizeof(SOCKET)) == SOCKET_ERROR) {
		printf("setsockopt failed with error %d\n", WSAGetLastError());

		initMessage(reply, RESPONE, SERVER_FAIL, "setsockopt failed");
		replyObj = getIoObject(IO_OBJ::SEND_C, session, reply, strlen(reply) + 1);
		session->EnListPendingOperation(replyObj);
	}

	if (CreateIoCompletionPort((HANDLE)session->fileSock, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());

		initMessage(reply, RESPONE, SERVER_FAIL, "CreateIoCompletionPort failed");
		replyObj = getIoObject(IO_OBJ::SEND_C, session, reply, strlen(reply) + 1);
		session->EnListPendingOperation(replyObj);
	}

	switch (session->fileobj->operation) {
	case FILEOBJ::RETR:
		LPIO_OBJ sendFObj;
		
		sendFObj = getIoObject(IO_OBJ::SEND_F, session, NULL, 0);
		sendFObj->dataBuff.len = min(session->fileobj->size, TRANSMITFILE_MAX);
		session->EnListPendingOperation(sendFObj);
		break;
	case FILEOBJ::STOR:
		LPIO_OBJ recvFobj;
		int i = 0;

		//Receive and write file in chunks
		while (session->fileobj->bytesRecved < session->fileobj->size && i++ < MAX_IOOBJ_PER_FILEOBJ) {

			recvFobj = getIoObject(IO_OBJ::RECV_F, session, NULL, BUFFSIZE);
			if (recvFobj == NULL) {
				initMessage(reply, RESPONE, SERVER_FAIL, "Heap out of memory?");
				replyObj = getIoObject(IO_OBJ::SEND_C, session, reply, strlen(reply) + 1);
				session->EnListPendingOperation(replyObj);
				break;
			}

			recvFobj->setFileOffset(session->fileobj->bytesRecved);
			session->EnListPendingOperation(recvFobj);
			session->fileobj->bytesRecved += recvFobj->dataBuff.len;
		}
	}
}

void handleAccpetCommand(_In_ LPLISTEN_OBJ listenobj, _Out_ LPSESSION session, _Inout_ LPIO_OBJ acceptObj) {

}

void ProcessPendingOperations(_In_ LPSESSION session) {
	EnterCriticalSection(&session->cs);
	bool noError;

	while (!session->pending->empty()) {
		LPIO_OBJ ioobj = session->pending->front();

		switch (ioobj->operation) {
		case IO_OBJ::RECV_C:
			if (!(strstr(ioobj->buffer, ENDING_DELIMITER) == NULL)) {
				handleRecieve(session, ioobj, strlen(ioobj->buffer));

				session->pending->pop_front();
				LeaveCriticalSection(&session->cs);
				return;
			}
			else
				noError = PostRecv(session->cmdSock, ioobj);
			break;
		case IO_OBJ::SEND_C:
			noError = PostSend(session->cmdSock, ioobj);
			break;
		case IO_OBJ::RECV_F:
			if (!session->fileobj || session->fileSock == INVALID_SOCKET || !PostRecv(session->fileSock, ioobj)) {
				noError = FALSE;
				session->closeFile(TRUE);
			}
			else noError = TRUE;
			break;
		case IO_OBJ::WRTE_F:
			if (!session->fileobj || session->fileSock == INVALID_SOCKET || !PostWrite(session->fileobj->file, ioobj)) {
				noError = FALSE;
				session->closeFile(TRUE);
			}
			else noError = TRUE;
			break;
		case IO_OBJ::SEND_F:
			if (!session->fileobj || session->fileSock == INVALID_SOCKET || !PostSendFile(session->fileSock, session->fileobj->file, ioobj)) {
				noError = FALSE;
				session->closeFile(FALSE);
			}
			else noError = TRUE;
			break;
		}

		if (noError)
			InterlockedIncrement(&session->oustandingOp);
		else
			freeIoObject(ioobj);

		session->pending->pop_front();
	}
	LeaveCriticalSection(&session->cs);
}

unsigned __stdcall serverWorkerThread(LPVOID completionPortID) {
	HANDLE completionPort = (HANDLE)completionPortID;
	DWORD transferredBytes, flag;
	LPLISTEN_OBJ listen = NULL;
	LPSESSION session = NULL;
	LPIO_OBJ ioobj = NULL;
	SOCKET s;
	HANDLE h;
	ULONG_PTR key = NULL;
	int rc;

	while (true) {
		rc = GetQueuedCompletionStatus(completionPort, &transferredBytes, (PULONG_PTR)&key, (LPOVERLAPPED *)&ioobj, INFINITE);
		if (rc == FALSE) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
		}

		switch (ioobj->operation) {
			case IO_OBJ::RECV_C:
			case IO_OBJ::SEND_C:
				session = (LPSESSION)key;
				s = session->cmdSock; 
				rc = WSAGetOverlappedResult(s, &ioobj->overlapped, &transferredBytes, FALSE, &flag);
				if (rc == FALSE) {
					printf("WSAGetOverlappedResult failed with error %d\n", WSAGetLastError());
					/*shutdown(s, SD_BOTH);
					if (CancelIoEx((HANDLE)session->cmdSock, NULL)) {
						printf("CancelIoEx failed with error %d\n", GetLastError());
					}*/
					session->closeFile(TRUE);
					InterlockedExchange(&session->bclosing, 1);
				}
				break;
			case IO_OBJ::RECV_F:
			case IO_OBJ::SEND_F:
				session = (LPSESSION)key;
				s = session->fileSock;
				rc = WSAGetOverlappedResult(s, &ioobj->overlapped, &transferredBytes, FALSE, &flag);
				if (rc == FALSE) {
					printf("WSAGetOverlappedResult failed with error %d\n", WSAGetLastError());
					session->closeFile(TRUE);
				}
				break;
			case IO_OBJ::WRTE_F:
				session = (LPSESSION)key;
				EnterCriticalSection(&session->cs);
				if (session->fileobj != NULL) {
					h = session->fileobj->file;
					rc = GetOverlappedResult(h, &ioobj->overlapped, &transferredBytes, FALSE);
					if (rc == FALSE) {
						printf("GetOverlappedResult failed with error %d\n", GetLastError());
						session->closeFile(TRUE);
					}
				}
				LeaveCriticalSection(&session->cs);
				break;
			case IO_OBJ::ACPT_C:
			case IO_OBJ::ACPT_F:
				s = ((LPLISTEN_OBJ)key)->sock; break;
				rc = WSAGetOverlappedResult(s, &ioobj->overlapped, &transferredBytes, FALSE, &flag);
				if (rc == FALSE)
					printf("WSAGetOverlappedResult failed with error %d\n", WSAGetLastError());
				break;
			}
		
		if (ioobj->operation == IO_OBJ::ACPT_C || ioobj->operation == IO_OBJ::ACPT_F) {
			listen = (LPLISTEN_OBJ) key;

			switch (ioobj->operation) {
				case IO_OBJ::ACPT_C: 
					handleAccpetCommand(listen, session, ioobj); 
					break;
				case IO_OBJ::ACPT_F: 
					session = ioobj->session;
					handleAccpetFile(listen, session, ioobj); 
					break;
			}

			if (InterlockedCompareExchange(&session->bclosing, 0, 0) == 0)
				ProcessPendingOperations(session);
		}	
		else {
			session = (LPSESSION) key;

			if (rc == FALSE || InterlockedCompareExchange(&session->bclosing, 0, 0)) {
				freeIoObject(ioobj);
				
			}
			else {

				if (transferredBytes == 0) {
					freeIoObject(ioobj);
					continue;
				}

				switch (ioobj->operation) {
					case IO_OBJ::RECV_C: handleRecieve(session, ioobj, transferredBytes); break;
					case IO_OBJ::SEND_C: handleSend(session, ioobj, transferredBytes); break;
					case IO_OBJ::RECV_F: handleRecvFile(session, ioobj, transferredBytes); break;
					case IO_OBJ::SEND_F: hanldeSendFile(session, ioobj, transferredBytes); break;
					case IO_OBJ::WRTE_F: handleWriteFile(session, ioobj, transferredBytes); break;
				}
			}

			if (InterlockedCompareExchange(&session->bclosing, 0, 0) == 0)
				ProcessPendingOperations(session);
		
			InterlockedDecrement(&session->oustandingOp);
			if (InterlockedCompareExchange(&session->oustandingOp, 0, 0) == 0 &&
				InterlockedCompareExchange(&session->bclosing, -1, 1) == 1) {
				freeSession(session);
			}
		}	
	}

}
