#include <windows.h>
#include "MinHook.h"

#include "svr.h"

HMODULE hDllNcmAudioPlayer = NULL;
i08 gActive = 0;

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

    gActive = 1;
    MH_Initialize();
    CreateThread(
      NULL,
      0,
      svrWaitForNcm,
      NULL,
      0,
      NULL);
  } else if (dwReason == DLL_PROCESS_DETACH && gActive) {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
  }

  return TRUE;
}
