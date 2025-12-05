#include "main.h"

HANDLE hEvent;
Hotkey_t hotkey;
f32 targetVol = 1.0
  , fadeInOutTime = 0.2;
u32 undoTimeout = 3000;

f32 lerp(f32 x, f32 a, f32 b) {
  x = clamp(x, 0., 1.);
  return a + x * (b - a);
}

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

  // Get the default device.
  if (!getDefaultDevice(&device))
    goto Exit;
  // Get an AudioSessionEnumerator.
  if (!getAudioSessionEnumerator(device, &sessionEnumerator))
    goto Exit;

  if (setProcessVolume(sessionEnumerator, pid, prevVol, volume))
    goto Exit;
  log("Set volume of %lu to %f\n", pid, volume);

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
  DWORD undoTimer = 0
    , fadeTimer = 0
    , lastTargetPid = PID_ILLEGAL
    , processId;
  i08 undoResetFlag = 0
    , lastOperation = 0;
  u32 frameInteval = 1000 / FADE_UPDATE_FREQ;
  f32 previousVol = 0.0
    , fadeCtrl = 0.0
    , beginVol, endVol;
  wchar_t windowName[8] = {0};

  // Register hotkey.
  if (!registerHotkeyWith(NULL, 1, &hotkey))
    return 1;
  // Registered hotkey successfully.
  SetEvent(hEvent);

  if (fadeInOutTime > frameInteval)
    // Start the timer only when the fade time is effective.
    fadeTimer = SetTimer(NULL, fadeTimer, frameInteval, NULL);

  while (GetMessageW(&msg, NULL, 0, 0)) {
    if (msg.message == WM_HOTKEY && msg.wParam == 1) {
      // Get foreground window and reset pid.
      hForegroundWnd = GetForegroundWindow();
      processId = PID_ILLEGAL;
      if (
        !GetWindowTextW(hForegroundWnd, windowName, 7)
        || wcscmp(windowName, GAME_WND_NAME)
        || !GetWindowThreadProcessId(hForegroundWnd, &processId)
      )
        // Foreground window is not the game window, or get pid failed.
        // The global pid is not used because it may be need to reset 
        // different windows with the same name.
        continue;

      if (lastTargetPid != processId || !undoResetFlag) {
        lastTargetPid = processId;
        // Set undo flag and timer.
        undoResetFlag = 1;
        // Replace existing timer.
        undoTimer = SetTimer(NULL, undoTimer, undoTimeout, NULL);
        // Try to read current volume.
        if (!fadeTimer || !doSetVolume(processId, &beginVol, -1))
          // Directly set volume if no fade or get current volume failed.
          lastOperation = doSetVolume(processId, &previousVol, targetVol);
        else
          // Or dispatch the volume to another timer for the fading.
          fadeCtrl = 1.0;
      } else if (!fadeTimer)
        // If pressed hotkey again when the timer is not set, undo the
        // previous volume reset.
        doSetVolume(processId, &previousVol, previousVol);
      else {

      }
    } else if (msg.message == WM_TIMER && msg.wParam == undoTimer) {
      // Timed out and the shortcut key was not pressed, clear the flag.
      KillTimer(NULL, undoTimer);
      undoTimer = 0;
      undoResetFlag = 0;
    } else if (msg.message == WM_TIMER && msg.wParam == fadeTimer) {
      fadeCtrl += 1000 / FADE_UPDATE_FREQ;
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
  else if (!wcscmp(key, L"fade_in_out_time"))
    fadeInOutTime = clamp(wcstof(value, NULL), 0, 60) * 1000;
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
  DWORD skyGamePid = PID_ILLEGAL
    , threadId;
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

  // Read config file.
  if (!GetModuleFileNameW(hInstance, cfgPath, MAX_PATH))
    // Read config failed, use default.
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
