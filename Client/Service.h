#pragma once
#include <sal.h>
#include "Session.h"

void parseReply(const char *reply, char *cmd, char *p1, char *p2);

void chooseService(_Inout_ LpSession session, _Inout_ char *sendBuff);

bool handleReply(_Inout_ LpSession session, _In_ const char *reply);

void LoginRequest(_Inout_ char * sendBuff, _In_ const char * username, _In_ const char * password);

void LogoutRequest(_Inout_ char * sendBuff);

void RegisterRequest(_Inout_ char * sendBuff, _In_ const char * username, _In_ const char * password);

bool StoreRequest(_Inout_ LpSession session, _Inout_ char * sendBuff, _In_ const char * serverFile, _In_ const char * localFile);

bool RetrieveRequest(_Inout_ LpSession session, _Inout_ char * sendBuff, _In_ const char * serverFile, _In_ const char * localFile);

template <typename T, typename X>
void initMessage(_Inout_ char *mess, _In_ const char *header, _In_opt_ const T p1, _In_opt_ const X p2);

template <typename T, typename X>
void initParam(_Inout_ char *param, _In_opt_ const T p1, _In_opt_ const X p2);