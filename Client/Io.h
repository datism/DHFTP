#pragma once
#include "Session.h"

/**
 * @brief  wrapper for block wsasend
 * 
 * @param sock 
 * @param sendBuff send buffer
 * @return true if wsasend doesnt fail 
 * @return false if wsasend fail 
 */
bool blockSend(_In_ SOCKET sock, _In_ char *sendBuff);

/**
 * @brief wrapper for block wsarecv
 * 
 * @param sock 
 * @param recvBuff receive buffer
 * @param length number of bytes want to receive
 * @return number of bytes receive
 */
int blockRecv(_In_ SOCKET sock, _In_ char *recvBuff, _In_ DWORD length);

/**
 * @brief wrapper for block transmitFile
 * 
 * @param sock file socket
 * @param hfile file handle 
 * @param size file size
 * @return true if transmitFile doesnt fail 
 * @return false if transmitFile fail
 */
bool sendFile(SOCKET sock, HANDLE hfile, LONG64 size);

/**
 * @brief block wsarecv and  block writefile
 * 
 * @param sock file socket
 * @param hfile file handle 
 * @param size file size
 * @return true doesnt fail
 * @return false fail
 */
bool recvFile(SOCKET sock, HANDLE hfile, LONG64 size);
