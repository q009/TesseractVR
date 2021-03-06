#include "cube.h"
#include "engine.h"

#ifdef VR_OPENVR

#include "openvrinterface.h"

static matrix4 convposematrix(vr::HmdMatrix34_t m)
{
    return vr::matrixrh2lh(matrix4x3(vec(m.m[0][0], m.m[1][0], m.m[2][0]),
                                     vec(m.m[0][1], m.m[1][1], m.m[2][1]),
                                     vec(m.m[0][2], m.m[1][2], m.m[2][2]),
                                     vec(m.m[0][3], m.m[1][3], m.m[2][3])));
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

    memset(devmap, NULL, sizeof(devmap));
    ctrlrroles[VR_CONTROLLER_LEFT] = ctrlrroles[VR_CONTROLLER_RIGHT] = -1;

    return true;
}

void vr::openvrinterface::cleanup()
{
    VR_Shutdown();
    sys = NULL;
}

int vr::openvrinterface::getdevicetype(int index)
{
    int ret = VR_DEV_OTHER;

    switch(sys->GetTrackedDeviceClass(index))
    {
        case TrackedDeviceClass_Controller:
            ret = VR_DEV_CONTROLLER; break;
        case TrackedDeviceClass_HMD:
            ret = VR_DEV_HMD;        break;
        case TrackedDeviceClass_Invalid:
            ret = VR_DEV_INVALID;    break;
        case TrackedDeviceClass_GenericTracker:
            ret = VR_DEV_TRACKER;    break;
        case TrackedDeviceClass_TrackingReference:
            ret = VR_DEV_TRACK_REF;  break;
    }

    return ret;
}

void vr::openvrinterface::recalcdevice(vrdevices &devices, int index)
{
    TrackedDevicePose_t &p = trackinfo[index];
    if(p.bPoseIsValid)
    {
        if(!devmap[index]) newdevice(devices, index);
        else devmap[index]->update(convposematrix(p.mDeviceToAbsoluteTracking));
    }
}

void vr::openvrinterface::updatedevices(vrdevices &devices)
{
    VRCompositor()->WaitGetPoses(trackinfo, getmaxdevices(), NULL, 0);
    loopi(getmaxdevices()) recalcdevice(devices, i);
}

int vr::openvrinterface::getcontrollerrole(int index)
{
    int role = VR_DEV_NO_ROLE;

    ETrackedControllerRole r = sys->GetControllerRoleForTrackedDeviceIndex(index);
    conoutf("OpenVR: controller role %d, index %d", r, index);

    switch(r)
    {
        case TrackedControllerRole_LeftHand:
            role = VR_CONTROLLER_LEFT;
            break;

        case TrackedControllerRole_RightHand:
            role = VR_CONTROLLER_RIGHT;
            break;
    }

    return role;
}

void vr::openvrinterface::updatecontrollerrole(vrdevices &devices, int index)
{
    if(!devmap[index])
    {
        conoutf("OpenVR Error: attempted to set a role of a NULL device at index %d", index);
        return;
    }

    if(devmap[index]->type == VR_DEV_CONTROLLER)
    {
        int role = getcontrollerrole(index);

        if(devmap[index]->role != role)
        {
            if(index == ctrlrroles[devmap[index]->role]) ctrlrroles[role] = -1;
            devices.setrole(devmap[index], role);
            if(role != VR_DEV_NO_ROLE) ctrlrroles[role] = index;
        }
    }
}

void vr::openvrinterface::newdevice(vrdevices &devices, int index)
{
    devmap[index] = devices.newdevice(getdevicetype(index));
    updatecontrollerrole(devices, index);

    conoutf("OpenVR: new device index %d", index);
}

void vr::openvrinterface::removedevice(vrdevices &devices, int index)
{
    if(!devmap[index])
    {
        conoutf("OpenVR Error: attempted to remove a NULL device at index %d", index);
        return;
    }

    devices.removedevice(devmap[index]);
    devmap[index] = NULL;

    conoutf("OpenVR: removed device index %d", index);
}

