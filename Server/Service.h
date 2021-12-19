#pragma once
#include "Session.h"

#define LOGIN "LOGI"
#define LOGOUT "LOGO"
#define REGISTER "REG"
#define RETRIEVE "RETR"
#define STORE "STOR"
#define DONE "DONE"
#define RENAME "RN"
#define DELETE "DEL"
#define MAKEDIR "MKD"
#define REMOVEDIR "RMD"
#define CHANGEWDIR "CWD"
#define PRINTWDIR "PWD"
#define LISTDIR "LIST"
#define RESPONE "RES"

void handleMess(LPSESSION session, char *mess, char *reply);