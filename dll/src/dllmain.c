#include <windows.h>

#include "MinHook.h"
#include "svr.h"

HMODULE hDllNcmAudioPlayer = NULL;

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
    MH_Initialize();
    CreateThread(
      NULL,
      0,
      svrWaitForNcm,
      NULL,
      0,
      NULL);
  } else if (dwReason == DLL_PROCESS_DETACH) {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
  }

  return TRUE;
}
