#include <initguid.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <strsafe.h>

typedef enum {
  false = 0,
  true = 1
} __boolean;

#define bool __boolean

#define RELEASE(v) ((v != NULL) && (v->lpVtbl->Release(v), v = NULL))
#define clamp(x, a, b) (x > a ? x < b ? x : b : a)

bool getDefaultDevice(IMMDevice **device);
bool getAudioSessionEnumerator(IMMDevice *device, IAudioSessionEnumerator **sessionEnumerator);
bool getSessionName(IAudioSessionControl *sessionControl, wchar_t *name);
bool setSessionVolume(IAudioSessionEnumerator *enumerator, const wchar_t *sessionName, float *prevVol, float targetVol);