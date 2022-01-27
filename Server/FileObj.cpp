#include "FileObj.h"
#include "Session.h"
#include <stdio.h>

LPFILEOBJ GetFileObj(HANDLE hfile, LONG64 size, FILEOBJ::OP op) {
	LPFILEOBJ newobj = NULL;

	if ((newobj = (LPFILEOBJ)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FILEOBJ))) == NULL)
		printf("HeapAlloc() failed with error %d\n", GetLastError());

	if (newobj) {
		newobj->file = hfile;
		newobj->size = size;
		newobj->operation = op;
	}

	return newobj;
}

void FreeFileObj(LPFILEOBJ fileobj) {
	CloseHandle(fileobj->file);
	HeapFree(GetProcessHeap(), NULL, fileobj);
}


