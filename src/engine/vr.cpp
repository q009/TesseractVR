#include "cube.h"
#include "engine.h"

#ifdef VR_OPENVR
#include "openvrinterface.h"
#endif

extern void screenres(int w, int h);

vr::vrcontext *vrc;

FVARP(vrscalefactor, 0.0001f, 12, 1000);
VARP(vrmovestyle, vr::VR_MOVE_STYLE_LCTRLR, vr::VR_MOVE_STYLE_LCTRLR, vr::VR_MOVE_STYLE_HMD);

ICOMMAND(vrmove, "D", (int *down), { if(vr::isenabled()) vrc->moving = (*down != 0); });

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

static void freebuffer(vr::vrbuffer &buf)
{
    glDeleteTextures(1, &buf.resolvetex);
    glDeleteBuffers_(1, &buf.resolvefb);
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
            initscreen();
            loopi(VR_NUM_VIEWS) initbuffer(vrc->buffers[i]);

            vrc->interface->update(vrc->devices);
            vrc->active = true;
        }
    }
}

void vr::cleanup()
{
    if(isenabled())
    {
        vrc->interface->cleanup();
        screenres(vrc->normalw, vrc->normalh);

        loopi(VR_NUM_VIEWS) freebuffer(vrc->buffers[i]);
    }

    delete vrc;
    vrc = NULL;
}

void vr::setview(int view)
{
    if(isenabled()) vrc->curview = view;
}

static void updatecamangles()
{
    vr::vrdev *hmd = vr::gethmd();
    if(hmd) hmd->getangles(camera1->yaw, camera1->pitch, camera1->roll);
}

static void calcctrlrmovement(vr::vrcontroller *ctrlr)
{
    if(ctrlr->axes.iszero()) player->movedir = ctrlr->getdir();
    else
    {
        float yaw, pitch;
        vectoyawpitch(ctrlr->getdir(), yaw, pitch);

        vec dir;
        vecfromyawpitch(yaw, pitch, vec(ctrlr->axes).normalize(), dir);

        player->movedir = dir;
    }
}

static void updatemovement()
{
    if(vrc->moving)
    {
        vr::vrcontroller *ctrlr = NULL;

        if(vrmovestyle == vr::VR_MOVE_STYLE_HMD) player->movedir = vec(1, 0, 0);
        else if((ctrlr = vr::getcontroller(vrmovestyle))) calcctrlrmovement(ctrlr);
    }
    else player->movedir = vec(0);
}

void vr::update()
{
    if(!isenabled()) return;
    
    vrc->interface->update(vrc->devices);
    updatecamangles();
    updatemovement();
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
            hmd = gethmd()->pose;

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

int vr::getnumdevices(int type)
{
    if(!isenabled() || type >= VR_NUM_DEV_TYPES) return 0;
    return type == VR_DEV_HMD ? 1 : vrc->devices.periph[type].length();
}

vr::vrdev *vr::getdevice(int type, int index)
{
    ASSERT(index < getnumdevices(type));
    return type == VR_DEV_HMD ? vrc->devices.hmd : vrc->devices.periph[type][index];
}

vr::vrdev *vr::gethmd()
{
    return getdevice(VR_DEV_HMD);
}

vr::vrcontroller *vr::getcontroller(int role)
{
    ASSERT(role < VR_NUM_CONTROLLERS);
    return (vrcontroller*)vrc->devices.ctrlrmap[role];
}

matrix4 vr::matrixrh2lh(matrix4 m)
{
    matrix4 result;

    result.muld(invviewmatrix, m);
    result.muld(matrix4(vec(-1, 0, 0), vec(0, -1, 0), vec(0, 0, -1)));
    result.rotate_around_x(90 * RAD);

    return result;
}
