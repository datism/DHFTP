#pragma once
#include "Session.h"

bool blockSend(_In_ SOCKET sock, _In_ char *sendBuff);
int blockRecv(_In_ SOCKET sock, _In_ char *recvBuff, _In_ DWORD length);
bool sendFile(_Inout_ LpSession session);
bool recvFile(_Inout_ LpSession session);
