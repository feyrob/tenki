#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION 1

#include "stb_image.h"

#include <stdint.h>
#include <intrin.h>

#if SLOW
#define Assert(Expression) if(!(Expression)) {__debugbreak();}
#else
#define Assert(...)
#endif


#define InvalidCodePath Assert(0);

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;
typedef float f32;


#define KiB(Value) ((Value)*1024LL)
#define MiB(Value) (KiB(Value)*1024LL)

#define QUAD_BUFFER_SIZE 1024
#define QUAD_BUFFER_MAX 256


inline u32
RoundF32ToUint32(f32 Value)
{
	u32 Result = (u32)_mm_cvtss_si32(_mm_set_ss(Value));
	return Result;
}

typedef struct Game_Memory
{
	u64 persistent_memory_size;
	void* persistent_memory;
} Game_Memory;

typedef struct Vector2 {
	f32 x;
	f32 y;
} Vector2;

Vector2 operator+(Vector2 const& lhs, Vector2 const& rhs)
{
	Vector2 result;
	result.x = lhs.x + rhs.x;
	result.y = lhs.y + rhs.y;
	return result;
}

Vector2& operator+=(Vector2& lhs, Vector2 const& rhs)
{
	lhs.x = lhs.x + rhs.x;
	lhs.y = lhs.y + rhs.y;
	return lhs;
}

Vector2 operator-(Vector2 const& lhs, Vector2 const& rhs)
{
	Vector2 result;
	result.x = lhs.x - rhs.x;
	result.y = lhs.y - rhs.y;
	return result;
}

Vector2 operator*(Vector2 const& lhs, Vector2 const& rhs)
{
	Vector2 result;
	result.x = lhs.x * rhs.x;
	result.y = lhs.y * rhs.y;
	return result;
}

Vector2 operator*(Vector2 const& lhs, float const& rhs)
{
	Vector2 result;
	result.x = lhs.x * rhs;
	result.y = lhs.y * rhs;
	return result;
}

Vector2 operator*(float const& lhs, Vector2 const& rhs)
{
	Vector2 result;
	result.x = lhs * rhs.x;
	result.y = lhs * rhs.y;
	return result;
}

static Vector2 Rotate(Vector2 const& vec, float rotate)
{
	Vector2 result = vec;
	f32 st = sinf(rotate);
	f32 ct = cosf(rotate);

	result.x = vec.x * ct + vec.y * st;
	result.y = vec.x * -st + vec.y * ct;
	return result;
}

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
	void* data;
} texture_data;

typedef struct Texture {
	int width, height;
	u32 handle;
} Texture;

struct read_file_result
{
	u32 dataSize;
	void* data;
};

typedef read_file_result read_entire_file(char* filename);
typedef Texture load_texture(char* filename);
typedef texture_data load_texture_data(char* file_name);

typedef struct button_state
{
	s32 HalfTransitionCount;
	bool ended_down;
} button_state;

inline void * Copy(size_t size, void* sourceInit, void* destInit)
{
	u8* source = (u8*)sourceInit;
	u8* dest = (u8*)destInit;
	while (size--) { *dest++ = *source++; }

	return destInit;
}

//useful stuff

#ifndef USEFUL_SUMMERJAM_STUFF

static f32 clamp(f32 in, f32 clamp)
{
	float out = in;
	if (out < -clamp)
		out = -clamp;
	if (out > clamp)
		out = clamp;
	return out;
}

