// -*- mode: c++ -*-
/**
   ExecBase (implementation)
   \file
*/

#include <exec/execbase.hpp>
#include <hw/amiga.hpp>
#include <exec/new.hpp>

#include <exec/buffer.hpp>

using namespace exec;

/*
  extern const Resident resident;// __attribute__((section(".text.exec_resident")));

  const Resident resident = {
  Resident::MATCHWORD, &resident, "end of text", 0, 47, Node::NT_LIBRARY, -128,
  "test.module", "test ID", NULL
  };
*/

const char ExecBase::NAME[] = "exec.library";
const char ExecBase::IDSTRING[] = "exec (" __DATE__ ")";

/** Implementation of AmigaOS-compatible library functions \todo ingroup? */
struct ExecBase::Vectors {
#include <gen/exec.vdef.inc>
};
const int32_t ExecBase::VECTORS[] = {
#include <gen/exec.vecs.inc>
    -1
};

namespace exec {
#include <gen/exec.cdef.inc>
}

static amiga::Custom volatile * const custom = reinterpret_cast<amiga::Custom *>(amiga::CustomBase);
static amiga::CIA volatile * const ciaa = reinterpret_cast<amiga::CIA *>(amiga::CIAABase);

/**
   \brief A container for startup memory. \todo ingroup? \ingroup todo
*/
struct startup_memory : public Buffer {
    //! The integer constant with repeated binary digits 10, useful for memory testing
    static const uint32_t patA=0xAAAAAAAA;
    //! The integer constant with repeated binary digits 01, useful for memory testing
    static const uint32_t pat5=0x55555555;

    startup_memory(uint32_t start_, uint32_t end_)
        : Buffer(reinterpret_cast<char *>(start_), reinterpret_cast<char *>(end_))
    {}

    static bool is_repeat(void *, void *);
    static bool is_writable(void *);
    static bool is_24bit(void);

    void probe_chip_ram(size_t = (256<<10));
    void probe_slow_ram(size_t = 4096);
    void probe_a3000_ram(size_t = (256<<10));
};

/**
   \brief Tests to see if two memory locations are aliased to each other.

   \warning This temporarily corrupts four bytes of memory each at the locations with addresses \a
   first and \a second . You should ensure that you have not loaded this routine at such an address,
   and have no active interrupt handlers or DMA using such addresses.

   \param first the address of the first location
   \param second the address of the second location

   \return true if \a first and \a second access the same physical location

*/
bool startup_memory::is_repeat(void *first, void *second) {
    volatile uint32_t *p1 = static_cast<volatile uint32_t *>(first);
    volatile uint32_t *p2 = static_cast<volatile uint32_t *>(second);

    // obviously, an address does repeat itself, but we're wanting to test if two distinct addresses
    // appear to be backed by the same RAM
    if(p1==p2) return false;

    // save original values, just in case they were important
    int v1=*p1, v2=*p2;
    // write different values into the locations
    *p1=patA; *p2=pat5;
    bool aliased = (*p1==pat5);
    // restore original values
    *p1=v1; *p2=v2;

    return aliased;
}

/**
   \brief Tests to see if a memory location is RAM

   \warning This temporarily corrupts eight bytes of memory starting at the address \a address . You
   should ensure that you have not loaded this routine at such an address, and have no active
   interrupt handlers or DMA using such addresses.

   \param address the address to test

   \return true if the location is RAM, false otherwise

*/
bool startup_memory::is_writable(void *address) {
    volatile uint32_t *p = static_cast<volatile uint32_t *>(address);
    bool good;                  // is the memory good?

    // save original values, just in case they were important
    int v1=p[0], v2=p[1];

    // write a value, then the inverse of that value to the following location, so that we don't
    // have the previous value still hanging around on the bus when reading back
    p[0]=patA; p[1]=pat5;

    if(p[0]!=patA) {
        good=false;             // memory is good
    } else {
        // might be OK, test it with different values

        p[0]=pat5; p[1]=patA;
        if(p[0]!=pat5) {
            good=false;         // memory is bad
        } else {
            good=true;          // memory is bad
        }
    }

    // restore original values
    p[0]=v1; p[1]=v2;

    return good;
}

