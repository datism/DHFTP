#pragma once
#include <WinSock2.h>

typedef struct FILEOBJ {
	HANDLE file;
	LONG64 size;

	int operation;
	enum OP {
		RETR,
		STOR
	};

	volatile LONG64 bytestoSend;
	volatile LONG64 bytestoRecv;
	volatile LONG64 bytestoWrite;

} FILEOBJ, *LPFILEOBJ;

_Ret_maybenull_ LPFILEOBJ GetFileObj(_In_ HANDLE hfile, _In_ LONG64 size, _In_ FILEOBJ::OP op);
void FreeFileObj(_In_ LPFILEOBJ fileobj);

