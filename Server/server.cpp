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

unsigned __stdcall serverWorkerThread(LPVOID completionPortID);

int main(int argc, char *argv[]) {
	WSADATA wsaData;
	SYSTEM_INFO systemInfo;
	LPLISTEN_OBJ listenobj;
	SOCKET acceptSock;
	LPSESSION session;
	LPIO_OBJ receiveObj;

	if (!connectSQL())
		return 1;

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

	listenobj = getListenObj(SERVER_PORT);

	if (listenobj == NULL)
		return 1;

	if (CreateIoCompletionPort((HANDLE)listenobj->sock, gCompletionPort, (ULONG_PTR)&listenobj, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return 1;
	}

	printf("Server started\n");

	while (true) {
		//Accept connections
		if ((acceptSock = WSAAccept(listenobj->sock, NULL, NULL, NULL, 0)) == SOCKET_ERROR) {
			printf("WSAAccept() failed with error %d\n", WSAGetLastError());
			return 1;
		}

		//Create a socket information structure to associate with the socket
		if ((session = getSession()) == NULL)
			continue;

		//Associate the accepted socket with the original completion port
		printf("Socket number %d got connected...\n", acceptSock);
		session->sock = acceptSock;
		if (CreateIoCompletionPort((HANDLE)acceptSock, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
			printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
			return 1;
		}

		receiveObj = getIoObject(IO_OBJ::RECV_C, NULL, BUFFSIZE);
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

		replyObj = getIoObject(IO_OBJ::SEND_C, reply, strlen(reply) + 1);
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

	//file is closed
	if (!session->fileobj) {
		freeIoObject(writeObj);
		LeaveCriticalSection(&(session->cs));
		return;
	}

	session->fileobj->bytesWritten += transferredBytes;

	//have receive all data
	if (session->fileobj->bytesWritten >= session->fileobj->size) {
		freeIoObject(writeObj);
		session->closeFile(FALSE);
	}
	//have write all data
	else if (session->fileobj->bytesRecved >= session->fileobj->size)
		freeIoObject(writeObj);
	//Still have data to receive
	else {
		ZeroMemory(&(writeObj->overlapped), sizeof(OVERLAPPED));

		//Change operation to receive file
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

	EnterCriticalSection(&(session->cs));
	//file is closed
	if (!session->fileobj) {
		freeIoObject(sendObj);
		LeaveCriticalSection(&(session->cs));
		return;
	};
	LeaveCriticalSection(&(session->cs));

	session->fileobj->bytesSended += transferredBytes;
	remain = session->fileobj->size - session->fileobj->bytesSended;

	//have send all data
	if (remain <= 0) {
		freeIoObject(sendObj);
		session->closeFile(FALSE);
		return;
	}

	sendObj->setFileOffset(session->fileobj->bytesSended);

	sendObj->dataBuff.len = min(remain, TRANSMITFILE_MAX);
	session->EnListPendingOperation(sendObj);
}

void handleAccept(_In_ LPLISTEN_OBJ listenobj, _Out_ LPSESSION session, _Inout_ LPIO_OBJ acceptObj) {

}

void ProcessPendingOperations(_In_ LPSESSION session) {
	EnterCriticalSection(&session->cs);
	std::list<LPIO_OBJ>::iterator itr = session->pending->begin();
	LPIO_OBJ ioobj;
	bool noError;
	
	while (itr != session->pending->end()) {
		ioobj = *itr;

		switch (ioobj->operation) {
			case IO_OBJ::RECV_C:
				//Not receiving command while receiving file
				if (session->fileobj && session->fileobj->operation == FILEOBJ::STOR) {
					itr++;
					continue;
				}

				//Receive buffer still have data to process
				if (!(strstr(ioobj->buffer, ENDING_DELIMITER) == NULL)) {
					handleRecieve(session, ioobj, strlen(ioobj->buffer));

					session->pending->pop_front();
					LeaveCriticalSection(&session->cs);
					return;
				}
				else
					noError = PostRecv(session->sock, ioobj);
				break;
			case IO_OBJ::SEND_C:
				//Not send reply while sending file
				noError = PostSend(session->sock, ioobj);
				break;
			case IO_OBJ::RECV_F:
				if (!session->fileobj || !PostRecv(session->sock, ioobj)) {
					noError = FALSE;
					session->closeFile(TRUE);
				}
				else noError = TRUE;
				break;
			case IO_OBJ::WRTE_F:
				if (!session->fileobj || !PostWrite(session->fileobj->file, ioobj)) {
					noError = FALSE;
					session->closeFile(TRUE);
				}
				else noError = TRUE;
				break;
			case IO_OBJ::SEND_F:
				if (!session->fileobj || !PostSendFile(session->sock, session->fileobj->file, ioobj)) {
					noError = FALSE;
					session->closeFile(FALSE);
				}
				else noError = TRUE;
				break;
		}

		if (noError)
			InterlockedIncrement(&session->oustandingOp);
		else {
			freeIoObject(ioobj);
			shutdown(session->sock, SD_BOTH);
			if (CancelIoEx((HANDLE)session->sock, NULL))
				printf("CancelIoEx failed with error %d\n", GetLastError());
			InterlockedExchange(&session->bclosing, 1);
		}

		itr = session->pending->erase(itr);
	}
	LeaveCriticalSection(&session->cs);
}

unsigned __stdcall serverWorkerThread(LPVOID completionPortID) {
	HANDLE completionPort = (HANDLE)completionPortID;
	DWORD transferredBytes, flag;
	LPLISTEN_OBJ listenobj = NULL;
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
			continue;
		}
		
		if (ioobj->operation == IO_OBJ::ACCEPT) {
			listenobj = (LPLISTEN_OBJ) key;

			handleAccept(listenobj, session, ioobj);

			if (InterlockedCompareExchange(&session->bclosing, 0, 0) == 0 && session != NULL)
				ProcessPendingOperations(session);
		}	
		else {
			session = (LPSESSION) key;

			//operation have error or session is closing
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

			//Closing session
			if (InterlockedCompareExchange(&session->oustandingOp, 0, 0) == 0 &&
				InterlockedCompareExchange(&session->bclosing, -1, 1) == 1) {
				freeSession(session);
			}
		}	
	}

}
