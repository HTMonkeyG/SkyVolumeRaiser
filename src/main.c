#include "main.h"

IMMDevice *device = NULL;
IAudioSessionEnumerator *sessionEnumerator = NULL;
DWORD skyGamePID = -1;

// Gets the default render device for the console role
bool getDefaultDevice(IMMDevice **device) {
  HRESULT hr;
  bool success = false;
  IMMDeviceEnumerator *pEnumerator = NULL;

  // Get a device enumerator
  hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  if (FAILED(hr)) {
    printf("Unable to instantiate device enumerator: %x\n", hr);
    goto Exit;
  }

  // Get the default audio device
  hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(pEnumerator, eRender, eConsole, device);
  if (FAILED(hr)) {
    printf("Unable to retrive default audio device: %x\n", hr);
    goto Exit;
  }

  success = true;
Exit:
  RELEASE(pEnumerator);
  return success;
}

// Prints the master volume scalar of the specified device
bool printEndpointVolume(IMMDevice *device) {
  HRESULT hr;
  IAudioEndpointVolume *endpointVolume = NULL;
  bool success = false;

  // Get the endpoint volume controls
  hr = device->lpVtbl->Activate(device, &IID_IAudioEndpointVolume, CLSCTX_INPROC_SERVER, NULL, (void **)&endpointVolume);
  if (FAILED(hr)) {
    printf("Unable to get endpoint volume: %x\n", hr);
    goto Exit;
  }

  // Print the endpoint volume
  float masterVolumeScalar;
  endpointVolume->lpVtbl->GetMasterVolumeLevelScalar(endpointVolume, &masterVolumeScalar);
  printf("Master Volume Scalar: %f\n", masterVolumeScalar);

  success = true;
Exit:
  RELEASE(endpointVolume);
  return success;
}

// Gets an AudioSessionEnumerator for the specified device
bool getAudioSessionEnumerator(IMMDevice *device, IAudioSessionEnumerator **sessionEnumerator) {
  HRESULT hr;
  IAudioSessionManager2 *sessionManager2 = NULL;
  bool success = false;

  // Get an AudioSessionManager2
  hr = device->lpVtbl->Activate(device, &IID_IAudioSessionManager2, CLSCTX_INPROC_SERVER, NULL, (void **)&sessionManager2);
  if (FAILED(hr)) {
    printf("Unable to get session manager 2: %x\n", hr);
    goto Exit;
  }

  // Get a session enumerator
  hr = sessionManager2->lpVtbl->GetSessionEnumerator(sessionManager2, sessionEnumerator);
  if (FAILED(hr)) {
    printf("Unable to get session enumerator: %x\n", hr);
    goto Exit;
  }

  success = true;
Exit:
  RELEASE(sessionManager2);
  return success;
}

// Prints the DisplayName of an audio session
// If no display name is present, this will instead print the name of the process controlling the session
bool getSessionName(IAudioSessionControl *sessionControl, wchar_t *name) {
  HRESULT hr;
  IAudioSessionControl2 *sessionControl2 = NULL;
  HANDLE process = NULL;
  bool success = false;

  // Get the session display name
  PWSTR sessionName;
  hr = sessionControl->lpVtbl->GetDisplayName(sessionControl, &sessionName);
  if (FAILED(hr)) {
    printf("Unable to get session display name: %x\n", hr);
    goto Exit;
  }

  // Print the session name
  if (*sessionName) {
    _swprintf(name, L"%s", sessionName);
  } else {
    // Or if null, the process name
    hr = sessionControl->lpVtbl->QueryInterface(sessionControl, &IID_IAudioSessionControl2, (void **)&sessionControl2);
    if (FAILED(hr)) {
      printf("Unable to get session control 2 interface: %x\n", hr);
      goto Exit;
    }

    DWORD processId;
    hr = sessionControl2->lpVtbl->GetProcessId(sessionControl2, &processId);
    if (FAILED(hr)) {
      printf("Unable to get session process ID: %x\n", hr);
      goto Exit;
    }

    process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, processId);
    if (process == NULL) {
      printf("Unable to get process handle: %u\n", GetLastError());
      goto Exit;
    }

    wchar_t processName[MAX_PATH] = L"Unknown"
      , *p;
    GetModuleFileNameExW(process, 0, processName, MAX_PATH);
    if (!(p = wcsrchr(processName, L'\\') + 1) || !*p)
      p = processName;
    _swprintf(name, L"%s", p);
  }

  success = true;
Exit:
  CoTaskMemFree(sessionName);
  RELEASE(sessionControl2);
  CloseHandle(process);
  return success;
}

