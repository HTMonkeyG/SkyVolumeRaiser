#include "audio.h"

// Gets the default render device
i08 getDefaultDevice(IMMDevice **device) {
  HRESULT hr;
  i08 success = 0;
  IMMDeviceEnumerator *pEnumerator = NULL;

  if (!device)
    return 0;

  // Get a device enumerator
  hr = CoCreateInstance(
    &CLSID_MMDeviceEnumerator,
    NULL,
    CLSCTX_INPROC_SERVER,
    &IID_IMMDeviceEnumerator,
    (void**)&pEnumerator
  );
  if (FAILED(hr)) {
    log("Unable to instantiate device enumerator: %lx\n", hr);
    goto Exit;
  }

  // Get the default audio device
  hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(
    pEnumerator,
    eRender,
    eConsole,
    device
  );
  if (FAILED(hr)) {
    log("Unable to retrive default audio device: %lx\n", hr);
    goto Exit;
  }

  success = 1;
Exit:
  RELEASE(pEnumerator);
  return success;
}

// Gets an AudioSessionEnumerator for the specified device
i08 getAudioSessionEnumerator(
  IMMDevice *device,
  IAudioSessionEnumerator **sessionEnumerator
) {
  HRESULT hr;
  IAudioSessionManager2 *sessionManager2 = NULL;
  i08 success = 0;

  if (!device || !sessionEnumerator)
    return 0;

  // Get an AudioSessionManager2
  hr = device->lpVtbl->Activate(
    device,
    &IID_IAudioSessionManager2,
    CLSCTX_INPROC_SERVER,
    NULL,
    (void **)&sessionManager2
  );
  if (FAILED(hr)) {
    log("Unable to get session manager 2: 0x%lx\n", hr);
    goto Exit;
  }

  // Get a session enumerator
  hr = sessionManager2->lpVtbl->GetSessionEnumerator(
    sessionManager2,
    sessionEnumerator
  );
  if (FAILED(hr)) {
    log("Unable to get session enumerator: 0x%lx\n", hr);
    goto Exit;
  }

  success = 1;
Exit:
  RELEASE(sessionManager2);
  return success;
}

i08 getSessionPid(IAudioSessionControl *sessionControl, DWORD *pid) {
  HRESULT hr;
  IAudioSessionControl2 *sessionControl2 = NULL;
  i08 success = 0;

  if (!sessionControl || !pid)
    return 0;

  // Get the process name.
  hr = sessionControl->lpVtbl->QueryInterface(
    sessionControl,
    &IID_IAudioSessionControl2,
    (void **)&sessionControl2
  );
  if (FAILED(hr)) {
    log("Unable to get session control 2 interface: 0x%lx\n", hr);
    goto Exit;
  }

  hr = sessionControl2->lpVtbl->GetProcessId(sessionControl2, pid);
  if (FAILED(hr)) {
    log("Unable to get session process ID: 0x%lx\n", hr);
    goto Exit;
  }

  success = 1;

Exit:
  RELEASE(sessionControl2);
  return success;
}

i08 setProcessVolume(
  IAudioSessionEnumerator *enumerator,
  DWORD processId,
  float *prevVol,
  float targetVol
) {
  i32 sessionCount;
  HRESULT hr;
  IAudioSessionControl *sessionControl = NULL;
  ISimpleAudioVolume *sessionSimpleVolume = NULL;
  DWORD currentPid;
  f32 volume;
  i08 success = 0;

  if (!enumerator)
    return 0;

  // Get session count.
  hr = enumerator->lpVtbl->GetCount(enumerator, &sessionCount);
  if (FAILED(hr)) {
    log("Unable to get session count: 0x%lx\n", hr);
    goto Exit;
  }

  for (int i = 0; i < sessionCount; i++) {
    // Get audio session control
    hr = enumerator->lpVtbl->GetSession(enumerator, i, &sessionControl);
    if (FAILED(hr)) {
      log("Unable to get session control: 0x%lx\n", hr);
      goto Exit;
    }

    if (!getSessionPid(sessionControl, &currentPid) || currentPid != processId)
      // Get session pid failed or not the specified session.
      continue;

    // Get session volume control
    hr = sessionControl->lpVtbl->QueryInterface(
      sessionControl,
      &IID_ISimpleAudioVolume,
      (void **)&sessionSimpleVolume
    );
    if (FAILED(hr)) {
      log("Unable to get session volume controls: 0x%lx\n", hr);
      goto Exit;
    }

    // Get current volume of session
    hr = sessionSimpleVolume->lpVtbl->GetMasterVolume(
      sessionSimpleVolume,
      &volume
    );
    if (FAILED(hr)) {
      log("Unable to get session volume: 0x%lx\n", hr);
      goto Exit;
    }
    if (prevVol)
      *prevVol = volume;

    // Set session volume to max
    hr = sessionSimpleVolume->lpVtbl->SetMasterVolume(
      sessionSimpleVolume,
      targetVol,
      NULL
    );
    if (FAILED(hr)) {
      log("Unable to set session volume: 0x%lx\n", hr);
      goto Exit;
    }

    RELEASE(sessionSimpleVolume);
    RELEASE(sessionControl);
  }

  success = 1;
Exit:
  RELEASE(sessionControl);
  RELEASE(sessionSimpleVolume);
  return success;
}