static s32 clamp_min_max(s32 min, s32 value, s32 max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

static f32 clampf_min_max(f32 min, f32 value, f32 max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

static f32 move_towards(f32 value, f32 target, f32 speed)
{
	if (value > target) return clampf_min_max(target, value - speed, value);
	if (value < target) return clampf_min_max(value, value + speed, target);
	return value;
}

static f32 lerp(f32 a, f32 t, f32 b)
{
	return (1.0f - t) * a + t * b;
}

static f32 abs(f32 in)
{
	if (in < 0)
		return -in;

	return in;
}

static f32 minf(f32 a, f32 b)
{
	return a < b ? a : b;
}

static f32 maxf(f32 a, f32 b)
{
	return a > b ? a : b;
}

static s32 maxint32(s32 a, s32 b)
{
	return a > b ? a : b;
}

static f32 round_down_to_dp(f32 num, int dp)
{
	u32 pow = 1;
	for (int i = 0; i < dp; i++)
	{
		pow = pow * 10;
	}
	int val = 0;
	if (num > 0.f) val = (int)(num * pow);
	else if (num < 0.f) val = (int)(num * pow) - 1;
	return ((f32)val) / pow;
}

static f32 round_up_to_dp(f32 num, int dp)
{
	u32 pow = 1;
	for (int i = 0; i < dp; i++)
	{
		pow = pow * 10;
	}
	int val = 0;
	if (num > 0.f) val = (int)(num * pow) + 1;
	else if (num < 0.f) val = (int)(num * pow);
	return ((f32)val) / pow;
}

static f32 round_to_dp(f32 num, int dp)
{
	u32 pow = 1;
	for (int i = 0; i < dp; i++)
	{
		pow = pow * 10;
	}
	int val = 0;
	if (num < 0.0f) val = (int)((num * pow) - .5f);
	else val = (int)((num * pow) + .5f);
	return ((f32)val) / pow;
}

inline void AppendCString(char* StartAt, const char* Text)
{
	while (*Text)
	{
		*StartAt++ = *Text++;
	}

	while (*StartAt)
	{
		*StartAt++ = 0;
	}
}


#define USEFUL_SUMMERJAM_STUFF
#endif //USEFUL_SUMMERJAM_STUFF

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

typedef struct QuadBuffer {
	u32 texture_handle;
	Quad quads[QUAD_BUFFER_SIZE];
	u32 quad_count = 0;
} QuadBuffer;

typedef struct RenderBuffer {
	Vector2 camera_pos;
	int buffer_count;
	QuadBuffer* quadBuffers[QUAD_BUFFER_MAX];
} RenderBuffer;

RenderBuffer global_render_buffer;

char ExeFilePath[MAX_PATH];

inline u32
SafeTruncateUInt64(u64 Value)
{
	Assert(Value <= 0xFFFFFFFF);
	u32 Result = (u32)Value;
	return(Result);
}

static void
win32_free_file_memory(void* Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

read_file_result win32_read_file(char* fileName)
{
	read_file_result result = {};

	if ((fileName[0] == '.') &&
		(fileName[1] == '.'))
	{
		char Path[MAX_PATH] = {};
		size_t PathLength = strlen(ExeFilePath);

		Copy(PathLength, ExeFilePath, Path);
		Path[PathLength] = '\\';
		AppendCString(Path + PathLength + 1, fileName);
		fileName = Path;
	}

	HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	Assert(fileHandle != INVALID_HANDLE_VALUE);

	LARGE_INTEGER FileSize;
	if (GetFileSizeEx(fileHandle, &FileSize))
	{
		u32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
		result.data = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (result.data)
		{
			DWORD BytesRead;
			if (ReadFile(fileHandle, result.data, FileSize32, &BytesRead, 0) &&
				(FileSize32 == BytesRead))
			{
				result.dataSize = FileSize32;
			}
			else
			{
				win32_free_file_memory(result.data);
				result.data = 0;
			}
		}
	}
	return result;
}

//read file and append null char
read_file_result win32_read_file_to_ntchar(char* fileName)
{
	read_file_result result = {};

	HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	Assert(fileHandle != INVALID_HANDLE_VALUE)

		LARGE_INTEGER FileSize;
	if (GetFileSizeEx(fileHandle, &FileSize))
	{
		u32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
		result.data = VirtualAlloc(0, FileSize32 + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (result.data)
		{
			DWORD BytesRead;
			if (ReadFile(fileHandle, result.data, FileSize32, &BytesRead, 0) &&
				(FileSize32 == BytesRead))
			{
				result.dataSize = FileSize32;
			}
			else
			{
				win32_free_file_memory(result.data);
				result.data = 0;
			}
		}
		char* text = (char*)result.data;
		text += sizeof(char) * FileSize32;
		*text = '\0';
	}

	return result;
}


//input

//win32_input.cpp

typedef DWORD x_input_get_state(DWORD dwUserIndex, XINPUT_STATE* pState);
static x_input_get_state* Win32XInputGetState;

static void Win32InitXInput()
{
	HMODULE Library = LoadLibraryA("xinput1_4.dll");
	if (Library)
	{
		Win32XInputGetState = (x_input_get_state*)GetProcAddress(Library, "XInputGetState");
		if (!Win32XInputGetState)
		{
			// Error loading XInputSetState
			InvalidCodePath;
		}
	}
	else
	{
		// Error xinput dll not found
		InvalidCodePath;
	}
}

static f32 Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZone)
{
	f32 Result = 0.0f;
	if (Value < -DeadZone)
	{
		Result = (Value + DeadZone) / (32767.0f - DeadZone);
	}
	else if (Value > DeadZone)
	{
		Result = (Value - DeadZone) / (32767.0f - DeadZone);
	}
	return Result;
}

static inline void Win32ProcessKeyboardButton(button_state* State, bool IsDown)
{
	State->ended_down = IsDown;
	++State->HalfTransitionCount;
}

static void Win32ProcessXInputButton(DWORD XInputButtonState,
	button_state* OldState, button_state* NewState,
	DWORD ButtonBit)
{
	NewState->ended_down = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->ended_down != NewState->ended_down) ? 1 : 0;
}

static inline bool is_button_pressed(DWORD buttons, DWORD ButtonBit)
{
	return (buttons & ButtonBit) == ButtonBit;
}

static void win32_button_check(XINPUT_GAMEPAD pad,
	XINPUT_GAMEPAD old_pad,
	button_state* buttonState,
	DWORD ButtonBit)
{
	if (is_button_pressed(pad.wButtons, ButtonBit))
	{
		buttonState->ended_down = true;
	}

	if ((is_button_pressed(pad.wButtons, ButtonBit)) != (is_button_pressed(old_pad.wButtons, ButtonBit)))
	{
		buttonState->HalfTransitionCount++;
		buttonState->ended_down = is_button_pressed(pad.wButtons, ButtonBit);
	}
}

static void get_gamepad_input(XINPUT_STATE& XInputState, XINPUT_STATE& Old_XInputState, Input_State& input_result)
{
	XINPUT_GAMEPAD* pad = &XInputState.Gamepad;
	XINPUT_GAMEPAD* old_pad = &Old_XInputState.Gamepad;

	f32 StickAverageX = Win32ProcessXInputStickValue(pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	f32 StickAverageY = Win32ProcessXInputStickValue(pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

	if (StickAverageX < 0.0f)
	{
		pad->wButtons = pad->wButtons | XINPUT_GAMEPAD_DPAD_LEFT;
	}

	if (StickAverageX > 0.0f)
	{
		pad->wButtons = pad->wButtons | XINPUT_GAMEPAD_DPAD_RIGHT;
	}

	if (StickAverageY > 0.0f)
	{
		pad->wButtons = pad->wButtons | XINPUT_GAMEPAD_DPAD_UP;
	}

	if (StickAverageY < 0.0f)
	{
		pad->wButtons = pad->wButtons | XINPUT_GAMEPAD_DPAD_DOWN;
	}

	win32_button_check(*pad, *old_pad, &input_result.MoveLeft, XINPUT_GAMEPAD_DPAD_LEFT);
	win32_button_check(*pad, *old_pad, &input_result.MoveRight, XINPUT_GAMEPAD_DPAD_RIGHT);
	win32_button_check(*pad, *old_pad, &input_result.MoveUp, XINPUT_GAMEPAD_DPAD_UP);
	win32_button_check(*pad, *old_pad, &input_result.MoveDown, XINPUT_GAMEPAD_DPAD_DOWN);

	win32_button_check(*pad, *old_pad, &input_result.ActionUp, XINPUT_GAMEPAD_Y);
	win32_button_check(*pad, *old_pad, &input_result.ActionLeft, XINPUT_GAMEPAD_X);
	win32_button_check(*pad, *old_pad, &input_result.ActionDown, XINPUT_GAMEPAD_A);
	win32_button_check(*pad, *old_pad, &input_result.ActionRight, XINPUT_GAMEPAD_B);
	win32_button_check(*pad, *old_pad, &input_result.LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER);
	win32_button_check(*pad, *old_pad, &input_result.RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER);
	win32_button_check(*pad, *old_pad, &input_result.Back, XINPUT_GAMEPAD_BACK);
	win32_button_check(*pad, *old_pad, &input_result.Start, XINPUT_GAMEPAD_START);
}

static void MatrixToIdentity(float mOut[4][4])
{
	mOut[0][0] = 1.0f;
	mOut[0][1] = 0.0f;
	mOut[0][2] = 0.0f;
	mOut[0][3] = 0.0f;

	mOut[1][0] = 0.0f;
	mOut[1][1] = 1.0f;
	mOut[1][2] = 0.0f;
	mOut[1][3] = 0.0f;

	mOut[2][0] = 0.0f;
	mOut[2][1] = 0.0f;
	mOut[2][2] = 1.0f;
	mOut[2][3] = 0.0f;

	mOut[3][0] = 0.0f;
	mOut[3][1] = 0.0f;
	mOut[3][2] = 0.0f;
	mOut[3][3] = 1.0f;
}

static void MatrixMul44(float m1[4][4], float m2[4][4], float mOut[4][4])
{
	mOut[0][0] = (m1[0][0] * m2[0][0]) + (m1[0][1] * m2[1][0]) + (m1[0][2] * m2[2][0]) + (m1[0][3] * m2[3][0]);
	mOut[0][1] = (m1[0][0] * m2[0][1]) + (m1[0][1] * m2[1][1]) + (m1[0][2] * m2[2][1]) + (m1[0][3] * m2[3][1]);
	mOut[0][2] = (m1[0][0] * m2[0][2]) + (m1[0][1] * m2[1][2]) + (m1[0][2] * m2[2][2]) + (m1[0][3] * m2[3][2]);
	mOut[0][3] = (m1[0][0] * m2[0][3]) + (m1[0][1] * m2[1][3]) + (m1[0][2] * m2[2][3]) + (m1[0][3] * m2[3][3]);

	mOut[1][0] = (m1[1][0] * m2[0][0]) + (m1[1][1] * m2[1][0]) + (m1[1][2] * m2[2][0]) + (m1[1][3] * m2[3][0]);
	mOut[1][1] = (m1[1][0] * m2[0][1]) + (m1[1][1] * m2[1][1]) + (m1[1][2] * m2[2][1]) + (m1[1][3] * m2[3][1]);
	mOut[1][2] = (m1[1][0] * m2[0][2]) + (m1[1][1] * m2[1][2]) + (m1[1][2] * m2[2][2]) + (m1[1][3] * m2[3][2]);
	mOut[1][3] = (m1[1][0] * m2[0][3]) + (m1[1][1] * m2[1][3]) + (m1[1][2] * m2[2][3]) + (m1[1][3] * m2[3][3]);

	mOut[2][0] = (m1[2][0] * m2[0][0]) + (m1[2][1] * m2[1][0]) + (m1[2][2] * m2[2][0]) + (m1[2][3] * m2[3][0]);
	mOut[2][1] = (m1[2][0] * m2[0][1]) + (m1[2][1] * m2[1][1]) + (m1[2][2] * m2[2][1]) + (m1[2][3] * m2[3][1]);
	mOut[2][2] = (m1[2][0] * m2[0][2]) + (m1[2][1] * m2[1][2]) + (m1[2][2] * m2[2][2]) + (m1[2][3] * m2[3][2]);
	mOut[2][3] = (m1[2][0] * m2[0][3]) + (m1[2][1] * m2[1][3]) + (m1[2][2] * m2[2][3]) + (m1[2][3] * m2[3][3]);

	mOut[3][0] = (m1[3][0] * m2[0][0]) + (m1[3][1] * m2[1][0]) + (m1[3][2] * m2[2][0]) + (m1[3][3] * m2[3][0]);
	mOut[3][1] = (m1[3][0] * m2[0][1]) + (m1[3][1] * m2[1][1]) + (m1[3][2] * m2[2][1]) + (m1[3][3] * m2[3][1]);
	mOut[3][2] = (m1[3][0] * m2[0][2]) + (m1[3][1] * m2[1][2]) + (m1[3][2] * m2[2][2]) + (m1[3][3] * m2[3][2]);
	mOut[3][3] = (m1[3][0] * m2[0][3]) + (m1[3][1] * m2[1][3]) + (m1[3][2] * m2[2][3]) + (m1[3][3] * m2[3][3]);
}

static void CreateQuaternion(float angleDeg, float x, float y, float z, float quat[4])
{
	float invMag = 1.0f / sqrtf((x * x) + (y * y) + (z * z));
	float sin = sinf(angleDeg / 2);
	quat[0] = (x * invMag) * sin;
	quat[1] = (y * invMag) * sin;
	quat[2] = (z * invMag) * sin;
	quat[3] = cosf(angleDeg / 2);
}

static void QuaternionToRotMatrix(float quat[4], float mat[4][4])
{
	float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	// calculate coefficients
	x2 = quat[0] + quat[0];
	y2 = quat[1] + quat[1];
	z2 = quat[2] + quat[2];
	xx = quat[0] * x2;
	xy = quat[0] * y2;
	xz = quat[0] * z2;
	yy = quat[1] * y2;
	yz = quat[1] * z2;
	zz = quat[2] * z2;
	wx = quat[3] * x2;
	wy = quat[3] * y2;
	wz = quat[3] * z2;

	mat[0][0] = 1.0f - (yy + zz);
	mat[1][0] = xy - wz;
	mat[2][0] = xz + wy;
	mat[3][0] = 0.0;

	mat[0][1] = xy + wz;
	mat[1][1] = 1.0f - (xx + zz);
	mat[2][1] = yz - wx;
	mat[3][1] = 0.0;

	mat[0][2] = xz - wy;
	mat[1][2] = yz + wx;
	mat[2][2] = 1.0f - (xx + yy);
	mat[3][2] = 0.0;

	mat[0][3] = 0;
	mat[1][3] = 0;
	mat[2][3] = 0;
	mat[3][3] = 1;
}

static void MatrixTranslate44(float x, float y, float z, float mat[4][4])
{
	mat[3][0] += x;
	mat[3][1] += y;
	mat[3][2] += z;
}

//open gl
#include <gl/gl.h>

#define WGL_CONTEXT_VERSION_ARB                 0X2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

#define GL_ARRAY_BUFFER             0x8892
#define GL_ELEMENT_ARRAY_BUFFER     0x8893
#define GL_STATIC_DRAW              0x88E4

//shader defines
#define GL_FRAGMENT_SHADER          0x8B30
#define GL_VERTEX_SHADER            0x8B31
#define GL_COMPILE_STATUS           0x8B81
#define GL_LINK_STATUS              0x8B82
#define GL_VALIDATE_STATUS          0x8B83

typedef HGLRC type_wglCreateContextAttribsARB(HDC hdc, HGLRC hShareContext, const int* attribsList);

typedef void type_glEnableVertexAttribArray(GLuint index);
typedef void type_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, u32 stride, const void* pointer);
typedef void type_glDisableVertexAttribArray(GLuint index);
typedef void type_glGenBuffers(u32 n, GLuint* buffers);
typedef void type_glBindBuffer(GLenum target, GLuint buffer);
typedef void type_glBufferData(GLenum target, u32 sizePtr, const void* data, GLenum usage);

//shader typedefs
typedef GLuint type_glCreateShader(GLenum shaderType);
typedef void type_glShaderSource(GLuint shader, u32 count, char** string, const GLint* length);
typedef void type_glCompileShader(GLuint shader);
typedef GLuint type_glCreateProgram();
typedef void type_glAttachShader(GLuint program, GLuint shader);
typedef void type_glLinkProgram(GLuint program);
typedef GLuint type_glGetUniformLocation(GLuint program, const char* name);
typedef void type_glUseProgram(GLuint program);
typedef void type_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void type_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void type_glGetShaderiv(GLuint shaderId, GLenum pname, GLint* params);

#define OpenGLGlobalFunctionPtr(Name) static type_##Name * Name;
#define OPEN_GL_GET_PROC_ADDRESS(Name) (type_##Name *)wglGetProcAddress(#Name);

OpenGLGlobalFunctionPtr(wglCreateContextAttribsARB);

OpenGLGlobalFunctionPtr(glEnableVertexAttribArray);
OpenGLGlobalFunctionPtr(glVertexAttribPointer);
OpenGLGlobalFunctionPtr(glDisableVertexAttribArray);
OpenGLGlobalFunctionPtr(glGenBuffers);
OpenGLGlobalFunctionPtr(glBindBuffer);
OpenGLGlobalFunctionPtr(glBufferData);

//shader function ptrs
OpenGLGlobalFunctionPtr(glCreateShader);
OpenGLGlobalFunctionPtr(glShaderSource);
OpenGLGlobalFunctionPtr(glCompileShader);
OpenGLGlobalFunctionPtr(glCreateProgram);
OpenGLGlobalFunctionPtr(glAttachShader);
OpenGLGlobalFunctionPtr(glLinkProgram);
OpenGLGlobalFunctionPtr(glGetUniformLocation);
OpenGLGlobalFunctionPtr(glUseProgram);
OpenGLGlobalFunctionPtr(glUniform4f);
OpenGLGlobalFunctionPtr(glUniformMatrix4fv);
OpenGLGlobalFunctionPtr(glGetShaderiv);

struct ogl_shader {
	GLuint program_id;
	GLuint MVP_location;
};

static u32 Global_Index_Buffer[QUAD_BUFFER_SIZE * 6];

static GLuint GlobalQuadBufferHandle;
static GLuint GlobalQuadIndexBufferHandle;

static GLuint WhiteTexHandle;
static GLuint RedTexHandle;
static GLuint BlueTexHandle;
static ogl_shader Global_tex_shader;
static ogl_shader Global_terrain_shader;

static float GlobalPerspectiveMatrix[4][4];

struct win32_window_dimensions
{
	int width;
	int height;
};

win32_window_dimensions GetWindowDimensions(HWND window)
{
	RECT clientRect;
	win32_window_dimensions dimensions;
	GetClientRect(window, &clientRect);

	dimensions.width = clientRect.right - clientRect.left;
	dimensions.height = clientRect.bottom - clientRect.top;
	return dimensions;
}

static void get_orthographic(float left, float right,
	float top, float bottom,
	float zNear, float zFar, float matrix[4][4])
{
	matrix[0][0] = 2 / (right - left);
	matrix[0][1] = 0;
	matrix[0][2] = 0;
	matrix[0][3] = 0;

	matrix[1][0] = 0;
	matrix[1][1] = 2 / (top - bottom);
	matrix[1][2] = 0;
	matrix[1][3] = 0;

	matrix[2][0] = 0;
	matrix[2][1] = 0;
	matrix[2][2] = -2 / (zFar - zNear);
	matrix[2][3] = 0;

	matrix[3][0] = -(right + left) / (right - left);
	matrix[3][1] = -(top + bottom) / (top - bottom);
	matrix[3][2] = -(zFar + zNear) / (zFar - zNear);
	matrix[3][3] = 1;
}

static void Win32GetOpenGlContext(HDC windowDC)
{
	PIXELFORMATDESCRIPTOR desiredPfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
		PFD_TYPE_RGBA,       // The kind of framebuffer. RGBA or palette.
		32,                  // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		8,
		0,
		0, 0, 0, 0,
		24,                  // Number of bits for the depthbuffer
		8,                   // Number of bits for the stencilbuffer
		0,                   // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	int pixelFormat = ChoosePixelFormat(windowDC, &desiredPfd);

	PIXELFORMATDESCRIPTOR actualPfd;
	DescribePixelFormat(windowDC, pixelFormat,
		sizeof(PIXELFORMATDESCRIPTOR),
		&actualPfd);
	SetPixelFormat(windowDC, pixelFormat, &actualPfd);

	HGLRC OpenGLRC = wglCreateContext(windowDC);
	HGLRC shareContext = 0;

	//get a default context
	if (wglMakeCurrent(windowDC, OpenGLRC))
	{
		//use the default context to try to get the real context
		wglCreateContextAttribsARB = OPEN_GL_GET_PROC_ADDRESS(wglCreateContextAttribsARB);
		if (wglCreateContextAttribsARB)
		{
			int attribsList[] =
			{
				WGL_CONTEXT_VERSION_ARB, 3,
				WGL_CONTEXT_MINOR_VERSION_ARB, 3,
				WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
				0,
			};
			HGLRC NewOpenGLRC = wglCreateContextAttribsARB(windowDC, shareContext, attribsList);
			if (NewOpenGLRC)
			{
				wglDeleteContext(OpenGLRC);
				OpenGLRC = NewOpenGLRC;
			}
		}
	}

	//assign the real context
	if (wglMakeCurrent(windowDC, OpenGLRC))
	{
		glEnableVertexAttribArray = OPEN_GL_GET_PROC_ADDRESS(glEnableVertexAttribArray);
		if (!glEnableVertexAttribArray)
		{
			OutputDebugString(L"glEnableVertexAttribArray() not found");
		}
		glVertexAttribPointer = OPEN_GL_GET_PROC_ADDRESS(glVertexAttribPointer);
		if (!glVertexAttribPointer)
		{
			OutputDebugString(L"glVertexAttribPointer() not found");
		}
		glDisableVertexAttribArray = OPEN_GL_GET_PROC_ADDRESS(glDisableVertexAttribArray);
		if (!glDisableVertexAttribArray)
		{
			OutputDebugString(L"glDisableVertexAttribArray() not found");
		}
		glGenBuffers = OPEN_GL_GET_PROC_ADDRESS(glGenBuffers);
		if (!glGenBuffers)
		{
			OutputDebugString(L"glGenBuffers() not found");
		}
		glBindBuffer = OPEN_GL_GET_PROC_ADDRESS(glBindBuffer);
		if (!glBindBuffer)
		{
			OutputDebugString(L"glBindBuffer() not found");
		}
		glBufferData = OPEN_GL_GET_PROC_ADDRESS(glBufferData);
		if (!glBufferData)
		{
			OutputDebugString(L"glBufferData() not found");
		}

		glCreateShader = OPEN_GL_GET_PROC_ADDRESS(glCreateShader);
		if (!glCreateShader)
		{
			OutputDebugString(L"glCreateShader() not found");
		}
		glShaderSource = OPEN_GL_GET_PROC_ADDRESS(glShaderSource);
		if (!glShaderSource)
		{
			OutputDebugString(L"glShaderSource() not found");
		}
		glCompileShader = OPEN_GL_GET_PROC_ADDRESS(glCompileShader);
		if (!glCompileShader)
		{
			OutputDebugString(L"glCompileShader() not found");
		}
		glCreateProgram = OPEN_GL_GET_PROC_ADDRESS(glCreateProgram);
		if (!glCreateProgram)
		{
			OutputDebugString(L"glCreateProgram() not found");
		}
		glAttachShader = OPEN_GL_GET_PROC_ADDRESS(glAttachShader);
		if (!glAttachShader)
		{
			OutputDebugString(L"glAttachShader() not found");
		}
		glLinkProgram = OPEN_GL_GET_PROC_ADDRESS(glLinkProgram);
		if (!glLinkProgram)
		{
			OutputDebugString(L"glLinkProgram() not found");
		}
		glGetUniformLocation = OPEN_GL_GET_PROC_ADDRESS(glGetUniformLocation);
		if (!glGetUniformLocation)
		{
			OutputDebugString(L"glGetUniformLocation() not found");
		}
		glUseProgram = OPEN_GL_GET_PROC_ADDRESS(glUseProgram);
		if (!glUseProgram)
		{
			OutputDebugString(L"glUseProgram() not found");
		}
		glUniform4f = OPEN_GL_GET_PROC_ADDRESS(glUniform4f);
		if (!glUniform4f)
		{
			OutputDebugString(L"glUniform4f() not found");
		}
		glUniformMatrix4fv = OPEN_GL_GET_PROC_ADDRESS(glUniformMatrix4fv);
		if (!glUniformMatrix4fv)
		{
			OutputDebugString(L"glUniformMatrix4fv() not found");
		}
		glGetShaderiv = OPEN_GL_GET_PROC_ADDRESS(glGetShaderiv);
		if (!glGetShaderiv)
		{
			OutputDebugString(L"glGetShaderiv() not found");
		}
	}
	else
	{
		//something went wrong
		OutputDebugString(L"wglMakeCurrent Failed");
	}
}

static void ogl_init_shaders()
{
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	char Path[MAX_PATH] = {};
	size_t PathLength = strlen(ExeFilePath);

	Copy(PathLength, ExeFilePath, Path);
	AppendCString(Path + PathLength, "\\vertexshader.vertexshader");
	read_file_result vsFile = win32_read_file_to_ntchar(Path);

	Copy(PathLength, ExeFilePath, Path);
	AppendCString(Path + PathLength, "\\fragmentshader.fragmentshader");
	read_file_result fsFile = win32_read_file_to_ntchar(Path);

	GLint success = 0;

	glShaderSource(vertexShaderId, 1, (char **)(&vsFile.data), NULL);
	glCompileShader(vertexShaderId);
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);

	glShaderSource(fragmentShaderId, 1, (char**)(&fsFile.data), NULL);
	glCompileShader(fragmentShaderId);
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);

	GLuint programId = glCreateProgram();
	Global_tex_shader.program_id = programId;
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);
	glLinkProgram(programId);

	Global_tex_shader.MVP_location = glGetUniformLocation(programId, "MVP");

	win32_free_file_memory(vsFile.data);
	win32_free_file_memory(fsFile.data);
}

