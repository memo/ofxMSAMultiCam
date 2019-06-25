#include "MSAMultiCam.h"

namespace msa {

	void MultiCam::GrabberNode::setup(DeviceType deviceType) {
		ofLogNotice("ofxMSAMultiCam") << __func__ << " | id:" << id << ", device:" << init.deviceid;
		close();
		if (!ctrl.enabled) return;

		grabber = makeGrabber(deviceType);
		try {
#ifdef USE_OFXMACHINEVISION
			auto initSettings = grabber->getDefaultInitialisationSettings();
			try { // TODO is this the best way to do this?
				if (initSettings->contains("Device ID")) initSettings->getInt("Device ID").set(init.deviceid);
				if (initSettings->contains("Width")) initSettings->getInt("Width").set(init.w);
				if (initSettings->contains("Height"))initSettings->getInt("Height").set(init.h);
				if (initSettings->contains("Ideal frame rate"))initSettings->getInt("Ideal frame rate").set(init.fps);
			}
			catch (...) {
				ofLogError("ofxMultiCam") << "Error setting initialising settings";
			}
			grabber->open(initSettings);
			//grabber->open();

			//start the grabber capturing using default trigger
			grabber->startCapture();
#else
			grabber->setDeviceID(init.deviceid);
			grabber->setDesiredFrameRate(init.fps);
			grabber->setup(init.w, init.h);
#endif
		} catch (const std::exception &exc) {
			ofLogError("ofxMultiCam") << "Couldn't open device " << __func__ << " | id:" << id << ", device:" << init.deviceid;
			ofLogError("ofxMultiCam") << exc.what();
		}
	}

	void MultiCam::GrabberNode::close() {
		if (grabber) {
			ofLogNotice("ofxMSAMultiCam") << __func__ << " | id:" << id << ", device:" << init.deviceid;
			grabber->close();
			grabber = nullptr;
		}
	}

	void MultiCam::GrabberNode::update(DeviceType deviceType) {
		if (!ctrl.enabled) {
			close();
			return;
		}

		if (!grabber) {
			//        init.reinit = false;
			setup(deviceType);
		}

		if (grabber) {
			if (ctrl.showSettings) {
				ctrl.showSettings = false;
#ifndef USE_OFXMACHINEVISION
				grabber->videoSettings();
#endif
			}

			grabber->update();
			info.hasNewFrame = grabber->isFrameNew();
			if (info.hasNewFrame) {
				info.w = grabber->getWidth();
				info.h = grabber->getHeight();
				float nowTime = ofGetElapsedTimef();
				if (nowTime > info.lastCaptureTime) {
					info.fps = 1.0f / (nowTime - info.lastCaptureTime);
					info.fpsAvg += (info.fps - info.fpsAvg) * 0.1;
				}
				info.lastCaptureTime = nowTime;
			}
		}
	}

	void MultiCam::GrabberNode::draw() const {
		if (grabber) {
			draw(ctrl.x, ctrl.y, grabber->getWidth()*ctrl.scale.x, grabber->getHeight()*ctrl.scale.y);
		}
	}

	// ofBaseDraws
	void MultiCam::GrabberNode::draw(float x, float y, float w, float h) const {
		if (!ctrl.enabled) return;
		if (grabber) {
			ofPushMatrix();
			ofTranslate(x, y);
			if (ctrl.hflip) {
				ofTranslate(w, 0);
				ofScale(-1, 1, 1);
			}
			if (ctrl.vflip) {
				ofTranslate(0, h);
				ofScale(1, -1, 1);
			}
			grabber->draw(0, 0, w, h);
			ofPopMatrix();
		}
	}

	float MultiCam::GrabberNode::getWidth() const {
		return grabber ? grabber->getWidth() : 320;
	}

	float MultiCam::GrabberNode::getHeight() const {
		return grabber ? grabber->getHeight() : 320 / 1.7777;
	}

	//--------------------------------------------------------------
	//--------------------------------------------------------------
	void MultiCam::initCameras() {
		ofLogNotice("ofxMSAMultiCam") << __func__;
		closeCameras();
		for (auto&& cam : cams) cam.setup(deviceType);
	}


	// toggle hflip on all cameras (matching them all)
	void MultiCam::flipCamerasH() {
		ofLogNotice("ofxMSAMultiCam") << __func__;
		for (int i = 0; i < cams.size(); i++) cams[i].ctrl.hflip = (i == 0) ? !cams[0].ctrl.hflip : cams[0].ctrl.hflip;
	}

