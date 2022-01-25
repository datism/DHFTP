#pragma once
#include <sal.h>
#include "Session.h"
#include "Envar.h"

void parseReply(const char *reply, char *cmd, char *p1, char *p2);

void chooseService(_Inout_ LpSession session, _Inout_ char *sendBuff);

void handleReply(_Inout_ LpSession session, _In_ const char *reply);

void LoginRequest(_Inout_ char * sendBuff, _In_ const char * username, _In_ const char * password);

void LogoutRequest(_Inout_ char * sendBuff);

void RegisterRequest(_Inout_ char * sendBuff, _In_ const char * username, _In_ const char * password);

bool StoreRequest(_Inout_ LpSession session, _Inout_ char * sendBuff, _In_ const char * fileName);

bool RetrieveRequest(_Inout_ LpSession session, _Inout_ char * sendBuff, _In_ const char * serverFile, _In_ const char * localFile);

void initParam(char *param);

template <typename P>
void initParam(char *param, P p);

template <typename P, typename... Args>
void initParam(char *param, P p, Args... paras);

template <typename... Args>
void initMessage(char *mess, const char *header, Args... paras) {
	char param[BUFFSIZE] = "";

	initParam(param, paras...);

	if (strlen(param) == 0)
		sprintf_s(mess, BUFFSIZE, "%s%s", header, ENDING_DELIMITER);
	else
		sprintf_s(mess, BUFFSIZE, "%s%s%s%s", header, HEADER_DELIMITER, param, ENDING_DELIMITER);
}