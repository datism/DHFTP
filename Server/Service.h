#pragma once
#include "Session.h"

/**
* @brief handle message form client
*
* @param[in] session
* @param[in] mess message form client
* @param[out] reply reply to client
*/
void handleMess(LPSESSION session, char *mess, char *reply);
void initMessage(_Inout_ char *mess, _In_ const char *header, _In_opt_ const char *p1, _In_opt_ const char *p2);
void initParam(_Inout_ char *param, _In_opt_ const char *p1, _In_opt_ const char *p2);

/**
* @brief parse message into command and paramaters
*
* @param[in] mess message
* @param[out] cmd command
* @param[out] p1 paramater1
* @param[out] p2 paramater2
*/
void parseMess(const char *mess, char *cmd, char *p1, char *p2);

/**
* @brief
*
* @param[in] session
* @param[in] username
* @param[in] password
* @param[out] reply
*/
void handleLOGIN(LPSESSION session, char *username, char *password, char *reply);

/**
* @brief
*
* @param[in] session
* @param[out] reply
*/
void handleLOGOUT(LPSESSION session, char *reply);

/**
* @brief
*
* @param[in] username
* @param[in] password
* @param[out] reply
*/
void handleREGISTER(char *username, char *password, char* reply);

void handleRETRIVE(LPSESSION session, char *filename, char *reply);

void handleRECEIVE(LPSESSION session, char *reply);

void handleSTORE(LPSESSION session, char * filename, LONG64 fileSize, char *reply);

/**
* @brief
*
* @param[in] session
* @param[in] oldname
* @param[in] newname
* @param[out] reply
*/
void handleRENAME(LPSESSION session, char *oldname, char *newname, char *reply);

/**
* @brief
*
* @param[in] session
* @param[in] pathname
* @param[out] reply
*/
void handleDELETE(LPSESSION session, char *pathname, char *reply);

/**
* @brief
*
* @param[in] session
* @param[in] pathname
* @param[out] reply
*/
void handleMAKEDIR(LPSESSION session, char *pathname, char *reply);

/**
* @brief
*
* @param[in] session
* @param[in] pathname
* @param[out] reply
*/
void handleREMOVEDIR(LPSESSION session, char *pathname, char *reply);

/**
* @brief
*
* @param[in] session
* @param[in] pathname
* @param[out] reply
*/
void handleCHANGEWDIR(LPSESSION session, char *pathname, char *reply);

/**
* @brief
*
* @param[in] session
* @param[out] reply
*/
void handlePRINTWDIR(LPSESSION session, char *reply);

/**
* @brief
*
* @param[in] session
* @param[out] pathname
*/
void handleLISTDIR(LPSESSION session, char *pathname, char *reply);
