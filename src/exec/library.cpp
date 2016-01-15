#include <exec/execbase.hpp>
#include <exec/library.hpp>

#include <exec/libc.hpp>
// #include <new.hpp>

//! \defgroup exec_library exec.library library support

/**
   \ingroup exec_library

   A library is a singleton object in RAM which contains both library-local data and a jump table
   for its functions. It is a concrete subclass of exec::Library, and thus the local data appears
   after the Library object in memory.

   The jump table grows downwards in memory from the start of the object---in AmigaOS terminology,
   the library base---and consists of MC680x0 "JMP" instructions that call the real implementation,
   which may appear either in RAM or ROM. Because this jump table is in RAM, it allows ROM functions
   to be hooked or patched at run time. This functinality allows the disk subsystem to hook
   OpenLibrary() to enable loading libraries from disk.

   "JMP" is a six-byte instruction, and so the function offsets appear at multiples of -6 relative
   to the library base. The first four functions are reserved for library management, so
   user-defined functions start at -30, and continue with -36, -42 and so on.

   Libraries have a version number, which indicate which revision of the library API they offer.
   They also must be backwards-compatible with all previous versions of the API. Essentially, they
   state the *maximum* version of the API they offer.

   Client code cannot access libraries directly since they do not know their base. So they must open
   the library using the OpenLibrary() function and state the *minimum* version they will accept.
   They will either get the pointer to the library, or NULL indicating their request could not be
   satisfied.

   The astute will wonder how one obtains the base address of exec.library to call OpenLibrary(). It
   is a pre-opened library and the pointer to it appears at address 4. If you're feeling perverse,
   you can OpenLibrary("exec.library") if you wish, although this might also be useful if you want
   to use functions from a newer version and would like to check you have the right version first.

   AmigaOS libraries normally use a register-passing ABI with the library base in %a6, parameters
   appearing in %d0 upwards for integers and %a0 upwards for pointers, with the result returned in
   %d0. Registers %d2-d7 and %a2-a6 are to be preserved by the library function unless documented
   otherwise.

   \todo how the thunking works will probably change.

   This is where this reimplementation gets a bit more interesting, as g++ doesn't support that ABI,
   but passes parameters on the stack instead. A pair of thunks are mechanically generated for each
   function. To call *from* C++, there is an inline method in the library's class with the same name
   as the AmigaOS library call that plonks "this" in %a6 and the other parameters in their
   registers, then does a "JSR -xxx(%a6)". To *implement* the AmigaOS call mechanism, a separate
   compilation unit is created for each function which defines the register-based parameters as
   unit-global variables and a function which takes no parameters. Thus the function can access the
   registers as if they were parameters. A small stub body is included which will usually do a
   stack-parameter call to the implementation, although it may also just inline the implementation
   instead.

   Because this is a nice generic mechanism of providing a memory-resident bag of data and
   functions, it is no surprise that both exec::Device and exec::Resource are just abstract
   subclasses of Library.

   The next question is, where do libraries come from, and how can we create them?

   Libraries are normally initialised through the ROMTag mechanism. A ROMTag is a structure (an
   exec::Resident) which describes a library, device, or resource (or indeed any other bag of bits
   that finds the mechanism useful) that wishes to become created and initialised.

   The ROMTag gives the contains a magic number (the MC680x0 "INVALID" instruction) and a pointer to
   itself and the end of the library code, which helps avoid false positives when scanning the ROM
   for them. It is good practice to place the ROMTag right before the library code, and its "end"
   pointer to one-past-the-end of the code, so that none of the library code is picked up as a false
   positive.

   The ROMTag also includes the library's name, version, and initialisation priority, as well as a
   set of flags that indicate which phase of system startup the library should be initialised. The
   resource name must be unique. This is because exec de-duplicates all of the modules it finds by
   name, and only initialises the highest version of each.

   The other important field is the auto_init. If the RTF_AUTOINIT flag is set in the ROMTag,
   auto_init is treated as an exec::Resident::AutoInit, and used to automatically create the
   library. Otherwise, it's the address of a funtion that is called to create the library manually.

   exec::Resident::AutoInit is really just a way of statically packaging the arguments to
   MakeLibrary(), which is how you will normally want to create a library.

   MakeLibrary() takes as its parameters, an optional vector table, which may be packed; an optional
   exec::PackedStruct, an optional initialisation/verification function, the size of the structure,
   and a segment list.

   ROMTags are initialised in this order:

   - RTF_SINGLETASK (V36+?) - while still in single-tasking mode (and possibly supervisor mode)

   - RTF_COLDSTART - after multitasking enabled (but note that task switching might not have started
   yet)

   - RTF_AFTERDOS (V36+) - after DOS has started

   FIXME: continue documenting this.

*/

