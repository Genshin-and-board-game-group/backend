#include "common.h"
#include "HttpSendRecv.h"

#pragma comment(lib, "httpapi.lib")
#pragma comment(lib, "Websocket.lib")

// The number of requests for queueing, when
#define OUTSTANDING_REQUESTS 8
// The number of requests per processor
#define REQUESTS_PER_PROCESSOR 2

VOID PrintErrorMessage(
    _In_opt_ LPCSTR ErrorMessage,
    _In_ DWORD dwError)
{
    HLOCAL hlocal = NULL;   // Buffer that gets the error message string

    // Use the default system locale since we look for Windows messages.
    // Note: this MAKELANGID combination has 0 as value
    DWORD systemLocale = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    // Get the error code's textual description
    BOOL fOk = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, dwError, systemLocale,
        (PSTR)&hlocal, 0, NULL);

    if (!fOk)
    {
        // Is it a network-related error?
        HMODULE hDll = LoadLibraryEx(TEXT("netmsg.dll"), NULL,
            DONT_RESOLVE_DLL_REFERENCES);

        if (hDll != NULL)
        {
            fOk = FormatMessageA(
                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS |
                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                hDll, dwError, systemLocale,
                (PSTR)&hlocal, 0, NULL);
            FreeLibrary(hDll);
        }
    }

    if (fOk && (hlocal != NULL))
    {
        if (ErrorMessage) printf("%s: ", ErrorMessage);
        printf("%s", (PCSTR)LocalLock(hlocal));
        LocalFree(hlocal);
    }
    else
    {
        printf("No text found for this error number.");
    }
}

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

int main()
{
    if (!StartHTTPServer(GetRequestCount()))
    {
        return 1;
    }
    while (1)
    {
        char command[128] = { 0 };
        scanf_s("%s", command, (UINT)_countof(command));

        if (strcmp(command, "stop") == 0)
        {
            break;
        }
        printf("unknown command: %s\n", command);
    }
    StopHTTPServer();
    return 0;
}
