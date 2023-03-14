#include "AudioRecorder.h"

#include <fstream>
#include <iostream>

int main()
{
	const DWORD duration = 20;

	const std::string outputFileName = "recording.wav";

	std::cout << "Recording " << duration << " seconds of audio..." << std::endl;

	std::vector<BYTE> recorded = AudioRecorder().record(duration);

	std::cout << "Saving as " << outputFileName << "..." << std::endl;

	std::ofstream fout(outputFileName, std::ios::out | std::ios::binary);
	
	fout.write((char*)recorded.data(), recorded.size());

	return 0;
}