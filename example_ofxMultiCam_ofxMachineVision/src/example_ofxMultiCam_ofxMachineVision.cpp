#include "ofMain.h"
#include "ofxSimpleGuiToo.h"
#include "MSAMultiCam.h"

class ofApp : public ofBaseApp {
public:
	msa::MultiCam multiCam;
	ofxSimpleGuiToo gui;

	//--------------------------------------------------------------
	void setup() {
		multiCam.setup(gui);
		gui.setAlignRight(false);
		gui.setDefaultKeys(true);
		gui.setPage("ofxMultiCam");
		gui.show();
		ofSetBackgroundColor(0);
	}

	//--------------------------------------------------------------
	void update() {
		multiCam.update();
	}

	//--------------------------------------------------------------
	void draw() {
		multiCam.draw(0, 0, ofGetWidth(), ofGetHeight());
		gui.draw();
	}
};

//========================================================================
int main() {
	ofSetupOpenGL(1280, 1000, OF_WINDOW);
	ofRunApp(new ofApp());
}
