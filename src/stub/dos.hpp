// sample dos.library classes

//! dos.library
namespace dos {

    using namespace exec;

    class BPTR { uint32_t bptr; };

    // dos/datetime.h
    class DateTime : DateStamp {
        enum Format : uint8_t {
            DOS = 0, INT = 1, USA = 2, CDN = 3
                };
        enum Flags : uint8_t {
            SUBST = 1, FUTURE = 2
                };
        uint8_t format;
        Flags flags;
        char *day;
        char *date;
        char *time;
    };

    // dos/dos.h
    class DateStamp {
        uint32_t days;
        uint32_t minute;
        uint32_t tick;
    };
    class FileInfoBlock;
    class InfoData;

    // dos/dosasl.h
    class AnchorPath;

    // dos/dosextens.h
    class Process : public exec::Task {
        Port port;
        // ...
    };
    class DosPacket;
    class StandardPacket : public Message, public DosPacket;
    class ErrorString;
    class DosLibrary : public Library {
        // ...
    };
    class RootNode;
    class CliProcList : public MinNode {
        // ...
    };
    class DosInfo;
    class Segment;
    class CommandLineInterface;
    class DeviceList;
    class DevInfo;
    class DosList;
    class AssignList;
    class DevProc;
    class FileLock;

    // dos/doshunks.h - no structs
    // dos/dostags.h - no structs

    // dos/exall.h
    class ExAllData;
    class ExAllControl;

    // dos/filehandler.h
    class DosEnvec;
    class FileSysStartupMsg;
    class DeviceNode;

    // dos/notify.h
    class NotifyMessage : public Message {
        // ...
    };
    class NotifyRequest;

    // dos/rdargs.h
    class dos::CSource;
    class RDArgs : public dos::CSource {
        // ...
    };

    // dos/record.h
    class RecordLock;

    // dos/stdio.h - no structs

    // dos/var.h
    class LocalVar : public Node {
        // ...
    };
}
