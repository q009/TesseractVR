#include "cube.h"

#include "engine.h"
#include "vrdev.h"

static const char *getdevicetypename(int type)
{
    static const char * const names[vr::VR_NUM_DEV_TYPES] =
    {
        "tracking reference", "controller", "tracker", "other", "HMD"
    };

    return names[type];
}

vr::vrdevices::vrdevices() : numdevices(0), hmd(NULL)
{
    memset(ctrlrmap, NULL, sizeof(ctrlrmap));
}

vr::vrdevices::~vrdevices()
{
    delete hmd;
    loopi(ARRAY_SIZE(periph)) loopj(periph[i].length()) delete periph[i][j];
}

void vr::vrdev::update(const matrix4 &m)
{
    pose = m;
}

vec vr::vrdev::getpos()
{
    return vec(pose.d).mul(vrscalefactor);
}

vec vr::vrdev::getposdelta()
{
    vec pos = getpos();
    vec delta = pos;

    delta.sub(prevpos);
    prevpos = pos;

    return delta;
}

vec vr::vrdev::getworldpos()
{
    if(type == VR_DEV_HMD) return camera1->o;

    vec pos = getpos();
    vec origin = vec(camera1->o).sub(gethmd()->getpos());

    return pos.add(origin);
}

vec vr::vrdev::getdir(vec basedir)
{
    return getorient().rotate(basedir);
}

void vr::vrdev::getangles(float &yaw, float &pitch, float &roll)
{
    // TODO: quat.calcangles() gives bad angles, investigate
    //vec angles = getorient().calcangles().div(RAD);

    //angles.div(RAD);

    //yaw = angles.y;
    //pitch = angles.x;
    //roll = angles.z;

    // Temporary measure:
    vectoyawpitch(getdir(), yaw, pitch);
    roll = 0;
}

quat vr::vrdev::getorient()
{
    return quat(pose);
}

void vr::vrdevices::mapdevice(vrdev *dev)
{
    switch(dev->type)
    {
        case VR_DEV_CONTROLLER:
            ctrlrmap[dev->role] = dev;
            break;
    }
}

void vr::vrdevices::setrole(vrdev *dev, int role)
{
    if(dev->role != VR_DEV_NO_ROLE && ctrlrmap[dev->role] == dev) ctrlrmap[dev->role] = NULL;

    // If the interface didn't inform us about the role of this controller, so let's assume one
    if(dev->type == VR_DEV_CONTROLLER && role == VR_DEV_NO_ROLE)
    {
        if(!ctrlrmap[VR_CONTROLLER_LEFT]) role = VR_CONTROLLER_LEFT;
        else if(!ctrlrmap[VR_CONTROLLER_RIGHT]) role = VR_CONTROLLER_RIGHT;
    }

    conoutf("VR: controller role: %d", role);
    dev->role = role;

    if(role != VR_DEV_NO_ROLE) mapdevice(dev);
}

vr::vrdev *vr::vrdevices::newdevice(int type)
{
    vrdev *dev = NULL;
    if(type == VR_DEV_CONTROLLER) dev = new vrcontroller();
    else dev = new vrdev(type);

    if((type == VR_DEV_HMD && hmd == dev) || (type != VR_DEV_HMD && periph[type].find(dev) >= 0))
    {
        conoutf("VR Warning: Attempted to add an already attached device");
        return NULL;
    }

    if(type == VR_DEV_HMD) hmd = dev;
    else periph[type].addunique(dev);

    ++numdevices;

    conoutf("VR: new device: %s", getdevicetypename(type));

    return dev;
}

void vr::vrdevices::removedevice(vrdev *dev)
{
    int type = dev->type;

    if(type == VR_DEV_HMD)
    {
        if(dev == hmd) hmd = NULL;
        else conoutf("VR Warning: removing a dangling HMD");
    }
    else
    {
        if(type < VR_NUM_DEV_TYPES)
        {
            int idx = periph[type].find(dev);
            if(idx >= 0) periph[type].remove(idx);
            else conoutf("VR Warning: removing a dangling device type %s", getdevicetypename(type));
        }
    }

    --numdevices;

    conoutf("VR: removed device: %s", getdevicetypename(type));

    delete dev;
}

void vr::vrcontroller::updateaxes(vec2 v)
{
    axes = vec2(-v.x, v.y);
}
