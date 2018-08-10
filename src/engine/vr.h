#ifndef __VR_H__
#define __VR_H__

#include "cube.h"
#include "vrdev.h"

extern float vrscalefactor;
extern int vrmovestyle;

#define VR_KEYCODE_BASE          0xF0000000
#define VR_BUTTON_TOUCH_BIT      8          // Identifies touch events
#define VR_BUTTON_TOUCH_LEFT_BIT 16         // Identifies the left controller

namespace vr
{
    enum
    {
        VR_VIEW_LEFT = 0,
        VR_VIEW_RIGHT,

        VR_NUM_VIEWS
    };

    // Button codes
    enum
    {
        VR_BUTTON_INVALID = 0,
        VR_BUTTON_MENU,
        VR_BUTTON_GRIP,
        VR_BUTTON_TOUCHPAD,
        VR_BUTTON_TRIGGER
    };

    enum
    {
        VR_MOVE_STYLE_LCTRLR = 0,
        VR_MOVE_STYLE_RCTRLR,
        VR_MOVE_STYLE_HMD
    };

    struct vrbuffer
    {
        GLuint resolvefb;
        GLuint resolvetex;
    };

    struct vrinterface
    {
        virtual bool init() = 0;
        virtual void cleanup() = 0;
        virtual void update(vrdevices &devices) = 0;
        virtual void submitrender(vrbuffer &buf, int view) = 0;
        virtual void getresolution(uint &w, uint &h) = 0;
        virtual int getmaxdevices() = 0;
        virtual matrix4 getviewprojection(int view) = 0;
        virtual matrix4x3 getviewtransform(int view) = 0;
    };

    struct vrcontext
    {
        uint normalw, normalh;
        bool active;
        int curview;
        vrinterface *interface;
        vrdevices devices;
        vrbuffer buffers[VR_NUM_VIEWS];

        bool moving;

        vrcontext() : active(false), curview(VR_VIEW_LEFT), moving(false) {}
    };

    void init();
    void cleanup();
    void update();
    void setview(int view);
    void finishrender();
    void submitrender();
    bool isenabled();
    matrix4 getviewtransform();
    matrix4 getviewprojection();
    int getnumdevices(int type);
    vrdev *getdevice(int type, int index = 0);
    vrdev *gethmd();
    vrcontroller *getcontroller(int role);
}

#endif
