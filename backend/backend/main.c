#include "common.h"
#include "HttpSendRecv.h"
#include "RoomManager.h"
#include <locale.h>

#pragma comment(lib, "httpapi.lib")
#pragma comment(lib, "Websocket.lib")

// The number of requests for queueing, when
#define OUTSTANDING_REQUESTS 8
// The number of requests per processor
#define REQUESTS_PER_PROCESSOR 2

DWORD GetRequestCount()
{
    DWORD_PTR dwProcessAffinityMask, dwSystemAffinityMask;
    WORD wRequestsCounter;
    BOOL bGetProcessAffinityMaskSucceed;

    bGetProcessAffinityMaskSucceed = GetProcessAffinityMask(
        GetCurrentProcess(),
        &dwProcessAffinityMask,
        &dwSystemAffinityMask);

    if (bGetProcessAffinityMaskSucceed)
    {
        for (wRequestsCounter = 0; dwProcessAffinityMask; dwProcessAffinityMask >>= 1)
        {
            if (dwProcessAffinityMask & 0x1) wRequestsCounter++;
        }

        wRequestsCounter = REQUESTS_PER_PROCESSOR * wRequestsCounter;
    }
    else
    {
        wRequestsCounter = OUTSTANDING_REQUESTS;
    }

    return wRequestsCounter;
}

int wmain()
{
    setlocale(LC_ALL, "");
    InitLog();
    Log(LOG_INFO, L"backend started.");
    InitRoomManager();

    if (!StartHTTPServer(GetRequestCount()))
    {
        return 1;
    }
    while (1)
    {
        WCHAR command[128] = { 0 };
        wscanf_s(L"%s", command, (UINT)_countof(command));

        if (wcscmp(command, L"stop") == 0)
        {
            break;
        }
        Log(LOG_ERROR, L"unknown command: %1", command);
    }
    StopHTTPServer();
    return 0;
}
