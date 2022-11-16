#include "common.h"
#include "Log.h"

#define LOG_DEFAULT "\x1b[0m"
#define LOG_BOLD    "\x1b[1m"
#define LOG_RED     "\x1b[31m"
#define LOG_GREEN   "\x1b[32m"
#define LOG_YELLOW  "\x1b[33m"
#define LOG_BLUE    "\x1b[34m"

BOOL EnableVT = FALSE;

VOID InitLog()
{
    // enable VT Sequnce output explicitly.
    DWORD ConsoleMode;
    if (!GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &ConsoleMode))
        return;

    if (!SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ConsoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        return;

    EnableVT = TRUE;
}

static VOID GetVTFormatString(_In_ INT LogLevel, _Outptr_result_maybenull_ PCHAR *pVTMsgFmt, _Outptr_result_maybenull_ PCHAR *pVTLevelFmt)
{
    if (!EnableVT)
    {
        *pVTMsgFmt = *pVTLevelFmt = "";
        return;
    }
    *pVTMsgFmt = LogLevel == LOG_CRITICAL ? LOG_DEFAULT LOG_RED : LOG_DEFAULT;
    switch (LogLevel)
    {
    case LOG_DEBUG:
        *pVTLevelFmt = LOG_BOLD LOG_GREEN;
        break;
    case LOG_INFO:
        *pVTLevelFmt = LOG_BOLD;
        break;
    case LOG_WARNING:
        *pVTLevelFmt = LOG_YELLOW;
        break;
    case LOG_ERROR:
        *pVTLevelFmt = LOG_BOLD LOG_RED;
        break;
    case LOG_CRITICAL:
        *pVTLevelFmt = LOG_RED;
        break;
    default:
        // TODO: assert
        *pVTLevelFmt = NULL;
        break;
    }
}

VOID Log(_In_ INT LogLevel, _In_z_ LPCSTR pMessage, ...)
{
    LPSTR pBuffer = NULL;
    va_list args = NULL;
    SYSTEMTIME LocalTime;
    char* VTMsgFmt = NULL;
    char* VTLevelFmt = NULL;

#ifdef NDEBUG
    if (LogLevel == LOG_DEBUG)
        return;
#endif

    va_start(args, pMessage);

    if (FormatMessageA(
        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        pMessage,
        0,
        0,
        (LPSTR)&pBuffer,
        0,
        &args) == 0)
        return;

    va_end(args);

    GetLocalTime(&LocalTime);

    GetVTFormatString(LogLevel, &VTMsgFmt, &VTLevelFmt);

    char* LevelText[] = { "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL" };

    printf(
        "%s"                             // MsgFmt
        "%02d-%02d-%02d %02d:%02d:%02d " // Date Time
        "[%s%s%s] "                      // LevelFmt, LevelText, MsgFmt
        "%s\n",                          // pBuffer
        VTMsgFmt,
        LocalTime.wYear % 100, LocalTime.wMonth, LocalTime.wDay,
        LocalTime.wHour, LocalTime.wMinute, LocalTime.wSecond,
        VTLevelFmt,
        LevelText[LogLevel],
        VTMsgFmt,
        pBuffer);

    LocalFree(pBuffer);
}

VOID LogErrorMessage(
    _In_opt_ LPCSTR Message,
    _In_ DWORD dwError)
{
    HLOCAL hlocal = NULL;   // Buffer that gets the error message string

    // Use the default system locale since we look for Windows messages.
    // Note: this MAKELANGID combination has 0 as value
    DWORD systemLocale = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    // Get the error code's textual description
    DWORD dwLen = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, dwError, systemLocale,
        (PSTR)&hlocal, 0, NULL);

    if (!dwLen)
    {
        // Is it a network-related error?
        HMODULE hDll = LoadLibraryEx(TEXT("netmsg.dll"), NULL,
            DONT_RESOLVE_DLL_REFERENCES);

        if (hDll != NULL)
        {
            dwLen = FormatMessageA(
                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS |
                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                hDll, dwError, systemLocale,
                (PSTR)&hlocal, 0, NULL);
            FreeLibrary(hDll);
        }
    }

    if (dwLen && (hlocal != NULL))
    {
        PCSTR ErrorText = (PCSTR)LocalLock(hlocal);
        if (!ErrorText)
            return;

        // well... I don't want the "\r\n" followed after the message.
        if (dwLen >= 2 && ErrorText[dwLen - 2] == '\r' && ErrorText[dwLen - 1] == '\n')
            dwLen -= 2;

        if (Message)
            Log(LOG_ERROR, "%1!s!: %2!.*s!", Message, dwLen, ErrorText);
        else
            Log(LOG_ERROR, "%1!.*s!", dwLen, ErrorText);

        LocalFree(hlocal);
    }
    else
    {
        Log(LOG_ERROR, "No text found for this error number.");
    }
}