/**
   \brief Tests to see if the CPU has a 24 or 32 bit address bus

   \warning This temporarily corrupts eight bytes of memory at locations 0 and 0x07000000. You
   should ensure that you have not loaded this routine at such an address, and have no active
   interrupt handlers or DMA using such addresses.

   \return true if the CPU has a 24 bit address bus, false otherwise.

*/
bool startup_memory::is_24bit(void) {
    // are we on a processor with just a 24 bit address bus?
    // find out by checking for aliasing between Chip RAM and A3000 RAM.

    void *chip_ram=reinterpret_cast<void *>(0);
    void *a3000_ram=reinterpret_cast<void *>(0x07000000);

    // Is the A3000 memory region writable?
    // Nope, so it must be a 32 bit processor with no A3000 RAM.
    if(!is_writable(a3000_ram))
        return false;

    // Either this is a box with A3000 RAM, or the addressing has wrapped
    // round and we're actually poking Chip RAM, so check for aliasing.
    return is_repeat(chip_ram, a3000_ram);
}

/**
   \brief Probes for Chip RAM

   Chip RAM appears from physical location 0 upwards, with a maximum of 2MB of Chip RAM in real
   Amigas, and 8MB under UAE emulation. Where there is less than 2MB of Chip RAM present in the
   system, the RAM will repeat throughout the 2MB range due to incomplete address decoding.

   This RAM is tested by walking upwards through the range until one finds a location that is
   aliased with the first location (because of the incomplete address decoding) or a location that
   is not RAM (because we've walked off the end).

   \warning This temporarily corrupts eight bytes of memory at all locations in the test range with
   addresses that are multiples of the \c step . You should ensure that you have not loaded this
   routine at such an address, and have no active interrupt handlers or DMA using such addresses.

   \param start the (virtual) address of the start of Chip RAM
   \param end the (virtual) address of the end of Chip RAM
   \param step the step size for testing

   \return a startup_memory containing the usable address range of Chip RAM

*/
void startup_memory::probe_chip_ram(size_t step) {
    // basically, the test is to step up the range until we either hit the top, the memory fails to
    // respond, or it's aliased with location 0 (i.e. wraparound).
    char *new_end = start;

    while(new_end < end) {
        if(!is_writable(new_end))
            break;              // finish if we can't write to the location
        if(is_repeat(start, new_end))
            break;              // finish if we alias with first location
        new_end += step;        // test OK, continue to next block
    }

    end = new_end;
}

/**
   \brief Probes for Slow RAM

   Slow RAM appears from physical location 0xC00000 upwards with a theoretical top address of
   0xDFF000 where it meets the custom chips. Real Amigas have up to 512kB of this, whereas UAE will
   emulate up to 1984kB (with a top address of 0xDF0000.) The custom chips will appear where there
   is no RAM, so a memory test has to be specially aware of this and not poke random registers as if
   they were RAM.

   \warning This temporarily corrupts the four bytes of memory at all locations in the test range
   with addresses that are multiples of the \c step plus 0x1c, 0x1d, 0x9a and 0x9b (the offsets of
   custom chip registers INTENAR and INTENA). You should ensure that you have not loaded this
   routine at such an address, and have no active interrupt handlers or DMA using such addresses.

   \param start the (virtual) address of the start of Slow RAM
   \param end the (virtual) address of the end of Slow RAM
   \param step the step size for testing

   \return a startup_memory containing the usable address range of Slow RAM

*/
void startup_memory::probe_slow_ram(size_t step) {
    // The Slow RAM (0xC00000) probe is rather cunning. Due to incomplete address decoding, the
    // custom chips appear where there is no RAM. So we scan through the space treating it like
    // custom chips and poking values into INTENA and seeing if they affect INTENAR. If it's RAM,
    // the aliased INTENAR location does not update in sympathy.
    char *new_end = start;

    while(new_end < end) {
        amiga::Custom * test = new (new_end) amiga::Custom;
        // Firstly, we clear INTENA and then read INTENAR.
        test->intena(0x3fff); // clear all bits in INTENA
        if(!test->intena()) {
            // we got zero back, so either we read INTENAR back, or the RAM
            // happened to contain zero. Redo the INTENA dance with a different
            // value to see which it was.
            test->intena(0xbfff); // set all bits except master enable in INTENA
            if(test->intena() == 0x3fff) {
                // OK, we're definitely looking at INTENAR rather than RAM here, so
                // exit test loop.
                break;
            }
        }
        // However, while we've checked that we're not looking at custom chips, we've not actually
        // checked to see if it's RAM. (UAE with AGA and no Slow RAM has nothing at $C00000, for
        // example.) So we do that as well:
        if(!is_writable(new_end)) {
            break;
        }
        new_end += step;        // test OK, continue to next block
    }
    end = new_end;
}

