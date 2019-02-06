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
    bool doScaleDraw = false;

    // auto layouts
    struct {
        bool enabled = false;
        int width = 1920;
        int height = 1080;
        bool tileHorizontal = true;
    } autoLayoutSettings;

    //--------------------------------------------------------------
    ofFbo fbo;
    ofPixels pixels;

    class Cam : public ofBaseDraws {
    public:
        ofVideoGrabber grabber;

        struct {
            int deviceid=0; // TODO: make this work by name?
            int w=1280;
            int h=720;
            int fps=30;
            bool reinit=false;
            bool dummy;
        } init;

        struct {
            bool enabled=true;
            bool showSettings=false;
            ofVec2f pos;
            ofVec2f scale=ofVec2f(1, 1);
        } ctrl;

        struct {
            float fps;
        } stats;

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

    void initCameras();
    void close();
    void autoLayout();
    void update();
    void draw(float x=0, float y=0, float w=0, float h=0);

    void setupGui(string settingsPath="settings/multicam.xml");

protected:
    ofRectangle boundingBox;

    void updateBoundingBox();
    void drawToFbo();
    void drawFbo(float x=0, float y=0, float w=0, float h=0);

};

} // namespace msa

