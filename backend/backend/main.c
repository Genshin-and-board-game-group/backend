#include "common.h"
#include "HttpSendRecv.h"
#include "RoomManager.h"

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

int wmain(INT argc, WCHAR ** argv)
{
    InitLog();

    WCHAR* WebsocketListenURL = L"http://+:80/api";
    if (argc > 2)
    {
        Log(LOG_ERROR, L"Usage: %1 <Websocket URL>", argv[0]);
        return 1;
    }
    if (argc == 2)
    {
        WebsocketListenURL = argv[1];
    }

    Log(LOG_INFO, L"backend started.");
    InitRoomManager();

    Log(LOG_INFO, L"listening on URL %1 for Websocket API", WebsocketListenURL);

    if (!StartHTTPServer(GetRequestCount(), WebsocketListenURL))
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
