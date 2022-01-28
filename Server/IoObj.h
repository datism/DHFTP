#pragma once
#include <WinSock2.h>
#include "ListenObj.h"

typedef struct SESSION *LPSESSION;


typedef struct IO_OBJ {
	WSAOVERLAPPED overlapped;

	//for recv, send and write
	WSABUF dataBuff;
	_Field_z_
	CHAR *buffer;

	//for accept
	SOCKET acceptSock;
	LPSESSION session;

	int operation;
	enum OP {
		RECV_C,
		SEND_C,
		ACPT_C,
		RECV_F,
		SEND_F,
		WRTE_F,
		ACPT_F,
	};

	void setBufferSend(_In_z_ char *i_buffer);
	void setBufferRecv(_In_z_ char *i_buffer);
	void setFileOffset(_In_ LONG64 fileOffset);
} IO_OBJ, *LPIO_OBJ;


_Ret_maybenull_ LPIO_OBJ getIoObject(_In_ IO_OBJ::OP operation, _In_opt_ LPSESSION session, _In_opt_ char *buffer, _In_ DWORD length);
void freeIoObject(_In_ LPIO_OBJ ioobj);

bool PostSend(_In_ SOCKET sock, _In_ LPIO_OBJ sendObj);
bool PostRecv(_In_ SOCKET sock, _In_ LPIO_OBJ recvObj);
bool PostWrite(_In_ HANDLE hfile, _In_ LPIO_OBJ writeObj);
bool PostSendFile(_In_ SOCKET sock, _In_ HANDLE hfile, _In_ LPIO_OBJ sendFObj);
bool PostAcceptEx(_In_ LPLISTEN_OBJ listen, LPIO_OBJ acceptobj);