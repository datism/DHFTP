#pragma once

#include <WinSock2.h>
#include <fileapi.h>
#include "EnvVar.h"

typedef struct IO_OBJ {
	WSAOVERLAPPED overlapped;
	WSABUF dataBuff;

	_Field_z_
	CHAR buffer[BUFFSIZE];

	_Field_range_(0, 2)
	int operation;
	enum OP {
		RECV_C,
		SEND_C,
		RECV_F,
		WRTE_F
	};

	HANDLE file;

	void setBufferSend(_In_z_ char *i_buffer);

	void setBufferRecv(_In_z_ char *i_buffer);

	bool setFile(_In_z_ char *fileName) {
		this->file = CreateFileA(fileName, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (this->file == INVALID_HANDLE_VALUE)
			return false;
		return true;
	}

} IO_OBJ, *LPIO_OBJ;


_Ret_maybenull_ LPIO_OBJ getIoObject(_In_opt_ IO_OBJ::OP operation);
void freeIoObject(_In_ LPIO_OBJ ioobj);