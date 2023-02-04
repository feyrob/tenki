#pragma once
//win32_sound.cpp

#include "summerjam.h"
#include "audio.h"
#include <windows.h>

void Win32ClearSoundBuffer(game_sound_buffer* SoundBuffer, LPDIRECTSOUNDBUFFER win32soundbuffer);

void Win32FillSoundBuffer(game_sound_buffer* SoundBuffer, LPDIRECTSOUNDBUFFER win32soundbuffer, DWORD ByteToLock, DWORD BytesToWrite);

void Win32UpdateAudioThread(void* Data, LPDIRECTSOUNDBUFFER win32soundbuffer);