#pragma once

// GetTickCount64() を使うにはvista以降であることを表明する必要がある
// 本当は603 としたいところだが、おそらくoF for VSの制約で 601 としないと怒られる
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include "ofMain.h"
#include "ofThread.h"
#include "util.h"
#include <Kinect.h>

class AudioProcThread : public ofThread{
public:
	AudioProcThread();
	void start(WAITABLE_HANDLE *_frameArrivedEvent, IAudioBeamFrameReader*	_audioBeamFrameReader);
	void stop();
	void threadedFunction();
	void processAudio(IAudioBeamSubFrame* audioBeamSubFrame);
	void update();
	void draw();


protected:

private:
	static const int		DISPLAY_HEIGHT = 200;
	// Audio samples per second in Kinect audio stream
	static const int        cAudioSamplesPerSecond = 16000;

	// Number of audio samples captured from Kinect audio stream accumulated into a single
	// energy measurement that will get displayed.
	static const int        cAudioSamplesPerEnergySample = 40;

	// Number of energy samples that will be visible in display at any given time.
	static const int        cEnergySamplesToDisplay = 780;

	// Number of energy samples that will be stored in the circular buffer.
	// Always keep it higher than the energy display length to avoid overflow.
	static const int        cEnergyBufferLength = 1000;

	// Minimum energy of audio to display (in dB value, where 0 dB is full scale)
	static const int        cMinEnergy = -90;

	WAITABLE_HANDLE *frameArrivedEvent;
	IAudioBeamFrameReader*	audioBeamFrameReader;

	float	beamAngle;
	float	beamAngleConfidence;
	float	accumulatedSquareSum;

	// Buffer used to store audio stream energy data as we read audio.
	float	energyBuffer[cEnergyBufferLength];

	// Buffer used to store audio stream energy data ready to be displayed.
	float	energyDisplayBuffer[cEnergySamplesToDisplay];

	// Error between time slice we wanted to display and time slice that we ended up
	// displaying, given that we have to display in integer pixels.
	float	energyError;

	// Number of audio samples accumulated so far to compute the next energy value.
	int		accumulatedSampleCount;

	// Index of next element available in audio energy buffer.
	int		energyIndex;

	// Number of newly calculated audio stream energy values that have not yet been displayed.
	int		newEnergyAvailable;

	// Index of first energy element that has never (yet) been displayed to screen.
	int		energyRefreshIndex;

	// Last time energy visualization was rendered to screen.
	ULONGLONG	lastEnergyRefreshTime;
};
