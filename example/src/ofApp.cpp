#include "ofMain.h"
#include "ofxSimpleGuiToo.h"

#include "MSAMultiCam.h"

using namespace msa;

class ofApp : public ofBaseApp{

public:
    MultiCam multiCam;

    //--------------------------------------------------------------
    void setup(){
        multiCam.setupGui();
        multiCam.initCameras();

        gui.setAlignRight(false);
        gui.setDefaultKeys(true);

        ofSetBackgroundColor(0);
    }

    //--------------------------------------------------------------
    void update(){
        multiCam.update();
    }

    //--------------------------------------------------------------
    void draw(){
        multiCam.draw(0, 0, ofGetWidth(), ofGetHeight());
        gui.draw();
    }
};

//========================================================================
int main( ){
    ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context

    // this kicks off the running of my app
    // can be OF_WINDOW or OF_FULLSCREEN
    // pass in width and height too:
    ofRunApp(new ofApp());

}
