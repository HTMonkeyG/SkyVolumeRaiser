#include "main.h"

DWORD skyGamePID = -1;
HANDLE hEvent;
int undoResetFlag = 0;
float previousVol = 0.;
unsigned int hotkeyMod = 0
  , hotkeyVK = 0
  , undoTimeout = 0;

void getArgvHotkey(int *pargc) {
  wchar_t *s = GetCommandLineW()
    , **wargv = CommandLineToArgvW(s, pargc);
  
  for (int i = 0; i < *pargc; i++) {
    if (!wcscmp(wargv[i], L"-alt"))
      hotkeyMod |= MOD_ALT;
    else if (!wcscmp(wargv[i], L"-shift"))
      hotkeyMod |= MOD_SHIFT;
    else if (!wcscmp(wargv[i], L"-ctrl"))
      hotkeyMod |= MOD_CONTROL;
    else if (!memcmp(wargv[i], L"-vk", 3 * sizeof(wchar_t)))
      hotkeyVK = wargv[i][3] & 0xFF;
    else if (!wcscmp(wargv[i], L"-t") && i + 1 < *pargc)
      swscanf(wargv[i + 1], L"%u", &undoTimeout);
  }

  if (!undoTimeout)
    undoTimeout = 3000;
  if (!hotkeyVK) {
    hotkeyMod = MOD_ALT | MOD_SHIFT;
    hotkeyVK = 'R';
  }
}

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
  if (!RegisterHotKey(NULL, 1, hotkeyMod, hotkeyVK))
    return 1;
  SetEvent(hEvent);

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
          timerID = SetTimer(NULL, 1, 3000, NULL);
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
      MB_ICONERROR
    );
    return 1;
  }

  if (Proc_detectRunAsAdmin(&argc))
    return 1;

  getArgvHotkey(&argc);

  // Initialise COM
  hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  if (FAILED(hr)) {
    _swprintf(buf, L"Unable to initialize COM: %x\n", hr);
    MessageBoxW(NULL, buf, L"ERROR", MB_ICONERROR);
    ret = 1;
    goto Exit;
  }

  hEvent = CreateEventW(NULL, 1, 0, L"__HOTKEY_REG__");
  hThread = CreateThread(NULL, 0, hotkeyThread, 0, 0, &threadId);
  if (!hThread) {
    MessageBoxW(NULL, L"Create thread failed", L"ERROR", MB_ICONERROR);
    ret = 1;
    goto Exit;
  }

  if (WaitForSingleObject(hEvent, 100) != WAIT_OBJECT_0) {
    MessageBoxW(NULL, L"Register hotkey failed", L"ERROR", MB_ICONERROR);
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
  CloseHandle(hEvent);
  return ret;
}