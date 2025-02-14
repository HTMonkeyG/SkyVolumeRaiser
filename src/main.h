#include <initguid.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <strsafe.h>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include "runas.h"

const char *SESSION_STATES[] = { "Inactive", "Active", "Expired" };

typedef enum {
  false = 0,
  true = 1
} __boolean;

#define bool __boolean

#define RELEASE(v) ((v != NULL) && (v->lpVtbl->Release(v), v = NULL))
#define clamp(x, a, b) (x > a ? x < b ? x : b : a)

#define WM_APP1 0x8001