#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include "./reflective/loadlibrary.h"
#include "main.h"

#define svrMessageBoxError(text, type) (MessageBoxW(NULL, text, L"Error", MB_ICONERROR | type))

DWORD svrGetPid(
  const wchar_t *exeName
) {
  PROCESSENTRY32W pe32;
  HANDLE hProcessSnap;
  BOOL bMore;

  hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hProcessSnap == INVALID_HANDLE_VALUE)
    return PID_ILLEGAL;
  pe32.dwSize = sizeof(pe32);
  bMore = Process32FirstW(hProcessSnap, &pe32);

  while (bMore) {
    if (!wcscmp(pe32.szExeFile, exeName)) {
      CloseHandle(hProcessSnap);
      return pe32.th32ProcessID;
    }
    bMore = Process32NextW(hProcessSnap, &pe32);
  }
  CloseHandle(hProcessSnap);
  return PID_ILLEGAL;
}

BOOL svrLoadBinaryResource(
  i32 resourceId,
  const char *resourceType, 
  i08 **ppData,
  DWORD *pSize
) {
  HINSTANCE hInstance = GetModuleHandleA(NULL);
  HRSRC hRes = FindResourceA(
    hInstance, 
    MAKEINTRESOURCEA(resourceId), 
    resourceType);
  if (!hRes)
    return FALSE;

  DWORD size = SizeofResource(hInstance, hRes);
  if (!size)
    return FALSE;

  HGLOBAL hData = LoadResource(hInstance, hRes);
  if (!hData)
    return FALSE;

  *ppData = (i08 *)LockResource(hData);
  *pSize = size;

  return TRUE;
}

int main() {
  DWORD gamePid, dllFileSize;
  i08 *dllFile = NULL;
  HANDLE hGameProcess;

  SetProcessDpiAwarenessContext(
    DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
  
  gamePid = svrGetPid(L"Sky.exe");
  if (gamePid == PID_ILLEGAL) {
    // Check pid in order to avoid the situation that the window is not
    // created but the process is started.
    svrMessageBoxError(svrText_GameNotRunning, 0);
    goto ErrRet;
  }

  if (!svrLoadBinaryResource(2, RT_RCDATA, &dllFile, &dllFileSize)) {
    // Load dll binary data from binded files.
    svrMessageBoxError(svrText_LoadResourceFailed, 0);
    goto ErrRet;
  }

  hGameProcess = OpenProcess(
    PROCESS_CREATE_THREAD
      | PROCESS_QUERY_INFORMATION
      | PROCESS_VM_OPERATION
      | PROCESS_VM_WRITE
      | PROCESS_VM_READ,
    FALSE,
    gamePid);
  if (!hGameProcess) {
    svrMessageBoxError(svrText_InjectFailed, 0);
    goto ErrRet;
  }

  if (!LoadRemoteLibraryR(hGameProcess, (void *)dllFile, dllFileSize, NULL)) {
    svrMessageBoxError(svrText_InjectFailed, 0);
    goto ErrRet;
  }

ErrRet:
  return 0;
}