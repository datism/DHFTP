#pragma once
#include <WinSock2.h>
#include "ListenObj.h"
#include "FileObj.h"

typedef struct IO_OBJ {
	WSAOVERLAPPED overlapped;

	//for recv, send and write
	WSABUF dataBuff;
	_Field_z_
	CHAR *buffer;

	//for accept
	SOCKET acceptSock;

	int operation;
	enum OP {
		//receive command
		RECV_C,
		//send command
		SEND_C,
		//accept new command connection
		ACPT_C,
		//receive file
		RECV_F,
		//send file
		SEND_F,
		//write file
		WRTE_F,
		//accept new file connection
		ACPT_F
	};

	void setBufferSend(_In_z_ char *i_buffer);
	void setBufferRecv(_In_z_ char *i_buffer);
	void setFileOffset(_In_ LONG64 fileOffset);
} IO_OBJ, *LPIO_OBJ;


/**
 * @brief Get IO_OBJ
 * 
 * @param operation ioobj operaion
 * @param buffer if not null copy to buffer
 * @param length length of buffer (include '/0')
 * @return new ioobj or null if dont have enough memory
 */
_Ret_maybenull_ LPIO_OBJ getIoObject(_In_ IO_OBJ::OP operation, _In_opt_ char *buffer, _In_ DWORD length);

/**
 * @brief Free IO_OBJ
 * 
 * @param ioobj 
 */
void freeIoObject(_In_ LPIO_OBJ ioobj);

/**
 * @brief wrapper for overlapped wsasend
 * 
 * @param sock 
 * @param sendObj send obj
 * @return true if wsasend doesnt fail 
 * @return false if wsasend fail 
 */
bool PostSend(_In_ SOCKET sock, _In_ LPIO_OBJ sendObj);

/**
 * @brief wrapper for overlapped wsarecv
 * 
 * @param sock 
 * @param recvObj receive obj
 * @return true if wsarecv doesnt fail 
 * @return false if wsarecv fail
 */
bool PostRecv(_In_ SOCKET sock, _In_ LPIO_OBJ recvObj);

/**
 * @brief wrapper for overlapped writefile
 * 
 * @param hfile file handle 
 * @param writeObj write obj
 * @return true if writefile doesnt fail 
 * @return false if writefile fail
 */
bool PostWrite(_In_ HANDLE hfile, _In_ LPIO_OBJ writeObj);

/**
 * @brief wrapper for overlapped transmitFile
 * 
 * @param sock 	file socket
 * @param hfile file handle 
 * @param sendFObj send file obj
 * @return true if transmitFile doesnt fail 
 * @return false if transmitFile fail
 */
bool PostSendFile(_In_ SOCKET sock, _In_ HANDLE hfile, _In_ LPIO_OBJ sendFObj);

/**
 * @brief wrapper for overlapped acceptEx
 * 
 * @param listen 
 * @param acceptobj accept obj
 * @return true if acceptEx doesnt fail 
 * @return false if acceptEx fail
 */
bool PostAcceptEx(_In_ LPLISTEN_OBJ listen, LPIO_OBJ acceptobj);