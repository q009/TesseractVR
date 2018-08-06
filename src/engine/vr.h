#ifndef __VR_H__
#define __VR_H__

#include "cube.h"
#include "vrdev.h"

extern float vrscalefactor;

namespace vr
{
    enum
    {
        VR_VIEW_LEFT = 0,
        VR_VIEW_RIGHT,

        VR_NUM_VIEWS
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

        vrcontext() : active(false), curview(VR_VIEW_LEFT) {}
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
    vrdev *getcontroller(int role);
}

#endif
