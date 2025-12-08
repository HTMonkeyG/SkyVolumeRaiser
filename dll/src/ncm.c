#include <windows.h>
#include "MinHook.h"

#include "svr.h"

typedef u64 (__fastcall *PFN_CNCMAudioPlayer_BufferFill)(
  void *, void *, int *, u64 *);
typedef u64 (__fastcall *PFN_Wasapi_SetMasterVolume)(
  void *, f32);

// Hard-coded offset.
static const u64 off_WasApi_SetMasterVolume = 0x000531E0ull;
static const u64 off_CNCMAudioPlayer_BufferFill = 0x0002DCA0ull;

static PFN_CNCMAudioPlayer_BufferFill fn_CNCMAudioPlayer_BufferFill = NULL;
static PFN_Wasapi_SetMasterVolume fn_Wasapi_SetMasterVolume = NULL;

static u64 hook_CNCMAudioPlayer_BufferFill(
  void *a1,
  void *a2,
  int *a3,
  u64 *a4
) {
  u64 result = fn_CNCMAudioPlayer_BufferFill(
    a1,
    a2,
    a3,
    a4);
  f32 volume = *(f32 *)((char *)a1 + 616) * *(f64 *)((char *)a1 + 608);

  // FIXME: Support more audio formats except f32.
  i32 element = *a3 / 4;
  for (i32 i = 0; i < element; i++)
    ((f32 *)a2)[i] *= volume;

  return result;
}

static u64 hook_WasApi_SetMasterVolume(
  void *a1,
  f32 a2
) {
  f32 volume = 1.0f;

  // Ignore all SetMasterVolume requests.
  if (!svrGetProcessVolume(GetCurrentProcessId(), &volume))
    volume = 1.0f;

  return fn_Wasapi_SetMasterVolume(a1, volume);
}

void svrInstallHooks() {
  if (!hDllNcmAudioPlayer)
    return;

  MH_CreateHook(
    (char *)hDllNcmAudioPlayer + off_CNCMAudioPlayer_BufferFill,
    (void *)hook_CNCMAudioPlayer_BufferFill,
    (void **)&fn_CNCMAudioPlayer_BufferFill);
  MH_CreateHook(
    (char *)hDllNcmAudioPlayer + off_WasApi_SetMasterVolume,
    (void *)hook_WasApi_SetMasterVolume,
    (void **)&fn_Wasapi_SetMasterVolume);
  MH_EnableHook(MH_ALL_HOOKS);
}

void svrRemoveHooks() {
  MH_RemoveHook(hDllNcmAudioPlayer + off_CNCMAudioPlayer_BufferFill);
  MH_RemoveHook(hDllNcmAudioPlayer + off_WasApi_SetMasterVolume);
}