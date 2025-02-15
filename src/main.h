#include <initguid.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <strsafe.h>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include "runas.h"
#include "audio.h"

#define WM_APP1 0x8001