#include "IoObj.h"
#include <stdio.h>


_Ret_maybenull_ LPIO_OBJ getIoObject(_In_opt_ IO_OBJ::OP operation) {
	LPIO_OBJ ioObj;
	if (operation == NULL)
		operation = IO_OBJ::RECV_C;

	if ((ioObj = (LPIO_OBJ)GlobalAlloc(GPTR, sizeof(IO_OBJ))) == NULL) {
		printf("GlobalAlloc() failed with error %d\n", GetLastError());
		return NULL;
	}

	ZeroMemory(&(ioObj->overlapped), sizeof(OVERLAPPED));
	ioObj->dataBuff.len = BUFFSIZE;
	ioObj->dataBuff.buf = ioObj->buffer;
	ioObj->operation = operation;
	return ioObj;
}

void freeIoObject(_In_ LPIO_OBJ ioobj) {
	GlobalFree(ioobj);
}

void IO_OBJ::setBufferSend(_In_z_ char *i_buffer) {
	strcpy_s(this->buffer, BUFFSIZE, i_buffer);
	this->dataBuff.buf = this->buffer;
	this->dataBuff.len = strlen(this->buffer);
}

void IO_OBJ::setBufferRecv(_In_z_ char *i_buffer) {
	int ret = strcpy_s(this->buffer, BUFFSIZE, i_buffer);
	this->dataBuff.buf = this->buffer + strlen(buffer);
	this->dataBuff.len = BUFFSIZE;
}