using namespace exec;

// -------------------- PackedStruct --------------------

/** \todo need to test this code */
void PackedStruct::unpack(char *target, size_t size) const {
    bzero(target, size);

    const uint8_t *input = this->table;

    while(char code = *input) {
        size_t count = 1 + (code & 0x0f);
        if((code & 0xc0) == 0x40) { // repeat
            switch(code & 0x30) {
            case 0x00: { // repeat long
                uint32_t v = *reinterpret_cast<const uint32_t *>(input);
                input += sizeof(uint32_t);
                while(count--) {
                    *reinterpret_cast<uint32_t *>(target) = v;
                    target += sizeof(uint32_t);
                }
                break;
            }
            case 0x10: { // repeat word
                uint16_t v = *reinterpret_cast<const uint16_t *>(input);
                input += sizeof(uint16_t);
                while(count--) {
                    *reinterpret_cast<uint16_t *>(target) = v;
                    target += sizeof(uint16_t);
                }
                break;
            }
            case 0x20: { // repeat byte
                uint8_t v = *reinterpret_cast<const uint8_t *>(input);
                input += sizeof(uint8_t);
                while(count--) {
                    *reinterpret_cast<uint8_t *>(target) = v;
                    target += sizeof(uint8_t);
                }
                break;
            }
            case 0x30: // FIXME: panic
                break;
            }
        } else { // copy
            switch(code & 0xc0) {
            case 0x00: // no increment
                break;
            case 0x80: // increment by value of next byte
                target += *reinterpret_cast<const uint16_t *>(input) & 0x00ff;
                input += sizeof(uint16_t);
                break;
            case 0xc0: // increment by value of next 24 bits
                target += *reinterpret_cast<const uint32_t *>(input) & 0x00ffffff;
                input += sizeof(uint32_t);
                break;
            }
            switch(code & 0x30) {
            case 0x30: // FIXME: panic
                break;
            case 0x00: count *= 2;
            case 0x10: count *= 2;
            }
            while(count--)
                *target++ = *input++;
            //if(reinterpret_cast<uint32_t>(input) & 1)
            //++input;
            input = reinterpret_cast<uint8_t *>
                ( (reinterpret_cast<address_t>(input) + 1) & ~1);

        }
    }
}

// -------------------- PackedFunctions --------------------

/** a 680x0 JMP instruction \ingroup exec_library */
size_t PackedFunctions::count16(size_t offset) const {
    const int16_t *input = &displacements[offset];
    size_t bytes = 0;
    while(*input++ != -1)
        bytes += 6;
    return bytes;
}
size_t PackedFunctions::count32(size_t offset) const {
    const int32_t *input = &pointers[offset];
    size_t bytes = 0;
    while(*input++ != -1)
        bytes += 6;
    return bytes;
}
size_t PackedFunctions::unpack16(
    Library *library, const PackedFunctions *reference, size_t table_offset
  ) const {
    const int16_t *input = &displacements[table_offset];
    int16_t offset = 0;
    while(*input != -1) {
        offset -= 6;
        library->set_function(offset, reinterpret_cast<uint32_t>(reference) + *input++);
    }
    return -offset;
}
size_t PackedFunctions::unpack32(Library *library, size_t table_offset) const {
    const int32_t *input = &pointers[table_offset];
    int16_t offset = 0;
    while(*input != -1) {
        offset -= 6;
        library->set_function(offset, *input++);
    }
    return -offset;
}

size_t PackedFunctions::count(void) const {
    if(displacements[0] == -1) {
        return count16(1);
    } else {
        return count32(0);
    }
}
size_t PackedFunctions::unpack(Library *library) const {
    library->set_function_before();
    if(displacements[0] == -1) {
        return unpack16(library, this, 1);
    } else {
        return unpack32(library, 0);
    }
}
size_t PackedFunctions::count(const PackedFunctions *reference) const {
    if(reference) {
        return count16(0);
    } else {
        return count32(0);
    }
}
size_t PackedFunctions::unpack(Library *library, const PackedFunctions *reference) const {
    library->set_function_before();
    if(reference) {
        return unpack16(library, reference, 0);
    } else {
        return unpack32(library, 0);
    }
    library->set_function_after();
}

