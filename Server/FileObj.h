#pragma once
#include <WinSock2.h>

typedef struct FILEOBJ {
	HANDLE file;
	volatile LONG64 size;

} FILEOBJ, *LPFILEOBJ;

_Ret_maybenull_ LPFILEOBJ GetFileObj(_In_ HANDLE hfile, _In_ LONG64 size);
void FreeFileObj(_In_ LPFILEOBJ fileobj);

