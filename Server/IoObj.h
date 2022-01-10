#pragma once
#include <WinSock2.h>

typedef struct IO_OBJ {
	WSAOVERLAPPED overlapped;
	WSABUF dataBuff;

	_Field_z_
	CHAR *buffer;

	int operation;
	enum OP {
		RECV_C,
		SEND_C,
		RECV_F,
		SEND_F,
		WRTE_F
	};

	int sequence;

	void setBufferSend(_In_z_ char *i_buffer);

	void setBufferRecv(_In_z_ char *i_buffer);
} IO_OBJ, *LPIO_OBJ;


_Ret_maybenull_ LPIO_OBJ getIoObject(_In_ IO_OBJ::OP operation, _In_opt_ char *buffer, _In_ DWORD length);
void freeIoObject(_In_ LPIO_OBJ ioobj);

bool PostSend(_In_ SOCKET sock, _In_ LPIO_OBJ sendObj);
bool PostRecv(_In_ SOCKET sock, _In_ LPIO_OBJ recvObj);
bool PostWrite(_In_ HANDLE hfile, _In_ LPIO_OBJ writeObj);