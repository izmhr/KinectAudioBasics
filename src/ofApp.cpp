#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	HRESULT hr = S_OK;
	IAudioSource* audioSource = NULL;

	hr = GetDefaultKinectSensor(&kinectSensor);
	if (FAILED(hr)){
		return;
	}

	hr = kinectSensor->Open();
	if (SUCCEEDED(hr)){
		hr = kinectSensor->get_AudioSource(&audioSource);
	}

	if (SUCCEEDED(hr)){
		hr = audioSource->OpenReader(&audioBeamFrameReader);
	}

	if (SUCCEEDED(hr)){
		hr = audioBeamFrameReader->SubscribeFrameArrived(&frameArrivedEvent);
	}

	audioProcThread.start(&frameArrivedEvent, audioBeamFrameReader);

	SafeRelease(audioSource);

	//ofSetFrameRate(60);
}

//--------------------------------------------------------------
void ofApp::update(){
	std::stringstream strm;
	strm << "fps: " << ofGetFrameRate();
	ofSetWindowTitle(strm.str());
	
	audioProcThread.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
	audioProcThread.draw();
}

void ofApp::exit(){
	audioProcThread.stop();

	if (NULL != audioBeamFrameReader){
		if (NULL != frameArrivedEvent){
			audioBeamFrameReader->UnsubscribeFrameArrived(frameArrivedEvent);
		}
		SafeRelease(audioBeamFrameReader);
	}
	
	if (kinectSensor){
		kinectSensor->Close();
	}

	SafeRelease(kinectSensor);

	std::cout << "exit" << std::endl;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
