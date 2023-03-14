#include "AudioRecorder.h"

#include <string>
#include <stdexcept>

// - wFormatTag - For one- or two-channel PCM data, this value should be WAVE_FORMAT_PCM.
// - nChannels - Number of channels in the waveform - audio data.
// Monaural data uses one channel and stereo data uses two channels.
// Monaural (mono) - chyba nieprzestrzenny a stereo to chyba przestrzenny
// - nSamplesPerSec - Sample rate, in samples per second(hertz).If wFormatTag is
// WAVE_FORMAT_PCM, then common values for nSamplesPerSec are 8.0 kHz, 11.025 kHz, 22.05 kHz, and 44.1 kHz
// - nAvgBytesPerSec - Required average data - transfer rate, in bytes per second, for the format tag.
// If wFormatTag is WAVE_FORMAT_PCM, nAvgBytesPerSec should be equal to the product of nSamplesPerSec and nBlockAlign. // product - iloczyn => nSamplesPerSec * nBlockAlign
// - nBlockAlign - Block alignment, in bytes.The block alignment is the minimum atomic unit of data for
// the wFormatTag format type. If wFormatTag is WAVE_FORMAT_PCM or WAVE_FORMAT_EXTENSIBLE,
// nBlockAlign must be equal to the product of nChannels and wBitsPerSample divided by 8 (bits per byte). => (nChannels * wBitsPerSample) / 8
// Software must process a multiple of nBlockAlign bytes of data at a time. Data written to and
// read from a device must always start at the beginning of a block. For example,
// it is illegal to start playback of PCM data in the middle of a sample (that is, on a non-block-aligned boundary).
// - wBitsPerSample - Bits per sample for the wFormatTag format type. If wFormatTag is WAVE_FORMAT_PCM,
// then wBitsPerSample should be equal to 8 or 16.
// - cbSize - Size, in bytes, of extra format information appended to the end of the WAVEFORMATEX structure.
// For WAVE_FORMAT_PCM formats (and only WAVE_FORMAT_PCM formats), this member is ignored.
/*
	44.1 kHz is the current playback standard for most applications. There are specialty services like Tidal that use higher sample rates but whether
	it’s Youtube, Spotify, a CD or almost any other digital media – most of the music and audio you hear is played back at 44.1 kHz.

	For most people and most applications, 44.1 kHz is the best sample rate to go for. Higher sample rates can have advantages for
	professional music and audio work, but many professionals also work at 44.1 kHz. Using higher sample rates can have disadvantages and
	should only be considered in professional applications.

	For consumer applications, a bit depth of 16 bits is perfectly fine. For professional use (recording, mixing, mastering or professional video editing)
	a bit depth of 24 bits is good. This ensures a good dynamic range and better precision when editing.
*/
AudioRecorder::AudioRecorder():
	audioDeviceHandle(NULL)
{
	audioDataFormat.wFormatTag = WAVE_FORMAT_PCM;
	audioDataFormat.nChannels = 2;
	audioDataFormat.nSamplesPerSec = SAMPLING_RATE; // Other common sampling rates are 48 KHz, 96 KHz, 192KHz, 8KHz and anywhere in between.
	audioDataFormat.wBitsPerSample = 16;
	audioDataFormat.nBlockAlign = audioDataFormat.nChannels * audioDataFormat.wBitsPerSample / 8;
	audioDataFormat.nAvgBytesPerSec = audioDataFormat.nSamplesPerSec * audioDataFormat.nBlockAlign;
	audioDataFormat.cbSize = NULL;

	waveInOpen(&audioDeviceHandle, WAVE_MAPPER, &audioDataFormat, NULL, NULL, CALLBACK_NULL);
}

AudioRecorder::~AudioRecorder()
{
	// The waveInClose function closes the given waveform-audio input device.
	// If there are input buffers that have been sent with the waveInAddBuffer function
	// and that haven't been returned to the application, the close operation will fail.
	// Call the waveInReset function to mark all pending buffers as done.
	// If the function succeeds, the handle is no longer valid after this call.
	
	waveInReset(audioDeviceHandle);

	waveInClose(audioDeviceHandle);
}

