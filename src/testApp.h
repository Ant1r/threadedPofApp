/*
 * Copyright (c) 2020 Antoine Rousseau <antoine@metalu.net>
 * BSD Simplified License, see the file "LICENSE.txt" in this distribution.
 * See https://github.com/Ant1r/ofxPof for documentation and updates
 */
#pragma once

#include "ofMain.h"
#include "ofxPd.h"

// a namespace for the Pd types
using namespace pd;

class testApp : public ofBaseApp, public PdReceiver{

	public:

		void setup();
		void update();
		void draw();
		void exit();

		void keyPressed  (int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		
		void audioReceived(float * input, int bufferSize, int nChannels);
		void audioRequested(float * output, int bufferSize, int nChannels);

		void touchDown(ofTouchEventArgs &touch);
		void touchMoved(ofTouchEventArgs &touch);
		void touchUp(ofTouchEventArgs &touch);
		void touchCancelled() {};

		// pd message receiver callbacks
		void print(const std::string& message);
		void receiveBang(const std::string& dest);
		void receiveFloat(const std::string& dest, float value);
		void receiveSymbol(const std::string& dest, const std::string& symbol);
		void receiveList(const std::string& dest, const List& list);
		void receiveMessage(const std::string& dest, const std::string& msg, const List& list);

		ofxPd pd;
		// pd instance handles, not full fledged ofxPd objects yet, but internal
		// pd types which tell libpd to address a separate internal "instance"
		t_pdinstance *pdgui, *pdaudio;

		vector<string> arguments;
};
