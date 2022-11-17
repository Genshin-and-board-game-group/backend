#pragma once
#include <Windows.h>

#define LOG_DEBUG    0
#define LOG_INFO     1
#define LOG_WARNING  2
#define LOG_ERROR    3
#define LOG_CRITICAL 4

VOID InitLog();

VOID LogErrorMessage(
    _In_opt_ LPCWSTR Message,
    _In_ DWORD dwError);

VOID Log(_In_ INT LogLevel, _In_z_ LPCWSTR pMessage, ...);

#define FIXME(szMessage, ...) Log(LOG_WARNING, L"FIXME: " szMessage, __VA_ARGS__)
