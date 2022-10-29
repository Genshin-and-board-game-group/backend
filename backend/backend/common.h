#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#include <stdio.h>

VOID PrintErrorMessage(
    _In_opt_ LPCSTR ErrorMessage,
    _In_ DWORD dwError);
