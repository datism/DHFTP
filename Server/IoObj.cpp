#include "IoObj.h"
#include <stdio.h>

LPIO_OBJ getIoObject(IO_OBJ::OP operation) {
	LPIO_OBJ ioObj;
	if ((ioObj = (LPIO_OBJ)GlobalAlloc(GPTR, sizeof(IO_OBJ))) == NULL) {
		printf("GlobalAlloc() failed with error %d\n", GetLastError());
		return NULL;
	}
	ZeroMemory(&(ioObj->overlapped), sizeof(OVERLAPPED));
	ioObj->length = 0;
	ioObj->dataBuff.len = BUFFSIZE;
	ioObj->dataBuff.buf = ioObj->buffer;
	ioObj->operation = operation;
	return ioObj;
}

void freeIoObject(LPIO_OBJ ioobj) {
	GlobalFree(ioobj);
}