static void ogl_init_quad_buffers()
{
	glGenBuffers(1, &GlobalQuadBufferHandle);
	glGenBuffers(1, &GlobalQuadIndexBufferHandle);

	//this never changes so it only needs to be set once at startup
	for (u32 i = 0; i < QUAD_BUFFER_SIZE; i++)
	{
		Global_Index_Buffer[(i * 6)] = (i * 4);
		Global_Index_Buffer[(i * 6) + 1] = (i * 4) + 1;
		Global_Index_Buffer[(i * 6) + 2] = (i * 4) + 2;
		Global_Index_Buffer[(i * 6) + 3] = (i * 4);
		Global_Index_Buffer[(i * 6) + 4] = (i * 4) + 2;
		Global_Index_Buffer[(i * 6) + 5] = (i * 4) + 3;
	}
}

static GLuint ogl_init_texture(u32 textureWidth, u32 textureHeight,
	u32 bytesPerPixel, void* textureBytes)
{
	GLuint TextureHandle;

	glGenTextures(1, &TextureHandle);

	glBindTexture(GL_TEXTURE_2D, TextureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0,
		GL_RGBA8, textureWidth, textureHeight,
		0, GL_RGBA, GL_UNSIGNED_BYTE,
		textureBytes);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	return TextureHandle;
}

