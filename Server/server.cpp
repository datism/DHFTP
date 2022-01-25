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
		if (CreateIoCompletionPort((HANDLE)acceptSock, gCompletionPort, (ULONG_PTR)&(session->cmdSock), 0) == NULL) {
			printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());
			return 1;
		}

		receiveObj = getIoObject(IO_OBJ::RECV_C, session, NULL, BUFFSIZE);
		if (receiveObj == NULL) {
			freeSession(session);
			continue;
		}

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
	for (; (pos = strstr(mess, ENDING_DELIMITER)) != NULL; mess = pos + strlen(ENDING_DELIMITER)) {
		*pos = 0;
		handleMess(session, mess, reply);

		if (strlen(reply) == 0)
			continue;

		replyObj = getIoObject(IO_OBJ::SEND_C, session, reply, strlen(reply) + 1);
		if (replyObj == NULL)
			continue;

		session->EnListPendingOperation(replyObj);
	}

	//The remaining buffer which doesnt end with ending delimiter
	recieveObj->setBufferRecv(mess);

	session->EnListPendingOperation(recieveObj);
}

void handleSend(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ sendObj, _In_ DWORD transferredBytes) {
	if (transferredBytes != sendObj->dataBuff.len)
		printf("Internal error?\n");
	freeIoObject(sendObj);
}

void handleRecvFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ recvObj, _In_ DWORD transferredBytes) {
	if (!session->fileobj)
		return;
	//Change operation to write file
	recvObj->operation = IO_OBJ::WRTE_F;
	session->EnListPendingOperation(recvObj);
}

void handleWriteFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ writeObj, _In_ DWORD transferredBytes) {
	LONG64 offset;

	if (!session->fileobj)
		return;

	EnterCriticalSection(&(session->cs));

	session->fileobj->bytestoWrite -= transferredBytes;

	if (session->fileobj->bytestoWrite == 0) {
		freeIoObject(writeObj);
		session->closeFile();
	}
	else if (session->fileobj->bytestoRecv == 0)
		freeIoObject(writeObj);
	else {
		offset = session->fileobj->size - session->fileobj->bytestoRecv;
		ZeroMemory(&(writeObj->overlapped), sizeof(OVERLAPPED));

		writeObj->operation = IO_OBJ::RECV_F;
		writeObj->setFileOffset(offset);
		writeObj->dataBuff.len = min(session->fileobj->bytestoRecv, BUFFSIZE);

		session->EnListPendingOperation(writeObj);

		session->fileobj->bytestoRecv -= writeObj->dataBuff.len;
	}

	LeaveCriticalSection(&(session->cs));
}

void hanldeSendFile(_Inout_ LPSESSION session, _Inout_ LPIO_OBJ sendObj, _In_ DWORD transferredBytes) {
	if (!session->fileobj)
		return;

	session->fileobj->bytestoSend = -transferredBytes;
	if (session->fileobj->bytestoSend == 0) {
		freeIoObject(sendObj);
		session->closeFile();
	}

	LONG64 offset = session->fileobj->size - session->fileobj->bytestoSend;
	sendObj->setFileOffset(offset);

	session->EnListPendingOperation(sendObj);
}

void handleAccpetFile(_In_ LPLISTEN_OBJ listenobj, _Inout_ LPSESSION session, _Inout_ LPIO_OBJ acceptObj) {
	SOCKADDR_STORAGE *LocalSockaddr = NULL, 
		*RemoteSockaddr = NULL;
	LPIO_OBJ replyObj = NULL;
	char reply[BUFFSIZE];
	int LocalSockaddrLen, RemoteSockaddrLen, rc;

	/*listenobj->lpfnGetAcceptExSockaddrs(
		acceptObj->buffer,
		SIZE_OF_ADDRESSES,
		SIZE_OF_ADDRESS,
		SIZE_OF_ADDRESS,
		(SOCKADDR **)&LocalSockaddr,
		&LocalSockaddrLen,
		(SOCKADDR **)&RemoteSockaddr,
		&RemoteSockaddrLen
	);*/

	session->fileSock = acceptObj->acceptSock;
	if (CreateIoCompletionPort((HANDLE)session->fileSock, gCompletionPort, (ULONG_PTR)session, 0) == NULL) {
		printf("CreateIoCompletionPort() failed with error %d\n", GetLastError());

		initMessage(reply, RESPONE, SERVER_FAIL, "CreateIoCompletionPort() failed");
		replyObj = getIoObject(IO_OBJ::SEND_C, session, reply, strlen(reply) + 1);
		session->EnListPendingOperation(replyObj);
	}

	switch (session->fileobj->operation) {
	case FILEOBJ::RETR:
		LPIO_OBJ sendFObj;

		sendFObj = getIoObject(IO_OBJ::SEND_F, session, NULL, 0);
		session->EnListPendingOperation(sendFObj);
		break;
	case FILEOBJ::STOR:
		LPIO_OBJ recvFobj;
		LONG64 offset = 0, remain = 0;
		int i = 0;

		while (offset < session->fileobj->size && i < MAX_IOOBJ_PER_FILEOBJ) {
			remain = session->fileobj->size - offset;

			recvFobj = getIoObject(IO_OBJ::RECV_F, session, NULL, min(remain, BUFFSIZE));
			if (recvFobj == NULL) {
				initMessage(reply, RESPONE, SERVER_FAIL, "Heap out of memory?");
				replyObj = getIoObject(IO_OBJ::SEND_C, session, reply, strlen(reply) + 1);
				session->EnListPendingOperation(replyObj);
				break;
			}

			recvFobj->setFileOffset(offset);
			session->EnListPendingOperation(recvFobj);

			session->fileobj->bytestoRecv -= recvFobj->dataBuff.len;
			offset = ++i * BUFFSIZE;
		}
	}
}

void handleAccpetCommand(_In_ LPLISTEN_OBJ listenobj, _Out_ LPSESSION session, _Inout_ LPIO_OBJ acceptObj) {

}

unsigned __stdcall serverWorkerThread(LPVOID completionPortID) {
	HANDLE completionPort = (HANDLE)completionPortID;
	DWORD transferredBytes;
	LPLISTEN_OBJ listen = NULL;
	LPSESSION session = NULL;
	LPIO_OBJ ioobj = NULL;
	ULONG_PTR key = NULL;

	while (true) {
		if (GetQueuedCompletionStatus(completionPort, &transferredBytes, (PULONG_PTR)&key, (LPOVERLAPPED *)&ioobj, INFINITE) == 0) {
			printf("GetQueuedCompletionStatus() failed with error %d\n", GetLastError());
			continue;
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
		}	
		else {
			session = (LPSESSION) key;

			// Check to see if an error has occurred on the socket and if so
			// then close the socket and cleanup the SOCKET_INFORMATION structure
			// associated with the socket
			if (transferredBytes == 0) {
				freeSession(session);
				freeIoObject(ioobj);
				continue;
			}

			switch (ioobj->operation) {
				case IO_OBJ::RECV_C: handleRecieve(session, ioobj, transferredBytes); break;
				case IO_OBJ::SEND_C: handleSend(session, ioobj, transferredBytes); break;
				case IO_OBJ::RECV_F: handleRecvFile(session, ioobj, transferredBytes); break;
				case IO_OBJ::SEND_F: hanldeSendFile(session, ioobj, transferredBytes); break;
				case IO_OBJ::WRTE_F: handleWriteFile(session, ioobj, transferredBytes); break;
				case IO_OBJ::ACPT_F: handleAccpetFile(listen, ioobj->session, ioobj); break;
			}
		}

		session->ProcessPendingOperations();
	}

	
}
