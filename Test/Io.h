#pragma once
#include "Session.h"

bool blockSend(_In_ SOCKET sock, _In_ char *sendBuff);
int blockRecv(_In_ SOCKET sock, _In_ char *recvBuff, _In_ DWORD length);
bool sendFile(SOCKET sock, HANDLE hfile, LONG64 size);
bool recvFile(SOCKET sock, HANDLE hfile, LONG64 size);
