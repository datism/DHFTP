#pragma once
#include <sal.h>
#include <vector>
#include <string>
#include "Session.h"
#include "Envar.h"

void LoginRequest(_Inout_ char * sendBuff, _In_ const char * username, _In_ const char * password);

void LogoutRequest(_Inout_ char * sendBuff);

void RegisterRequest(_Inout_ char * sendBuff, _In_ const char * username, _In_ const char * password);

bool StoreRequest(_Inout_ LpSession session, _Inout_ char * sendBuff, _In_ const char * fileName);

bool RetrieveRequest(_Inout_ LpSession session, _Inout_ char * sendBuff, _In_ const char * fileName);

void chooseService(_Inout_ LpSession session, _Inout_ char *sendBuff);

void handleReply(_Inout_ LpSession session, _In_ const char *reply);

void newParseReply(const char *reply, char * cmd, std::vector<std::string>& para);

/**
 * @brief overloard for empty parameter
 * 
 * @param param 
 */
void initParam(_Out_ char *param);

/**
 * @brief overloard for base case
 * 
 * @param param 
 * @param p 
 */
template <typename P>
void initParam(_Out_ char *param, P p);

/**
 * @brief apending parameters into param according to protocol
 * 
 * @param param 
 * @param p 
 * @param paras 
 */
template <typename P, typename... Args>
void initParam(_Out_ char *param, _In_ P p, _In_ Args... paras);

/**
 * @brief initialize message according to protocol
 *
 * @param mess 
 * @param header 
 * @param paras parameters
 */
template <typename... Args>
void initMessage(char *mess, const char *header, Args... paras) {
	char param[BUFFSIZE] = "";

	initParam(param, paras...);

	if (strlen(param) == 0)
		//header + ending delimiter
		sprintf_s(mess, BUFFSIZE, "%s%s", header, ENDING_DELIMITER);
	else
		//header + header delimiter + para + para delimiter + para + paradelimiter + ... para + ending delimiter
		sprintf_s(mess, BUFFSIZE, "%s%s%s%s", header, HEADER_DELIMITER, param, ENDING_DELIMITER);
}