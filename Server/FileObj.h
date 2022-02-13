#pragma once
#include <WinSock2.h>

/**
 * @brief use for transmitting file
 */
typedef struct FILEOBJ {
	//file connection
	SOCKET fileSock;
	//file handle
	HANDLE file;

	int operation;
	enum OP {
		//retrieve file
		RETR,
		//store file
		STOR
	};

	LONG64 size;

	//for overlapped operation
	volatile LONG64 bytesSended;
	volatile LONG64 bytesRecved;
	volatile LONG64 bytesWritten;

} FILEOBJ, *LPFILEOBJ;

/**
 * @brief Get the File object
 * 
 * @param hfile file handle 
 * @param size size of file
 * @param op opration of fileobj
 * @return new file obj or NULL if dont have enough memory
 */
_Ret_maybenull_ LPFILEOBJ GetFileObj(_In_ HANDLE hfile, _In_ LONG64 size, _In_ FILEOBJ::OP op);

/**
 * @brief Free file obj
 * 
 * @param fileobj 
 */
void FreeFileObj(_In_ LPFILEOBJ fileobj);

