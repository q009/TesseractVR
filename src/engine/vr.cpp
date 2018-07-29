#include "cube.h"
#include "engine.h"

#ifdef VR_OPENVR
#include "openvrinterface.h"
#endif

extern void screenres(int w, int h);

FVARP(vrscalefactor, 0.0001f, 12, 1000);

vr::vrcontext *vrc;

static vr::vrdevice &gethmd()
{
    return vrc->devices[0];
}

static void initbuffer(vr::vrbuffer &buf)
{
    glGenFramebuffers_(1, &buf.resolvefb);
    glBindFramebuffer_(GL_FRAMEBUFFER, buf.resolvefb);

    glGenTextures(1, &buf.resolvetex);
    glBindTexture(GL_TEXTURE_2D, buf.resolvetex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenw, screenh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glFramebufferTexture2D_(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buf.resolvetex, 0);

    GLenum res = glCheckFramebufferStatus_(GL_FRAMEBUFFER);

    if(res != GL_FRAMEBUFFER_COMPLETE)
        conoutf("failed to create VR buffers: 0x%X", res);

    glBindFramebuffer_(GL_FRAMEBUFFER, 0);
}

static void initscreen()
{
    vrc->normalw = screenw;
    vrc->normalh = screenh;

    uint neww, newh;
    vrc->interface->getresolution(neww, newh);

    screenres(neww, newh);
}

void vr::init()
{
    if(!vrc) vrc = new vrcontext;

#ifdef VR_OPENVR
    vrc->interface = getopenvrinterface();
#endif

    if(vrc->interface)
    {
        if(!vrc->interface->init()) vrc->interface = NULL;
        else
        {
            vrc->devices = new vrdevice[vrc->interface->getmaxdevices()];

            initscreen();
            loopi(vr::VR_NUM_VIEWS) initbuffer(vrc->buffers[i]);

            vrc->active = true;
        }
    }
}

void vr::cleanup()
{
    if(isenabled()) vrc->interface->cleanup();
    delete[] vrc->devices;

    screenres(vrc->normalw, vrc->normalh);

    delete vrc;
}

void vr::setview(int view)
{
    if(isenabled()) vrc->curview = view;
}

void vr::update()
{
    if(!isenabled()) return;
    
    vrc->interface->update(vrc->devices);
    
    vec angles = getangles();
    camera1->yaw = angles.x;
    camera1->pitch = angles.y;
    camera1->roll = angles.z;
}

void vr::finishrender()
{
    if(!isenabled()) return;

    glFlush();
    glFinish();

    glBindFramebuffer_(GL_FRAMEBUFFER, 0);

    glBindFramebuffer_(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer_(GL_DRAW_FRAMEBUFFER, vrc->buffers[vrc->curview].resolvefb);

    glBlitFramebuffer_(0, 0, screenw, screenh, 0, 0, screenw, screenh, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer_(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer_(GL_DRAW_FRAMEBUFFER, 0);

    // TODO: Handle multisampling
    //glDisable(GL_MULTISAMPLE);
}

void vr::submitrender()
{
    if(!isenabled()) return;

    loopi(VR_NUM_VIEWS) vrc->interface->submitrender(vrc->buffers[i], i);
}

bool vr::isenabled()
{
    return vrc && vrc->active;
}

matrix4 vr::getviewtransform()
{
    if(!isenabled()) return matrix4(vec(1, 0, 0), vec(0, 1, 0), vec(0, 0, 1));

    matrix4 eye = vrc->interface->getviewtransform(vrc->curview),
            hmd = gethmd().pose;

    hmd.settranslation(0, 0, 0); // Strip the pose of its translation
    eye.d.mul3(vrscalefactor); // TODO: Move this out of here as well

    eye.invert(eye);
    hmd.invert(hmd);

    matrix4 m;
    m.muld(eye, hmd);

    return m;
}

matrix4 vr::getviewprojection()
{
    if(!isenabled()) return matrix4(vec(1, 0, 0), vec(0, 1, 0), vec(0, 0, 1));

    return vrc->interface->getviewprojection(vrc->curview);
}

vec vr::getpos()
{
    if(!isenabled) return vec(0);

    vec tr = gethmd().pose.d;

    tr.mul(vrscalefactor).mul(vec(-1, 1, -1));
    swap(tr.y, tr.z);

    return tr;
}

vec vr::getstep()
{
    static vec prevpos(0);

    vec pos = vr::getpos();
    vec step = pos;

    step.sub(prevpos);
    prevpos = pos;

    return step;
}

vec vr::getangles()
{
    if(!isenabled) return vec(0);

    vec angles = quat(gethmd().pose).calcangles(); // FIXME: Get the angles straight out of the matrix

    angles.div(RAD).mul(vec(-1, 1, 1));
    swap(angles.y, angles.z);

    return angles;
}
