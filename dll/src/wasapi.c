#define COBJMACROS

#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

#include "svr.h"

f32 svrGetMajorVolume(
  i32 *mute
) {
  HRESULT hr = S_OK;
  IMMDeviceEnumerator *pEnumerator = NULL;
  IMMDevice *pDevice = NULL;
  IAudioEndpointVolume *pEndpointVol = NULL;
  f32 result = 0.0f;

  hr = CoInitializeEx(
    NULL,
    COINIT_APARTMENTTHREADED);
  hr = CoCreateInstance(
    &CLSID_MMDeviceEnumerator,
    NULL,
    CLSCTX_ALL,
    &IID_IMMDeviceEnumerator,
    (void **)&pEnumerator);

  hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(
    pEnumerator,
    eRender,
    eConsole,
    &pDevice);

  hr = IMMDevice_Activate(
    pDevice,
    &IID_IAudioEndpointVolume,
    CLSCTX_ALL,
    NULL,
    (void **)&pEndpointVol);

  hr = IAudioEndpointVolume_GetMasterVolumeLevelScalar(
    pEndpointVol,
    &result);

  if (mute)
    hr = IAudioEndpointVolume_GetMute(
      pEndpointVol,
      mute);

  svrComRelease(pEndpointVol);
  svrComRelease(pDevice);
  svrComRelease(pEnumerator);

  CoUninitialize();

  return hr;
}
