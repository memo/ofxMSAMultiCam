#pragma once

#include "ofMain.h"

namespace msa {

class MultiCam
{
public:
    bool enabled = true;
    bool useFbo = true;
    bool readFboToPixels = false;
    bool doDraw = true;
    bool doDrawStretched = false;
    float drawAlpha = 1;

    // auto layouts
    struct {
        bool enabled = false;
        int width = 1920;
        int height = 1080;
        bool tileHorizontal = true;
    } autoLayoutSettings;

    //--------------------------------------------------------------
    class Cam : public ofBaseDraws {
    public:
        ofVideoGrabber grabber;
        int id = -1;

        struct {
            int deviceid=0; // TODO: make this work by name?
            int w=1280;
            int h=720;
            int fps=30;
//            bool reinit=false;
//            bool dummy;
        } init;

        struct {
            bool enabled = true;
            bool hflip = false;
            bool vflip = false;
            bool showSettings = false;
            ofVec2f pos = ofVec2f(0, 0);
            ofVec2f scale = ofVec2f(1, 1);
        } ctrl;

        struct {
            bool hasNewFrame = false;
            int w = 0;
            int h = 0;
            float fps = 0;
            float lastCaptureTime = 0; // for calculating fps
        } info;

        void setup();
        void close();
        void update();
        void draw() const;

        // ofBaseDraws
        virtual void draw(float x, float y, float w, float h) const override;
        virtual float getWidth() const override;
        virtual float getHeight() const override;

    };
    vector<Cam> cams;

    void setupGui(string settingsPath="settings/multicam.xml");
    void autoLayout();
    void update();
    void draw(float x=0, float y=0, float w=0, float h=0);

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

    void updateBoundingBox();
    void drawToFbo();
    void drawFbo(float x=0, float y=0, float w=0, float h=0);
};

} // namespace msa

