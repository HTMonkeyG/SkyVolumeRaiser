#define COBJMACROS

#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

#include "svr.h"

// COM release function.
#define svrComRelease(v) (((v) != NULL) ? ((v)->lpVtbl->Release(v), (v) = NULL) : NULL)

static DWORD svrGetSessionPid(
  IAudioSessionControl *pSessionControl
) {
  HRESULT hr;
  IAudioSessionControl2 *pSessionControl2 = NULL;
  DWORD result = PID_ILLEGAL
    , pid;

  if (!pSessionControl)
    return PID_ILLEGAL;

  hr = IAudioSessionControl_QueryInterface(
    pSessionControl,
    &IID_IAudioSessionControl2,
    (void **)&pSessionControl2);
  if (FAILED(hr))
    goto Err;

  hr = IAudioSessionControl2_GetProcessId(
    pSessionControl2,
    &pid);
  if (FAILED(hr))
    goto Err;

  result = pid;

Err:
  svrComRelease(pSessionControl2);
  return result;
}

i32 svrGetProcessVolume(
  DWORD pid,
  f32 *volume
) {
  i32 result = 0
    , sessionCount;
  HRESULT hr = S_OK;
  IMMDeviceEnumerator *pEnumerator = NULL;
  IMMDevice *pDevice = NULL;
  IAudioSessionManager2 *pSessionManager2 = NULL;
  IAudioSessionEnumerator *pSessionEnumerator = NULL;
  IAudioSessionControl *pSessionControl = NULL;
  ISimpleAudioVolume *pSimpleVolume = NULL;

  hr = CoInitializeEx(
    NULL,
    COINIT_APARTMENTTHREADED);
  if (FAILED(hr))
    goto Err;

  hr = CoCreateInstance(
    &CLSID_MMDeviceEnumerator,
    NULL,
    CLSCTX_ALL,
    &IID_IMMDeviceEnumerator,
    (void **)&pEnumerator);
  if (FAILED(hr))
    goto Err;

  hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(
    pEnumerator,
    eRender,
    eConsole,
    &pDevice);
  if (FAILED(hr))
    goto Err;

  hr = IMMDevice_Activate(
    pDevice,
    &IID_IAudioSessionManager2,
    CLSCTX_INPROC_SERVER,
    NULL,
    (void **)&pSessionManager2);
  if (FAILED(hr))
    goto Err;

  hr = IAudioSessionManager2_GetSessionEnumerator(
    pSessionManager2,
    &pSessionEnumerator);
  if (FAILED(hr))
    goto Err;

  hr = IAudioSessionEnumerator_GetCount(
    pSessionEnumerator,
    &sessionCount);
  if (FAILED(hr))
    goto Err;

  for (i32 i = 0; i < sessionCount; i++) {
    hr = IAudioSessionEnumerator_GetSession(
      pSessionEnumerator,
      i,
      &pSessionControl);
    if (FAILED(hr) || svrGetSessionPid(pSessionControl) != pid)
      goto Next;

    hr = IAudioSessionControl_QueryInterface(
      pSessionControl,
      &IID_ISimpleAudioVolume,
      (void **)&pSimpleVolume);
    if (FAILED(hr))
      goto Next;

    hr = ISimpleAudioVolume_GetMasterVolume(
      pSimpleVolume,
      volume);
    if (FAILED(hr))
      goto Next;

    // Break the loop.
    i = sessionCount;
    result = 1;

Next:
    svrComRelease(pSimpleVolume);
    svrComRelease(pSessionControl);
  }

Err:
  svrComRelease(pSessionEnumerator);
  svrComRelease(pSessionManager2);
  svrComRelease(pDevice);
  svrComRelease(pEnumerator);

  CoUninitialize();

  return result;
}
