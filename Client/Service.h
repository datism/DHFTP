#pragma once
#include <sal.h>
#include <vector>
#include <string>
#include "Session.h"
#include "Envar.h"

/**
 * @brief Request login service from server
 * 
 * @param[out] sendBuff buffer for sending to server
 * @param[in] username account's username
 * @param[in] password account's password
 */
void LoginRequest(_Inout_ char * sendBuff, _In_ const char * username, _In_ const char * password);

/**
 * @brief Request logout service from server
 * 
 * @param[out] sendBuff buffer for sending to server
 */
void LogoutRequest(_Inout_ char * sendBuff);

/**
 * @brief Request register service from server
 * 
 * @param[out] sendBuff buffer for sending to server
 * @param[in] username account's username
 * @param[in] password account's password
 */
void RegisterRequest(_Inout_ char * sendBuff, _In_ const char * username, _In_ const char * password);

/**
 * @brief Request change password service from server
 * 
 * @param[out] sendBuff buffer for sending to server
 * @param[in] oPassword account's old password
 * @param[in] nPassword account's new password
 */
void ChangePasRequest(char * sendBuff, const char * oPassword, const char * nPassword);

/**
 * @brief Request store service from server
 * 
 * @param[inout] session 
 * @param[out] sendBuff buffer for sending to server
 * @param[in] localFile file want to store 
 * @param[in] remoteFile server's file path
 * @return true if can find local file
 * @return false 
 */
bool StoreRequest(LpSession session, char * sendBuff, const char * localFile, const char * remoteFile);

/**
 * @brief Request retrieve service from server
 * 
 * @param[inout] session 
 * @param[out] sendBuff buffer for sending to server
 * @param[in] localFile local file to retrieve data
 * @param[in] remoteFile server's file path
 * @return true if can creat local file
 * @return false 
 */
bool RetrieveRequest(LpSession session, char * sendBuff, const char * localFile, const char * remoteFile);

/**
 * @brief Request move service from server
 * 
 * @param[out] sendBuf buffer for sending to server
 * @param[in] oldPath old file/folder path
 * @param[in] newPath new file/folder path
 */
void MoveRequest(char * sendBuf, const char * oldPath, const char * newPath);

/**
 * @brief Request delete file service from server
 * 
 * @param[out] sendBuf buffer for sending to server
 * @param[in] serverFile server file path
 */
void DeleteRequest(char *sendBuf, const char *serverFile);

/**
 * @brief Request make directory service from server
 * 
 * @param[out] sendBuf buffer for sending to server
 * @param[in] dirPath directory path
 */
void MakeDirRequest(char *sendBuf, const char *dirPath);

/**
 * @brief Request remove directory service from server
 * 
 * @param[out] sendBuf buffer for sending to server
 * @param[in] dirPath directory path
 */
void RemoveDirRequest(char *sendBuf, const char *dirPath);

/**
 * @brief Request change working directory service from server
 * 
 * @param[out] sendBuf buffer for sending to server
 * @param[in] dirPath directory path
 */
void ChangeWDirRequest(char *sendBuf, const char *dirPath);

/**
 * @brief Request print working directory service from server
 * 
 * @param[out] sendBuf buffer for sending to server
 */
void PrintWDirRequest(char *sendBuf);

/**
 * @brief Request list working directory service from server
 * 
 * @param[out] sendBuf buffer for sending to server
 * @param[in] dirPath directory path
 */
void ListDirRequest(char *sendBuf, const char *dirPath);

/**
 * @brief 
 * 
 * @param[inout] session 
 * @param[out] sendBuff buffer for sending to server
 */
void ChooseService(_Inout_ LpSession session, _Inout_ char *sendBuff);

/**
 * @brief 
 * 
 * @param[inout] session 
 * @param[in] reply reply from server
 */
void HandleReply(_Inout_ LpSession session, _In_ const char *reply);

/**
 * @brief list of service
 * 
 */
void Help();

/**
 * @brief split message
 * 
 * @param[in] mess message
 * @param[out] cmd header
 * @param[out] para list of paramaters
 */
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