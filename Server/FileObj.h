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

	volatile LONG64 bytesSended;
	volatile LONG64 bytesRecved;
	volatile LONG64 bytesWritten;

} FILEOBJ, *LPFILEOBJ;

_Ret_maybenull_ LPFILEOBJ GetFileObj(_In_ HANDLE hfile, _In_ LONG64 size, _In_ FILEOBJ::OP op);
void FreeFileObj(_In_ LPFILEOBJ fileobj);

