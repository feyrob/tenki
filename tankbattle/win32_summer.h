#pragma once

#include "useful_summerjam.cpp"
#include "summerjam.h"
#include "audio.h"
#include <dsound.h>

inline uint32
SafeTruncateUInt64(uint64 Value);

void
win32_free_file_memory(void* Memory);

read_file_result win32_read_file(char* fileName);

//read file and append null char
read_file_result win32_read_file_to_ntchar(char* fileName);

typedef HRESULT WINAPI direct_sound_create(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter);

void Win32InitAudio(game_sound_buffer* SoundBuffer, LPDIRECTSOUNDBUFFER* win32soundbuffer, HWND Window);

FILETIME
Win32GetLastWriteTime(char* filename);

void
Win32GetFilePaths(Win32_State* win32State);