static void open_gl_init(HWND window)
{
	HDC windowDC = GetDC(window);
	Win32GetOpenGlContext(windowDC);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);   // Enable depth testing for z-culling
	glDepthFunc(GL_LEQUAL);    // Set the type of depth-test

	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	win32_window_dimensions windowDimensions = GetWindowDimensions(window);

	float fW, fH;
	const GLdouble pi = 3.1415926535897932384626433832795;
	float aspect = (float)windowDimensions.width / (float)windowDimensions.height;
	float halfAspect = aspect / 2.0f;
	halfAspect *= 30.f;
	float zNear = -0.1f;
	float zFar = -100.0f;
	float fov = 45.0f;

	fH = (float)tan(fov / 360 * pi) * zNear;
	fW = fH * aspect;

	glViewport(0, 0, windowDimensions.width, windowDimensions.height);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	get_orthographic(-halfAspect, halfAspect, 15.0f, -15.f, zNear, zFar, GlobalPerspectiveMatrix);

	ogl_init_quad_buffers();
	ogl_init_shaders();

	ReleaseDC(window, windowDC);
}

static void draw_quads(QuadBuffer* buffer)
{
	QuadBuffer quad_buffer = *buffer;
	glBindTexture(GL_TEXTURE_2D, quad_buffer.texture_handle);

	//send buffer data to card
	glBindBuffer(GL_ARRAY_BUFFER, GlobalQuadBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Quad) * quad_buffer.quad_count, quad_buffer.quads, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GlobalQuadIndexBufferHandle);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (u32)(6 * sizeof(u32) * quad_buffer.quad_count),
		Global_Index_Buffer, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 36, (void*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 36, (void*)(12));
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 36, (void*)(20));
	glDrawElements(GL_TRIANGLES, 6 * quad_buffer.quad_count, GL_UNSIGNED_INT, (void*)(0));
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

