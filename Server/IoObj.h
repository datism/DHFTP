#pragma once
#include <WinSock2.h>
#include "EnvVar.h"

typedef struct {
	WSAOVERLAPPED overlapped;
	WSABUF dataBuff;
	CHAR buffer[BUFFSIZE];
	int length;
	int operation;
	enum OP {
		RECV_C,
		SEND_C,
		RECV_F
	};

	void setBuffer(char *i_buffer) {
		strcpy_s(buffer, BUFFSIZE, i_buffer);
		length = strlen(buffer);
		dataBuff.len = length;
	}

} IO_OBJ, *LPIO_OBJ;


LPIO_OBJ getIoObject(IO_OBJ::OP operation);
void freeIoObject(LPIO_OBJ ioobj);