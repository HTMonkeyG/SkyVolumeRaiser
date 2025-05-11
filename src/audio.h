#include <initguid.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <strsafe.h>
#include <wchar.h>

#include "macros.h"

#define RELEASE(v) (((v) != NULL) ? ((v)->lpVtbl->Release(v), (v) = NULL) : NULL)
#define clamp(x, a, b) ((x) > (a) ? (x) < (b) ? (x) : (b) : (a))

i08 getDefaultDevice(IMMDevice **device);
i08 getAudioSessionEnumerator(
  IMMDevice *device,
  IAudioSessionEnumerator **sessionEnumerator
);
i08 getSessionPid(IAudioSessionControl *sessionControl, DWORD *pid);
i08 setProcessVolume(
  IAudioSessionEnumerator *enumerator,
  DWORD processId,
  float *prevVol,
  float targetVol
);
