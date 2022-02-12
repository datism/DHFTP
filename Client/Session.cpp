#include "Session.h"
#include <stdio.h>

void Session::closeFile(BOOL deleteFile) {
	//close file
	if (this->fileobj != NULL) {
		//not delete file if file use for store
		if (this->fileobj->operation == FILEOBJ::RETR) {
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
}

LpSession getSession() {
	LpSession newobj = NULL;

	if ((newobj = (LpSession)GlobalAlloc(GPTR, sizeof(Session))) == NULL)
		printf("GlobalAlloc() failed with error %d\n", GetLastError());

	return newobj;
}

void FreeSession(LpSession session) {
	if (session->fileobj != NULL)
		session->closeFile(TRUE);
	
	printf("Closing cmd socket\n");
	closesocket(session->cmdSock);
	GlobalFree(session);
}
