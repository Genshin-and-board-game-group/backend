#pragma once
#include "common.h"
#include "HttpSendRecv.h"
#include "yyjson.h"

BOOL HandleCreateRoom(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandleJoinRoom(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandleChangeAvatar(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandleLeaveRoom(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);
