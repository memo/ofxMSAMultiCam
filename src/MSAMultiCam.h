#pragma once

// you can also define these in project settings
//#define USE_OFXMACHINEVISION
//#define USE_OFXSPINNAKER

#ifdef USE_OFXSPINNAKER
#include "ofxSpinnaker.h"
#define USE_OFXMACHINEVISION
#endif

#ifdef USE_OFXMACHINEVISION
#include "ofxMachineVision.h"
#endif


#include "ofMain.h"
#include "ofxSimpleGuiToo.h"

namespace msa {

	class MultiCam
	{
	public:
		bool enabled = true;
		bool reinitialise = false; // rebuilds gui and reinits cameras
		bool playVideo = false; // play a video file
		bool readFboToPixels = false; // read multi camera composite fbo back to cpu (slow)
		bool doDraw = true; // draw to screen
		bool doDrawStretched = false; // stretch drawing to target rect
		float drawAlpha = 1; // alpha while drawing
		int numDevicesToUse = 0; // number of devices to use

		enum DeviceType { WebCam, Spinnaker };
		DeviceType deviceType = WebCam; // making this global, otherwise not sure which deviceType to enumerate at the start

		// auto layouts
		struct {
			bool enabled = false;
			int width = 1920;
			int height = 1080;
			bool tileHorizontal = true;
		} autoLayoutSettings;

#ifdef USE_OFXMACHINEVISION
#define GRABBER ofxMachineVision::Grabber::Simple
#define GRABBER_STR "ofxMachineVision"

		static shared_ptr<GRABBER> makeGrabber(DeviceType deviceType) {
			switch (deviceType) {
#ifdef USE_OFXSPINNAKER
			case Spinnaker: return make_shared<ofxMachineVision::SimpleGrabber<ofxMachineVision::Device::Spinnaker>>();
#endif
			}
			return make_shared<ofxMachineVision::SimpleGrabber<ofxMachineVision::Device::Webcam>>();
		}
#else
#define GRABBER ofVideoGrabber
#define GRABBER_STR "ofVideoGrabber"
		static shared_ptr<GRABBER> makeGrabber(DeviceType deviceType) {
			return make_shared<ofVideoGrabber>();
		}
#endif
		//--------------------------------------------------------------
		class Cam : public ofBaseDraws {
		public:
			shared_ptr<GRABBER> grabber;
			int id = -1;

			struct {
				int deviceid = 0; // TODO: make this work by name?
				int w = 1280;
				int h = 720;
				int fps = 30;
				//DeviceType deviceType = WebCam; // making this global, otherwise not sure which deviceType to enumerate at the start
			} init;

			struct {
				bool enabled = false;
				bool hflip = false;
				bool vflip = false;
				bool showSettings = false;
				int x = 0, y = 0;
				ofVec2f scale = ofVec2f(1, 1);
			} ctrl;

			struct {
				bool hasNewFrame = false;
				int w = 0;
				int h = 0;
				float fps = 0;
				float fpsAvg = 0;
				float lastCaptureTime = 0; // for calculating fps
			} info;

			void setup(DeviceType deviceType);
			void close();
			void update(DeviceType deviceType);
			void draw() const;

			// ofBaseDraws
			virtual void draw(float x, float y, float w, float h) const override;
			virtual float getWidth() const override;
			virtual float getHeight() const override;

		};
		vector<Cam> cams;

		void setup(ofxSimpleGuiToo& gui, string settingsPath = "settings/multicam.xml");
		void setupGui();
		void autoLayout();
		void update();
		void draw(float x = 0, float y = 0, float w = 0, float h = 0);

		void initCameras();
		void closeCameras();
		void flipCamerasH();
		void flipCamerasV();

		ofTexture& getTexture() { return fbo.getTexture(); }
		ofPixels& getPixels() { return pixels; }

		int getWidth() const { return width; }
		int getHeight() const { return height; }

	protected:
		int width, height;

		ofFbo fbo;
		ofPixels pixels; // only updated if readFboToPixels is true

		ofVideoPlayer player;
		string playerFilename = "resources/memo_wonderworks.mov";

		ofxSimpleGuiPage* guiPage = NULL;

		void updateBoundingBox();
	};

} // namespace msa

