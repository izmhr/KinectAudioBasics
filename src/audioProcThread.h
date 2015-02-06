#pragma once

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
	void draw();


protected:

private:
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

	// Number of audio samples accumulated so far to compute the next energy value.
	int		accumulatedSampleCount;

	// Index of next element available in audio energy buffer.
	int		energyIndex;
};
