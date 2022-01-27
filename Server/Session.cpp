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

//void SESSION::ProcessPendingOperations() {
//	EnterCriticalSection(&this->cs);
//	bool noError;
//
//	while (!this->pending->empty()) {
//		LPIO_OBJ ioobj = this->pending->front();
//
//		switch (ioobj->operation) {
//		case IO_OBJ::RECV_C:
//			noError = PostRecv(this->cmdSock, ioobj);
//			break;
//		case IO_OBJ::SEND_C:
//			noError = PostSend(this->cmdSock, ioobj);
//			break;
//		case IO_OBJ::RECV_F:
//			if (!this->fileobj || this->fileSock == INVALID_SOCKET || !PostRecv(this->fileSock, ioobj)) {
//				noError = FALSE;
//				this->closeFile(TRUE);
//			}
//			else noError = TRUE;
//			break;
//		case IO_OBJ::WRTE_F:
//			if (!this->fileobj || this->fileSock == INVALID_SOCKET || !PostWrite(this->fileobj->file, ioobj)) {
//				noError = FALSE;
//				this->closeFile(TRUE);
//			}
//			else noError = TRUE;
//			break;
//		case IO_OBJ::SEND_F:
//			if (!this->fileobj || this->fileSock == INVALID_SOCKET || !PostSendFile(this->fileSock, this->fileobj->file, ioobj)) {
//				noError = FALSE;
//				this->closeFile(FALSE);
//			}
//			else noError = TRUE;
//			break;
//		}
//
//		if (noError)
//			InterlockedIncrement(&this->oustandingOp);
//		else 
//			freeIoObject(ioobj);
//
//		this->pending->pop_front();
//	}
//	LeaveCriticalSection(&this->cs);
//}

void SESSION::closeFile(BOOL deleteFile) {
	EnterCriticalSection(&this->cs);

	if (this->fileSock != INVALID_SOCKET) {
		/*if (shutdown(this->fileSock, SD_BOTH) == SOCKET_ERROR) {
			printf("shutdown failed with error %d\n", WSAGetLastError());
		}
		if (CancelIoEx((HANDLE)this->fileSock, NULL)) {
			printf("CancelIoEx failed with error %d\n", WSAGetLastError());
		}*/

		if (closesocket(this->fileSock) == SOCKET_ERROR) {
			printf("closesocket failed with error %d\n", WSAGetLastError());
		}
		this->fileSock = INVALID_SOCKET;
	}

	if (this->fileobj != NULL) {
		if (!(this->fileobj->operation == FILEOBJ::STOR)) {
			/*if (CancelIoEx(this->fileobj->file, NULL)) {
				printf("CancelIoEx failed with error %d\n", WSAGetLastError());
			}*/

			FILE_DISPOSITION_INFO fdi;
			fdi.DeleteFile = deleteFile;
			if (!SetFileInformationByHandle(this->fileobj->file,
				FileDispositionInfo, &fdi, sizeof(FILE_DISPOSITION_INFO))) {
				printf("SetFileInformationByHandle failed with error %d\n", GetLastError());
			}
		}

		FreeFileObj(this->fileobj);
		fileobj = NULL;
	}

	LeaveCriticalSection(&this->cs);
}

LPSESSION getSession() {
	LPSESSION session;

	if ((session = (LPSESSION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SESSION))) == NULL)
		printf("HeapAlloc failed with error %d\n", GetLastError());

	if (session) {
		InitializeCriticalSection(&(session->cs));
		session->fileSock = INVALID_SOCKET;
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

	session->closeFile(TRUE);
	for (LPIO_OBJ ioobj : *session->pending)
		freeIoObject(ioobj);
	free(session->pending);
	DeleteCriticalSection(&(session->cs));
	HeapFree(GetProcessHeap(), NULL, session);
}


