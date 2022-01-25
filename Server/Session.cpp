#include "Session.h"
#include <stdio.h>
#include "Service.h"

void SESSION::setUsername(const char * iUsername) {
	strcpy_s(this->username, MAX_PATH, iUsername);
}

void SESSION::setWorkingDir(const char * iWorkingDir) {
	strcpy_s(this->workingDir, MAX_PATH, iWorkingDir);
}

void SESSION::EnListPendingOperation(LPIO_OBJ ioObj){
	EnterCriticalSection(&this->cs);
	this->pending->push_back(ioObj);
	LeaveCriticalSection(&this->cs);
}


void SESSION::ProcessPendingOperations() {
	EnterCriticalSection(&this->cs);
	while (!this->pending->empty()) {
		LPIO_OBJ ioobj = this->pending->front();

		switch (ioobj->operation) {
		case IO_OBJ::RECV_C:
			if (!PostRecv(this->cmdSock, ioobj))
				freeIoObject(ioobj);
			break;
		case IO_OBJ::SEND_C:
			if (!PostSend(this->cmdSock, ioobj))
				freeIoObject(ioobj);
			break;
		case IO_OBJ::RECV_F:
			if (!this->fileobj || this->fileSock == INVALID_SOCKET || !PostRecv(this->fileSock, ioobj))
				freeIoObject(ioobj);
			break;
		case IO_OBJ::WRTE_F:
			if (!this->fileobj || this->fileSock == INVALID_SOCKET || !PostWrite(this->fileobj->file, ioobj))
				freeIoObject(ioobj);
			break;
		case IO_OBJ::SEND_F:
			if (!this->fileobj || this->fileSock == INVALID_SOCKET || !PostSendFile(this->cmdSock, this->fileobj, ioobj))
				freeIoObject(ioobj);
			break;
		}
		this->pending->pop_front();
	}
	LeaveCriticalSection(&this->cs);
}

void SESSION::closeFile() {
	if (this->fileobj != NULL)
		FreeFileObj(this->fileobj);
	fileobj = NULL;
}

LPSESSION getSession() {
	LPSESSION session;

	if ((session = (LPSESSION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SESSION))) == NULL)
		printf("aligned_malloc() failed with error %d\n", GetLastError());

	if (session) {
		InitializeCriticalSection(&(session->cs));
		session->pending = new std::list<LPIO_OBJ>();
	}

	return session;
}

void freeSession(LPSESSION session) {
	printf("Closing socket %d\n", session->cmdSock);
	if (closesocket(session->cmdSock) == SOCKET_ERROR) {
		printf("closesocket failed with error %d\n", WSAGetLastError());
	}

	if (strlen(session->username) != 0) {
		char s[BUFFSIZE];
		handleLOGOUT(session, s);
	}

	DeleteCriticalSection(&(session->cs));
	session->closeFile();

	HeapFree(GetProcessHeap(), NULL, session);
}


