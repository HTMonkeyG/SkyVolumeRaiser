#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include "audio.h"
#include "macros.h"
#include "hotkey.h"
#include "config.h"

#define MBError(text, type) (MessageBoxW(NULL, text, L"Error", MB_ICONERROR | type))

#define WM_USER_EXIT (0x8000 + 1)
#define PID_INVALID (0xFFFFFFFF)
#define GAME_PROC_NAME (L"Sky.exe")
#define GAME_WND_NAME (L"光·遇")
