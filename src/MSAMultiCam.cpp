#include "MSAMultiCam.h"
#include "ofxSimpleGuiToo.h"

namespace msa {

void MultiCam::Cam::setup() {
    close();
    grabber.setDeviceID(init.deviceid);
    grabber.setDesiredFrameRate(init.fps);
    grabber.setup(init.w, init.h);
}

void MultiCam::Cam::close() {
    grabber.close();
}

void MultiCam::Cam::update() {
    if(!ctrl.enabled) return;

    if(init.reinit) {
        init.reinit = false;
        setup();
    }

    if(ctrl.showSettings) {
        ctrl.showSettings = false;
        grabber.videoSettings();
    }

    grabber.update();
}

void MultiCam::Cam::draw() const {
    draw(ctrl.pos.x, ctrl.pos.y, grabber.getWidth()*ctrl.scale.x, grabber.getHeight()*ctrl.scale.y);
}

// ofBaseDraws
void MultiCam::Cam::draw(float x, float y, float w, float h) const {
    if(!ctrl.enabled) return;
    if(grabber.isInitialized()) grabber.draw(x, y, w, h);
}

float MultiCam::Cam::getWidth() const {
    return grabber.isInitialized() ? grabber.getWidth() : 320;
}

float MultiCam::Cam::getHeight() const {
    return grabber.isInitialized() ? grabber.getHeight() : 320/1.7777;
}

//--------------------------------------------------------------
//--------------------------------------------------------------
void MultiCam::initCameras() {
    close();
    for(auto&& cam : cams) cam.setup();
}


void MultiCam::close() {
    for(auto&& cam : cams) cam.close();
}


void MultiCam::autoLayout() {
    if(autoLayoutSettings.enabled) {
        // assuming simple equal dimensions for all images
        ofVec2f camSize(autoLayoutSettings.width, autoLayoutSettings.height);
        int numActive = 0;
        for(auto&& cam : cams) if(cam.ctrl.enabled) numActive++;
        if(numActive==0) return;
        if(autoLayoutSettings.tileHorizontal) camSize.x /= numActive;
        ofVec2f pos = ofVec2f(0);
        for(auto&& cam : cams) {
            if(cam.ctrl.enabled) {
                cam.ctrl.scale = camSize / ofVec2f(cam.getWidth(), cam.getHeight());
                cam.ctrl.pos = pos;
                if(autoLayoutSettings.tileHorizontal) pos.x += cam.ctrl.scale.x * cam.getWidth();

            }
        }
    }
}


void MultiCam::update() {
    if(!enabled) return;
    for(auto&& cam : cams) cam.update();
    autoLayout();
    updateBoundingBox();
    if(useFbo) {
        drawToFbo();
        if(readFboToPixels) fbo.readToPixels(pixels);
    }
}


void MultiCam::draw(float x, float y, float w, float h) {
    if(!enabled) return;
    if(!doDraw) return;

    if(w<1 || !doScaleDraw) w = boundingBox.width;
    if(h<1 || !doScaleDraw) h = boundingBox.height;
    if(useFbo) {
        drawFbo(x, y, w, h);
    } else {
        ofPushStyle();
        ofPushMatrix();
        ofTranslate(x, y);
        ofScale(w/boundingBox.width, h/boundingBox.height);
        ofSetColor(255);
        ofDisableAlphaBlending();
        for(auto&& cam : cams) cam.draw();
        ofPopMatrix();
        ofPopStyle();
    }
}


void MultiCam::setupGui(string settingsPath) {
    ofVideoGrabber grabber;
    auto devices = grabber.listDevices();

    gui.addPage("MULTICAM").setXMLName(settingsPath);
    gui.addToggle("enabled", enabled);
    gui.addToggle("useFbo", useFbo);
    gui.addToggle("readFboToPixels", readFboToPixels);
    gui.addToggle("doDraw", doDraw);
    gui.addToggle("doScaleDraw", doScaleDraw);

    gui.addTitle("autoLayout");
    gui.addToggle("autoLayout.enabled", autoLayoutSettings.enabled);
    gui.addSlider("autoLayout.width", autoLayoutSettings.width, 0, 3840);
    gui.addSlider("autoLayout.height", autoLayoutSettings.height, 0, 1080);
    gui.addToggle("autoLayout.tileH", autoLayoutSettings.tileHorizontal);
    gui.addTitle();
    gui.addContent("fbo", fbo);

    int nCams = devices.size();
    cams.resize(nCams);

    for(int i=0; i<cams.size(); i++) {
        auto& cam = cams[i];
        string si = ofToString(i);
        gui.addTitle();
        gui.addTitle(si);
        gui.addContent(si, cam);
        gui.addToggle(si+".ctrl.enabled", cam.ctrl.enabled);
        gui.addSlider(si+".ctrl.pos.x", cam.ctrl.pos.x, 0, 3840);
        gui.addSlider(si+".ctrl.pos.y", cam.ctrl.pos.y, 0, 1080);
        gui.addSlider(si+".ctrl.scale.x", cam.ctrl.scale.x, 0, 10);
        gui.addSlider(si+".ctrl.scale.y", cam.ctrl.scale.y, 0, 10);
        gui.addToggle(si+".ctrl.showSettings", cam.ctrl.showSettings);

        gui.addSlider(si+".init.deviceid", cam.init.deviceid, 0, devices.size()).setValue(i);
        gui.addSlider(si+".init.w", cam.init.w, 0, 1920);
        gui.addSlider(si+".init.h", cam.init.h, 0, 1080);
        gui.addSlider(si+".init.fps", cam.init.fps, 0, 240);
        gui.addToggle(si+".init.reinit", cam.init.reinit);
    }


    gui.loadFromXML();
}


void MultiCam::updateBoundingBox() {
    boundingBox = ofRectangle();
    for(auto&& cam : cams) {
        if(cam.ctrl.enabled) {
            ofRectangle bb;
            bb.setPosition(cam.ctrl.pos.x, cam.ctrl.pos.y);
            bb.width = cam.grabber.getWidth() * cam.ctrl.scale.x;
            bb.height = cam.grabber.getHeight() * cam.ctrl.scale.y;
            boundingBox.growToInclude(bb);
        }
    }
}



void MultiCam::drawToFbo() {
    updateBoundingBox();
    if(!fbo.isAllocated() || (fbo.getWidth() != boundingBox.width) || (fbo.getHeight() != boundingBox.height)) {
        ofLogVerbose("ofxMSAMultiCam") << "Allocating FBO " << boundingBox;
        fbo.allocate(boundingBox.width, boundingBox.height, GL_RGB);
    }
    if(!fbo.isAllocated()) return;

    ofPushStyle();
    fbo.begin();
    ofClear(0);
    ofSetColor(255);
    ofDisableAlphaBlending();
    for(auto&& cam : cams) cam.draw();
    fbo.end();
    ofPopStyle();
}


void MultiCam::drawFbo(float x, float y, float w, float h) {
    ofPushStyle();
    ofPushMatrix();
    ofSetColor(255);
    if(w>0 && h>0) fbo.draw(x, y, w, h);
    else fbo.draw(x, y);
    ofPopMatrix();
    ofPopStyle();
}



} // namespace msa