	// toggle vflip on all cameras (matching them all)
	void MultiCam::flipCamerasV() {
		ofLogNotice("ofxMSAMultiCam") << __func__;
		for (int i = 0; i < cams.size(); i++) cams[i].ctrl.vflip = (i == 0) ? !cams[0].ctrl.vflip : cams[0].ctrl.vflip;
	}


	void MultiCam::closeCameras() {
		ofLogNotice("ofxMSAMultiCam") << __func__;
		for (auto&& cam : cams) cam.close();
	}


	void MultiCam::autoLayout() {
		if (autoLayoutSettings.enabled) {
			// assuming simple equal dimensions for all images
			ofVec2f camSize(autoLayoutSettings.width, autoLayoutSettings.height);
			int numActive = 0;
			for (auto&& cam : cams) if (cam.ctrl.enabled) numActive++;
			if (numActive == 0) return;
			if (autoLayoutSettings.tileHorizontal) camSize.x /= numActive;
			int x = 0, y = 0;
			for (auto&& cam : cams) {
				if (cam.ctrl.enabled) {
					cam.ctrl.scale = camSize / ofVec2f(cam.getWidth(), cam.getHeight());
					cam.ctrl.x = x;
					cam.ctrl.y = y;
					if (autoLayoutSettings.tileHorizontal) x += cam.ctrl.scale.x * cam.getWidth();

				}
			}
		}
	}


	void MultiCam::update() {
		if (!enabled) return;

		if (reinitialise) {
			reinitialise = false;
			closeCameras();
			setupGui();
			initCameras();
		}

		// update cameras
		for (auto&& cam : cams) cam.update(deviceType);

		// update video
		if (playVideo) {
			if (!player.isLoaded()) {
				ofLogWarning("ofxMSAMultiCam") << __func__ << " | video player is not loaded. Loading " << playerFilename;
				player.load(playerFilename);
			}
			player.update();
		}

		// auto layout and update bounding box
		autoLayout();
		updateBoundingBox();

		// allocate fbo
		if (!fbo.isAllocated() || (fbo.getWidth() != width) || (fbo.getHeight() != height)) {
			ofLogWarning("ofxMSAMultiCam") << __func__ << " | FBO is " << ofVec2f(fbo.getWidth(), fbo.getHeight()) << ". Allocating " << ofVec2f(width, height);
			fbo.allocate(width, height, GL_RGB);
		}
		if (!fbo.isAllocated()) {
			ofLogError("ofxMSAMultiCam") << __func__ << " Could not allocate fbo " << ofVec2f(width, height);
			return;
		}

		// draw fbo
		ofPushStyle();
		fbo.begin();
		ofClear(0);
		ofSetColor(255);
		ofDisableAlphaBlending();
		ofDisableDepthTest();
		for (auto&& cam : cams) cam.draw();
		if (playVideo) player.draw(0, 0, width, height);
		fbo.end();
		ofPopStyle();

		// read back 
		if (readFboToPixels) fbo.readToPixels(pixels);
	}


	void MultiCam::draw(float x, float y, float w, float h) {
		if (!enabled) return;
		if (!(doDraw || doDrawStretched)) return;

		if (w < 1 || !doDrawStretched) w = width;
		if (h < 1 || !doDrawStretched) h = height;

		ofPushStyle();
		ofEnableAlphaBlending();
		ofSetColor(255, 255 * drawAlpha);
		if (w > 0 && h > 0) fbo.draw(x, y, w, h);
		else fbo.draw(x, y);
		ofPopStyle();

		//} else {
			//ofPushStyle();
			//ofPushMatrix();
			//ofTranslate(x, y);
			//ofScale(w/width, h/height);
			//ofEnableAlphaBlending();
			//ofSetColor(255, 255*drawAlpha);
			//for(auto&& cam : cams) cam.draw();
			//ofPopMatrix();
			//ofPopStyle();
		//}
	}



	void MultiCam::setup(ofxSimpleGuiToo& gui, string settingsPath) {
		guiPage = &gui.addPage("MULTICAM").setXMLName(settingsPath);
		setupGui();
	}


