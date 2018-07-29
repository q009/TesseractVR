#include "vr.h"
#include "openvr.h"

namespace vr
{
    struct openvrinterface : vrinterface
    {
        IVRSystem *sys;
        TrackedDevicePose_t trackinfo[k_unMaxTrackedDeviceCount];

        int getdevicetype(int index);
        void updateposes(vrdevice *devices);

        virtual bool init();
        virtual void cleanup();
        virtual void update(vrdevice *devices);
        virtual void submitrender(vrbuffer &buf, int view);
        virtual void getresolution(uint &w, uint &h);
        virtual uint getmaxdevices();
        virtual matrix4 getviewprojection(int view);
        virtual matrix4x3 getviewtransform(int view);
    };

    openvrinterface *getopenvrinterface();
}