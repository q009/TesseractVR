#include "vr.h"
#include "openvr.h"

namespace vr
{
    struct openvrinterface : vrinterface
    {
        IVRSystem *sys;
        TrackedDevicePose_t trackinfo[k_unMaxTrackedDeviceCount];
        vrdev *devmap[k_unMaxTrackedDeviceCount];
        int ctrlrroles[VR_NUM_CONTROLLERS];

        int getdevicetype(int index);
        void recalcdevice(vrdevices &devices, int index);
        void updatedevices(vrdevices &devices);
        int getcontrollerrole(int index);
        void updatecontrollerrole(vrdevices &devices, int index);
        void newdevice(vrdevices &devices, int index);
        void removedevice(vrdevices &devices, int index);
        int getbuttoncode(VREvent_t event);
        void handleevent(vrdevices &devices, VREvent_t event);
        void pollevents(vrdevices &devices);

        virtual bool init();
        virtual void cleanup();
        virtual void update(vrdevices &devices);
        virtual void submitrender(vrbuffer &buf, int view);
        virtual void getresolution(uint &w, uint &h);
        virtual int getmaxdevices();
        virtual matrix4 getviewprojection(int view);
        virtual matrix4x3 getviewtransform(int view);
    };

    openvrinterface *getopenvrinterface();
}