// -------------------- Resident --------------------

// might also be a Device * or Resource *...
// implementation of InitResident()
Library *Resident::initialise(void *seg_list) const {
    if(!(flags & RTF_AUTOINIT)) {
        // this module wants to initialise itself
        register uint32_t in_d0 asm("%d0") = 0;
        register void *in_a0 asm("%a0") = seg_list;
        register Library *(*in_a1)(void) asm("%a1") = auto_init_fn;
        register ExecBase *in_a6 asm("%a6") = execbase;
        register Library *ret asm("%d0");
        asm("jsr (%3)" : "=d"(ret) : "d"(in_d0), "a"(in_a0), "a"(in_a1), "a"(in_a6) : "%d1");
        return ret;
    }

    // this module is to be auto-initialised
    const AutoInit *ai = auto_init_data;
    Library *library = Library::make_library(
        ai->functions,
        ai->packedstruct,
        ai->library_init_fn,
        ai->data_size,
        seg_list
      );
    if(!library)
        return NULL;
    switch(library->type) {
    case Node::NT_DEVICE:
        execbase->AddDevice(reinterpret_cast<Device *>(library));
        break;
    case Node::NT_RESOURCE:
        execbase->AddResource(reinterpret_cast<Resource *>(library));
        break;
    case Node::NT_LIBRARY:
        execbase->AddLibrary(library);
        break;
    default:
        /* do nothing */
        break;
    }
    return library;
}

// -------------------- ResidentArray::* --------------------

ResidentArray::BuilderNode::BuilderNode(
    uint8_t priority_, const char *name_, const Resident *resident_
  )
    : Node(NT_UNKNOWN, priority_, name_),
      resident(resident_)
{}

void ResidentArray::BuilderList::add(const Resident *resident) {
    // now we try and stuff it into the list. We first look to see if it
    // is already present.
    BuilderNode *rn = find_name(resident->name);
    if(rn) {
        // if the ROMTag we've found is newer than the one in the list,
        // we replace the older one. The priority is used as a
        // tie-breaker if the romtags are the same version.
        if(resident->version > rn->resident->version
            || resident->priority > rn->priority) {
            rn->resident = resident;
        }
    } else {
        // create a new entry and enqueue it
        enqueue(new BuilderNode(resident->priority, resident->name, resident));
        ++count;
    }
}

void ResidentArray::BuilderList::search(address_t start, address_t end_) {
    const uint16_t *p = reinterpret_cast<const uint16_t *>(start);
    while(p < reinterpret_cast<const uint16_t *>(end_)) {
        const Resident *resident = reinterpret_cast<const Resident *>(p);
        if(resident->match_word == Resident::MATCHWORD
            && resident->match_tag == resident) {
            add(resident);
            p = reinterpret_cast<const uint16_t *>(resident->end);
        } else {
            ++p;
        }
    }
}
ResidentArray *ResidentArray::BuilderList::flatten(void) {
    const Resident **ret = new (Heap::MEMF_PUBLIC) const Resident *[count],
        ** p = ret;
    for(iterator i = begin(); i != end(); ++i) {
        *p++ = i->resident;
        delete *i;
    }
    *p++ = NULL;
    return reinterpret_cast<ResidentArray *>(ret);
}

// FindResident()
const Resident *ResidentArray::find_name(const char *name) const {
    const Resident * const *next = &entries[0];
    while(*next) {
        uint32_t nint = reinterpret_cast<uint32_t>(next);
        if(nint & (1<<31)) {
            // high bit set, so this is a chain pointer to another ResidentArray
            next = reinterpret_cast<const Resident **>(nint & ~(1<<31));
        } else {
            // this is a normal pointer to a Resident*
            const Resident *resident = *next++;
            if(!strcmp(name, resident->name))
                return resident;
        }
    }
    return NULL;
}

// InitCode()
void ResidentArray::initialise(Resident::Flags start_class, uint8_t min_version) const {
    const Resident * const *next = &entries[0];
    while(*next) {
        uint32_t nint = reinterpret_cast<uint32_t>(next);
        if(nint & (1<<31)) {
            // high bit set, so this is a chain pointer to another ResidentArray
            next = reinterpret_cast<const Resident **>(nint & ~(1<<31));
        } else {
            // this is a normal pointer to a Resident*, so check version and class, and initialise
            // it if it matches
            const Resident *resident = *next++;
            if(resident->flags & start_class
                && resident->version >= min_version) {
                // ROM-resident modules don't have a segment list
                resident->initialise(NULL);
            }
        }
    }
}

