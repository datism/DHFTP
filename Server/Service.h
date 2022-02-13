#pragma once
#include <sstream>
#include <vector>
#include "Session.h"


void handleLOGIN(LPSESSION session, const char *username, const char *password, char *reply);

void changePass(LPSESSION session, char *reply);

void handleLOGOUT(LPSESSION session, char *reply);

void handleREGISTER(const char *username, const char *password, char *reply);

void handleRETRIVE(LPSESSION session, const char *filename, char *reply);

void handleSTORE(LPSESSION session, const char *filename, const char *fileSize, char *reply);

void handleRENAME(LPSESSION session, const char *pathname, const char *newname, char *reply);

void handleDELETE(LPSESSION session, const char *pathname, char *reply);

void handleMAKEDIR(LPSESSION session, const char *pathname, char *reply);

void handleREMOVEDIR(LPSESSION session, const char *pathname, char *reply);

void handleCHANGEWDIR(LPSESSION session, const char *pathname, char *reply);

void handlePRINTWDIR(LPSESSION session, char *reply);

void handleLISTDIR(LPSESSION session, const char *pathname, char *reply);

void handleMess(LPSESSION session, char *mess, char *reply);

bool connectSQL();

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
* @brief check if file's name is valid
*
* @param[in] name
* @retun true if name's valid
*/
bool checkName(const char *name);

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

