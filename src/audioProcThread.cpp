#include "audioProcThread.h"

AudioProcThread::AudioProcThread() :
	beamAngle(0.0f),
	beamAngleConfidence(0.0f),
	accumulatedSquareSum(0.0f),
	accumulatedSampleCount(0),
	energyIndex(0)
{
	ZeroMemory(energyBuffer, sizeof(energyBuffer));
	ZeroMemory(energyDisplayBuffer, sizeof(energyDisplayBuffer));
	std::cout << "constructor" << std::endl;
}

void AudioProcThread::start(WAITABLE_HANDLE *_frameArrivedEvent, IAudioBeamFrameReader*	_audioBeamFrameReader){
	this->frameArrivedEvent = _frameArrivedEvent;
	this->audioBeamFrameReader = _audioBeamFrameReader;
	startThread();
}

void AudioProcThread::stop(){
	this->frameArrivedEvent = NULL;
	this->audioBeamFrameReader = NULL;
	//stopThread();
	waitForThread();
}

void AudioProcThread::threadedFunction(){
	HRESULT hr = S_OK;
	DWORD timeout = 2000;
	UINT32 subFrameCount = 0;
	IAudioBeamFrameArrivedEventArgs* audioBeamFrameArrivedEventArgs = NULL;
	IAudioBeamFrameReference* audioBeamFrameReference = NULL;
	IAudioBeamFrameList* audioBeamFrameList = NULL;
	IAudioBeamFrame* audioBeamFrame = NULL;
	HANDLE handles[] = { (HANDLE)*frameArrivedEvent };

	while (isThreadRunning()){
		DWORD result = WaitForMultipleObjects(_countof(handles), handles, FALSE, timeout);

		if (WAIT_OBJECT_0 == result){
			hr = audioBeamFrameReader->GetFrameArrivedEventData(*frameArrivedEvent, &audioBeamFrameArrivedEventArgs);

			if (SUCCEEDED(hr)){
				hr = audioBeamFrameArrivedEventArgs->get_FrameReference(&audioBeamFrameReference);
			}

			if (SUCCEEDED(hr)){
				hr = audioBeamFrameReference->AcquireBeamFrames(&audioBeamFrameList);
			}

			if (SUCCEEDED(hr)){
				// Only one audio beam is currently supported
				hr = audioBeamFrameList->OpenAudioBeamFrame(0, &audioBeamFrame);
			}

			if (SUCCEEDED(hr)){
				hr = audioBeamFrame->get_SubFrameCount(&subFrameCount);
			}

			if (SUCCEEDED(hr) && subFrameCount > 0)
			{
				for (UINT32 i = 0; i < subFrameCount; i++)
				{
					// Process all subframes
					IAudioBeamSubFrame* audioBeamSubFrame = NULL;

					hr = audioBeamFrame->GetSubFrame(i, &audioBeamSubFrame);

					if (SUCCEEDED(hr))
					{
						processAudio(audioBeamSubFrame);
					}

					SafeRelease(audioBeamSubFrame);
				}
			}

			SafeRelease(audioBeamFrame);
			SafeRelease(audioBeamFrameList);
			SafeRelease(audioBeamFrameReference);
			SafeRelease(audioBeamFrameArrivedEventArgs);

			//if (SUCCEEDED(hr)){
			//	std::cout << "succeeded" << std::endl;
			//}
		}
		else if (WAIT_OBJECT_0 + 1 == result) {
			std::cout << "WAIT_OBJECT_1" << std::endl;
			break;
		}
		else if (WAIT_TIMEOUT == result){
			std::cout << "TIMEOUT" << std::endl;
		}
		else {
			hr = E_FAIL;
			break;
		}
		//if (lock()){
		//	// do something
		//	std::cout << "thread" << std::endl;
		//	unlock();
		//	sleep(1000);
		//}
		//else {
		//	// do something
		//}
	}
}

void AudioProcThread::processAudio(IAudioBeamSubFrame* audioBeamSubFrame){
	HRESULT hr = S_OK;
	float* audioBuffer = NULL;
	UINT cbRead = 0;

	hr = audioBeamSubFrame->AccessUnderlyingBuffer(&cbRead, (BYTE **)&audioBuffer);

	if (FAILED(hr)){
		
	}
	else if (cbRead > 0){
		DWORD nSampleCount = cbRead / sizeof(float);
		float fBeamAngle = 0.f;
		float fBeamAngleConfidence = 0.0f;

		// Get audio beam angle and confidence
		audioBeamSubFrame->get_BeamAngle(&fBeamAngle);
		audioBeamSubFrame->get_BeamAngleConfidence(&fBeamAngleConfidence);

		// Calculate energy from audio
		for (UINT i = 0; i < nSampleCount; i++){
			accumulatedSquareSum += audioBuffer[i] * audioBuffer[i];

			++accumulatedSampleCount;

			if (accumulatedSampleCount < cAudioSamplesPerEnergySample){
				continue;
			}

			// Each energy value will represent the logarithm of the mean of the
			// sum of squares of a group of audio samples.
			float fMeanSquare = accumulatedSquareSum / cAudioSamplesPerEnergySample;

			if (fMeanSquare > 1.0f)
			{
				// A loud audio source right next to the sensor may result in mean square values
				// greater than 1.0. Cap it at 1.0f for display purposes.
				fMeanSquare = 1.0f;
			}

			float fEnergy = cMinEnergy;
			if (fMeanSquare > 0.f)
			{
				// Convert to dB
				fEnergy = 10.0f*log10(fMeanSquare);
			}

			if (lock()){
				beamAngle = fBeamAngle;
				beamAngleConfidence = fBeamAngleConfidence;
				//energyBuffer[energyIndex]

				unlock();
			}
			else{

			}

		}

		// å„èàóù
		accumulatedSquareSum = 0.f;
		accumulatedSampleCount = 0;

		//if (lock()){
		//	beamAngle = fBeamAngle;
		//	beamAngleConfidence = fBeamAngleConfidence;

		//	unlock();
		//}
		//else{

		//}

		std::cout << "angle: " << 180.0f * fBeamAngle / static_cast<float>(PI) << " confidence: " << fBeamAngleConfidence << std::endl;
	}
}


void AudioProcThread::draw(){
	if (lock()){
		ofSetColor(255, 255, 255);
		ofCircle(100, 100, 50);
		ofSetColor(255, 0, 0);
		ofSetLineWidth(2);
		ofPushMatrix();
		{
			ofTranslate(100, 100, 0);
			ofRotateZ(-180.0f * beamAngle / static_cast<float>(PI));
			ofLine(0, 0, 0, 50);
		}
		ofPopMatrix();

		unlock();
	}
	else{

	}
}