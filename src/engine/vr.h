#ifndef __VR_H__
#define __VR_H__

#include "cube.h"

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

    struct vrdevice
    {
        enum
        {
            VR_DEV_INVALID = -1,
            VR_DEV_NONE,
            VR_DEV_HMD,
            VR_DEV_CONTROLLER,
            VR_DEV_TRACKER,
            VR_DEV_TRACK_REF,
            VR_DEV_OTHER
        };

        int type;
        matrix4x3 pose;

        vrdevice() : type(VR_DEV_NONE) {}
    };

    struct vrinterface
    {
        virtual bool init() = 0;
        virtual void cleanup() = 0;
        virtual void update(vrdevice *devices) = 0;
        virtual void submitrender(vrbuffer &buf, int view) = 0;
        virtual void getresolution(uint &w, uint &h) = 0;
        virtual uint getmaxdevices() = 0;
        virtual matrix4 getviewprojection(int view) = 0;
        virtual matrix4x3 getviewtransform(int view) = 0;
    };

    struct vrcontext
    {
        uint normalw, normalh;
        bool active;
        int curview;
        vrinterface *interface;
        vrdevice *devices;
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
    vec getpos();
    vec getstep();
    vec getangles();
}

#endif