// Prints some properties for all audio sessions in the provided enumerator,
// and resets their relative volume to 1.0
bool setSessionVolume(IAudioSessionEnumerator *enumerator, wchar_t *sessionName, float targetVol) {
  HRESULT hr;
  IAudioSessionControl *sessionControl = NULL;
  ISimpleAudioVolume *sessionSimpleVolume = NULL;
  bool success = false;
  wchar_t buffer[MAX_PATH];

  // Get session count
  int sessionCount;
  hr = enumerator->lpVtbl->GetCount(enumerator, &sessionCount);
  if (FAILED(hr)) {
    printf("Unable to get session count: %x\n", hr);
    goto Exit;
  }

  for (int i = 0; i < sessionCount; i++) {
    // Get audio session control
    hr = enumerator->lpVtbl->GetSession(enumerator, i, &sessionControl);
    if (FAILED(hr)) {
      printf("Unable to get session control: %x\n", hr);
      goto Exit;
    }

    if (!getSessionName(sessionControl, buffer))
      goto Exit;

    if (sessionName != NULL && wcscmp(sessionName, buffer))
      continue;

    wprintf(L"Name: %s\n", sessionName);

    // Get and print session state
    AudioSessionState state;
    hr = sessionControl->lpVtbl->GetState(sessionControl, &state);
    if (FAILED(hr)) {
      printf("Unable to get session state: %x\n", hr);
      goto Exit;
    }
    printf("\t- State: %s\n", SESSION_STATES[state]);

    // Get session volume control
    hr = sessionControl->lpVtbl->QueryInterface(sessionControl, &IID_ISimpleAudioVolume, (void **)&sessionSimpleVolume);
    if (FAILED(hr)) {
      printf("Unable to get session volume controls: %x\n", hr);
      goto Exit;
    }

    // Get and print session volume
    float volume;
    hr = sessionSimpleVolume->lpVtbl->GetMasterVolume(sessionSimpleVolume, &volume);
    if (FAILED(hr)) {
      printf("Unable to get session volume: %x\n", hr);
      goto Exit;
    }
    printf("\t- Volume: %f\n", volume);

    // Set session volume to max
    hr = sessionSimpleVolume->lpVtbl->SetMasterVolume(sessionSimpleVolume, targetVol, NULL);
    if (FAILED(hr)) {
      printf("Unable to set session volume: %x\n", hr);
      goto Exit;
    }

    RELEASE(sessionSimpleVolume);
    RELEASE(sessionControl);
  }

  success = true;
Exit:
  RELEASE(sessionControl);
  RELEASE(sessionSimpleVolume);
  return success;
}

DWORD WINAPI hotkeyThread(LPVOID lpParam) {
  MSG msg;

  // Alt + Shift + R
  if (!RegisterHotKey(NULL, 1, MOD_ALT | MOD_SHIFT, 'R')) {
    MessageBoxW(NULL, L"Register hotkey failed", L"ERROR", MB_ICONERROR);
    return 1;
  }

  while (GetMessageW(&msg, NULL, 0, 0)) {
    if (msg.message == WM_HOTKEY) {
      // Ensure the game window is active
      HWND foregroundWindow = GetForegroundWindow();
      DWORD processId;
      GetWindowThreadProcessId(foregroundWindow, &processId);
      printf("%d, %d\n", skyGamePID, processId);
      if (skyGamePID == -1 || skyGamePID != processId)
        continue;

      // Get the default render device
      if (!getDefaultDevice(&device))
        goto Exit;

      // Print the master volume scalar of the default device
      if (!printEndpointVolume(device))
        goto Exit;

      // Get an AudioSessionEnumerator
      if (!getAudioSessionEnumerator(device, &sessionEnumerator))
        goto Exit;

      printf("\nManipulating audio sessions...\n");
      setSessionVolume(sessionEnumerator, L"Sky.exe", 1.0f);
      printf("\nVolumes Reset!\n");
    }

    if (msg.message == WM_APP1)
      break;

Exit:
    RELEASE(device);
    RELEASE(sessionEnumerator);
  }

  UnregisterHotKey(NULL, 1);
  RELEASE(device);
  RELEASE(sessionEnumerator);

  return 0;
}

int main(int argc, char *argv_[]) {
  HRESULT hr;
  DWORD threadId;
  HANDLE hThread, mutexHandle;
  wchar_t buf[MAX_PATH];
  int ret = 0;

  // Only one instance can be running
  mutexHandle = CreateMutexW(NULL, TRUE, L"__SKY_VOLRST__");
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    ShowWindow(GetConsoleWindow(), SW_HIDE);
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
  
  ShowWindow(GetConsoleWindow(), SW_HIDE);

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

  // Waiting for the game
  while ((skyGamePID = Proc_getRunningState(L"Sky.exe")) != -1)
    Sleep(500);

  // Terminate thread
  PostThreadMessageW(threadId, WM_APP1, 0, 0);
  WaitForSingleObject(hThread, INFINITE);

Exit:
  RELEASE(device);
  RELEASE(sessionEnumerator);
  CoUninitialize();
  ReleaseMutex(mutexHandle);
  CloseHandle(mutexHandle);
  return ret;
}