static void win32_ogl_render(HDC device_context, RenderBuffer* buffer)
{
	glClearColor(0.4f, 0.6f, 0.9f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set up camera matrix
	float model_matrix[4][4] = {};
	float camera_matrix[4][4];
	float MVMatrix[4][4];
	float MVPMatrix[4][4];
	MatrixToIdentity(model_matrix);
	MatrixToIdentity(camera_matrix);
	
	MatrixTranslate44(-25.0f, -14.5f, 5.0f, camera_matrix);

	MatrixMul44(model_matrix, camera_matrix, MVMatrix);
	MatrixMul44(MVMatrix, GlobalPerspectiveMatrix, MVPMatrix);

	//shader stuff
	glUseProgram(Global_tex_shader.program_id);
	glUniformMatrix4fv(Global_tex_shader.MVP_location, 1, false, (GLfloat*)MVPMatrix);

	for (int i = 0; i < buffer->buffer_count; i++)
	{
		draw_quads(buffer->quadBuffers[i]);
	}

	SwapBuffers(device_context);
}

static void add_quad_to_render_buffer(Quad quad, u32 texture_handle)
{
	int index = -1;
	for (int i = 0; i < global_render_buffer.buffer_count; i++)
	{
		if (global_render_buffer.quadBuffers[i]->texture_handle == texture_handle)
		{
			index = i;
			break;
		}
	}

	if (index == -1)
	{
		Assert(global_render_buffer.buffer_count < QUAD_BUFFER_MAX);
		global_render_buffer.quadBuffers[global_render_buffer.buffer_count]
			= (QuadBuffer*)VirtualAlloc(0, sizeof(QuadBuffer), MEM_COMMIT, PAGE_READWRITE);
		global_render_buffer.quadBuffers[global_render_buffer.buffer_count]->texture_handle = texture_handle;
		index = global_render_buffer.buffer_count;
		global_render_buffer.buffer_count++;
	}

	QuadBuffer* quad_buffer = global_render_buffer.quadBuffers[index];
	Assert(quad_buffer->quad_count < QUAD_BUFFER_SIZE);
	quad_buffer->quads[quad_buffer->quad_count] = quad;
	quad_buffer->quad_count += 1;
}

static void SetCameraPosition(Vector2 cameraPos)
{
	global_render_buffer.camera_pos = cameraPos;
}

static void reset_quad_buffers(RenderBuffer* buffer)
{
	for (int i = 0; i < buffer->buffer_count; i++)
	{
		VirtualFree(buffer->quadBuffers[i], 0, MEM_RELEASE);
	}
	buffer->buffer_count = 0;
}

static bool keep_running = true;

LRESULT CALLBACK MainWindowCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch (uMsg)
	{
	case WM_SIZE:
	{
	} break;

	case WM_DESTROY:
	{
		keep_running = false;
	} break;

	case WM_CLOSE:
	{
		keep_running = false;
	} break;

	case WM_ACTIVATEAPP:
	{
	} break;

	case WM_PAINT:
	{
		//if we resize the window we can deal with that here
		PAINTSTRUCT paint;
		BeginPaint(hwnd, &paint);
		EndPaint(hwnd, &paint);
	} break;

	default:
	{
		result = DefWindowProc(hwnd, uMsg, wParam, lParam);
	} break;
	}
	return result;
}


static WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };

static void ToggleFullscreen(HWND Window)
{
	DWORD Style = GetWindowLong(Window, GWL_STYLE);
	if (Style & WS_OVERLAPPEDWINDOW)
	{
		MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
		if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
			GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
		{
			SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(Window, HWND_TOP,
				MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
				MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
				MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}
	else
	{
		SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(Window, &GlobalWindowPosition);
		SetWindowPos(Window, 0, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

static Texture load_image_with_stbi(char* file_name)
{
	int channels;
	Texture tex;

	void* texture_data = (void*)stbi_load(file_name, &tex.width, &tex.height, &channels, 0);
	tex.handle = ogl_init_texture(tex.width, tex.height, channels, texture_data);
	stbi_image_free(texture_data);

	return tex;
}

static texture_data get_texture_data(const char* file_name)
{
	char* base_path = ExeFilePath;
	char Path[MAX_PATH] = {};
	size_t PathLength = strlen(base_path);

	Copy(PathLength, base_path, Path);
	AppendCString(Path + PathLength, "\\");
	PathLength += 1;
	AppendCString(Path + PathLength, file_name);

	texture_data result;
	result.data = (void*)stbi_load(Path, &result.width, &result.height, &result.channels, 0);
	return result;
}

static Texture Get_Texture(const char* path)
{
	char* base_path = ExeFilePath;
	char Path[MAX_PATH] = {};
	size_t PathLength = strlen(base_path);

	Copy(PathLength, base_path, Path);
	AppendCString(Path + PathLength, "\\");
	PathLength += 1;
	AppendCString(Path + PathLength, path);

	return load_image_with_stbi(Path);
}

static void Win32ProcessPendingMessages(Input_State& input_result)
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		switch (message.message)
		{
		case WM_QUIT:
		{
			keep_running = false;
		} break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			u32 VKCode = (u32)message.wParam;
			
			bool AltKeyWasDown = (message.lParam & (1 << 29));
			bool WasDown = ((message.lParam & (1 << 30)) != 0);
			bool IsDown = ((message.lParam & (1 << 31)) == 0);
			if (WasDown != IsDown)
			{
				switch (VKCode)
				{
					case VK_F4: {
						if (IsDown && AltKeyWasDown) {
							keep_running = false;
						}
					}break;
					case VK_F11:
					{
						if (IsDown && message.hwnd)
						{
							ToggleFullscreen(message.hwnd);
						}
					
					} break;

					case 'W': { Win32ProcessKeyboardButton(&input_result.MoveUp, IsDown); } break;
					case 'A': { Win32ProcessKeyboardButton(&input_result.MoveLeft, IsDown); } break;
					case 'S': { Win32ProcessKeyboardButton(&input_result.MoveDown, IsDown); } break;
					case 'D': { Win32ProcessKeyboardButton(&input_result.MoveRight, IsDown); } break;
					case 'Q': { Win32ProcessKeyboardButton(&input_result.LeftShoulder, IsDown); } break;
					case 'E': { Win32ProcessKeyboardButton(&input_result.RightShoulder, IsDown); } break;
					case VK_UP: { Win32ProcessKeyboardButton(&input_result.ActionUp, IsDown); } break;
					case VK_LEFT: { Win32ProcessKeyboardButton(&input_result.ActionLeft, IsDown); } break;
					case VK_RIGHT: { Win32ProcessKeyboardButton(&input_result.ActionRight, IsDown); } break;
					case VK_ESCAPE: { Win32ProcessKeyboardButton(&input_result.Back, IsDown); } break;
					case VK_SPACE: { Win32ProcessKeyboardButton(&input_result.ActionDown, IsDown); } break;
				}
			}
		} break;

		default:
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		} break;
		}
	}
}

# define M_PI           3.14159265358979323846  /* pi */

struct Gameplay_Data;

typedef Input_State Update_Bot(Gameplay_Data data, int player_number);

typedef struct Entity {
	Vector2 pos;
	Vector2 velocity;
	f32 rotation;
	f32 width;
	f32 height;
	Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
	bool is_active = true;
	int health = 100;
} Entity;

typedef struct Tank {
	Entity ent;
	float fire_timer;
} Tank;

#define NUM_BLOCKS_MAP 1000
#define NUM_BULLETS 50

typedef struct Gameplay_Data {
	Tank player1 = {};
	Tank player2 = {};
	Entity blocks[NUM_BLOCKS_MAP];
	Entity bullets[NUM_BULLETS];
	int block_count;
	Vector2 Camera_Pos = {};
	Vector2 starting_pos;
	Texture tank_texture;
	Texture turret_texture;
	Texture block_texture;
	Texture bullet_texture;
	texture_data map_tex;
	Update_Bot* update_player1_func;
	Update_Bot* update_player2_func;
} Gameplay_Data;


static f32 GlobalFrequencyCounter;

inline f32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	f32 Result = ((f32)(End.QuadPart - Start.QuadPart) /
		(f32)GlobalFrequencyCounter);
	return Result;
}

