// sample trackdisk.device classes

//! trackdisk.device
namespace trackdisk {
    using namespace exec;

    // devices/trackdisk.h

    class IOExtTD : public IOStdReq { /* ... */ };
    class DriveGeometry;
    class TDU_PublicUnit : public Unit { /* ... */ };
}