std::vector<BYTE> AudioRecorder::getAudioFileContent(void* audioDataBuffer, DWORD audioDataBufferSize)
{
	// ====================== I. THE "RIFF" CHUNK DESCRIPTOR ======================
	// The Format of concern here is "WAVE", which requires two sub-chunk: "fmt " and "data"
	// The canonical WAVE format starts with the RIFF header:

	// 1. ChunkID - Contains the letters "RIFF" in ASCII form ('RIFF' => 0x52494646 => 1380533830 big - endian form).
	/*
		2. ChunkSize -
		36 + SubChunk2Size, or more precisely:
		4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
		This is the size of the rest of the chunk
		following this number.This is the size of the
		entire file in bytes minus 8 bytes for the
		two fields not included in this count: ChunkID and ChunkSize.
	*/
	// 3. Format - Contains the letters "WAVE" ('WAVE' => 0x57415645 => 1463899717 big - endian form).
	DWORD riffChunkDescriptor[3] =
	{
		'FFIR', // ChunkID
		36 + audioDataBufferSize, // ChunkSize
		'EVAW' // Format
	};

	DWORD fileCurrentWritingPosition = 0;

	// because riffChunkDescriptor[1] is ChunkSize that's equal to file size (8 bytes)
	// so after adding 8 bytes it'll have the exact file size
	std::vector<BYTE> audioFileContent(riffChunkDescriptor[1] + 8);
	
	for(; fileCurrentWritingPosition < sizeof riffChunkDescriptor; fileCurrentWritingPosition++)
		audioFileContent[fileCurrentWritingPosition] = ((BYTE*)&riffChunkDescriptor)[fileCurrentWritingPosition];

	// II ====================== THE "fmt " SUB-CHUNK ======================
	// describes the format of the sound information in the data sub-chunk
	// The "WAVE" format consists of two subchunks: "fmt " and "data":
	// The "fmt " subchunk describes the sound data's format:

	// 1. Subchunk1ID - contains the letters "fmt " ('fmt ' => 0x666d7420 => 1718449184 big - endian form).
	// 2. Subchunk1Size - 16 for PCM. This is the size of the rest of the Subchunk which follows this number.
	DWORD subchunk1[2] =
	{
		' tmf', // Subchunk1ID
		16 // Subchunk1Size
	};

	for(BYTE i = 0; i < sizeof subchunk1; i++, fileCurrentWritingPosition++)
		audioFileContent[fileCurrentWritingPosition] = ((BYTE*)&subchunk1)[i];

	// AudioFormat - PCM = 1 (i.e. Linear quantization) Values other than 1 indicate some
	for(BYTE i = 0; i < sizeof audioDataFormat.wFormatTag; i++, fileCurrentWritingPosition++)
		audioFileContent[fileCurrentWritingPosition] = ((BYTE*)&audioDataFormat.wFormatTag)[i];

	// NumChannels - Mono = 1, Stereo = 2, etc.
	for(BYTE i = 0; i < sizeof audioDataFormat.nChannels; i++, fileCurrentWritingPosition++)
		audioFileContent[fileCurrentWritingPosition] = ((BYTE*)&audioDataFormat.nChannels)[i];

	// SampleRate - 8000, 44100, etc.
	for(BYTE i = 0; i < sizeof audioDataFormat.nSamplesPerSec; i++, fileCurrentWritingPosition++)
		audioFileContent[fileCurrentWritingPosition] = ((BYTE*)&audioDataFormat.nSamplesPerSec)[i];

	// ByteRate == SampleRate * NumChannels * BitsPerSample/8
	DWORD byteRate = audioDataFormat.nSamplesPerSec * audioDataFormat.nChannels * audioDataFormat.wBitsPerSample / 8;

	for(BYTE i = 0; i < sizeof byteRate; i++, fileCurrentWritingPosition++)
		audioFileContent[fileCurrentWritingPosition] = ((BYTE*)&byteRate)[i];

	// BlockAlign == NumChannels * BitsPerSample/8 The number of bytes for one sample including all channels.
	for(BYTE i = 0; i < sizeof audioDataFormat.nBlockAlign; i++, fileCurrentWritingPosition++)
		audioFileContent[fileCurrentWritingPosition] = ((BYTE*)&audioDataFormat.nBlockAlign)[i];

	// BitsPerSample - 8 bits = 8, 16 bits = 16, etc., - audioDataFormat.wBitsPerSample
	for(BYTE i = 0; i < sizeof audioDataFormat.wBitsPerSample; i++, fileCurrentWritingPosition++)
		audioFileContent[fileCurrentWritingPosition] = ((BYTE*)&audioDataFormat.wBitsPerSample)[i];

	// ExtraParamSize - if PCM, then doesn't exist
	// ExtraParams - space for extra parameters

	// III ====================== THE "data" SUB-CHUNK ======================
	// indicates the size of the sound information and contains the raw sound data
	// The "data" subchunk contains the size of the data and the actual sound:

	// 1. Subchunk2ID - Contains the letters "data" ('data' => 0x64617461 => 1684108385 big - endian form).
	// 2. Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
	// This is the number of bytes in the data. You can also think of this as the size
	// of the read of the subchunk following this number.
	DWORD subchunk2[2] =
	{
		'atad', // Subchunk2ID
		audioDataBufferSize // Subchunk2Size
	};

	for(BYTE i = 0; i < sizeof subchunk2; i++, fileCurrentWritingPosition++)
		audioFileContent[fileCurrentWritingPosition] = ((BYTE*)&subchunk2)[i];

	// Data - The actual sound data.
	for(DWORD i = 0; i < audioDataBufferSize; i++, fileCurrentWritingPosition++)
		audioFileContent[fileCurrentWritingPosition] = ((BYTE*)audioDataBuffer)[i];

	return audioFileContent;
}

