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

void SESSION::closeFile(BOOL deleteFile) {
	EnterCriticalSection(&this->cs);
	//close file
	if (this->fileobj != NULL) {
		//not delete file if file use for retrieve
		if (this->fileobj->operation == FILEOBJ::STOR) {
			//mark file for delete after closehandle
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
		session->pending = new std::list<LPIO_OBJ>();
	}

	return session;
}

void freeSession(LPSESSION session) {
	//logout
	if (strlen(session->username) != 0) {
		char s[BUFFSIZE]= "";
		handleLOGOUT(session, s);
	}

	//close file
	session->closeFile(TRUE);

	//close connection
	printf("Closing socket %d\n", session->sock);
	if (closesocket(session->sock) == SOCKET_ERROR) {
		printf("closesocket failed with error %d\n", WSAGetLastError());
	}

	//free ioobj in pending list
	for (LPIO_OBJ ioobj : *session->pending)
		freeIoObject(ioobj);
	free(session->pending);

	DeleteCriticalSection(&(session->cs));
	HeapFree(GetProcessHeap(), NULL, session);
}


