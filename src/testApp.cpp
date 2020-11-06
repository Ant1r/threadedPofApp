/*
 * Copyright (c) 2014 Antoine Rousseau <antoine@metalu.net>
 * BSD Simplified License, see the file "LICENSE.txt" in this distribution.
 * See https://github.com/Ant1r/ofxPof for documentation and updates
 */

#include "testApp.h"
#include "pofBase.h"
#include "externals.h"
#include <sys/resource.h>
#include <stdio.h>

using namespace pd;

bool linkEnabled = false;

class pdPofThread: public ofThread {
	public :
	ofxPd *pd;
	t_pdinstance *instance;
	float buffer[4096];
	int numout, numin, bufsize;
	int loops;
	
	pdPofThread(ofxPd *_pd, t_pdinstance *_instance, int outs, int ins, int buflen):
		pd(_pd), instance(_instance), numout(outs), numin(ins), bufsize(buflen)
	{}

	void threadedFunction() {
		while(isThreadRunning()) {
			libpd_set_instance(instance);
			pd->audioIn(buffer, bufsize, numin);
			pd->audioOut(buffer, bufsize, numout);
			if(clock_gettimesince(0) > sys_getrealtime()*1000.0) sleep(3);
		}
	}
};

void testApp::setup() {
	std::vector<string>::iterator it; // for parsing commandline arguments

		int numOutputs = 2; 
	// the number of libpd ticks per buffer,
	// used to compute the audio buffer len: tpb * blocksize (always 64)
	#ifdef TARGET_LINUX_ARM
		// longer latency for Raspberry PI
		int ticksPerBuffer = 32; // 32 * 64 = buffer len of 2048
		int numInputs = 0; // no built in mic
	#else
		int ticksPerBuffer = 2; // 8 * 64 = buffer len of 512
		int numInputs = 1;
	#endif

	// ----------- parse arguments for audio parameters --------------
	it = find (arguments.begin(), arguments.end(), "-inchannels");
	if(it != arguments.end()) {
		it++;
		if(it != arguments.end()) {
			numInputs = (int)std::stol(*it);
			ofLogNotice("Requested audio input channels :") << numInputs;
		}
	}

	it = find (arguments.begin(), arguments.end(), "-outchannels");
	if(it != arguments.end()) {
		it++;
		if(it != arguments.end()) {
			numOutputs = (int)std::stol(*it);
			ofLogNotice("Requested audio output channels :") << numOutputs;
		}
	}

	// ----------- setup OF sound stream --------------
	ofSoundStreamSetup(numOutputs, numInputs, this, 44100, ofxPd::blockSize()*ticksPerBuffer, 3);

	libpd_init();
	pdgui = libpd_new_instance();
	pdaudio = libpd_new_instance();

	// set a "current" instance before pd.init() or else Pd will make
	// an unnecessary third "default" instance
	libpd_set_instance(pdgui);
	
	// setup Pd
	//
	// set 4th arg to true for queued message passing using an internal ringbuffer,
	// this is useful if you need to control where and when the message callbacks
	// happen (ie. within a GUI thread)
	//
	// note: you won't see any message prints until update() is called since
	// the queued messages are processed there, this is normal

	if(!pd.init(numOutputs, numInputs, 44100, ticksPerBuffer, /*false*/true)) {
		OF_EXIT_APP(1);
	}

	libpd_set_instance(pdaudio);
	if(!pd.init(numOutputs, numInputs, 44100, ticksPerBuffer, /*false*/true)) {
		OF_EXIT_APP(1);
	}

	externals_setup();
	
	pd.start();
	
	pd.openPatch("audio.pd");

	libpd_set_instance(pdgui);

	pofBase::setup();

	// subscribe to receive source names
	pd.subscribe("toSYSTEM");
	// add message receiver, required if you want to recieve messages
	pd.addReceiver(*this); // automatically receives from all subscribed sources

	pd.start();

	it = find (arguments.begin(), arguments.end(), "-hidecursor");
	if(it != arguments.end()) ofHideCursor();

	pd.openPatch("gui.pd");
	pdPofThread *thr = new pdPofThread(&pd, pdgui, numOutputs, numInputs, ofxPd::blockSize()*ticksPerBuffer);
	thr->startThread();
}

//--------------------------------------------------------------

void testApp::print(const std::string& message) {
	cout << message << endl ;
}

void testApp::receiveBang(const std::string& dest) {
//	cout << "OF: bang " << dest << endl;
}

void testApp::receiveFloat(const std::string& dest, float value) {
//	cout << "OF: float " << dest << ": " << value << endl;
}

void testApp::receiveSymbol(const std::string& dest, const std::string& symbol) {
//	cout << "OF: symbol " << dest << ": " << symbol << endl;
}

void testApp::receiveList(const std::string& dest, const List& list) {
//	cout << "OF: list " << dest << ": " << list << endl;
}

void testApp::receiveMessage(const std::string& dest, const std::string& msg, const List& list) {
	libpd_set_instance(pdgui);
	if(dest == "toSYSTEM") {
		if(msg == "showLinkSettings") {
			linkEnabled = !linkEnabled;
			pd.sendFloat("ablLinkEnabled", 1.0 * linkEnabled);
			pd.sendFloat("ablLink_connect", 1.0 * linkEnabled);
		}
	}
}
//--------------------------------------------------------------
void testApp::update() {
	libpd_set_instance(pdgui);
	if(pd.isQueued()) {
		// process any received messages, if you're using the queue and *do not*
		// call these, you won't receieve any messages or midi!
		pd.receiveMessages();
		pd.receiveMidi();
	}
	pofBase::updateAll();
}

//--------------------------------------------------------------
void testApp::draw() {
	libpd_set_instance(pdgui);
	pofBase::drawAll();
}

//--------------------------------------------------------------
void testApp::exit() {
	libpd_set_instance(pdgui);
	pofBase::release();
}

//--------------------------------------------------------------
void testApp::keyPressed(int key) { 
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y) {}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button) {
	libpd_set_instance(pdgui);
	pofBase::touchMovedAll(x, y, 0);	
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button) {
	libpd_set_instance(pdgui);
	pofBase::touchDownAll(x, y, 0);
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button) {
	libpd_set_instance(pdgui);
	pofBase::touchUpAll(x, y, 0);
}


//--------------------------------------------------------------
void testApp::touchDown(ofTouchEventArgs &touch){
	libpd_set_instance(pdgui);
	pofBase::touchDownAll(touch.x, touch.y, touch.id);
}

//--------------------------------------------------------------
void testApp::touchMoved(ofTouchEventArgs &touch){
	libpd_set_instance(pdgui);
	pofBase::touchMovedAll(touch.x, touch.y, touch.id);
}

//--------------------------------------------------------------
void testApp::touchUp(ofTouchEventArgs &touch){
	libpd_set_instance(pdgui);
	pofBase::touchUpAll(touch.x, touch.y, touch.id);
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h) {
	libpd_set_instance(pdgui);
	pofBase::windowResized(w,h);
}

//--------------------------------------------------------------
void testApp::audioReceived(float * input, int bufferSize, int nChannels) {
	libpd_set_instance(pdaudio);
	pd.audioIn(input, bufferSize, nChannels);
}

//--------------------------------------------------------------
void testApp::audioRequested(float * output, int bufferSize, int nChannels) {
	libpd_set_instance(pdaudio);
	pd.audioOut(output, bufferSize, nChannels);
}
