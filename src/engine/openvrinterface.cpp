#include "cube.h"
#include "engine.h"

#ifdef VR_OPENVR

#include "openvrinterface.h"

static matrix4x3 convposematrix(vr::HmdMatrix34_t m)
{
    return matrix4x3(vec(m.m[0][0], m.m[1][0], m.m[2][0]),
                     vec(m.m[0][1], m.m[1][1], m.m[2][1]),
                     vec(m.m[0][2], m.m[1][2], m.m[2][2]),
                     vec(m.m[0][3], m.m[1][3], m.m[2][3]));
}

static inline vr::EVREye geteye(int view)
{
    return view == vr::VR_VIEW_LEFT ? vr::Eye_Left : vr::Eye_Right;
}

bool vr::openvrinterface::init()
{
    vr::EVRInitError err;
    sys = VR_Init(&err, VRApplication_Scene);

    if(err != VRInitError_None)
    {
        conoutf("unable to init OpenVR runtime: %s", VR_GetVRInitErrorAsEnglishDescription(err));
        cleanup();
        return false;
    }

    return true;
}

void vr::openvrinterface::cleanup()
{
    VR_Shutdown();
    sys = NULL;
}

int vr::openvrinterface::getdevicetype(int index)
{
    int ret = vrdevice::VR_DEV_OTHER;

    switch(sys->GetTrackedDeviceClass(index))
    {
        case TrackedDeviceClass_Controller:
            ret = vrdevice::VR_DEV_CONTROLLER; break;
        case TrackedDeviceClass_HMD:
            ret = vrdevice::VR_DEV_HMD;        break;
        case TrackedDeviceClass_Invalid:
            ret = vrdevice::VR_DEV_INVALID;    break;
        case TrackedDeviceClass_GenericTracker:
            ret = vrdevice::VR_DEV_TRACKER;    break;
        case TrackedDeviceClass_TrackingReference:
            ret = vrdevice::VR_DEV_TRACK_REF;  break;
    }

    return ret;
}

void vr::openvrinterface::updateposes(vrdevice *devices)
{
    VRCompositor()->WaitGetPoses(trackinfo, getmaxdevices(), NULL, 0);
    loopi(getmaxdevices())
    {
        TrackedDevicePose_t &p = trackinfo[i];
        if(p.bPoseIsValid)
        {
            devices[i].pose = convposematrix(p.mDeviceToAbsoluteTracking);
            if(devices[i].type == vrdevice::VR_DEV_NONE) devices[i].type = getdevicetype(i);
        }
    }
}

void vr::openvrinterface::update(vrdevice *devices)
{
    updateposes(devices);
}

void vr::openvrinterface::submitrender(vrbuffer &buf, int view)
{
    Texture_t tex = { (void*)buf.resolvetex, TextureType_OpenGL, ColorSpace_Gamma };
    VRCompositor()->Submit(geteye(view), &tex);
}

void vr::openvrinterface::getresolution(uint &w, uint &h)
{
    sys->GetRecommendedRenderTargetSize(&w, &h);
}

uint vr::openvrinterface::getmaxdevices()
{
    return k_unMaxTrackedDeviceCount;
}

matrix4 vr::openvrinterface::getviewprojection(int view)
{
    HmdMatrix44_t m = sys->GetProjectionMatrix(geteye(view), nearplane, farplane);
    return matrix4(vec4(m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0]),
                   vec4(m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1]), 
                   vec4(m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2]), 
                   vec4(m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3]));
}

inline matrix4x3 vr::openvrinterface::getviewtransform(int view)
{
    return convposematrix(sys->GetEyeToHeadTransform(geteye(view)));
}

vr::openvrinterface *vr::getopenvrinterface()
{
    static openvrinterface openvr;
    return &openvr;
}

#endif
