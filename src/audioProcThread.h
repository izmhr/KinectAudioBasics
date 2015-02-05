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
	WAITABLE_HANDLE *frameArrivedEvent;
	IAudioBeamFrameReader*	audioBeamFrameReader;

	float beamAngle;
	float beamAngleConfidence;

};