#define IS_MMRESULT_ERROR(RESULT) ((RESULT) != MMSYSERR_NOERROR)

std::vector<BYTE> AudioRecorder::record(DWORD duration)
{
	/*
		Before you pass an audio data block to a device driver, you must prepare the data block
		by passing it to either waveInPrepareHeader or waveOutPrepareHeader. When the device driver
		is finished with the data block and returns it, you must clean up this preparation by passing
		the data block to either waveInUnprepareHeader or waveOutUnprepareHeader before any allocated memory can be freed.

		Even if a single data block is used, an application must be able to determine when a device driver is finished with
		the data block so the application can free the memory associated with the data block and header structure.
		There are several ways to determine when a device driver is finished with a data block - np callback zdefiniowany wy¿ej.

		The WAVEHDR structure defines the header used to identify a waveform-audio buffer.
		The lpData, dwBufferLength, and dwFlags members must be set before calling the waveInPrepareHeader
		or waveOutPrepareHeader function. (For either function, the dwFlags member must be set to zero.)
	*/
	WAVEHDR audioDataHeader;

	audioDataHeader.dwFlags = 0;
	audioDataHeader.dwBufferLength = AUDIO_DATA_BUFFER_LENGTH_FOR_1_SECOND_OF_RECORDING * duration;
	audioDataHeader.lpData = (char*)malloc(AUDIO_DATA_BUFFER_LENGTH_FOR_1_SECOND_OF_RECORDING * duration);

	// The waveInPrepareHeader function prepares a buffer for waveform-audio input.
	// The lpData, dwBufferLength, and dwFlags members of the WAVEHDR structure must be set before calling this function (dwFlags must be zero).
	MMRESULT result = waveInPrepareHeader(audioDeviceHandle, &audioDataHeader, sizeof audioDataHeader);

	if(IS_MMRESULT_ERROR(result))
	{
		free(audioDataHeader.lpData);

		throw std::runtime_error("waveInPrepareHeader failed with code " + std::to_string(result));
	}

	/*
		Before you begin recording by using waveInStart, you should send at least one buffer to the driver, or incoming data could be lost.
		The waveInAddBuffer function sends an input buffer to the given waveform-audio input device. When the buffer is filled, the application is notified.
		Sends a buffer to the device driver so it can be filled with recorded waveform-audio data.

		When the buffer is filled, the WHDR_DONE bit is set in the dwFlags member of the WAVEHDR structure.
		The buffer must be prepared with the waveInPrepareHeader function before it is passed to this function.
	*/
	result = waveInAddBuffer(audioDeviceHandle, &audioDataHeader, sizeof(audioDataHeader));

	if(IS_MMRESULT_ERROR(result))
	{
		waveInUnprepareHeader(audioDeviceHandle, &audioDataHeader, sizeof(audioDataHeader));

		free(audioDataHeader.lpData);

		throw std::runtime_error("waveInAddBuffer failed with code " + std::to_string(result));
	}

	// start recording audio
	result = waveInStart(audioDeviceHandle);

	// The waveInStart function starts input on the given waveform-audio input device.
	// uffers are returned to the application when full or when the waveInReset function
	// is called (the dwBytesRecorded member in the header will contain the length of data). 
	// If there are no buffers in the queue, the data is thrown away without notifying the application, and input continues.
	if(IS_MMRESULT_ERROR(result))
	{
		// By calling this function, all pending buffers are marked as done and returned to the application.
		waveInReset(audioDeviceHandle);

		waveInUnprepareHeader(audioDeviceHandle, &audioDataHeader, sizeof(audioDataHeader));

		free(audioDataHeader.lpData);

		throw std::runtime_error("waveInStart failed with code " + std::to_string(result));
	}

	/*
		You can use the WHDR_DONE flag to test the dwFlags member.
		As soon as the WHDR_DONE flag is set in the dwFlags member
		of the WAVEHDR structure, the driver is finished with the data block.
	*/

	while(!(audioDataHeader.dwFlags & WHDR_DONE))
		Sleep(1);

	// The waveInReset function stops input on the given waveform-audio
	// input device and resets the current position to zero.
	// All pending buffers are marked as done and returned to the application.
	waveInReset(audioDeviceHandle);

	// dwBytesRecorded - When the header is used in input, specifies how much data is in the buffer.
	std::vector<BYTE> audioFileContent = getAudioFileContent(audioDataHeader.lpData, audioDataHeader.dwBytesRecorded);

	// The waveInUnprepareHeader function cleans up the preparation performed by the waveInPrepareHeader function.
	// This function must be called after the device driver fills a buffer and returns it to the application.
	// You must call this function before freeing the buffer.
	waveInUnprepareHeader(audioDeviceHandle, &audioDataHeader, sizeof(audioDataHeader));

	free(audioDataHeader.lpData);

	return audioFileContent;
}
