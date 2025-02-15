#include "runas.h"

BOOL Proc_isRunAsAdmin(HANDLE hProcess) {
  if (!hProcess)
    hProcess = GetCurrentProcess();
  BOOL bElevated = FALSE;
  // Get current process token
  HANDLE hToken = NULL;
  if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
    return FALSE;
  TOKEN_ELEVATION tokenEle;
  // Retrieve token elevation information  
  DWORD dwRetLen = 0;
  if (GetTokenInformation(hToken, TokenElevation, &tokenEle, sizeof(tokenEle), &dwRetLen))
    if (dwRetLen == sizeof(tokenEle))
      bElevated = tokenEle.TokenIsElevated;
  CloseHandle(hToken);
  return bElevated;
}

VOID Proc_runAsAdmin(LPCWSTR exe, LPCWSTR param, INT nShow) {
  SHELLEXECUTEINFOW ShExecInfo;
  ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
  ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ShExecInfo.hwnd = NULL;
  ShExecInfo.lpVerb = L"runas";
  ShExecInfo.lpFile = exe;
  ShExecInfo.lpParameters = param;
  ShExecInfo.lpDirectory = NULL;
  ShExecInfo.nShow = nShow;
  ShExecInfo.hInstApp = NULL;
  BOOL ret = ShellExecuteExW(&ShExecInfo);
  CloseHandle(ShExecInfo.hProcess);
}

DWORD Proc_getRunningState(const wchar_t *exeName) {
  PROCESSENTRY32W pe32;
  pe32.dwSize = sizeof(pe32);
  HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hProcessSnap == INVALID_HANDLE_VALUE)
    return -1;
  BOOL bMore = Process32FirstW(hProcessSnap, &pe32);
  int l1 = wcslen(exeName), l2;

  while (bMore) {
    l2 = wcslen(pe32.szExeFile);
    if (!wcscmp(pe32.szExeFile, exeName)) {
      CloseHandle(hProcessSnap);
      return pe32.th32ProcessID;
    }
    bMore = Process32NextW(hProcessSnap, &pe32);
  }
  CloseHandle(hProcessSnap);
  return -1;
}

BOOL Proc_getExeFilePath(DWORD procId, char* path, int bufferSize) {
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procId);
  if (hProcess == NULL)
    return 0;
  DWORD result = GetModuleFileNameEx(hProcess, NULL, (LPSTR)path, (DWORD)bufferSize);
  if (result == 0)
    return 0;
  CloseHandle(hProcess);
  return 1;
}

BOOL Proc_setProcessSuspend(DWORD dwProcessID, BOOL fSuspend) {
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwProcessID);

  if (hSnapshot != INVALID_HANDLE_VALUE) {
    THREADENTRY32 te = { sizeof(te) };
    BOOL fOk = Thread32First(hSnapshot, &te);
    for (; fOk; fOk = Thread32Next(hSnapshot, &te)) {
      if (te.th32OwnerProcessID == dwProcessID) {
        HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME,FALSE, te.th32ThreadID);
        if (hThread != NULL)
          fSuspend ? SuspendThread(hThread) : ResumeThread(hThread);
        CloseHandle(hThread);
      }
    }
  }
  CloseHandle(hSnapshot);

  return TRUE;
}

BOOL Proc_detectRunAsAdmin(int *pargc) {
  wchar_t *r
    , *s = GetCommandLineW()
    , **argv = CommandLineToArgvW(s, pargc)
    , buf[MAX_PATH];
  if (!Proc_isRunAsAdmin(NULL)) {
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    size_t l = wcslen(buf);
    if (s[l] == L' ')
      r = s + l;
    else
      r = s + l + 2;
    wprintf(L"%s", argv[0]);
    // Reopen with admin, with original argv
    Proc_runAsAdmin(argv[0], r, SW_SHOWNORMAL);
    return 1;
  }
  return 0;
}