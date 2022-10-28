#include "common.h"
#include "HttpSendRecv.h"

BOOL ParseAndDispatchJsonMessage(_In_ PCONNECTION_INFO pConnInfo, _In_ PBYTE pJsonMessage, _In_ ULONG cbMessageLen);