/**
   \brief Probes for A3000-style RAM

   A3000 RAM appears in different forms depending on the system.

   Under UAE, A3000 RAM grows up from 0x07000000 and continues up indefinitely. However, since
   Kickstart likes to map Zorro III cards at 0x40000000, we cap this bank to that address (giving an
   effective maximum of 912MB) to keep things neat.

   On an A4000, the RAM grows down from 0x07ffffff. This bank has a maximum size of 16MB and thus
   the lowest possible address is 0x07000000. A slight added complication is MapROM hardware on the
   A3640 CPU board which can steal the top 512kB of RAM for reset-proof Kickstart shadowing.

   How the memory in a real A3000 is mapped is unknown, but it's assumed that it's a contiguous
   chunk somewhere in the 16MB area between 0x07000000 and 0x07ffffff.

   The approach used to locate RAM here is to scan from the bottom of the memory range until we find
   RAM (thus able to find memory in an A4000 with less than 16MB of RAM), and then keep going until
   it disappears again.

   \warning This temporarily corrupts eight bytes of memory at all locations in the test range with
   addresses that are multiples of 256kB. You should ensure that you have not loaded this routine at
   such an address, and have no active interrupt handlers or DMA using such addresses.

   \bug This won't work on a machine with a 24 bit address bus, and will misdetect Chip RAM (and any
   configured Zorro II RAM) as A3000 RAM.

   \param start the (virtual) address of the start of Slow RAM
   \param end the (virtual) address of the end of Slow RAM
   \param step the step size for testing

   \return a startup_memory containing the usable address range of A3000 RAM.

*/
void startup_memory::probe_a3000_ram(size_t step) {
    if(is_24bit()) {
        start = end = NULL;
        return;
    };
    char *new_start = start;
    // find first location that responds
    while(new_start < end) {
        if(is_writable(new_start))
            break;                      // finish if we *can* write to the location
        new_start += step;
    }

    char *new_end = new_start;
    // find first location that doesn't respond (result ends in "top")
    while(new_end < end) {
        if(!is_writable(new_end))
            break;                      // finish if we can't write to the location
        new_end += step;
    }

    start = new_start;
    end = new_end;
}

