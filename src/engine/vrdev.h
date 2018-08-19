#ifndef __VRDEV_H__
#define __VRDEV_H__

#include "cube.h"

#define VR_DEV_NO_ROLE -1

namespace vr
{
    // Device types
    enum
    {
        VR_DEV_TRACK_REF = 0,
        VR_DEV_CONTROLLER,
        VR_DEV_TRACKER,
        VR_DEV_OTHER,

        // Permament device, last on the list
        VR_DEV_HMD,

        VR_NUM_DEV_TYPES,

        VR_DEV_INVALID,
        VR_DEV_NONE,
    };

    // Controller roles
    enum
    {
        VR_CONTROLLER_LEFT = 0,
        VR_CONTROLLER_RIGHT,

        VR_NUM_CONTROLLERS
    };

    struct vrdev
    {
        int type;
        int role;
        matrix4 pose;
        vec prevpos;

        vrdev(int _type) : type(_type), role(-1), pose(vec(1, 0, 0), vec(0, 1, 0), vec(0, 0, 1)),
                           prevpos(0) {}

        void update(const matrix4 &m);
        vec getpos();
        vec getposdelta();
        vec getworldpos();
        vec getdir(vec basedir = vec(0, 1, 0));
        void getangles(float &yaw, float &pitch, float &roll);
        quat getorient();
    };

    struct vrcontroller : vrdev
    {
        vec2 axes;

        vrcontroller() : vrdev(VR_DEV_CONTROLLER) {}
        void updateaxes(vec2 v);
    };

    struct vrdevices
    {
        uint numdevices;
        vrdev *hmd;
        vector<vrdev*> periph[VR_NUM_DEV_TYPES - 1];

        // Convenience map
        vrdev *ctrlrmap[VR_NUM_CONTROLLERS];

        vrdevices();
        ~vrdevices();

        void mapdevice(vrdev *dev);
        void setrole(vrdev *dev, int role);
        vrdev *newdevice(int type);
        void removedevice(vrdev *dev);
    };
}

#endif
