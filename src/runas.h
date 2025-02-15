#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

BOOL Proc_isRunAsAdmin(HANDLE hProcess);
VOID Proc_runAsAdmin(LPCWSTR exe, LPCWSTR param, INT nShow);
DWORD Proc_getRunningState(const wchar_t *exeName);
BOOL Proc_getExeFilePath(DWORD procId, char* path, int bufferSize);
BOOL Proc_setProcessSuspend(DWORD dwProcessID, BOOL fSuspend);
BOOL Proc_detectRunAsAdmin(int *pargc);