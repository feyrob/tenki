#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION 1
#include "includes/stb_image.h"
#include "summerjam.h"
#include "audio.h"
#include "game.h"

static platform_api Platform;
static bool GlobalRunning = true;

#include "useful_summerjam.cpp"
#include "summer_opengl.h"
#include "win32_summer.h"
#include "win32_sound.h"
#include "win32_input.cpp"

LRESULT CALLBACK MainWindowCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	switch(uMsg)
	{
		case WM_SIZE: 
		{
		} break;
		
		case WM_DESTROY: 
		{
			GlobalRunning = false;
		} break;
		
		case WM_CLOSE: 
		{
			GlobalRunning = false;
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

static WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};

static void ToggleFullscreen(HWND Window)
{
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if(Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
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

static void Win32ProcessPendingMessages(Input_State &input_result)
{
	MSG message;
	while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{	
		switch(message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;
			
			case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
				uint32 VKCode = (uint32)message.wParam;
                bool32 AltKeyWasDown = (message.lParam & (1 << 29));
                bool32 WasDown = ((message.lParam & (1 << 30)) != 0);
                bool32 IsDown = ((message.lParam & (1 << 31)) == 0);
                if(WasDown != IsDown)
                {
                    switch(VKCode)
                    {
                        case VK_F4:
                        {
                            if(IsDown && AltKeyWasDown)
                            {
                                GlobalRunning = false;
                            }
                        } break;
                        case VK_RETURN:
                        {
                            if(IsDown && AltKeyWasDown)
                            {
                                if(message.hwnd)
                                {
                                    ToggleFullscreen(message.hwnd);
                                }
                            }
                        } break;
                        
						case 'W':       { Win32ProcessKeyboardButton(&input_result.MoveUp, IsDown); } break;
						case 'A':       { Win32ProcessKeyboardButton(&input_result.MoveLeft, IsDown); } break;
						case 'S':       { Win32ProcessKeyboardButton(&input_result.MoveDown, IsDown); } break;
						case 'D':       { Win32ProcessKeyboardButton(&input_result.MoveRight, IsDown); } break;
						case 'Q':       { Win32ProcessKeyboardButton(&input_result.LeftShoulder, IsDown); } break;
						case 'E':       { Win32ProcessKeyboardButton(&input_result.RightShoulder, IsDown); } break;
						case VK_UP:     { Win32ProcessKeyboardButton(&input_result.ActionUp, IsDown); } break;
						case VK_LEFT:   { Win32ProcessKeyboardButton(&input_result.ActionLeft, IsDown); } break;
						case VK_RIGHT:  { Win32ProcessKeyboardButton(&input_result.ActionRight, IsDown); } break;
                        case VK_ESCAPE: { Win32ProcessKeyboardButton(&input_result.Back, IsDown); } break;
						case VK_SPACE:  { Win32ProcessKeyboardButton(&input_result.ActionDown, IsDown); } break;
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

static Texture load_image_with_stbi(char * file_name)
{	
	int channels;
	Texture tex;
    
	void * texture_data = (void *)stbi_load(file_name, &tex.width, &tex.height, &channels, 0);
	tex.handle = ogl_init_texture(tex.width, tex.height, channels, texture_data);
	stbi_image_free(texture_data);
    
	return tex;
}

static texture_data get_texture_data(char * file_name)
{
	char* base_path = win32State_.ExeFilePath;
	char Path[MAX_PATH] = {};
	size_t PathLength = strlen(base_path);

	Copy(PathLength, base_path, Path);
	AppendCString(Path + PathLength, "\\");
	PathLength += 1;
	AppendCString(Path + PathLength, file_name);

	texture_data result;
	result.data = (void *)stbi_load(Path, &result.width, &result.height, &result.channels, 3);
	return result;
}

static Texture Get_Texture(char * path)
{
	char * base_path = win32State_.ExeFilePath;
	char Path[MAX_PATH] = {};
	size_t PathLength = strlen(base_path);

	Copy(PathLength, base_path, Path);
	AppendCString(Path + PathLength, "\\");
	PathLength += 1;
	AppendCString(Path + PathLength, path);

	return load_image_with_stbi(Path);
}

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

int CALLBACK WinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPSTR lpCmdLine,
					 int nCmdShow)
{
    Win32InitXInput();
    
    LARGE_INTEGER FrequencyCounterLarge;
    QueryPerformanceFrequency(&FrequencyCounterLarge);
    GlobalFrequencyCounter = (f32)FrequencyCounterLarge.QuadPart;
    
    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);

	Platform.ReadEntireFile = win32_read_file;
	Platform.LoadTexture = Get_Texture;
	Platform.LoadTextureData = get_texture_data;
	Platform.AddQuadToRenderBuffer = add_quad_to_render_buffer;
    
    WNDCLASS windowClass = {};
	
	windowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	windowClass.lpfnWndProc = MainWindowCallback;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = L"SummerjamWindow";
	
	if (!RegisterClass(&windowClass))
		return 0;
    
	HWND window = CreateWindowEx(0, windowClass.lpszClassName,
                                 L"Summer Jam Building Blocks Handmade",
                                 WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 0, 0, hInstance, 0);
    
	if(!window)
	{
		//todo - some error handling goes here
		return(0);
	}
    
    ToggleFullscreen(window);
    
    Game_Memory memory = {};
	memory.persistent_memory_size = Megabytes(16);
	memory.persistent_memory = VirtualAlloc(0, memory.persistent_memory_size,
                                            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);    
	
    Win32_State *win32State = &win32State_;    
    Win32GetFilePaths(win32State);    
	open_gl_init(win32State, window);
		
	HDC windowDC = GetDC(window);
    LARGE_INTEGER LastPerformanceCounter = Win32GetWallClock();
	LPDIRECTSOUNDBUFFER Win32SoundBuffer;
	RenderBuffer render_buffer = {};
    
    Win32InitAudio(&win32State->SoundBuffer, &Win32SoundBuffer, window);
    Win32ClearSoundBuffer(&win32State->SoundBuffer, Win32SoundBuffer);
    if(!SUCCEEDED(Win32SoundBuffer->Play(0, 0, DSBPLAY_LOOPING)))
    {
        InvalidCodePath;
    }
    win32State->SoundBuffer.Samples = (int16 *)VirtualAlloc(0, win32State->SoundBuffer.SizeInBytes, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    
	XINPUT_STATE XInputState = {};
	XINPUT_STATE Old_XInputState = {};
	Input_State input_state = {};
	Input_State input_state2 = {};
    
    f32 TargetSeconds = 1.0f / 60.0f;

	HMODULE player1 = LoadLibraryA(win32State->BotDll1FilePath);
	HMODULE player2 = LoadLibraryA(win32State->BotDll2FilePath);

	if (player1)
	{
		Gameplay_Data * data = (Gameplay_Data *)memory.persistent_memory;
		data->update_player1_func = (Update_Bot *)GetProcAddress(player1, "UpdateBot");
	}

	if (player2)
	{
		Gameplay_Data * data = (Gameplay_Data *)memory.persistent_memory;
		data->update_player2_func = (Update_Bot *)GetProcAddress(player2, "UpdateBot");
	}
	    
    while(GlobalRunning)
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
		Gameplay_Data * data = (Gameplay_Data *)memory.persistent_memory;
		if (data->update_player1_func)
		{
			input_state = data->update_player1_func(*data, 1);
		}
		
		if (data->update_player2_func)
		{
			input_state2 = data->update_player2_func(*data, 2);
		}

		UpdateGamePlay(&Platform, &memory, input_state, input_state2, TargetSeconds);
		RenderGameplay(&Platform, &memory, render_buffer);

		win32_ogl_render(windowDC, &render_buffer);
		reset_quad_buffers(&render_buffer);
        
		Win32UpdateAudioThread(win32State, Win32SoundBuffer);
        
        if(Platform.QuitRequested)
        {
            break;
        }
        
        f32 FrameSeconds = Win32GetSecondsElapsed(LastPerformanceCounter, Win32GetWallClock());
        
        if(FrameSeconds < TargetSeconds)
        {
            DWORD Miliseconds = (DWORD)(1000.0f * (TargetSeconds - FrameSeconds));
            if(Miliseconds > 0)
            {
                Sleep(Miliseconds);
            }
            
            FrameSeconds = Win32GetSecondsElapsed(LastPerformanceCounter, Win32GetWallClock());            
            while(FrameSeconds < TargetSeconds)
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