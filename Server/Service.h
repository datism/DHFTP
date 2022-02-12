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
* @brief check the access of current user with the path
*
* @param[in] session
* @param[in, out] path
*          full path if return true
*          "" if return false
* @return true if user have access to path
* @return false else
*/
bool checkAccess(LPSESSION session,const char *path, char *fullPath);

bool connectSQL();

/**
* @brief check if file's name is valid
*
* @param[in] name
* @retun true if name's valide
*/
bool checkName(const char *name);

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

