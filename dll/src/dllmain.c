#include <windows.h>
#include "MinHook.h"

#include "svr.h"

HMODULE hDllNcmAudioPlayer = NULL;

static HANDLE hMutex = NULL;
static i08 gActive = 0;

static DWORD WINAPI svrWaitForNcm(
  LPVOID lpParam
) {
  while (!hDllNcmAudioPlayer) {
    hDllNcmAudioPlayer = GetModuleHandleA("NcmAudioPlayer.dll");
    if (hDllNcmAudioPlayer)
      break;

    Sleep(100);
  }

  svrInstallHooks();

  return 0;
}

BOOL APIENTRY DllMain(
  HMODULE hModule,
  DWORD dwReason,
  LPVOID lpReserved
) {
  if (dwReason == DLL_PROCESS_ATTACH) {
    if (!GetModuleHandleA("Sky.exe"))
      return TRUE;

    // In most cases, there will only be one game instance. This code is used
    // to prevent duplicate injection.
    // If want to disable it, comment the #define statement in svr.h
#ifdef svrDuplicateCheck
    SetLastError(ERROR_SUCCESS);
    hMutex = CreateMutexW(NULL, FALSE, L"__SKY_VOLRST__");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
      return TRUE;
#endif

    gActive = 1;
    MH_Initialize();
    CreateThread(
      NULL,
      0,
      svrWaitForNcm,
      NULL,
      0,
      NULL);
  } else if (dwReason == DLL_PROCESS_DETACH) {
    if (gActive) {
      MH_DisableHook(MH_ALL_HOOKS);
      MH_Uninitialize();
    }
    if (hMutex)
      CloseHandle(hMutex);
  }

  return TRUE;
}
