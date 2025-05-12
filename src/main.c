#include "main.h"

DWORD skyGamePid = PID_ILLEGAL;
HANDLE hEvent;
Hotkey_t hotkey;
i08 undoResetFlag = 0;
f32 targetVol = 1.0
  , previousVol = 0.0;
u32 undoTimeout = 3000;

DWORD getPidOf(const wchar_t *exeName) {
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

i08 doSetVolume(DWORD pid, f32 *prevVol, f32 volume) {
  IMMDevice *device = NULL;
  IAudioSessionEnumerator *sessionEnumerator = NULL;
  i08 r = 0;

  // Get the default device
  if (!getDefaultDevice(&device))
    goto Exit;
  // Get an AudioSessionEnumerator
  if (!getAudioSessionEnumerator(device, &sessionEnumerator))
    goto Exit;

  setProcessVolume(sessionEnumerator, pid, prevVol, volume);
  log("Set volume of %lu to %f\n", pid, volume);
  r = 1;

Exit:
  RELEASE(device);
  RELEASE(sessionEnumerator);
  return r;
}

// Send WM_KEYUP to the game window.
void sendKeyUpMsg(HWND hWnd, Hotkey_t *hk) {
  if (hk->mod | MOD_ALT) {
    // Left alt.
    SendMessageW(hWnd, WM_KEYUP, VK_MENU, 0xC0380001);
    // Right alt.
    SendMessageW(hWnd, WM_KEYUP, VK_MENU, 0xC1380001);
  }
  if (hk->mod | MOD_SHIFT) {
    // Left shift.
    SendMessageW(hWnd, WM_KEYUP, VK_SHIFT, 0xC02A0001);
    // Right shift.
    SendMessageW(hWnd, WM_KEYUP, VK_SHIFT, 0xC0360001);
  }
  if (hk->mod | MOD_CONTROL) {
    // Left ctrl.
    SendMessageW(hWnd, WM_KEYUP, VK_CONTROL, 0xC01D0001);
    // Right ctrl.
    SendMessageW(hWnd, WM_KEYUP, VK_CONTROL, 0xC11D0001);
  }
}

DWORD WINAPI hotkeyThread(LPVOID lpParam) {
  MSG msg;
  HWND hForegroundWnd;
  DWORD timerId = 0
    , processId
    , lastTargetPid = PID_ILLEGAL;
  wchar_t windowName[MAX_PATH];

  // Register hotkey.
  if (!registerHotkeyWith(NULL, 1, &hotkey))
    return 1;
  // Registered hotkey successfully.
  SetEvent(hEvent);

  while (GetMessageW(&msg, NULL, 0, 0)) {
    if (msg.message == WM_HOTKEY && msg.wParam == 1) {
      // Get foreground window and reset pid.
      hForegroundWnd = GetForegroundWindow();
      processId = PID_ILLEGAL;
      if (
        !GetWindowTextW(hForegroundWnd, windowName, MAX_PATH)
        || wcscmp(windowName, GAME_WND_NAME)
        || !GetWindowThreadProcessId(hForegroundWnd, &processId)
      )
        // Foreground window is not the game window, or get pid failed.
        // The global pid is not used because it may be need to reset 
        // different windows with the same name.
        continue;

      if (lastTargetPid != processId || !undoResetFlag) {
        lastTargetPid = processId;
        // Reset volume.
        doSetVolume(processId, &previousVol, targetVol);
        // Set undo flag and timer.
        undoResetFlag = 1;
        timerId = SetTimer(NULL, timerId, 3000, NULL);
      } else
        // If pressed hotkey again when the timer is not set, undo the
        // previous volume reset.
        doSetVolume(processId, &previousVol, previousVol);
    } else if (msg.message == WM_TIMER) {
      // Timed out and the shortcut key was not pressed, clear the flag.
      KillTimer(NULL, timerId);
      timerId = 0;
      undoResetFlag = 0;
    } else if (msg.message == WM_USER_EXIT)
      PostQuitMessage(0);
    else if (msg.message == WM_QUIT)
      break;
  }

  UnregisterHotKey(NULL, 1);
  return msg.wParam;
}

void cfgCallback(const wchar_t *key, const wchar_t *value, void *pUser) {
  if (!wcscmp(key, L"hotkey_volume"))
    buildHotkeyFrom(value, &hotkey);
  else if (!wcscmp(key, L"target_volume"))
    targetVol = clamp(wcstof(value, NULL), 0, 1);
  else if (!wcscmp(key, L"undo_timeout_ms"))
    undoTimeout = clamp(wcstol(value, NULL, 0), 0, 0x7FFFFFFF);
}

i32 WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nShowCmd
) {
  wchar_t cfgPath[MAX_PATH]
    , *dir;
  FILE *file;
  HRESULT hr;
  DWORD threadId;
  HANDLE hThread, mutexHandle;
  i32 ret = 0;

#ifndef DEBUG_CONSOLE
  FreeConsole();
#endif

  // Only one instance can be running.
  mutexHandle = CreateMutexW(NULL, TRUE, L"__SKY_VOLRST__");
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    MBError(L"实例已存在", 0);
    return 1;
  }

  if (!GetModuleFileNameW(hInstance, cfgPath, MAX_PATH))
    goto DefaultCfg;
  dir = wcsrchr(cfgPath, L'\\');
  if (!dir)
    goto DefaultCfg;
  *dir = 0;
  wcscat_s(cfgPath, MAX_PATH, L"\\skyvol-config.txt");
  file = _wfopen(cfgPath, L"r");
  if (!file)
    goto DefaultCfg;
  buildConfigFrom(file, cfgCallback, NULL);
  fclose(file);

DefaultCfg:
  // Read config failed, use default.

  // Initialise COM.
  hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  if (FAILED(hr)) {
    MBError(L"初始化COM失败", 0);
    ret = 1;
    goto Exit;
  }

  hEvent = CreateEventW(NULL, 1, 0, L"__HOTKEY_REG__");
  hThread = CreateThread(NULL, 0, hotkeyThread, 0, 0, &threadId);
  if (!hThread) {
    MBError(L"创建子线程失败", 0);
    ret = 1;
    goto Exit;
  }

  if (WaitForSingleObject(hEvent, 100) != WAIT_OBJECT_0) {
    MBError(L"注册快捷键失败", 0);
    ret = 1;
    goto Exit;
  }

  if (getPidOf(GAME_PROC_NAME) == PID_ILLEGAL)
    // Check pid in order to avoid the situation that the window is not
    // created but the process is started.
    MBError(L"游戏未运行", 0);

  // Waiting for the game.
  while ((skyGamePid = getPidOf(GAME_PROC_NAME)) != PID_ILLEGAL)
    Sleep(500);

  // Terminate thread.
  PostThreadMessageW(threadId, WM_USER_EXIT, 0, 0);
  WaitForSingleObject(hThread, INFINITE);

Exit:
  CoUninitialize();
  ReleaseMutex(mutexHandle);
  CloseHandle(mutexHandle);
  CloseHandle(hEvent);
  return ret;
}