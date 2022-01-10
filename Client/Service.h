#pragma once
#include <sal.h>
#include "Session.h"

void chooseService(_Inout_ LpSession session, _Inout_ char *sendBuff);
bool handleReply(_Inout_ LpSession session,_In_z_ const char *reply);

void initMessage(_Inout_ char *mess, _In_ const char *header, _In_opt_ const char *p1, _In_opt_ const char *p2);
void initParam(_Inout_ char *param, _In_opt_ const char *p1, _In_opt_ const char *p2);