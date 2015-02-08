#include "audioProcThread.h"

AudioProcThread::AudioProcThread() :
	beamAngle(0.0f),
	beamAngleConfidence(0.0f),
	accumulatedSquareSum(0.0f),
	accumulatedSampleCount(0),
	energyIndex(0),
	energyError(0.0f),
	energyRefreshIndex(0),
	newEnergyAvailable(0),
	lastEnergyRefreshTime(NULL)
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
	//stopThread();
	waitForThread();
	this->frameArrivedEvent = NULL;
	this->audioBeamFrameReader = NULL;
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
	}
	std::cout << "while end" << std::endl;
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

				energyBuffer[energyIndex] = (cMinEnergy - fEnergy) / cMinEnergy;
				newEnergyAvailable++;
				// Ring Buffer
				energyIndex = (energyIndex + 1) % cEnergyBufferLength;

				unlock();
			}
			else{

			}

			// å„èàóù
			accumulatedSquareSum = 0.f;
			accumulatedSampleCount = 0;

		}
	}
}


void AudioProcThread::update(){

	ULONGLONG previousRefreshTime = lastEnergyRefreshTime;
	//ULONGLONG now = GetTickCount();
	ULONGLONG now = GetTickCount64();
		
	lastEnergyRefreshTime = now;

	// No need to refresh if there is no new energy available to render
	if (newEnergyAvailable <= 0)
	{
		return;
	}

	if (lock()){
		if (previousRefreshTime != NULL)
		{
			// Calculate how many energy samples we need to advance since the last Update() call in order to
			// have a smooth animation effect.
			float energyToAdvance = energyError + (((now - previousRefreshTime) * cAudioSamplesPerSecond / (float)1000.0) / cAudioSamplesPerEnergySample);
			int energySamplesToAdvance = MIN(newEnergyAvailable, (int)(energyToAdvance));
			energyError = energyToAdvance - energySamplesToAdvance;
			energyRefreshIndex = (energyRefreshIndex + energySamplesToAdvance) % cEnergyBufferLength;
			newEnergyAvailable -= energySamplesToAdvance;
		}

		// Copy energy samples into buffer to be displayed, taking into account that energy
		// wraps around in a circular buffer.
		int baseIndex = (energyRefreshIndex + cEnergyBufferLength - cEnergySamplesToDisplay) % cEnergyBufferLength;
		int samplesUntilEnd = cEnergyBufferLength - baseIndex;
		if (samplesUntilEnd>cEnergySamplesToDisplay) {
			memcpy_s(energyDisplayBuffer, cEnergySamplesToDisplay*sizeof(float), energyBuffer + baseIndex, cEnergySamplesToDisplay*sizeof(float));
		}
		else {
			int samplesFromBeginning = cEnergySamplesToDisplay - samplesUntilEnd;
			memcpy_s(energyDisplayBuffer, cEnergySamplesToDisplay*sizeof(float), energyBuffer + baseIndex, samplesUntilEnd*sizeof(float));
			memcpy_s(energyDisplayBuffer + samplesUntilEnd, (cEnergySamplesToDisplay - samplesUntilEnd)*sizeof(float), energyBuffer, samplesFromBeginning*sizeof(float));
		}

		unlock();
	}
	else{

	}
}

void AudioProcThread::draw() {

	ofSetColor(255, 255, 255);
	ofCircle(480, 100, 50);
	ofSetColor(255, 0, 0);
	ofSetLineWidth(2);
	ofPushMatrix();
	{
		ofTranslate(480, 100, 0);
		ofRotateZ(-180.0f * beamAngle / static_cast<float>(PI));
		float scale = ofMap(beamAngleConfidence, 0.0f, 1.0f, 0.4f, 1.0f);
		ofScale(scale, scale);
		ofLine(0, 0, 0, 50);
	}
	ofPopMatrix();

	ofSetLineWidth(1);
	ofPushMatrix();
	{
		ofTranslate(100, 200);
		// Draw each energy sample as a centered vertical bar, where the length of each bar is
		// proportional to the amount of energy it represents.
		// Time advances from left to right, with current time represented by the rightmost bar.
		for (UINT i = 0; i < cEnergySamplesToDisplay; ++i)
		{
			const int cHalfImageHeight = DISPLAY_HEIGHT / 2;

			// Each bar has a minimum height of 1 (to get a steady signal down the middle) and a maximum height
			// equal to the bitmap height.
			int barHeight = static_cast<int>(MAX(1.0f, (energyDisplayBuffer[i] * DISPLAY_HEIGHT)));

			// Center bar vertically on image
			int top = cHalfImageHeight - (barHeight / 2);
			int bottom = top + barHeight;
			ofLine(i, top, i, bottom);
		}
	}
	ofPopMatrix();
}