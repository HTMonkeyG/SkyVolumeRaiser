#ifndef __SVR_H__
#define __SVR_H__

// Comment this line if want to disable duplicate instance check.
#define svrDuplicateCheck

// Type aliases.
#define i8 char
#define i08 char
#define u8 unsigned char
#define u08 unsigned char
#define i16 short
#define u16 unsigned short
#define i32 int
#define u32 unsigned int
#define i64 long long
#define u64 unsigned long long
#define f32 float
#define f64 double
#define nil void

// COM release function.
#define svrComRelease(v) (((v) != NULL) ? ((v)->lpVtbl->Release(v), (v) = NULL) : NULL)

extern HMODULE hDllNcmAudioPlayer;

void svrInstallHooks();
void svrRemoveHooks();

f32 svrGetMajorVolume(
  i32 *mute);

#endif
