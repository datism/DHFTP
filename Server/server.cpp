#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>
#include <sqltypes.h>
#include <process.h>
#include <stdio.h>
#include <vector>
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
CRITICAL_SECTION gCriticalSection;
std::set<ULONG> gSessionSet;
int gInitialAccepts = 100;

unsigned __stdcall serverWorkerThread(LPVOID completionPortID);

int main(int argc, char *argv[]) {
	SYSTEM_INFO systemInfo;
	SOCKET acceptSock;
	WSANETWORKEVENTS sockEvent;
	LPSESSION session;
	LPIO_OBJ receiveObj, acceptobj;
	WSAEVENT waitEvents[WSA_MAXIMUM_WAIT_EVENTS];
	int rc, waitCount = 0;

	if (!connectSQL())
		return 1;

	WSADATA wsaData;
	if (WSAStartup((2, 2), &wsaData) != 0) {
		printf("WSAStartup() failed with error %d\n", GetLastError());
		return 1;
	}

	InitializeCriticalSection(&gCriticalSection);

	//Setup an I/O completion port
	if ((gCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return 1;
	}

	// Determine how many processors are on the system
	GetSystemInfo(&systemInfo);

	//Create worker threads based on the number of processors available on the
	//system. Create two worker threads for each processor	
	for (; waitCount < (int)systemInfo.dwNumberOfProcessors * 2; waitCount++) {
		// Create a server worker thread and pass the completion port to the thread
		waitEvents[waitCount] = (HANDLE) _beginthreadex(0, 0, serverWorkerThread, (void*)gCompletionPort, 0, 0);
		if (waitEvents[waitCount] == INVALID_HANDLE_VALUE) {
			printf("Create thread failed with error %d\n", GetLastError());
			return 1;
		}
	}
	
	//Creat listenobj
	gCmdListen = getListenObj(CMD_PORT);
	gFileListen = getListenObj(FILE_PORT);

	if (gCmdListen == NULL || gFileListen == NULL)
		return 1;

	WSAEventSelect(gCmdListen->sock, gCmdListen->acceptEvent, FD_ACCEPT | FD_CLOSE);

	waitEvents[waitCount++] = gCmdListen->acceptEvent;
	waitEvents[waitCount++] = gFileListen->acceptEvent;

	//Attach listen socket to completion port
	if (CreateIoCompletionPort((HANDLE)gFileListen->sock, gCompletionPort, (ULONG_PTR)gFileListen, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return 1;
	}

	if (CreateIoCompletionPort((HANDLE)gCmdListen->sock, gCompletionPort, (ULONG_PTR)gCmdListen, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		return 1;
	}

	//Post some initial accepctex
	for (int i = 0; i < gInitialAccepts; ++i) {
		acceptobj = getIoObject(IO_OBJ::ACPT_C, NULL, BUFFSIZE);
		if (acceptobj == NULL) {
			printf("Out of memory!\n");
			return -1;
		}

		if (!PostAcceptEx(gCmdListen, acceptobj)) {
			return -1;
		}

		InterlockedIncrement(&gCmdListen->count);
	}

	printf("Server started\n");

	while (1) {
		rc = WSAWaitForMultipleEvents(waitCount, waitEvents, FALSE, WSA_INFINITE, FALSE);
		if (rc == WAIT_FAILED) {
			printf("WSAWaitForMultipleEvents failed: %d\n", WSAGetLastError());
			break;
		}
		else {
			int index;
			index = rc - WAIT_OBJECT_0;
			for (; index < waitCount; index++) {
				rc = WaitForSingleObject(waitEvents[index], 0);
				if (rc == WAIT_FAILED || rc == WAIT_TIMEOUT) {
					continue;
				}
				//shutdown
				if (index < (int)systemInfo.dwNumberOfProcessors) {
					FreeListenObj(gCmdListen);
					FreeListenObj(gFileListen);

					EnterCriticalSection(&gCriticalSection);
					for (ULONG session : gSessionSet)
						freeSession((LPSESSION)session);
					LeaveCriticalSection(&gCriticalSection);

					DeleteCriticalSection(&gCriticalSection);

					WSACleanup();

					ExitProcess(-1);
				}
				else {
					//New cmd connection and no oustanding acceptEx
					if (gCmdListen->acceptEvent == waitEvents[index]) {
						rc = WSAEnumNetworkEvents(gCmdListen->sock, gCmdListen->acceptEvent, &sockEvent);

						if (rc == SOCKET_ERROR)
							printf("WSAEnumNetworkEvents failed: %d\n", WSAGetLastError());
						if (sockEvent.lNetworkEvents & FD_ACCEPT) {
							EnterCriticalSection(&gCriticalSection);
							//Dont accept connection if session count over limit
							if (gSessionSet.size() + gInitialAccepts < MAX_CONCURENT_SESSION) {
								for (int i = 0; i < gInitialAccepts; ++i) {
									acceptobj = getIoObject(IO_OBJ::ACPT_C, NULL, BUFFSIZE);
									if (acceptobj == NULL) {
										printf("Out of memory!\n");
										return -1;
									}
									if (!PostAcceptEx(gCmdListen, acceptobj)) {
										return -1;
									}
									InterlockedIncrement(&gCmdListen->count);
								}
							}
							
							LeaveCriticalSection(&gCriticalSection);
						}
					}
					//Open file connection
					else if (gFileListen->acceptEvent == waitEvents[index]) {
						EnterCriticalSection(&gCriticalSection);
						WSAResetEvent(gFileListen->acceptEvent);

						while (gFileListen->count > 0) {
							acceptobj = getIoObject(IO_OBJ::ACPT_F, NULL, BUFFSIZE);
							if (acceptobj == NULL) {
								printf("Out of memory!\n");
								return -1;
							}

							if (!PostAcceptEx(gFileListen, acceptobj)) {
								return -1;
							}

							gFileListen->count--;
						}

						LeaveCriticalSection(&gCriticalSection);
					}
				}
			}
		}
	}

	return 0;
}

/**
 * @brief hanlde receive operation
 * 
 * @param session 
 * @param recieveObj 
 * @param transferredBytes 
 */
void handleRecieve(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ recieveObj, _In_ DWORD transferredBytes) {
	recieveObj->buffer[transferredBytes] = 0;
	LPIO_OBJ replyObj;
	char *mess = recieveObj->buffer,
		*pos = NULL,
		reply[BUFFSIZE];
	DWORD flags = 0;

	if (transferredBytes == 0) {
		freeIoObject(recieveObj);
		InterlockedExchange(&session->bclosing, 1);
		return;
	}

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

/**
 * @brief handle send operation
 * 
 * @param session 
 * @param sendObj 
 * @param transferredBytes 
 */
void handleSend(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ sendObj, _In_ DWORD transferredBytes) {
	if (transferredBytes == 0) {
		freeIoObject(sendObj);
		InterlockedExchange(&session->bclosing, 1);
		return;
	}

	if (transferredBytes != sendObj->dataBuff.len)
		printf("Internal error?\n");

	freeIoObject(sendObj);
	InterlockedDecrement(&session->outstandingSend);
}

/**
 * @brief handle receive file operation
 * 
 * @param session 
 * @param recvObj 
 * @param transferredBytes 
 */
void handleRecvFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ recvObj, _In_ DWORD transferredBytes) {
	if (transferredBytes == 0) {
		freeIoObject(recvObj);
		session->closeFile(TRUE);
		return;
	}

	//Change operation to write file
	recvObj->operation = IO_OBJ::WRTE_F;
	recvObj->dataBuff.len = transferredBytes;
	session->EnListPendingOperation(recvObj);
}

/**
 * @brief handle write file operation
 * 
 * @param session 
 * @param writeObj 
 * @param transferredBytes 
 */
void handleWriteFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ writeObj, _In_ DWORD transferredBytes) {
	EnterCriticalSection(&(session->cs));

	//file is closed
	if (!session->fileobj) {
		freeIoObject(writeObj);
		LeaveCriticalSection(&(session->cs));
		return;
	}

	if (transferredBytes == 0) {
		freeIoObject(writeObj);
		session->closeFile(TRUE);
		LeaveCriticalSection(&(session->cs));
		return;
	}

	session->fileobj->bytesWritten += transferredBytes;

	//have write all data
	if (session->fileobj->bytesWritten == session->fileobj->size) {
		freeIoObject(writeObj);
		session->closeFile(FALSE);
	}
	//have recv all data
	else if (session->fileobj->bytesRecved >= session->fileobj->size) {
		freeIoObject(writeObj);
	}
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

/**
 * @brief hanlde send file operation
 * 
 * @param session 
 * @param sendObj 
 * @param transferredBytes 
 */
void hanldeSendFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ sendObj, _In_ DWORD transferredBytes) {
	LONG64 remain;

	if (transferredBytes == 0) {
		freeIoObject(sendObj);
		session->closeFile(TRUE);
		return;
	}

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

/**
 * @brief handle accept file operation
 * 
 * @param listenobj 
 * @param session 
 * @param acceptObj 
 * @param transferredBytes 
 */
void handleAcceptFile(_In_ LPLISTEN_OBJ listenobj, _Out_ LPSESSION &session, _Inout_ LPIO_OBJ acceptObj, _In_ DWORD transferredBytes) {
	LPIO_OBJ replyObj = NULL;
	std::vector<std::string> para;
	char reply[BUFFSIZE], cmd[BUFFSIZE] = "";

	printf("File Socket number %d got connected...\n", acceptObj->acceptSock);

	//inherit properties of listen socket 
	if (setsockopt(acceptObj->acceptSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		(char *)&listenobj->sock, sizeof(SOCKET)) == SOCKET_ERROR) {
		printf("setsockopt failed with error %d\n", WSAGetLastError());
		closesocket(acceptObj->acceptSock);
		freeIoObject(acceptObj);
	}

	acceptObj->buffer[transferredBytes] = 0;
	//buffer dont end with ending delimiter
	if (strcmp(acceptObj->buffer + transferredBytes - strlen(ENDING_DELIMITER), ENDING_DELIMITER)) {
		closesocket(acceptObj->acceptSock);
		freeIoObject(acceptObj);
		session = NULL;
		return;
	}
	else {
		acceptObj->buffer[transferredBytes - strlen(ENDING_DELIMITER)] = 0;
		newParseMess(acceptObj->buffer, cmd, para);
	}

	//buffer dont have connect header
	if (strcmp(cmd, CONNECT) || para.size() != 1) {
		closesocket(acceptObj->acceptSock);
		freeIoObject(acceptObj);
		session = NULL;
		return;
	}

	//param is session
	session = (LPSESSION) atol(para[0].c_str());
		
	EnterCriticalSection(&gCriticalSection);

	//session doesnt exsit or have been closed
	if (gSessionSet.find((ULONG)session) == gSessionSet.end()) {
		closesocket(acceptObj->acceptSock);
		freeIoObject(acceptObj);
		session = NULL;
		LeaveCriticalSection(&gCriticalSection);
		return;
	}

	session->fileobj->fileSock = acceptObj->acceptSock;
	freeIoObject(acceptObj);

	//attach file socket to completionport
	if (CreateIoCompletionPort((HANDLE)session->fileobj->fileSock, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());

		initMessage(reply, RESPONE, SERVER_FAIL, "CreateIoCompletionPort failed");
		replyObj = getIoObject(IO_OBJ::SEND_C, reply, strlen(reply) + 1);
		session->EnListPendingOperation(replyObj);
	}

	switch (session->fileobj->operation) {
		//file for retrive
		case FILEOBJ::RETR:
			LPIO_OBJ sendFObj;
		
			sendFObj = getIoObject(IO_OBJ::SEND_F, NULL, 0);
			sendFObj->dataBuff.len = min(session->fileobj->size, TRANSMITFILE_MAX);
			session->EnListPendingOperation(sendFObj);
			break;

		//file for store
		case FILEOBJ::STOR:
			LPIO_OBJ recvFobj;
			int i = 0;

			//Receive and write file in chunks
			while (session->fileobj->bytesRecved < session->fileobj->size && i++ < MAX_IOOBJ_PER_FILEOBJ) {

				recvFobj = getIoObject(IO_OBJ::RECV_F, NULL, BUFFSIZE);
				if (recvFobj == NULL) {
					initMessage(reply, RESPONE, SERVER_FAIL, "Heap out of memory?");
					replyObj = getIoObject(IO_OBJ::SEND_C, reply, strlen(reply) + 1);
					session->EnListPendingOperation(replyObj);
					break;
				}

				recvFobj->setFileOffset(session->fileobj->bytesRecved);
				session->EnListPendingOperation(recvFobj);

				session->fileobj->bytesRecved += recvFobj->dataBuff.len;
			}
	}

	LeaveCriticalSection(&gCriticalSection);
}

/**
 * @brief handle accept cmd operation
 * 
 * @param listenobj 
 * @param session 
 * @param acceptObj 
 * @param transferredBytes 
 */
void handleAcceptCmd(_In_ LPLISTEN_OBJ listenobj, _Out_ LPSESSION &session, _Inout_ LPIO_OBJ acceptObj, _In_ DWORD transferredBytes) {
	InterlockedDecrement(&gCmdListen->count);
	printf("Cmd Socket number %d got connected...\n", acceptObj->acceptSock);

	//inherit properties of listen socket 
	if (setsockopt(acceptObj->acceptSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		(char *)&listenobj->sock, sizeof(SOCKET)) == SOCKET_ERROR) {
		printf("setsockopt failed with error %d\n", WSAGetLastError());
		closesocket(acceptObj->acceptSock);
		freeIoObject(acceptObj);
		session = NULL;
		return;
	}


	session = getSession();
	session->cmdSock = acceptObj->acceptSock;

	//attach new socket to completionport
	if (CreateIoCompletionPort((HANDLE)session->cmdSock, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
		freeIoObject(acceptObj);
		closesocket(session->cmdSock);
		freeSession(session);
		session = NULL;
		return;
	}

	//insert session
	EnterCriticalSection(&gCriticalSection);
	gSessionSet.insert((ULONG)session);
	LeaveCriticalSection(&gCriticalSection);

	//reuse acceptobj
	acceptObj->operation = IO_OBJ::RECV_C;
	handleRecieve(session, acceptObj, transferredBytes);
}

/**
 * @brief process pending operations
 * 
 * @param session 
 */
void ProcessPendingOperations(_In_ LPSESSION session) {
	EnterCriticalSection(&session->cs);
	bool noError;

	while (!session->pending->empty()) {
		LPIO_OBJ ioobj = session->pending->front();

		switch (ioobj->operation) {
		case IO_OBJ::RECV_C:
			//Receive buffer still have data to process
			if (!(strstr(ioobj->buffer, ENDING_DELIMITER) == NULL)) {
				handleRecieve(session, ioobj, strlen(ioobj->buffer));

				session->pending->pop_front();
				LeaveCriticalSection(&session->cs);
				return;
			}
			else
				noError = PostRecv(session->cmdSock, ioobj);

			if (!noError)
				InterlockedExchange(&session->bclosing, 1);

			break;
		case IO_OBJ::SEND_C:
			noError = PostSend(session->cmdSock, ioobj);

			if (!noError)
				InterlockedExchange(&session->bclosing, 1);

			break;
		case IO_OBJ::RECV_F:
			if (!session->fileobj || !PostRecv(session->fileobj->fileSock, ioobj)) {
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
			if (!session->fileobj || !PostSendFile(session->fileobj->fileSock, session->fileobj->file, ioobj)) {
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
	DWORD transferredBytes;
	LPLISTEN_OBJ listen = NULL;
	LPSESSION session = NULL;
	LPIO_OBJ ioobj = NULL;
	ULONG_PTR key = NULL;
	int rc;

	while (true) {
		rc = GetQueuedCompletionStatus(completionPort, &transferredBytes, (PULONG_PTR)&key, (LPOVERLAPPED *)&ioobj, INFINITE);
		if (rc == FALSE) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
			
			//connection failed
			if (ioobj->operation == IO_OBJ::ACPT_F || ioobj->operation == IO_OBJ::ACPT_C) {
				printf("Closing socket %d\n", ioobj->acceptSock);
				if (closesocket(ioobj->acceptSock) == SOCKET_ERROR) {
					printf("closesocket failed with error %d\n", WSAGetLastError());
				}
				freeIoObject(ioobj);
				continue;
			}
			//transmit failed
			else {
				session = (LPSESSION)key;
				if (session != NULL)
					InterlockedExchange(&session->bclosing, 1);
			}
		}
		
		if (ioobj->operation == IO_OBJ::ACPT_F || ioobj->operation == IO_OBJ::ACPT_C) {
			listen = (LPLISTEN_OBJ) key;

			switch (ioobj->operation) {
				case IO_OBJ::ACPT_C: handleAcceptCmd(listen, session, ioobj, transferredBytes); break;
				case IO_OBJ::ACPT_F: handleAcceptFile(listen, session, ioobj, transferredBytes); break;
			}

			if (session != NULL && InterlockedCompareExchange(&session->bclosing, 0, 0) == 0)
				ProcessPendingOperations(session);
		}	
		else {
			session = (LPSESSION) key;

			//session is closing
			if (InterlockedCompareExchange(&session->bclosing, 0, 0) != 0) {
				freeIoObject(ioobj);
			}
			else {
				switch (ioobj->operation) {
					case IO_OBJ::RECV_C: handleRecieve(session, ioobj, transferredBytes); break;
					case IO_OBJ::SEND_C: handleSend(session, ioobj, transferredBytes); break;
					case IO_OBJ::RECV_F: handleRecvFile(session, ioobj, transferredBytes); break;
					case IO_OBJ::SEND_F: hanldeSendFile(session, ioobj, transferredBytes); break;
					case IO_OBJ::WRTE_F: handleWriteFile(session, ioobj, transferredBytes); break;
				}

				ProcessPendingOperations(session);
			}
				
			
			InterlockedDecrement(&session->oustandingOp);

			//Closing session
			if (InterlockedCompareExchange(&session->oustandingOp, 0, 0) == 0 &&
				InterlockedCompareExchange(&session->bclosing, -1, 1) == 1) {
				
				//delete session
				EnterCriticalSection(&gCriticalSection);
				gSessionSet.erase((ULONG)session);
				freeSession(session);
				LeaveCriticalSection(&gCriticalSection);
			}

		}
	}

}
