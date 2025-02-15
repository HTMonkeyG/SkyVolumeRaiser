#include "audio.h"

// Gets the default render device
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

// Get the executable file name of session
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
  if (*sessionName)
    _swprintf(name, L"%s", sessionName);
  else {
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

bool setSessionVolume(IAudioSessionEnumerator *enumerator, const wchar_t *sessionName, float *prevVol, float targetVol) {
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

    // Get session volume control
    hr = sessionControl->lpVtbl->QueryInterface(sessionControl, &IID_ISimpleAudioVolume, (void **)&sessionSimpleVolume);
    if (FAILED(hr)) {
      printf("Unable to get session volume controls: %x\n", hr);
      goto Exit;
    }

    // Get current volume of session
    float volume;
    hr = sessionSimpleVolume->lpVtbl->GetMasterVolume(sessionSimpleVolume, &volume);
    if (FAILED(hr)) {
      printf("Unable to get session volume: %x\n", hr);
      goto Exit;
    }
    *prevVol = volume;

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