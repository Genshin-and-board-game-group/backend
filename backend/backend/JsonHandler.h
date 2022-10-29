#include "common.h"
#include "HttpSendRecv.h"
#include "yyjson.h"

BOOL ParseAndDispatchJsonMessage(_In_ PCONNECTION_INFO pConnInfo, _In_ PBYTE pJsonMessage, _In_ ULONG cbMessageLen);

BOOL SendJsonMessage(_In_ PCONNECTION_INFO pConnInfo, _In_ yyjson_mut_doc* JsonDoc);