/**
   \brief C++ early startup entry point

   This is where memory is probed and ExecBase set up. The location of the top of the supervisor
   stack is returned.

   The basic setup of classic Amiga exec goes as follows:

   - post-reset startup should check for ROMs at 0xf00000 and jump there (unless we're shadowed
   there and already running at that address.) Also needs to set up %sp before C code can run. It
   also ought to set up some initial exception vectors so we wedge properly on crash and don't
   wander off into the weeds. DMA is disabled, to make sure.

   - checks for HELP at location 0 and squirrels away the Guru data at 0x100/0x104 into registers.
   We can probably do that later.

   - C code should check for already-existing ExecBase, verify it's sound (is even, within expected
   memory regions, and has the correct complement pointer) and use it if so.

   if invalid: re-probe for RAM and recreate initial ExecBase. Set aside memory for system stack and
   store in sys_stack_upper/lower. RAM is wiped. (Which is why guru codes are hidden in registers).

   if valid: use existing ExecBase, call ColdCapture, after clearing the pointer so if it wedges, a
   reboot will come back. classic exec then validates RAM size; this is unnecessary if we probed it
   properly the first time.

   - ExecBase is mostly erased and re-initialised. Everything from intvects onwards, with the
   exception of kick_mem_ptr, kick_tag_ptr and kick_checksum because those are used for ROMTag
   scanning. (Basically, reset-proof RAM-resident libraries and other modules.)

   - CPU/FPU is identified by performing instructions and seeing which exceptions are called.

   - exec's lists are re-initialised.

   - Deferred guru codes in registers are written into ExecBase LastAlert[0..1] for alert.hook to
   pick up when we start scanning and launching ROMTags.

   - set tasktrapcode & taskexceptcode to point to crash handler

   - set taskexitcode to point to standard exit handler (which just does a RemTask(NULL)

   - preallocate signals 0..15 in tasksigalloc (store 0xffff)

   - preallocate trap #15 in tasktrapalloc (store 0x8000) for ROM-Wack breakpoints

   - probed memory is added to memory lists.

   - initialise execbase Library node portion entirely (type library, pri=0, name = "exec.library",
   flags LIBF_CHANGED|LIBF_SUMUSED, but not negsize, possize=sizeof(ExecBase), version = 0x21,
   revision = 0xc0, id = "exec.library...", checksum not set, opencnt = 1.

   - initialise vector table using MakeFunctions(), then set negsize.

   - add slow ram, then chip ram to memory list. (addresses of slow/chip ram are magically shuffled
   to account for existing ExecBase and CPU stack)

   - install proper CPU exception vectors from table. Mostly points to a load of jsr statements to
   handlers, some of which are the same implementation, so the so the return address indicates which
   vector was called.

   - if 010+: Replace bus/address error vectors with alternative to cope with larger stack frame of
   010/020. Replace Supervisor() and GetCC() with 010/020 version.

   - if 881+: Replace Switch() and Dispatch() with FPU-aware versions.

   - allocate and initialise five interrupt service chains for intvects[1..5] (yes, not softints!);
   rest seem to be populated later.

   - initialise ROM-Wack: RW reinitialises 0x200..+236 bytes, installs as execbase->debugentry,
   Debug() vector, and does RawIOInit().

   - calculate and store execbase checksum

   - AllocEntry a stack and task descriptor for exec.library itself, initialise, AddTask(), set
   %usp, mark task runnable (state = TS_RUN), unlink from ready queue, clear supervisor bit and
   we're now multitasking. Forbid()/Permit() for good measure, as Permit() will do a reschedule if
   necessary.

   - build a pri-queue List and scan ROMs for ROMTags (0xfc0000-0xffffff twice, then
   0xf00000-0xf80000), deduping along the way. Dupes are sorted with higher version, then higher
   priority. queue is on the stack, Nodes for ROMtags get allocated from the heap. The nodes are
   then counted and flattened into a NULL-terminated C array of pointers to ROMTags. (nodes
   deallocated, C array allocated).

   - verify checksum of kick_mem_ptr/kick_tag_ptr; if valid, try and AllocAbs the kick_mem_ptr.
   (Doesn't seem to free them again on failure.) If that works, iterate over the kick_tag_ptr and
   add those ROMtags too.

   - turn power LED on, call "cool capture" if not NULL.

   - InitCode() the resident modules with RTF_COLDSTART flag set and any version.

   [presumably the disk system takes over at this point]

   - call "warm capture" if not null.

   - push 14 longwords of zero onto the stack and pop them into d0-d7/a0-a6, then call Debug().

*/
char *ExecBase::startup(void) {
    // Real Amigas can have up to 2MB of Chip RAM, although UAE supports up to 8MB. We use 10MB as
    // the theoretical maximum since the probe that high doesn't hit any important MMIO hardware and
    // Chip RAM is more useful than the Zorro II RAM space.

    // The ceiling of Slow RAM is a bit more nebulous. Supposedly it is possible to have 1.75MB of
    // it (to 0xDC0000) but most RAM expansions were 512kB, UAE doesn't want to go that high, and
    // bits of MMIO start showing up at 0xD80000 upwards. So we use that as the ceiling.

    // The A4000's RAM grows down from 0x08000000, unless MapROM is in effect in which case it grows
    // down from 0x07800000. 16MB maximum can be fitted, giving the lowest start address of
    // 0x07000000. The A3000 appears to be similar, but I've not seen one to test. UAE's A3000 RAM
    // grows *up* from 0x07000000 and has a maximum of a3000mem_size=3984 which extends memory right
    // to the top of the 4GB address space. However, AmigaOS is not 32 bit clean, so only 2GB of
    // address space is useful for RAM, or a3000mem_size=1936.

    startup_memory
        chip_ram(          0,   0xa00000),
        slow_ram(   0xc00000,   0xd80000),
        a3000_ram(0x07000000, 0x80000000);

    chip_ram.probe_chip_ram();
    // chop off first 4kiB page of Chip RAM; %sp is currently at the top of here; this size also
    // corresponds to the common MMU page size, and is useful to stop stuff being placed in memory
    // that is protected by Enforcer.
    chip_ram.carve_bottom(0x1000);

    slow_ram.probe_slow_ram();
    // blow away the slow RAM pointers just to make sure; otherwise ExecBase will end up with a
    // nonsensical slow RAM end address of 0xC00000.
    if(!slow_ram) slow_ram.start = slow_ram.end = NULL;

    a3000_ram.probe_a3000_ram();

    //! \todo a real RAM test would be nice.

    // this is a temporary system memory list, which is eventually passed to the ExecBase
    // constructor.
    HeapList heaplist;

    if(a3000_ram)
        heaplist.add(a3000_ram.size(), Heap::Attributes(
                Heap::MEMF_PUBLIC | Heap::MEMF_FAST | Heap::MEMF_LOCAL | Heap::MEMF_KICK
              ), 30, a3000_ram.start, "A3000 RAM");

    // 0xC00000 RAM is marked neither Chip RAM nor Fast RAM.
    if(slow_ram)
        heaplist.add(slow_ram.size(), Heap::Attributes(
                Heap::MEMF_PUBLIC | Heap::MEMF_LOCAL | Heap::MEMF_DMA24 | Heap::MEMF_KICK
              ), 0, slow_ram.start, "Slow RAM");

    heaplist.add(chip_ram.size(), Heap::Attributes(
            Heap::MEMF_PUBLIC | Heap::MEMF_CHIP | Heap::MEMF_LOCAL | Heap::MEMF_DMA24 | Heap::MEMF_KICK
          ), -10, chip_ram.start, "Chip RAM");

    // now find somewhere to drop supervisor stack
    size_t supervisor_stack_size = 6 * 1024;
    char *supervisor_stack =
        heaplist.allocate(supervisor_stack_size, Heap::MEMF_PUBLIC, Heap::MEMF_REVERSE);
    if(!supervisor_stack) asm(".word 0x4afc");

    /// \todo deal with custom allocator
    ExecBase *new_execbase = new (
        &heaplist, reinterpret_cast<const PackedFunctions *>(VECTORS)
      ) ExecBase(
          supervisor_stack + supervisor_stack_size, supervisor_stack,
          chip_ram.end, slow_ram.end,
          &heaplist
        );
    if(!new_execbase) asm(".word 0x4afc");

    execbase = new_execbase;

    size_t exec_stack_size = 4096;

    // // create the initial exec.library task
    // MemEntryResponse mer =
    //     execbase->heap_list.allocate_multiple(
    //         Heap::MEMF_PUBLIC, sizeof(Task),
    //         Heap::MEMF_PUBLIC, exec_stack_size,
    //         NULL );
    // if(mer.failed) asm(".word 0x4afc");
    // MemEntry * me = mer.mementry;
    // *reinterpret_cast<uint32_t *>(8) = reinterpret_cast<uint32_t>(me);
    // asm volatile ("42: move.w #0xf00, 0xdff180\n bra.s 42b");
    // Task *exec_task = new ( me->entries[0].addr ) Task("exec.library");
    // char *exec_stack = me->entries[1].addr;

    // FIXME: new adds size block and thus wastes 8 bytes.
    Task *exec_task = new Task("exec.library");
    char *exec_stack = new char[exec_stack_size];
    execbase->task_ready.add(exec_task);
    execbase->this_task = exec_task;
    exec_task->stack_bottom = exec_stack;
    exec_task->stack_pointer = exec_task->stack_top = exec_stack + exec_stack_size;

    // asm(".word 0x4afc");

    // // asm(".word 0x4afc");
    // execbase->RawIOInit();
    // //asm("42: bra.s 42b");
    // execbase->RawPutChar('T');
    // execbase->RawPutChar('E');
    // execbase->RawPutChar('S');
    // execbase->RawPutChar('T');
    // execbase->RawPutChar('\n');

    asm("move.l %0, %%usp" : : "a"(exec_stack + exec_stack_size));

    //asm("move.l %0, %%usp \n move.l %1, %1" : : "a"(exec_stack + exec_stack_size), "a"(exec_stack));
    return supervisor_stack + supervisor_stack_size;
}