inline LARGE_INTEGER
Win32GetWallClock()
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return Result;
}

static bool WasPressed(button_state State)
{
	bool Result = ((State.HalfTransitionCount > 1) ||
		(State.HalfTransitionCount == 1) && State.ended_down);
	return Result;
}

#define PLAYER_GROUND_ACCELERATION 0.8f
#define PLAYER_DRAG 3.0f
#define MAX_VELOCITY 0.8f

Entity screen_space;

static void InitGameObjecets(Game_Memory* memory)
{
	Gameplay_Data* data = (Gameplay_Data*)memory->persistent_memory;

	data->player1.ent.width = 1.0f;
	data->player1.ent.height = 1.0f;
	data->player1.ent.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	data->player1.ent.pos = { 5.f, 14.f };
	data->player1.ent.health = 100;
	data->player1.ent.is_active = true;
	data->player1.fire_timer = 0.0f;

	data->player2.ent.width = 1.0f;
	data->player2.ent.height = 1.0f;
	data->player2.ent.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	data->player2.ent.pos = { 45.f, 14.f };
	data->player2.ent.health = 100;
	data->player2.ent.is_active = true;
	data->player2.fire_timer = 0.0f;

	for (int i = 0; i < NUM_BULLETS; i++)
	{
		data->bullets[i].width = 0.55f;
		data->bullets[i].height = 0.55f;
		data->bullets[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
	}

	int count = 0;
	u8* cursor = (u8*)data->map_tex.data;
	for (int i = 0; i < data->map_tex.height; i++)
	{
		for (int j = 0; j < data->map_tex.width; j++)
		{
			u8 r, g, b = 0;
			r = *cursor++;
			g = *(cursor++);
			b = *(cursor++);

			if (r == 0 && g == 0 && b == 0)
			{
				data->blocks[count].pos = { (f32)j, (f32)i };
				data->blocks[count].width = 1.00f;
				data->blocks[count].height = 1.00f;
				data->blocks[count].color = { 1.0f, 1.0f, 1.0f, 1.0f };
				data->blocks[count].is_active = true;
				data->blocks[count].health = 200;
				count++;
			}
		}
	}
	data->block_count = count;
	data->Camera_Pos = { 24.0f, 14.5f };

	screen_space.pos = { 24.0f, 14.5f };
	screen_space.width = 55.0f;
	screen_space.height = 35.0f;
}

static Quad make_quad(f32 pos_x, f32 pos_y, f32 width, f32 height, float rotation = 0.f, Color color = { 1.0f, 1.0f, 1.0f, 1.0f },
	bool flip_x = false)
{
	Quad result;

	float st = sinf(rotation);
	float ct = cosf(rotation);

	f32 hw = width / 2;
	f32 hh = height / 2;

	f32 uvl = 0.0f;
	f32 uvr = 1.0f;
	f32 uvt = 0.0f;
	f32 uvb = 1.0f;

	if (flip_x)
	{
		f32 temp = uvl;
		uvl = uvr;
		uvr = temp;
	}

	result.verts[0] = { { (-hw * ct + hh * st) + pos_x, (-hw * -st + hh * ct) + pos_y, 0.0f }, { uvl, uvt }, color };//0 - lt
	result.verts[1] = { { (hw * ct + hh * st) + pos_x, (hw * -st + hh * ct) + pos_y, 0.0f }, { uvr, uvt }, color };//1 - rt
	result.verts[2] = { { (hw * ct + -hh * st) + pos_x, (hw * -st + -hh * ct) + pos_y, 0.0f }, { uvr, uvb }, color };//2 - rb
	result.verts[3] = { { (-hw * ct + -hh * st) + pos_x, (-hw * -st + -hh * ct) + pos_y, 0.0f }, { uvl, uvb }, color };//3 - lb

	return result;
}

static Quad make_quad_from_entity(Entity entity)
{
	return make_quad(entity.pos.x, entity.pos.y,
		entity.width, entity.height, entity.rotation, entity.color);
}


typedef struct rect
{
	f32 left;
	f32 right;
	f32 top;
	f32 bottom;
} rect;

typedef struct Collision
{
	f32 time;
	Vector2 normal;
} Collision;

typedef struct Penetration
{
	Vector2 depth;
	Vector2 normal;
} Penetration;

static rect GetEntityRect(Entity ent)
{
	rect r;

	f32 half_width = ent.width / 2.0f;
	f32 half_height = ent.height / 2.0f;

	r.left = ent.pos.x - half_width;
	r.right = ent.pos.x + half_width;
	r.top = ent.pos.y + half_height;
	r.bottom = ent.pos.y - half_height;

	return r;
}

static rect GetExpandedRect(Entity ent, f32 halfWidth, f32 halfHeight, f32 modifier = 0.0f)
{
	rect r = GetEntityRect(ent);

	r.left = r.left - halfWidth - modifier;
	r.right = r.right + halfWidth + modifier;
	r.top = r.top + halfHeight + modifier;
	r.bottom = r.bottom - halfHeight - modifier;

	return r;
}

#define COLLISION_EPSILON 0.00001f

static bool Is_Penetration(Entity e1, Entity e2, Penetration& pen)
{
	if (!e2.is_active) {
		return false;
	}

	f32 offset_x = e1.pos.x - e2.pos.x;
	f32 offset_y = e1.pos.y - e2.pos.y;

	f32 offset_mag_x = abs(offset_x);
	f32 offset_mag_y = abs(offset_y);

	f32 combined_half_width = (e1.width / 2.f) + (e2.width / 2.f);
	f32 combined_half_height = (e1.height / 2.f) + (e2.height / 2.f);

	if (((combined_half_width - offset_mag_x) >= COLLISION_EPSILON) &&
		((combined_half_height - offset_mag_y) >= COLLISION_EPSILON))
	{
		pen.depth.x = combined_half_width - offset_mag_x;
		pen.depth.y = combined_half_height - offset_mag_y;

		//move apart in the axis of least penetration
		if (pen.depth.x <= pen.depth.y)
		{
			if (offset_x > 0)
				pen.normal = { 1, 0 };
			else
				pen.normal = { -1, 0 };
		}
		else
		{
			if (offset_y > 0)
				pen.normal = { 0, 1 };
			else
				pen.normal = { 0, -1 };
		}

		return true;
	}
	return false;
}

static int get_worst_pen_index(Penetration pens[], int pen_count)
{
	float pen_depth = 0.0f;
	int index = -1;
	for (int i = 0; i < pen_count; i++)
	{
		if (pens[i].depth.x >= pen_depth || pens[i].depth.y >= pen_depth)
		{
			pen_depth = maxf(pens[i].depth.x, pens[i].depth.y);
			index = i;
		}
	}

	return index;
}

static bool Is_Collision(Entity e1, Entity e2, Collision& col, f32 dt)
{
	if (!e2.is_active) {
		return false;
	}

	rect r = GetExpandedRect(e2, e1.width / 2, e1.height / 2, -0.01f);

	//check for obvious misses
	if (e1.velocity.x == 0.0f &&
		(e1.pos.x <= r.left || e1.pos.x >= r.right))
		return false;

	if (e1.velocity.y == 0.0f &&
		(e1.pos.y <= r.bottom || e1.pos.y >= r.top))
		return false;

	f32 left_offset = (r.left - e1.pos.x);
	f32 right_offset = (r.right - e1.pos.x);
	f32 top_offset = (r.top - e1.pos.y);
	f32 bottom_offset = (r.bottom - e1.pos.y);

	f32 leftIntercept = left_offset / (e1.velocity.x * dt);
	f32 rightIntercept = right_offset / (e1.velocity.x * dt);
	f32 topIntercept = top_offset / (e1.velocity.y * dt);
	f32 bottomIntercept = bottom_offset / (e1.velocity.y * dt);

	f32 x1, x2, y1, y2;

	if (leftIntercept < rightIntercept)
	{
		x1 = leftIntercept;
		x2 = rightIntercept;
	}
	else
	{
		x2 = leftIntercept;
		x1 = rightIntercept;
	}

	if (topIntercept < bottomIntercept)
	{
		y1 = topIntercept;
		y2 = bottomIntercept;
	}
	else
	{
		y2 = topIntercept;
		y1 = bottomIntercept;
	}

	if (x1 > y2 || y1 > x2) return false;

	f32 c1 = maxf(x1, y1);
	if (c1 < 0.0f || c1 >= 1.0f) return false;
	f32 c2 = minf(x2, y2);
	if (c2 < 0) return false;

	col.time = c1;

	if (x1 > y1)
	{
		if (e1.velocity.x < 0)
			col.normal = { 1, 0 };
		else
			col.normal = { -1, 0 };
	}
	else if (x1 < y1)
	{
		if (e1.velocity.y < 0)
			col.normal = { 0, 1 };
		else
			col.normal = { 0, -1 };
	}

	return true;
}

static void resolve_swept_collisions_with_terrain(Entity* ent, Collision cols[], int num_cols)
{
	for (int i = 0; i < num_cols; i++)
	{
		Collision col = cols[i];

		ent->velocity.x += abs(ent->velocity.x)
			* col.normal.x * (1 - col.time);
		ent->velocity.y += abs(ent->velocity.y)
			* col.normal.y * (1 - col.time);
	}
}

static bool Is_Penetration_Naive(Entity e1, Entity e2)
{
	rect r1 = GetExpandedRect(e1, e2.width / 2.f, e2.height / 2.f);

	if (e2.pos.x > r1.left && e2.pos.x < r1.right &&
		e2.pos.y > r1.bottom && e2.pos.y < r1.top)
	{
		return true;
	}
	return false;
}

bool is_point_collision(Entity ent, Vector2 pos)
{
	rect r1 = GetEntityRect(ent);

	if (pos.x > r1.left && pos.x < r1.right &&
		pos.y > r1.bottom && pos.y < r1.top)
	{
		return true;
	}
	return false;
}


void set_button(button_state& button, bool isPressed = true)
{
	if (isPressed)
	{
		button.ended_down = true;
		button.HalfTransitionCount = 1;
	}
	else
	{
		button.ended_down = false;
		button.HalfTransitionCount = 0;
	}
}

bool is_in_block(Gameplay_Data* data, Vector2 point)
{
	for (int i = 0; i < data->block_count; i++)
	{
		if (data->blocks[i].is_active && is_point_collision(data->blocks[i], point)) return true;
	}
	return false;
}

f32 get_bearing(const Vector2 from, const Vector2 to)
{
	Vector2 dir = from - to;
	//atan2 expects y-axis to be down not up or vice versa. Which ever way it is it's the other way to this game.
	f32 bearing = atan2f(-dir.y, dir.x);
	if (bearing <= (-M_PI / 2)) {
		bearing += 270.0f * ((f32)M_PI / 180.0f);
	} else bearing -= 90.0f * ((f32)M_PI / 180.0f); //in Atan2 0 is in the direction of the positive X-axis. In this game 0 degrees is positive Y-axis.

	return bearing;
}

void GetTankPointers(Tank** player, Tank** enemy, Gameplay_Data* data, int player_number) {
	if (player_number == 1)
	{
		*player = &(data->player1);
		*enemy = &(data->player2);
	}
	else
	{
		*player = &data->player2;
		*enemy = &data->player1;
	}
}

Input_State UpdateBot(Gameplay_Data data, int player_number)
{
	Input_State input_state = {};
	Tank* player;
	Tank* enemy;
	GetTankPointers(&player, &enemy, &data, player_number);
	return input_state;
}

#define DRAG_FACTOR 2.0f
#define TANK_SPEED 10.0f
#define TANK_ROTATION_SPEED 0.015f
#define TURRET_ROTATION_SPEED 0.03f
#define FIRE_COOLDOWN 0.5f

void update_player(Gameplay_Data* data, Tank* tank, Input_State Input, f32 dt)
{
	if (!tank->ent.is_active) return;

	if (tank->fire_timer >= 0.0f)
		tank->fire_timer -= dt;

	Entity* player = &tank->ent;
	Vector2 acceleration = {};

	if (Input.MoveLeft.ended_down)
	{
		player->rotation -= TANK_ROTATION_SPEED;
	}
	else if (Input.MoveRight.ended_down)
	{
		player->rotation += TANK_ROTATION_SPEED;
	}

	if (player->rotation >= M_PI)
		player->rotation -= 2 * M_PI;
	if (player->rotation < -M_PI)
		player->rotation += 2 * M_PI;

	Vector2 north = { 0.f, 1.f };
	Vector2 forward = Rotate(north, player->rotation);

	if (Input.MoveDown.ended_down)
	{
		acceleration = -TANK_SPEED * forward;
	}
	else if (Input.MoveUp.ended_down)
	{
		acceleration = TANK_SPEED * forward;
	}
	else acceleration = { 0.0f, 0.f };

	f32 dragX = -DRAG_FACTOR * player->velocity.x;
	acceleration.x += dragX;

	f32 dragY = -DRAG_FACTOR * player->velocity.y;
	acceleration.y += dragY;

	player->velocity.x += acceleration.x * dt;
	player->velocity.y += acceleration.y * dt;
		
	if (Input.ActionDown.ended_down && tank->fire_timer <= 0.0f)
	{
		//fire bullet
		for (int i = 0; i < NUM_BULLETS; i++)
		{
			if (data->bullets[i].is_active) continue;

			data->bullets[i].is_active = true;
			data->bullets[i].pos = player->pos;
			Vector2 up = { 0.f, 1.f };
			Vector2 bearing = Rotate(up, player->rotation);
			data->bullets[i].pos += bearing * 1.2f;
			data->bullets[i].velocity = (bearing * 15.0f);
			data->bullets[i].velocity += player->velocity;
			break;
		}

		tank->fire_timer += FIRE_COOLDOWN;
	}

	//sweapt phase collision detection - adjust velocity to prevent collision
	Collision collision = {};
	Collision cols[NUM_BLOCKS_MAP];
	int num_cols = 0;
	for (int i = 0; i < NUM_BLOCKS_MAP; i++)
	{
		if (Is_Collision(*player, data->blocks[i], collision, dt))
		{
			cols[num_cols] = collision;
			num_cols++;
		}
	}
	resolve_swept_collisions_with_terrain(player, cols, num_cols);

	//update character position	
	player->pos.x += player->velocity.x * dt;
	player->pos.y += player->velocity.y * dt;

	//penetration phase collision detection
	Penetration pen;
	Penetration pens[20];
	int num_pens = 0;
	for (int i = 0; i < NUM_BLOCKS_MAP; i++)
	{
		if (Is_Penetration(*player, data->blocks[i], pen))
		{
			pens[num_pens] = pen;
			num_pens++;
		}
	}

	int worst_pen_index = get_worst_pen_index(pens, num_pens);
	int loop_count = 0;
	while (worst_pen_index != -1)
	{
		pen = pens[worst_pen_index];
		player->pos = player->pos + (pen.depth * pen.normal);
		if (pen.normal.x == 1)
		{
			player->velocity.x = maxf(player->velocity.x, 0.0f);
		}
		else if (pen.normal.x == -1)
		{
			player->velocity.x = minf(player->velocity.x, 0.0f);
		}

		if (pen.normal.y == 1)
		{
			player->velocity.y = maxf(player->velocity.y, 0.0f);
		}
		else if (pen.normal.y == -1)
		{
			player->velocity.y = minf(player->velocity.y, 0.0f);
		}

		num_pens = 0;
		for (u32 i = 0; i < NUM_BLOCKS_MAP; i++)
		{
			if (Is_Penetration(*player, data->blocks[i], pen))
			{
				pens[num_pens] = pen;
				num_pens++;
			}
		}
		worst_pen_index = get_worst_pen_index(pens, num_pens);
		loop_count++;

		if (loop_count >= 100)
		{
			player->pos.y += 0.1f;
			break;
		}
	}

	if (player->health <= 0)
		player->is_active = false;
}

void UpdateGamePlay(Gameplay_Data* data, Input_State Input, Input_State Input2, f32 dt)
{
	
	update_player(data, &data->player1, Input, dt);
	update_player(data, &data->player2, Input2, dt);

	//update bullets
	for (int i = 0; i < NUM_BULLETS; i++)
	{
		if (!data->bullets[i].is_active) continue;
		for (int j = 0; j < data->block_count; j++)
		{
			if (Is_Penetration_Naive(data->bullets[i], data->blocks[j]) && data->blocks[j].is_active)
			{
				data->bullets[i].is_active = false;
				data->blocks[j].health -= 10;
				if (data->blocks[j].health <= 0)
					data->blocks[j].is_active = false;
			}

			if (Is_Penetration_Naive(data->bullets[i], data->player1.ent) && data->player1.ent.is_active)
			{
				data->bullets[i].is_active = false;
				data->player1.ent.health -= 10;
			}

			if (Is_Penetration_Naive(data->bullets[i], data->player2.ent) && data->player2.ent.is_active)
			{
				data->bullets[i].is_active = false;
				data->player2.ent.health -= 10;
			}

			if (!Is_Penetration_Naive(data->bullets[i], screen_space))
				data->bullets[i].is_active = false;
		}

		data->bullets[i].pos += (data->bullets[i].velocity * dt);
	}
}

void RenderGameplay(Gameplay_Data* data)
{
	

	if (data->player1.ent.is_active)
	{
		add_quad_to_render_buffer(make_quad_from_entity(data->player1.ent), data->tank_texture.handle);
		add_quad_to_render_buffer(
			make_quad(
				data->player1.ent.pos.x, 
				data->player1.ent.pos.y, 
				1.0f, 
				1.0f,
				data->player1.ent.rotation
			), 
			data->turret_texture.handle
		);
	}
	if (data->player2.ent.is_active)
	{
		add_quad_to_render_buffer(make_quad_from_entity(data->player2.ent), data->tank_texture.handle);
		add_quad_to_render_buffer(
			make_quad(
				data->player2.ent.pos.x, 
				data->player2.ent.pos.y, 
				1.0f, 
				1.0f,
				data->player2.ent.rotation
			), 
			data->turret_texture.handle
		);
	}
	for (int i = 0; i < data->block_count; i++)
	{
		if (!data->blocks[i].is_active) continue;
		add_quad_to_render_buffer(make_quad_from_entity(data->blocks[i]), data->block_texture.handle);
	}

	for (int i = 0; i < NUM_BULLETS; i++)
	{
		if (!data->bullets[i].is_active) continue;
		add_quad_to_render_buffer(make_quad_from_entity(data->bullets[i]), data->bullet_texture.handle);
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
	Win32InitXInput();

	LARGE_INTEGER FrequencyCounterLarge;
	QueryPerformanceFrequency(&FrequencyCounterLarge);
	GlobalFrequencyCounter = (f32)FrequencyCounterLarge.QuadPart;

	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);


	WNDCLASS windowClass = {};

	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = MainWindowCallback;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = L"SummerjamWindow";

	if (!RegisterClass(&windowClass))
		return 0;

	HWND window = CreateWindowEx(0, windowClass.lpszClassName,
		L"Summer Jam Building Blocks Handmade",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		0, 0, hInstance, 0);

	if (!window)
	{
		return(0);
	}

	ToggleFullscreen(window);

	Game_Memory memory = {};
	memory.persistent_memory_size = MiB(16);
	memory.persistent_memory = VirtualAlloc(0, memory.persistent_memory_size,
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	char Filename[MAX_PATH] = {};
	GetModuleFileNameA(0, Filename, MAX_PATH);
	size_t Length = strlen(Filename);

	char* At = Filename;
	if (*At == '\"')
	{
		++At;
		Length -= 2;
	}
	char* lastSlash = At + Length;
	while (*lastSlash != '\\')
	{
		--Length;
		--lastSlash;
	}

	Copy(Length, At, ExeFilePath);
	ExeFilePath[Length] = '\0';

	open_gl_init(window);

	HDC windowDC = GetDC(window);
	LARGE_INTEGER LastPerformanceCounter = Win32GetWallClock();

	XINPUT_STATE XInputState = {};
	XINPUT_STATE Old_XInputState = {};
	Input_State input_state = {};
	Input_State input_state2 = {};

	// NOTE: Otherwise Sleep will be ignored for requests less than 50? citation needed
	UINT MinSleepPeriod = 1;
	f32 TargetSeconds = 1.0f / 60.0f;


	Gameplay_Data* data = (Gameplay_Data*)(memory.persistent_memory);

	data->tank_texture = Get_Texture("tank_base.png");
	data->turret_texture = Get_Texture("tank_turret.png");
	data->block_texture = Get_Texture("block.png");
	data->map_tex = get_texture_data("map.bmp");
	data->bullet_texture = Get_Texture("bullet.png");

	InitGameObjecets(&memory);

	while (keep_running)
	{
		//input handling
		for (int i = 0; i < NUM_BUTTONS; i++)
		{
			input_state.Buttons[i].HalfTransitionCount = 0;
		}
		Win32ProcessPendingMessages(input_state);
		Old_XInputState = XInputState;
		DWORD xinput_result = Win32XInputGetState(0, &XInputState);
		if (xinput_result == ERROR_SUCCESS)
		{
			get_gamepad_input(XInputState, Old_XInputState, input_state);
		}
		Gameplay_Data* data = (Gameplay_Data*)memory.persistent_memory;
		
		input_state2 = UpdateBot(*data, 2);

		UpdateGamePlay(data, input_state, input_state2, TargetSeconds);
		RenderGameplay(data);

		win32_ogl_render(windowDC, &global_render_buffer);
		reset_quad_buffers(&global_render_buffer);

		f32 FrameSeconds = Win32GetSecondsElapsed(LastPerformanceCounter, Win32GetWallClock());

		if (FrameSeconds < TargetSeconds)
		{
			DWORD Miliseconds = (DWORD)(1000.0f * (TargetSeconds - FrameSeconds));
			if (Miliseconds > 0)
			{
				Sleep(Miliseconds);
			}

			FrameSeconds = Win32GetSecondsElapsed(LastPerformanceCounter, Win32GetWallClock());
			while (FrameSeconds < TargetSeconds)
			{
				FrameSeconds = Win32GetSecondsElapsed(LastPerformanceCounter, Win32GetWallClock());
				_mm_pause();
			}
		}

		LastPerformanceCounter = Win32GetWallClock();
	}
	ReleaseDC(window, windowDC);

    return 0;
}