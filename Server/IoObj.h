#pragma once

#include <WinSock2.h>
#include "EnvVar.h"

typedef struct {
	WSAOVERLAPPED overlapped;
	WSABUF dataBuff;
	CHAR buffer[BUFFSIZE];
	int operation;
	enum OP {
		RECV_C,
		SEND_C,
		RECV_F
	};

	void setBufferSend(char *i_buffer) {
		strcpy_s(buffer, BUFFSIZE, i_buffer);
		dataBuff.buf = buffer;
		dataBuff.len = strlen(buffer);
	}

	void setBufferRecv(char *i_buffer) {
		strcpy_s(buffer, BUFFSIZE, i_buffer);
		dataBuff.buf = buffer + strlen(buffer);
		dataBuff.len = BUFFSIZE;
	}

} IO_OBJ, *LPIO_OBJ;


LPIO_OBJ getIoObject(IO_OBJ::OP operation);
void freeIoObject(LPIO_OBJ ioobj);