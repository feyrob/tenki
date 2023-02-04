//summerjam.h
#pragma once

#ifndef SUMMERJAM_H

#include <stdint.h>
#include <intrin.h>
#include <windows.h>
#include <math.h>

#define Assert(Expression) if(!(Expression)) {__debugbreak();}

#define InvalidDefaultCase default: { Assert(0); }
#define InvalidCodePath Assert(0);

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef float f32;
typedef int32 bool32;

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define QUAD_BUFFER_SIZE 1024
#define QUAD_BUFFER_MAX 256

inline uint32
RoundF32ToUint32(f32 Value)
{
    uint32 Result = (uint32)_mm_cvtss_si32(_mm_set_ss(Value));
    return Result;
}

typedef struct Game_Memory
{
	uint64 persistent_memory_size;
	void * persistent_memory;
} Game_Memory;

typedef union Color
{
	f32 color[4];
	struct
	{
		f32 r, g, b, a;
	} compontents;
} Color;

typedef struct Vertex {
	float xyz[3];
	float uv[2];
	Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
} Vertex;

typedef struct Quad {
	Vertex verts[4];
} Quad;

typedef struct texture_data {
	int width, height, channels;
	void * data;
} texture_data;

typedef struct Texture {
	int width, height;
	uint32 handle;
} Texture;

struct read_file_result
{
	uint32 dataSize;
	void* data;
};

typedef read_file_result read_entire_file(char *filename);
typedef Texture load_texture(char *filename);
typedef texture_data load_texture_data(char * file_name);

typedef struct button_state
{
	int32 HalfTransitionCount;
	bool ended_down;
} button_state;

typedef struct RenderBuffer;

typedef void platform_add_quad_to_render_buffer(Quad quad, uint32 texture_handle, RenderBuffer &renderbuffer);

typedef struct
{
	platform_add_quad_to_render_buffer * AddQuadToRenderBuffer;
	read_entire_file *ReadEntireFile;    
	load_texture *LoadTexture;
	load_texture_data *LoadTextureData;
	bool32 QuitRequested;
} platform_api;

inline void *
	Copy(size_t size, void *sourceInit, void *destInit)
{
	uint8 *source = (uint8 *)sourceInit;
	uint8 *dest = (uint8 *)destInit;
	while(size--) {*dest++ = *source++;}
    
	return destInit;
}

#define NUM_BUTTONS 12
typedef struct Input_State
{    
	union
	{
		button_state Buttons[NUM_BUTTONS];
		struct
		{
			button_state MoveUp;
			button_state MoveDown;
			button_state MoveLeft;
			button_state MoveRight;
            
			button_state ActionUp;
			button_state ActionDown;
			button_state ActionLeft;
			button_state ActionRight;
            
			button_state LeftShoulder;
			button_state RightShoulder;
            
			button_state Back;
			button_state Start;
		};
	};
} controller_input;

//todo(shutton) - probably make the quad buffer variable size at runtime
typedef struct QuadBuffer {
	uint32 texture_handle;
	Quad quads[QUAD_BUFFER_SIZE];
	uint32 quad_count = 0;
} QuadBuffer;

typedef struct RenderBuffer {
	int buffer_count;
	QuadBuffer *quadBuffers[QUAD_BUFFER_MAX];
} RenderBuffer;

#include <dsound.h>

static f32 GlobalFrequencyCounter;

typedef struct game_sound_buffer
{
	int32 SizeInBytes;
	int32 ChannelCount;
	int32 SamplesPerSecond;
	int32 BytesPerSample;
	int32 RunningSampleIndex;

	int16* Samples;
	int32 SamplesToWrite;
} game_sound_buffer;

typedef struct Win32_State
{
	game_sound_buffer SoundBuffer;

	char ExeFilePath[MAX_PATH];
	char DllFullFilePath[MAX_PATH];
	char TempDllFullFilePath[MAX_PATH];
	char LockFullFilePath[MAX_PATH];
	char BotDll1FilePath[MAX_PATH];
	char BotDll2FilePath[MAX_PATH];
	FILETIME LastDLLWriteTime;
	HMODULE AppLibrary;
} Win32_State;

static Win32_State win32State_;;

#define SUMMERJAM_H
#endif //SUMMERJAM_H