int vr::openvrinterface::getbuttoncode(VREvent_t event)
{
    int res = VR_BUTTON_INVALID;
    bool touch = event.eventType == VREvent_ButtonTouch ||
                 event.eventType == VREvent_ButtonUntouch;

    switch(event.data.controller.button)
    {
        case k_EButton_ApplicationMenu:
            res = VR_BUTTON_MENU;
            break;

        case k_EButton_Grip:
            res = VR_BUTTON_GRIP;
            break;

        case k_EButton_SteamVR_Touchpad:
            res = VR_BUTTON_TOUCHPAD;
            break;

        case k_EButton_SteamVR_Trigger:
            res = VR_BUTTON_TRIGGER;
            break;
    }

    if(res != VR_BUTTON_INVALID)
    {
        res |= VR_KEYCODE_BASE;
        res |= touch << VR_BUTTON_TOUCH_BIT;
        res |= (ctrlrroles[VR_CONTROLLER_LEFT] == event.trackedDeviceIndex) << VR_BUTTON_TOUCH_LEFT_BIT;
    }

    return res;
}

void vr::openvrinterface::handleevent(vrdevices &devices, VREvent_t event)
{
    switch(event.eventType)
    {
        case VREvent_TrackedDeviceActivated:
            newdevice(devices, event.trackedDeviceIndex);
            break;

        case VREvent_TrackedDeviceDeactivated:
            removedevice(devices, event.trackedDeviceIndex);
            break;

        case VREvent_TrackedDeviceRoleChanged:
            loopi(getmaxdevices()) if(devmap[i]) updatecontrollerrole(devices, i);
            break;

        case VREvent_ButtonTouch:
            // The touch events for the triggers are quite erratic with HTC Vive wands, filter them out
            if(event.data.controller.button == k_EButton_SteamVR_Trigger) return;
        case VREvent_ButtonPress:
            processkey(getbuttoncode(event), true);
            break;

        case VREvent_ButtonUntouch:
            // The touch events for the triggers are quite erratic with HTC Vive wands, filter them out
            if(event.data.controller.button == k_EButton_SteamVR_Trigger) return;
        case VREvent_ButtonUnpress:
            processkey(getbuttoncode(event), false);
            break;
    }
}

void vr::openvrinterface::pollevents(vrdevices &devices)
{
    VREvent_t event;
    while(sys->PollNextEvent(&event, sizeof(VREvent_t))) handleevent(devices, event);
}

void vr::openvrinterface::updatecontrollers()
{
    loopi(VR_NUM_CONTROLLERS)
    {
        int idx = ctrlrroles[i];
        if(idx < 0) continue;

        VRControllerState_t state;
        sys->GetControllerState(idx, &state, sizeof(VRControllerState_t));

        vrcontroller *ctrlr = (vrcontroller *)devmap[idx];
        ctrlr->updateaxes(vec2(state.rAxis[0].x, state.rAxis[0].y));
    }
}

void vr::openvrinterface::update(vrdevices &devices)
{
    pollevents(devices);
    updatedevices(devices);
    updatecontrollers();
}

void vr::openvrinterface::submitrender(vrbuffer &buf, int view)
{
    Texture_t tex = { (void *)(uintptr_t)buf.resolvetex, TextureType_OpenGL, ColorSpace_Gamma };
    VRCompositor()->Submit(geteye(view), &tex);
}

void vr::openvrinterface::getresolution(uint &w, uint &h)
{
    sys->GetRecommendedRenderTargetSize(&w, &h);
}

int vr::openvrinterface::getmaxdevices()
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

inline matrix4 vr::openvrinterface::getviewtransform(int view)
{
    return convposematrix(sys->GetEyeToHeadTransform(geteye(view)));
}

vr::openvrinterface *vr::getopenvrinterface()
{
    static openvrinterface openvr;
    return &openvr;
}

#endif
