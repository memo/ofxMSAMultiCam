#include "MSAMultiCam.h"

namespace msa {

void MultiCam::Cam::setup() {
    ofLogNotice("ofxMSAMultiCam") << __func__ << " | id:" << id << ", device:" << init.deviceid;
    close();
    if(!ctrl.enabled) return;
    grabber.setDeviceID(init.deviceid);
    grabber.setDesiredFrameRate(init.fps);
    grabber.setup(init.w, init.h);
    info.w = grabber.getWidth();
    info.h = grabber.getHeight();
}

void MultiCam::Cam::close() {
    if(grabber.isInitialized()) {
        ofLogNotice("ofxMSAMultiCam") << __func__ << " | id:" << id << ", device:" << init.deviceid;
        grabber.close();
    }
}

void MultiCam::Cam::update() {
    if(!ctrl.enabled) {
        close();
        return;
    }

    if(!grabber.isInitialized()) {
        //        init.reinit = false;
        setup();
    }

    if(ctrl.showSettings) {
        ctrl.showSettings = false;
        grabber.videoSettings();
    }

    grabber.update();
    info.hasNewFrame = grabber.isFrameNew();
    if(info.hasNewFrame) {
        float nowTime = ofGetElapsedTimef();
        if(nowTime > info.lastCaptureTime) info.fps = 1.0f/(nowTime - info.lastCaptureTime);
        info.lastCaptureTime = nowTime;
    }
}

void MultiCam::Cam::draw() const {
    draw(ctrl.pos.x, ctrl.pos.y, grabber.getWidth()*ctrl.scale.x, grabber.getHeight()*ctrl.scale.y);
}

// ofBaseDraws
void MultiCam::Cam::draw(float x, float y, float w, float h) const {
    if(!ctrl.enabled) return;
    if(grabber.isInitialized()) {
        ofPushMatrix();
        ofTranslate(x, y);
        if(ctrl.hflip) {
            ofTranslate(w, 0);
            ofScale(-1, 1, 1);
        }
        if(ctrl.vflip) {
            ofTranslate(0, h);
            ofScale(1, -1, 1);
        }
        grabber.draw(0, 0, w, h);
        ofPopMatrix();
    }
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
    ofLogNotice("ofxMSAMultiCam") << __func__;
    closeCameras();
    for(auto&& cam : cams) cam.setup();
}


// toggle hflip on all cameras (matching them all)
void MultiCam::flipCamerasH() {
    ofLogNotice("ofxMSAMultiCam") << __func__;
    for(int i=0; i<cams.size(); i++) cams[i].ctrl.hflip = (i==0) ? !cams[0].ctrl.hflip : cams[0].ctrl.hflip;
}

// toggle vflip on all cameras (matching them all)
void MultiCam::flipCamerasV() {
    ofLogNotice("ofxMSAMultiCam") << __func__;
    for(int i=0; i<cams.size(); i++) cams[i].ctrl.vflip = (i==0) ? !cams[0].ctrl.vflip : cams[0].ctrl.vflip;
}


void MultiCam::closeCameras() {
    ofLogNotice("ofxMSAMultiCam") << __func__;
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
    if(!(doDraw || doDrawStretched)) return;

    if(w<1 || !doDrawStretched) w = width;
    if(h<1 || !doDrawStretched) h = height;
    if(useFbo) {
        drawFbo(x, y, w, h);
    } else {
        ofPushStyle();
        ofPushMatrix();
        ofTranslate(x, y);
        ofScale(w/width, h/height);
        ofEnableAlphaBlending();
        ofSetColor(255, 255*drawAlpha);
        for(auto&& cam : cams) cam.draw();
        ofPopMatrix();
        ofPopStyle();
    }
}


void MultiCam::setup(ofxSimpleGuiToo& gui, string settingsPath) {
    ofVideoGrabber grabber;
    auto devices = grabber.listDevices();

    gui.addPage("MULTICAM").setXMLName(settingsPath);
    gui.addToggle("enabled", enabled);
    gui.addToggle("useFbo", useFbo);
    gui.addToggle("readFboToPixels", readFboToPixels);
    gui.addToggle("doDraw", doDraw);
    gui.addToggle("doDrawStretched", doDrawStretched);
    gui.addSlider("drawAlpha", drawAlpha, 0, 1);

    gui.addTitle("autoLayout");
    gui.addToggle("autoLayout.enabled", autoLayoutSettings.enabled);
    gui.addSlider("autoLayout.width", autoLayoutSettings.width, 0, 4096);
    gui.addSlider("autoLayout.height", autoLayoutSettings.height, 0, 4096);
    gui.addToggle("autoLayout.tileH", autoLayoutSettings.tileHorizontal);
    gui.addTitle("Output");
    gui.addSlider("output.width", width, 0, 8192);
    gui.addSlider("output.height", height, 0, 8192);
    gui.addContent("fbo", fbo);

    int nCams = devices.size();
    cams.resize(nCams);
    for(int i=0; i<cams.size(); i++) {
        auto& cam = cams[i];
        cam.id = i;
        string si = ofToString(i);
        //        gui.addTitle(si).setNewColumn();
        gui.addTitle(si+".init").setNewColumn();;
        gui.addSlider(si+".init.deviceid", cam.init.deviceid, 0, devices.size()-1).setValue(i);
        gui.addSlider(si+".init.w", cam.init.w, 0, 1920);
        gui.addSlider(si+".init.h", cam.init.h, 0, 1080);
        gui.addSlider(si+".init.fps", cam.init.fps, 0, 240);
        //        gui.addToggle(si+".init.reinit", cam.init.reinit);

        gui.addTitle(si+".ctrl");
        gui.addToggle(si+".ctrl.enabled", cam.ctrl.enabled);
        gui.addToggle(si+".ctrl.hflip", cam.ctrl.hflip);
        gui.addToggle(si+".ctrl.vflip", cam.ctrl.vflip);
        gui.addSlider(si+".ctrl.pos.x", cam.ctrl.pos.x, 0, 3840);
        gui.addSlider(si+".ctrl.pos.y", cam.ctrl.pos.y, 0, 1080);
        gui.addSlider(si+".ctrl.scale.x", cam.ctrl.scale.x, 0, 10);
        gui.addSlider(si+".ctrl.scale.y", cam.ctrl.scale.y, 0, 10);
        gui.addToggle(si+".ctrl.showSettings", cam.ctrl.showSettings);

        gui.addTitle(si+".info");
        gui.addToggle(si+".info.hasNewFrame", cam.info.hasNewFrame);
        gui.addSlider(si+".info.w", cam.info.w, 0, 1920);
        gui.addSlider(si+".info.h", cam.info.h, 0, 1080);
        gui.addSlider(si+".info.fps", cam.info.fps, 0, 240);
        gui.addContent(si, cam);
    }

    gui.loadFromXML();

	fbo.allocate(width, height, GL_RGB);
}


void MultiCam::updateBoundingBox() {
    ofRectangle boundingBox;
    for(auto&& cam : cams) {
        if(cam.ctrl.enabled) {
            ofRectangle bb;
            bb.setPosition(cam.ctrl.pos.x, cam.ctrl.pos.y);
            bb.width = cam.grabber.getWidth() * cam.ctrl.scale.x;
            bb.height = cam.grabber.getHeight() * cam.ctrl.scale.y;
            boundingBox.growToInclude(bb);
        }
    }
    width = round(boundingBox.width);
    height = round(boundingBox.height);
}



void MultiCam::drawToFbo() {
    updateBoundingBox();
    if(!fbo.isAllocated() || (fbo.getWidth() != width) || (fbo.getHeight() != height)) {
        ofLogWarning("ofxMSAMultiCam") << __func__ << " | FBO is " << ofVec2f(fbo.getWidth(), fbo.getHeight()) << ". Allocating " << ofVec2f(width, height);
        fbo.allocate(width, height, GL_RGB);
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
    ofEnableAlphaBlending();
    ofSetColor(255, 255*drawAlpha);
    if(w>0 && h>0) fbo.draw(x, y, w, h);
    else fbo.draw(x, y);
    ofPopStyle();
}



} // namespace msa
