#pragma once

#include <vector>
#include <Windows.h>

#pragma comment(lib, "Winmm")

class AudioRecorder
{
	HWAVEIN audioDeviceHandle;

	WAVEFORMATEX audioDataFormat;

	// 44100 Hz = 44.1 KHz
	static const DWORD SAMPLING_RATE = 44100;

	// this much is exactly 1 second of audio
	static const DWORD AUDIO_DATA_BUFFER_LENGTH_FOR_1_SECOND_OF_RECORDING = 176400;

	std::vector<BYTE> getAudioFileContent(void* audioDataBuffer, DWORD audioDataBufferSize);

public:
	AudioRecorder();

	~AudioRecorder();

	// records from `duration` seconds and returns raw contents of the resulting .wav file
	std::vector<BYTE> record(DWORD duration);
};

