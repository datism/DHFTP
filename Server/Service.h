#pragma once
#include <sstream>
#include <vector>
#include "Session.h"

/**
 * @brief login request
 * 
 * @param[inout] session 
 * @param[in] username account's username
 * @param[in] password account's password
 * @param[out] reply paramater's buffer
 */
void handleLOGIN(LPSESSION session, const char *username, const char *password, char *reply);

/**
 * @brief change password
 * 
 * @param[in] session 
 * @param[in] oPass new password 
 * @param[in] nPass ol password
 * @param[out] reply paramater's buffer
 */
void handleChangePass(LPSESSION session, const char *oPass, const char *nPass, char * reply);

/**
 * @brief logout
 * 
 * @param[inout] session 
 * @param[out] reply paramater's buffer
 */
void handleLOGOUT(LPSESSION session, char *reply);

/**
 * @brief register new account
 * 
 * @param[in] username account's username
 * @param[in] password account's password
 * @param[out] reply paramater's buffer
 */
void handleREGISTER(const char *username, const char *password, char *reply);

/**
 * @brief retrieve file
 * 
 * @param[inout] session 
 * @param[in] filename file path
 * @param[out] reply paramater's buffer
 */
void handleRETRIVE(LPSESSION session, const char *filename, char *reply);

/**
 * @brief store file
 * 
 * @param[inout] session 
 * @param[in] filename file path
 * @param[in] fileSize file size
 * @param[out] reply paramater's buffer
 */
void handleSTORE(LPSESSION session, const char *filename, const char *fileSize, char *reply);

/**
 * @brief move file/directory
 * 
 * @param[in] session 
 * @param[in] oldpath old file/directory path
 * @param[in] newpath new file/directory path
 * @param[out] reply paramater's buffer
 */
void handleMOVE(LPSESSION session, const char *oldpath, const char *newpath, char *reply);

/**
 * @brief delete file
 * 
 * @param[in] session 
 * @param[in] pathname file path
 * @param[out] reply paramater's buffer
 */
void handleDELETE(LPSESSION session, const char *pathname, char *reply);

/**
 * @brief make new directory
 * 
 * @param[inout] session 
 * @param[in] pathname directory path
 * @param[out] reply paramater's buffer
 */
void handleMAKEDIR(LPSESSION session, const char *pathname, char *reply);

/**
 * @brief remove directory
 * 
 * @param[inout] session 
 * @param[in] pathname directory path
 * @param[out] reply paramater's buffer
 */
void handleREMOVEDIR(LPSESSION session, const char *pathname, char *reply);

/**
 * @brief change working directory
 * 
 * @param[inout] session 
 * @param[in] pathname directory path
 * @param[out] reply paramater's buffer
 */
void handleCHANGEWDIR(LPSESSION session, const char *pathname, char *reply);

/**
 * @brief get current working directory
 * 
 * @param[in] session 
 * @param[out] reply paramater's buffer
 */
void handlePRINTWDIR(LPSESSION session, char *reply);

/**
 * @brief list directory
 * 
 * @param[in] session 
 * @param[in] pathname directory's path
 * @param[out] reply paramater's buffer
 */
void handleLISTDIR(LPSESSION session, const char *pathname, char *reply);

/**
 * @brief handle message from client
 * 
 * @param[inout] session 
 * @param[in] mess message form client
 * @param[out] reply buffer for sending to client
 */
void HandleMess(LPSESSION session, char *mess, char *reply);



/**
 * @brief connect to sql database
 * 
 * @param[db] db database name
 * @param[in] username
 * @param[in] password
 * @return true if connect successfull
 * @return false 
 */
 bool connectSQL(const char * db, const char * username, const char * password);

/**
 * @brief split message
 * 
 * @param[in] mess message
 * @param[out] cmd header
 * @param[out] para list of paramaters
 */
void newParseMess(char *mess, char *cmd, std::vector<std::string>& para);

/**
 * @brief check if session have access to path
 * 
 * @param session 
 * @param path 
 * @param fullPath full path for another operation
 * @return true	if path valid and session have access to path 
 * @return false 
 */
bool checkAccess(LPSESSION session, _In_ const char *path, _Out_ char *fullPath);

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
void initParam(_Out_ char *param, _In_ P p);

/**
 * @brief apending parameters into param according to protocol
 * 
 * @param param 
 * @param p 
 * @param paras 
 */
template <typename P, typename... Args>
void initParam(_Out_ char *param, _In_ P p, _In_opt_ Args... paras);

/**
 * @brief initialize message according to protocol
 *
 * @param mess 
 * @param header 
 * @param paras parameters
 */
template <typename... Args>
void initMessage(_Out_ char *mess, _In_ const char *header, _In_opt_ Args... paras) {
	char param[BUFFSIZE] = "";

	initParam(param, paras...);

	if (strlen(param) == 0)
		//header + ending delimiter
		sprintf_s(mess, BUFFSIZE, "%s%s", header, ENDING_DELIMITER);
	else
		//header + header delimiter + para + para delimiter + para + paradelimiter + ... para + ending delimiter
		sprintf_s(mess, BUFFSIZE, "%s%s%s%s", header, HEADER_DELIMITER, param, ENDING_DELIMITER);
}