	void MultiCam::setupGui() {
		auto tempGrabber = makeGrabber(deviceType);
#ifdef USE_OFXMACHINEVISION
		ofLogNotice("ofxMSAMultiCam") << "Using ofxMachineVision";
		auto devices = tempGrabber->getDevice()->listDevices(); // returns empty for webcam. is this normal?
		for (auto deviceDescription : devices) cout << deviceDescription.manufacturer << ", " << deviceDescription.model << endl;
#else
		ofLogNotice("ofxMSAMultiCam") << "Using ofVideoGrabber";
		auto devices = tempGrabber->listDevices();
#endif
		int totalnumDevicesToUse = devices.size();
		//if (nCams == 0) nCams = 1; 
		totalnumDevicesToUse = 32; // ofxMachineVision returns no webcams, TODO bug?

		guiPage->clear();
		guiPage->addToggle("enabled", enabled);
		guiPage->addButton("reinitialise", reinitialise);
		guiPage->addToggle("playVideo", playVideo);
		guiPage->addToggle("readFboToPixels", readFboToPixels);
		guiPage->addToggle("doDraw", doDraw);
		guiPage->addToggle("doDrawStretched", doDrawStretched);
		guiPage->addSlider("drawAlpha", drawAlpha, 0, 1);
		guiPage->addTitle(GRABBER_STR);
#ifdef USE_OFXMACHINEVISION
		vector<string> choices = { "WebCam", "Spinnaker" };
		guiPage->addComboBox("deviceType", (int&)deviceType, 2, choices.data());
#endif
		guiPage->addTitle("Total devices found: " + ofToString(devices.size()));
		guiPage->addSlider("numDevicesToUse", numDevicesToUse, 1, totalnumDevicesToUse);
		guiPage->loadFromXML();

		guiPage->addTitle("autoLayout");
		guiPage->addToggle("autoLayout.enabled", autoLayoutSettings.enabled);
		guiPage->addSlider("autoLayout.width", autoLayoutSettings.width, 0, 8192);
		guiPage->addSlider("autoLayout.height", autoLayoutSettings.height, 0, 8192);
		guiPage->addToggle("autoLayout.tileH", autoLayoutSettings.tileHorizontal);
		guiPage->addTitle("Output");
		guiPage->addSlider("output.width", width, 0, 8192);
		guiPage->addSlider("output.height", height, 0, 8192);
		guiPage->addContent("fbo", fbo);


		cams.resize(numDevicesToUse);
		for (int i = 0; i < cams.size(); i++) {
			auto& cam = cams[i];
			cam.id = i;
			string si = ofToString(i);
			//        guiPage->addTitle(si).setNewColumn();
			guiPage->addTitle(si + ".init").setNewColumn();;
			guiPage->addSlider(si + ".init.deviceid", cam.init.deviceid, 0, totalnumDevicesToUse - 1).setValue(i);
			guiPage->addSlider(si + ".init.w", cam.init.w, 0, 1920);
			guiPage->addSlider(si + ".init.h", cam.init.h, 0, 1080);
			guiPage->addSlider(si + ".init.fps", cam.init.fps, 0, 240);

			guiPage->addTitle(si + ".ctrl");
			guiPage->addToggle(si + ".ctrl.enabled", cam.ctrl.enabled);
			guiPage->addToggle(si + ".ctrl.hflip", cam.ctrl.hflip);
			guiPage->addToggle(si + ".ctrl.vflip", cam.ctrl.vflip);
			guiPage->addSlider(si + ".ctrl.pos.x", cam.ctrl.x, 0, 3840);
			guiPage->addSlider(si + ".ctrl.pos.y", cam.ctrl.y, 0, 1080);
			guiPage->addSlider(si + ".ctrl.scale.x", cam.ctrl.scale.x, 0, 10);
			guiPage->addSlider(si + ".ctrl.scale.y", cam.ctrl.scale.y, 0, 10);
			guiPage->addToggle(si + ".ctrl.showSettings", cam.ctrl.showSettings);

			guiPage->addTitle(si + ".info");
			guiPage->addToggle(si + ".info.hasNewFrame", cam.info.hasNewFrame);
			guiPage->addSlider(si + ".info.w", cam.info.w, 0, 1920);
			guiPage->addSlider(si + ".info.h", cam.info.h, 0, 1080);
			guiPage->addSlider(si + ".info.fps", cam.info.fps, 0, 240);
			guiPage->addSlider(si + ".info.fpsAvg", cam.info.fpsAvg, 0, 240);
			guiPage->addContent(si, cam);
		}

		guiPage->loadFromXML();

		fbo.allocate(width, height, GL_RGB);
	}

	void MultiCam::updateBoundingBox() {
		ofRectangle boundingBox;
		for (auto&& cam : cams) {
			if (cam.ctrl.enabled && cam.grabber) {
				ofRectangle bb;
				bb.setPosition(cam.ctrl.x, cam.ctrl.y);
				bb.width = cam.grabber->getWidth() * cam.ctrl.scale.x;
				bb.height = cam.grabber->getHeight() * cam.ctrl.scale.y;
				boundingBox.growToInclude(bb);
			}
		}
		width = round(boundingBox.width);
		height = round(boundingBox.height);
	}


} // namespace msa
