//win32_sound.cpp

#include "win32_sound.h"

void Win32ClearSoundBuffer(game_sound_buffer *SoundBuffer, LPDIRECTSOUNDBUFFER win32soundbuffer)
{
    void *Region1;
    DWORD Region1Size;
    void *Region2;
    DWORD Region2Size;
    
    if(SUCCEEDED(win32soundbuffer->Lock(0, SoundBuffer->SizeInBytes, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
    {
        int16 *At = (int16 *)Region1;
        DWORD Region1SampleCount = Region1Size/SoundBuffer->BytesPerSample;
        for(DWORD Index = 0; Index < Region1SampleCount; ++Index)
        {
            *At++ = 0;
            *At++ = 0;
        }
        
        At = (int16 *)Region2;
        DWORD Region2SampleCount = Region2Size/SoundBuffer->BytesPerSample;
        for(DWORD Index = 0; Index < Region2SampleCount; ++Index)
        {
            *At++ = 0;
            *At++ = 0;
        }
        
        if(!SUCCEEDED(win32soundbuffer->Unlock(Region1, Region1Size, Region2, Region2Size)))
        {
            // TODO(kstandbridge): Failed to unlock sound buffer
            InvalidCodePath;
        }
    }
    else
    {
        // TODO(kstandbridge): Failed to lock sound buffer
        InvalidCodePath;
    }
}

void Win32FillSoundBuffer(game_sound_buffer *SoundBuffer, LPDIRECTSOUNDBUFFER win32soundbuffer, DWORD ByteToLock, DWORD BytesToWrite)
{
    void *Region1;
    DWORD Region1Size;
    void *Region2;
    DWORD Region2Size;
    
    if(SUCCEEDED(win32soundbuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
    {
        int16 *Dest = (int16 *)Region1;
        int16 *Source = SoundBuffer->Samples;
        DWORD Region1SampleCount = Region1Size/SoundBuffer->BytesPerSample;
        for(DWORD Index = 0; Index < Region1SampleCount; ++Index)
        {
            *Dest++ = *Source++;
            *Dest++ = *Source++;
            
            ++SoundBuffer->RunningSampleIndex;
        }
        
        Dest = (int16 *)Region2;
        DWORD Region2SampleCount = Region2Size/SoundBuffer->BytesPerSample;
        for(DWORD Index = 0; Index < Region2SampleCount; ++Index)
        {
            *Dest++ = *Source++;
            *Dest++ = *Source++;
            
            ++SoundBuffer->RunningSampleIndex;
        }
        
        if(!SUCCEEDED(win32soundbuffer->Unlock(Region1, Region1Size, Region2, Region2Size)))
        {
            // TODO(kstandbridge): Failed to unlock sound buffer
            InvalidCodePath;
        }
    }
    else
    {
        // TODO(kstandbridge): Failed to lock sound buffer
        InvalidCodePath;
    }
}

void Win32UpdateAudioThread(void *Data, LPDIRECTSOUNDBUFFER win32soundbuffer)
{
    Win32_State *Win32State = (Win32_State *)Data;
    game_sound_buffer *SoundBuffer = &Win32State->SoundBuffer;
    
    f32 TargetDeltaTime = 0.0166f;
    
	DWORD SafetyBytes = (DWORD)((f32)(SoundBuffer->SamplesPerSecond*SoundBuffer->BytesPerSample)*TargetDeltaTime*1); 
	SafetyBytes -= SafetyBytes % SoundBuffer->BytesPerSample;
	
	DWORD PlayCursor, WriteCursor;
    win32soundbuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
	
	DWORD ByteToLock = (SoundBuffer->RunningSampleIndex*SoundBuffer->BytesPerSample) % SoundBuffer->SizeInBytes;
	
	DWORD ExpectedBytesPerTick = (DWORD)((f32)(SoundBuffer->SamplesPerSecond*SoundBuffer->BytesPerSample)*TargetDeltaTime); 
	ExpectedBytesPerTick -= ExpectedBytesPerTick % SoundBuffer->BytesPerSample;
	DWORD ExpectedBoundaryByte = PlayCursor + ExpectedBytesPerTick;
	
	DWORD SafeWriteBuffer = WriteCursor;
	if(SafeWriteBuffer < PlayCursor)
	{
		SafeWriteBuffer += SoundBuffer->SizeInBytes;
	}
	else
	{
		SafeWriteBuffer += SafetyBytes;
	}
	
	DWORD TargetCursor;
	if(SafeWriteBuffer < ExpectedBoundaryByte)
	{
		TargetCursor = ExpectedBoundaryByte + ExpectedBytesPerTick;
	}
	else
	{
		TargetCursor = WriteCursor + ExpectedBytesPerTick + SafetyBytes;
	}
	TargetCursor %= SoundBuffer->SizeInBytes;
        
	DWORD BytesToWrite;
	if(ByteToLock > TargetCursor)
	{
		BytesToWrite = SoundBuffer->SizeInBytes - ByteToLock + TargetCursor;
	}
	else
	{
		BytesToWrite = TargetCursor - ByteToLock;
	}
	
	if(BytesToWrite)
	{
		SoundBuffer->SamplesToWrite = BytesToWrite/SoundBuffer->BytesPerSample;
		Assert(BytesToWrite % 4 == 0);
		
		UpdateGameAudio(&Win32State->SoundBuffer, TargetDeltaTime);
				
		Win32FillSoundBuffer(SoundBuffer, win32soundbuffer, ByteToLock, BytesToWrite);
	}
}