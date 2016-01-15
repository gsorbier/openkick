/**
   ExecBase (headers)
   \file
*/
#ifndef EXEC_EXECBASE_HPP
#define EXEC_EXECBASE_HPP

#include <exec/types.hpp>

#include <exec/list.hpp>
#include <exec/memory.hpp>
#include <exec/message.hpp>
#include <exec/library.hpp>
#include <exec/new.hpp>
#include <exec/todo.hpp>

namespace exec {
    extern ExecBase *execbase asm ("execbase");
}

//! exec global data
class exec::ExecBase : private Library {
    class BootInfo;
    class Vectors;
    friend class Vectors;

    static const int32_t VECTORS[] asm ("exec$VECTORS");
    static const char NAME[] asm ("exec$NAME");
    static const char IDSTRING[] asm ("exec$IDSTRING");

    // disable automatic methods
    ExecBase(void);
    ExecBase(const ExecBase &);
    ExecBase &operator=(const ExecBase &);

    // "static" variables, i.e. boot-time constants

    class BootInfo {
        const uint16_t soft_ver;       //!< kickstart version number (deprecated) (e.g. 0)
        const int16_t lowmem_checksum; //!< checksum of m68k trap vectors (e.g. 0)
        const uint32_t check_base; //!< \todo complement of system base pointer (e.g. 0xfffff989)
        /** The "cold" restart vector, called immediately after CPU reset and ROM remap */
        void (*cold_capture)(void); //
        /** The "cool" restart vector, called after exec init and multitasking started */
        void (*cool_capture)(void); //
        /** The "warm" restart vector, called after ROMtags loaded and shortly before we drop into
         * debugger */
        void (*warm_capture)(void); //
        const char *sys_stack_upper; //!< ??one-past-end address of system stack (i.e. bottom) (e.g. 0x200000)
        const char *sys_stack_lower; //!< ??lowest address of system stack (e.g. 0x1fe800)
        const char *chipmem_top; //!< one-past-end address of Chip RAM (e.g. 0x200000)
        void (*debug_entry)(void); //!< \todo debugger entry point (e.g. 0xfc237e)
        const void *debug_data; //!< \todo debugger data segment (possibly not used?) (e.g. 0)
        const void *alert_data; //!< \todo alert data segment (possibly not used?) (e.g. 0)
        const char *slowmem_top; //!< one-past-end address of 0xC00000 "Slow RAM", or NULL if none present
        /** checksum of (soft_ver, lowmen_checksum, check_base, cold_capture, cool_capture,
         * warm_capture, sys_stack_upper, sys_stack_lower, chipmem_top, debug_entry, debug_data,
         * alert_data, slowmem_top, -2) (e.g. 0xf99e) */
        const uint16_t checksum;
    public:
        BootInfo(ExecBase *, char *, char *, char *, char *);
    };

    BootInfo bootinfo;

    // interrupts

    IntVector intvects[16];

    Task *this_task;            //!< pointer to current task (e.g. 0x00001970)

    uint32_t idle_count;        // 0x00009cd6
    uint32_t disp_count;        // 0x00000055, c5
    uint16_t quantum;           // 0x0010 - initialised elsewhere (timer.device?)
    uint16_t elapsed;           // 0x0010, 0c - initialised elsewhere (timer.device?)
    uint16_t sys_flags;         /* FIXME: contains scheduling attention flag? */
    enum SysFlags {
        SWI_PENDING        = 1 << 5, //!< a software interrupt is pending
        TIMESLICE_EXPIRED  = 1 << 6, //!< the current task's timeslice has expired
        SCHEDULE_ATTENTION = 1 << 7  //!< reschedule task on return from interrupt
    };
    /** interrupt disable nesting count; -1 for enabled, 0 upwards for disabled */
    int8_t idnestcnt;
    /** task disable nesting count; -1 for enabled, 0 upwards for disabled */
    int8_t tdnestcnt;

    /** the available CPU and FPU instruction sets */
    enum CPUType : uint16_t {
        CPU_68010 = 0x01,       //!< 68010 instructions are available
            CPU_68020 = 0x02,   //!< 68020 instructions are available
            CPU_68030 = 0x04,   //!< 68030 instructions are available
            CPU_68040 = 0x08,   //!< 68040 instructions are available
            FPU_68881 = 0x10,   //!< 68881 FPU instructions are available
            FPU_68882 = 0x20,   //!< 68882 FPU instructions are available
            FPU_68040 = 0x40,   //!< 68040 FPU instructions are available
            CPU_68060 = 0x80    //!< 68060 instructions are available
            };
    CPUType attn_flags;

    uint16_t attnresched;
    ResidentArray *res_modules; // pointer to NULL-terminated array of pointers to const ROMTags
    void (*tasktrapcode)(void);
    void (*taskexceptcode)(void);
    void (*taskexitcode)(void);
    uint32_t tasksigalloc;
    uint16_t tasktrapalloc;

    // "private" system lists

    HeapList heap_list;         //!< the system memory
    ResourceList resource_list;
    DeviceList device_list;
    InterruptList intr_list;
    LibraryList library_list;
    PortList port_list;
    /* the list of tasks ready to run, but not the running task (which is in #this_task) */
    TaskList task_ready;
    /* the list of tasks waiting on signals etc */
    TaskList task_wait;

    SoftIntList softints[5];

    // "other globals"

    int32_t LastAlert[4];

    uint8_t vblank_frequency;
    uint8_t power_supply_frequency;

    SignalSemaphoreList semaphore_list;

    ListOf<MemEntry> *kick_mem_ptr; //! \todo make this a proper type
    // kick_tag_ptr appears to be a pointer to an array of ROMTag pointers; NULL terminates the
    // list, a value with high-bit-set is a pointer to another such list.
    void *kick_tag_ptr;
    void *kick_checksum;          // not really a pointer

private:
    ExecBase *open(void) {
        ++open_count;
        return this;
    };
    void close(void) {
        --open_count;
    };

public:
    ExecBase(char *, char *, char *, char *, HeapList *);

    static char *startup(void) asm("_init");
    static void startup2(void) asm("_init2") __attribute__((noreturn));
    static CPUType probe_cpu(void) asm("exec$probe_cpu");

    void forbid(void) {
        ++tdnestcnt;
    };
    void permit(void);
    void disable(void);
    void enable(void);

#include <gen/exec.cdec.inc>
};
#endif
