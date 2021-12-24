#pragma once

#include <WinSock2.h>
#include "EnvVar.h"

typedef struct {
	WSAOVERLAPPED overlapped;
	WSABUF dataBuff;

	_Field_z_
	CHAR buffer[BUFFSIZE];

	_Field_range_(0, 2)
	int operation;
	enum OP {
		RECV_C,
		SEND_C,
		RECV_F
	};

	void setBufferSend(_In_z_ char *i_buffer);

	void setBufferRecv(_In_z_ char *i_buffer) {
		int ret = strcpy_s(buffer, BUFFSIZE, i_buffer);
		dataBuff.buf = buffer + strlen(buffer);
		dataBuff.len = BUFFSIZE;
	}

} IO_OBJ, *LPIO_OBJ;


_Ret_maybenull_ LPIO_OBJ getIoObject(_In_opt_ IO_OBJ::OP operation);
void freeIoObject(_In_ LPIO_OBJ ioobj);