// -------------------- Library::Function --------------------

class exec::Library::Function {
    uint16_t instruction;
    uint32_t address;
public:
    void set_function(address_t fn) {
        instruction = 0x4ef9;
        address = fn;
        if(!address)
            instruction = 0x4afc;
    }

    address_t get_function(void) {
        return address;
    }
};

// -------------------- Library --------------------

Library::Library(void)
    : Node(Node::NT_LIBRARY, 0, NULL),
      version(), revision(), id()
{}

Library::Library(const char *name_, uint8_t version_, uint8_t revision_, const char *id_,
    Node::Type type_)
    : Node(type_, 0, name_),
      version(version_), revision(revision_),
      id(id_)
{}

Library *Library::make_library(
    const PackedFunctions *functions,
    const PackedStruct *initstruct,
    Library *(*init)(void),
    size_t data_size,
    void *seg_list
  ) {
    /// \todo type of init, type of seg_list
    size_t vector_size = 0;
    if(functions) {
        vector_size = functions->count();
        vector_size = (vector_size + 3) & ~3; // longword align to make sure
    }

    char *alloc = new(Heap::MEMF_PUBLIC) char[vector_size + data_size];
    if(!alloc)
        return NULL; // fail if no memory

    Library *library = reinterpret_cast<Library *>(alloc + vector_size);

    if(functions)
        functions->unpack(library);

    if(initstruct)
        initstruct->unpack(reinterpret_cast<char *>(library), data_size);

    if(init) {
        register uint32_t in_d0 asm("%d0") = data_size;
        register void *in_a0 asm("%a0") = seg_list;
        register Library *(*in_a1)(void) asm("%a1") = init;
        register ExecBase * const in_a6 asm("%a6") = execbase;
        register Library *ret asm("%d0") = library;
        asm("jsr (%3)" : "=r"(ret) : "r"(in_d0), "r"(in_a0), "r"(in_a1), "r"(in_a6) : "%%d1");
        return ret;
    }
    return library;
}

/// \bug not race-free, use Forbid()/Permit() pair
void Library::sum_library(void) {
    // don't bother summing a library if it doesn't want it
    if(!flags & LIBF_SUMUSED)
        return;
    // now calculate the checksum
    const size_t count = neg_size / sizeof(uint16_t);
    const uint16_t *top = reinterpret_cast<uint16_t *>(this),
        *p = top - count;
    uint16_t newsum = 0;
    while(p < top) newsum += *p++;
    // is the library marked as changed? If so, we just update the checksum
    if(flags & LIBF_CHANGED) {
        flags = Flags(flags & ~LIBF_CHANGED);
    } else if(sum && sum != newsum) {
        /// \bug should panic about checksum failure (AN_LibChkSum)
        execbase->Alert(0x81000003);
    }
    sum = newsum;
}

Library::Function *Library::get_function(int16_t offset) {
    return reinterpret_cast<Library::Function *>(
        reinterpret_cast<char *>(this) + offset
      );
}

void Library::set_function_before(void) {
    flags = Flags(flags | LIBF_CHANGED);
}

address_t Library::set_function(int16_t offset, address_t function) {
    Library::Function *vector = get_function(offset);
    address_t old = vector->get_function();
    vector->set_function(function);
    return old;
}

void Library::set_function_after(void) {
    // update library checksum, which won't panic since we had already set LIBF_CHANGED in
    // set_function_before
    sum_library();
    /// \bug this should flush the instruction cache after changing the jump table
}

// -------------------- LibraryList --------------------

void LibraryList::add_library(Library *library) {
    push(library);
    // \todo needs to also update the library vector checksum
}

//void *Library::operator new(size_t size, const PackedFunctions *fa) {
// AmigaOS library creation doesn't map all that nicely to C++ new and
// constructors. Libraries have a positive and negative size to record
// the size of the object. In the C++ world, those would normally be
// set by the constructor initialiser list.
//return heap->allocate(size);
//}

void *Library::operator new(size_t size, HeapList *heaplist, const PackedFunctions *fa) {
    size_t vector_size = fa->count();
    size_t vector_alloc = (vector_size + 3) & ~3;
    char * buf = heaplist->allocate(vector_alloc + size);
    Library * library = reinterpret_cast<Library *>(buf + vector_alloc);
    fa->unpack(library);
    return library;
}