void ExecBase::startup2(void) {
    // Now is an excellent time to start scanning ROMTags

    // we scan ROMtags here and accumulate them in the romtags list
    ResidentArray::BuilderList romtags;
    romtags.search(0x00f80000, 0x01000000);
    // TOD: scan other areas
    // romtags.search(0x00f00000, 0x00f80000);

    // TODO: process kick_mem_ptr/kick_tag_ptr and add to romtags

    // now flatten the list into an array of Resident
    execbase->res_modules = romtags.flatten();
    execbase->res_modules->initialise(Resident::RTF_COLDSTART, 34);

    // turn LED on
    ciaa->pra &= ~2;

    // TODO: call "cool capture" if not NULL

    // TODO: InitCode all of those resident modules

    // (it's expected that the disk system takes control at this point)

    // TODO: call "warm capture" if not NULL

    // TODO: bail into ROM-Wack

    // \todo random test pattern to indicate we've run out of things to do.

    /*
      custom->color(0, 0xfff);
      for(;;);
    */

    while(1) {
        //ciaa->pra ^= 2;
        custom->color(0, 0x00f);
        for(int n=0; n<(1<<5); ++n) {asm("");};
        custom->color(0, 0xff0);
        for(int n=0; n<(1<<5); ++n) {asm("");};
    }
}

