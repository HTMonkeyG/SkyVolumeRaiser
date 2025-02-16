#include "main.h"

DWORD skyGamePID = -1;
int undoResetFlag = 0;
float previousVol = 0.;

bool doSetVolume(const wchar_t *exe, float *prevVol, float volume) {
  IMMDevice *device = NULL;
  IAudioSessionEnumerator *sessionEnumerator = NULL;

  // Get the default device
  if (!getDefaultDevice(&device))
    goto Exit;
  // Get an AudioSessionEnumerator
  if (!getAudioSessionEnumerator(device, &sessionEnumerator))
    goto Exit;

  setSessionVolume(sessionEnumerator, exe, prevVol, volume);
  printf("\nVolume set to %f\n", volume);

Exit:
  RELEASE(device);
  RELEASE(sessionEnumerator);
}

DWORD WINAPI hotkeyThread(LPVOID lpParam) {
  MSG msg;
  DWORD timerID;

  // Alt + Shift + R
  if (!RegisterHotKey(NULL, 1, MOD_ALT | MOD_SHIFT, 'R')) {
    MessageBoxW(NULL, L"Register hotkey failed", L"ERROR", MB_ICONERROR);
    return 1;
  }

  while (GetMessageW(&msg, NULL, 0, 0)) {
    switch (msg.message) {
      case WM_HOTKEY:
        if (!undoResetFlag) {
          // Ensure the game window is active
          DWORD processId;
          GetWindowThreadProcessId(GetForegroundWindow(), &processId);
          if (skyGamePID == -1 || skyGamePID != processId)
            continue;
          doSetVolume(L"Sky.exe", &previousVol, 1.0f);
          undoResetFlag = 1;
          timerID = SetTimer(NULL, 1, 1000, NULL);
          break;
        } else
          doSetVolume(L"Sky.exe", &previousVol, previousVol);
      case WM_TIMER:
        KillTimer(NULL, timerID);
        undoResetFlag = 0;
        break;
      case WM_APP1:
        goto Exit;
    }
  }

Exit:
  UnregisterHotKey(NULL, 1);
  return 0;
}

int main(int argc, char *argv_[]) {
  HRESULT hr;
  DWORD threadId;
  HANDLE hThread, mutexHandle;
  wchar_t buf[MAX_PATH];
  int ret = 0;

  FreeConsole();

  // Only one instance can be running
  mutexHandle = CreateMutexW(NULL, TRUE, L"__SKY_VOLRST__");
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    MessageBoxW(
      NULL,
      L"An instance has already running.",
      L"Instance exists",
      MB_OK
    );
    return 1;
  }

  if (Proc_detectRunAsAdmin(&argc))
    return 1;

  // Initialise COM
  hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  if (FAILED(hr)) {
    _swprintf(buf, L"Unable to initialize COM: %x\n", hr);
    MessageBoxW(NULL, buf, L"ERROR", MB_ICONERROR);
    ret = 1;
    goto Exit;
  }

  hThread = CreateThread(NULL, 0, hotkeyThread, 0, 0, &threadId);
  if (!hThread) {
    MessageBoxW(NULL, L"Create thread failed", L"ERROR", MB_ICONERROR);
    ret = 1;
    goto Exit;
  }

  if (Proc_getRunningState(L"Sky.exe") == -1)
    MessageBoxW(NULL, L"Game is not running", L"ERROR", MB_ICONERROR);

  // Waiting for the game
  while ((skyGamePID = Proc_getRunningState(L"Sky.exe")) != -1)
    Sleep(500);

  // Terminate thread
  PostThreadMessageW(threadId, WM_APP1, 0, 0);
  WaitForSingleObject(hThread, INFINITE);

Exit:
  CoUninitialize();
  ReleaseMutex(mutexHandle);
  CloseHandle(mutexHandle);
  return ret;
}