#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <http.h>
#include <stdio.h>
#include <Websocket.h>

#pragma comment(lib, "httpapi.lib")
#pragma comment(lib, "Websocket.lib")

BOOL InitHTTPServer();

VOID UnInitHTTPServer();

VOID PrintErrorMessage(
    _In_opt_ LPCSTR ErrorMessage,
    _In_ DWORD dwError);


HANDLE hReqHandle = NULL;
HTTP_SERVER_SESSION_ID ServerSessionID = 0;
HTTP_URL_GROUP_ID UrlGroupID = 0;

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


BOOL InitHTTPServer()
{
    HTTPAPI_VERSION HttpApiVersion = HTTPAPI_VERSION_2;
    ULONG ret;

    BOOL bSuccess = FALSE;

    ret = HttpInitialize(HttpApiVersion, HTTP_INITIALIZE_SERVER, NULL);
    if (ret != NO_ERROR)
    {
        PrintErrorMessage("HttpInitialize", ret);
        return bSuccess;
    }

    __try
    {
        ret = HttpCreateRequestQueue(HttpApiVersion, NULL, NULL, 0, &hReqHandle);
        if (ret != NO_ERROR)
        {
            PrintErrorMessage("HttpCreateRequestQueue", ret);
            __leave;
        }

        ret = HttpCreateServerSession(HttpApiVersion, &ServerSessionID, 0);
        if (ret != NO_ERROR)
        {
            PrintErrorMessage("HttpCreateServerSession", ret);
            __leave;
        }

        ret = HttpCreateUrlGroup(ServerSessionID, &UrlGroupID, 0);
        if (ret != NO_ERROR)
        {
            PrintErrorMessage("HttpCreateUrlGroup", ret);
            __leave;
        }

        ret = HttpAddUrlToUrlGroup(UrlGroupID, L"http://+:80/api", 0, 0);
        if (ret != NO_ERROR)
        {
            PrintErrorMessage("HttpAddUrlToUrlGroup", ret);
            __leave;
        }

        HTTP_BINDING_INFO BindingInfo = { 0 };
        BindingInfo.Flags.Present = 1;
        BindingInfo.RequestQueueHandle = hReqHandle;

        ret = HttpSetUrlGroupProperty(UrlGroupID, HttpServerBindingProperty, &BindingInfo, sizeof(BindingInfo));
        if (ret != NO_ERROR)
        {
            PrintErrorMessage("HttpSetUrlGroupProperty", ret);
            __leave;
        }
        bSuccess = TRUE;
    }
    __finally
    {
        if (!bSuccess)
        {
            UnInitHTTPServer();
        }
    }
    return bSuccess;
}

VOID UnInitHTTPServer()
{
    if (UrlGroupID) HttpCloseUrlGroup(UrlGroupID);
    if (HttpCloseServerSession) HttpCloseServerSession(ServerSessionID);
    if (hReqHandle) HttpCloseRequestQueue(hReqHandle);
    HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);

    UrlGroupID = 0;
    ServerSessionID = 0;
    hReqHandle = NULL;
}


int main()
{
    if (!InitHTTPServer())
    {
        return 1;
    }

    UnInitHTTPServer();
    return 0;
}