ExecBase::ExecBase(
    char *sys_stack_upper, char *sys_stack_lower,
    char *chipmem_top, char *slowmem_top,
    HeapList *new_heap_list
  )
    : Library(ExecBase::NAME, 33, 0, ExecBase::IDSTRING)
    , bootinfo(this, sys_stack_upper, sys_stack_lower, chipmem_top, slowmem_top)
    , idnestcnt(0)
    , tdnestcnt(0)
    , attn_flags(probe_cpu())
    , heap_list(new_heap_list)
{
    // add it to the library list
    library_list.add_library(this);
    intvects[0] = {0, 0, 0};
}

ExecBase::BootInfo::BootInfo(ExecBase *execbase,
    char *sys_stack_upper_, char *sys_stack_lower_,
    char *chipmem_top_, char *slowmem_top_)
    : soft_ver(0),
      lowmem_checksum(),
      check_base(~reinterpret_cast<uint32_t>(execbase)),
      cold_capture(NULL),
      cool_capture(NULL),
      warm_capture(NULL),
      sys_stack_upper(sys_stack_upper_),
      sys_stack_lower(sys_stack_lower_),
      chipmem_top(chipmem_top_),
      debug_entry(),
      debug_data(),
      alert_data(),
      slowmem_top(slowmem_top_),
      checksum()
{}

void ExecBase::disable(void) {
    ++idnestcnt;
    custom->intena(0x4000);
}

void ExecBase::enable(void) {
    if(--idnestcnt < 0) {
        custom->intena(0xc000);
    }
}

void *Task::operator new(size_t size) {
    return execbase->AllocMem(size, Heap::MEMF_PUBLIC);
}
