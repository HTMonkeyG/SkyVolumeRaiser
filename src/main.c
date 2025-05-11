#include "main.h"

DWORD skyGamePID = -1;
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
    return PID_INVALID;
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
  return PID_INVALID;
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

DWORD WINAPI hotkeyThread(LPVOID lpParam) {
  MSG msg;
  HWND hForegroundWnd;
  DWORD timerID, processId;
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
      processId = 0;
      if (
        !GetWindowTextW(hForegroundWnd, windowName, MAX_PATH)
        || wcscmp(windowName, GAME_WND_NAME)
        || !GetWindowThreadProcessId(hForegroundWnd, &processId)
      )
        // Foreground window is not the game window, or get pid failed.
        // The global pid is not used because it may be need to reset 
        // different windows with the same name.
        continue;

      if (!undoResetFlag) {
        // Reset volume.
        doSetVolume(processId, &previousVol, targetVol);
        // Set undo flag and timer.
        undoResetFlag = 1;
        SetTimer(NULL, 1, 3000, NULL);
      } else
        // If pressed hotkey again when the timer is not set, undo the
        // previous volume reset.
        doSetVolume(processId, &previousVol, previousVol);
    } else if (msg.message == WM_TIMER && msg.wParam == 1) {
      // Timed out and the shortcut key was not pressed, clear the flag.
      KillTimer(NULL, timerID);
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

  if (getPidOf(GAME_PROC_NAME) == PID_INVALID)
    // Check pid in order to avoid the situation that the window is not
    // created but the process is started.
    MBError(L"游戏未运行", 0);

  // Waiting for the game.
  while ((skyGamePID = getPidOf(GAME_PROC_NAME)) != PID_INVALID)
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