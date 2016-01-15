// -*- mode: c++ -*-
/**
   Libraries (headers)
   \file
*/

#ifndef EXEC_LIBRARY_HPP
#define EXEC_LIBRARY_HPP

#include <exec/types.hpp>
#include <exec/list.hpp>

/** a packed data structure \ingroup exec_library */
class exec::PackedStruct {
    uint8_t table[0];
public:
    void unpack(char *memory, size_t size) const;
};

/** a packed function table. \ingroup exec_library

    This is essentially a union between an array of 32 bit pointers, and an array of 16 bit relative
    offsets to a pointer. -1 is used as the end-of-table marker.

    AmigaOS's MakeFunctions() decides what format the table is based on a pointer given to it; if
    that pointer is not NULL, it generates pointers based on the 16 bit offsets; otherwise it uses
    the 32 bit pointers.

    AmigaOS's MakeLibrary() takes a slightly different tack. It looks at the first 16 bit value, and
    if it is -1, treats the rest of the table as 16 bit offsets relative to the table's address,
    otherwise the whole table is absolute.

    The methods of this class thus have a with-pointer and without-pointer form, and use the
    MakeFunctions()-style or MakeLibrary()-style table as appropriate.

    \todo need to test this code

*/
class exec::PackedFunctions {
    union {
        int32_t pointers[0];
        int16_t displacements[0];
    };
    size_t count16(size_t offset) const;
    size_t count32(size_t offset) const;
    size_t unpack16(Library *library, const PackedFunctions *reference, size_t offset) const;
    size_t unpack32(Library *library, size_t offset) const;
public:
    size_t count(void) const;
    size_t unpack(Library *library) const;
    size_t count(const PackedFunctions *reference) const;
    size_t unpack(Library *library, const PackedFunctions *reference) const;
};

/** a resident module, aka ROMTag \ingroup exec_library */
struct exec::Resident {

    /* Only one (or none) of coldstart, singletask or afterdos is set, and decides which phase of
       startup the module is initialised. If none are set, the module will not be initialised unless
       some external process does a FindResident()/InitResident() on it. */

    /** resident module flags \ingroup exec_library */
    enum Flags : uint8_t {
        /** module is initialised immediately after starting multitasking (V33) */
        RTF_COLDSTART = 1<<0,
        /** module is initialised while still in single-tasking mode (poorly documented: definitely
            not V33, probably V36+) */
        RTF_SINGLETASK = 1<<1,
        /** module is initialised when DOS loads (V36+).
            \note should have priority -120 or lower. */
        RTF_AFTERDOS = 1<<2,
        /** set if auto_init is the address of the initialisation function, otherwise it's a pointer
            to a MakeLibrary structure */
        RTF_AUTOINIT = 1<<7, /** FIXME: rt_Init points to data structure */
    };

    /** The autoinit structure; essentially the parameters for MakeLibrary(). \ingroup exec_library */
    struct AutoInit {
        uint32_t data_size;
        PackedFunctions *functions;
        PackedStruct *packedstruct;
        Library *(*library_init_fn)(void); // \todo
    };

    /** magic number for this structure, must be 0x4afc */
    enum MatchWord : uint16_t {
        MATCHWORD = 0x4afc      //!< the magic number: the 68000 "illegal" instruction
    } match_word;

    const Resident *match_tag;  //!< link to self
    const char *end;            //!< end address of module and address where scanning continues
    Flags flags;                //!< module flags (a Resident::Flags)
    uint8_t version;            //!< module version number
    Node::Type type;            //!< the module type, one of NT_LIBRARY, NT_DEVICE or NT_RESOURCE
    int8_t priority;            //!< initialisation priority
    const char *name;           //!< name of module, e.g. "exec.library")
    const char *id;             //!< module ID string

    /** init code or makelibrary struct */
    union {
        Library *(*auto_init_fn)(void);
        const AutoInit *auto_init_data;
    };

    // InitResident()
    // FIXME seg_list probably shouldn't be a void *
    Library *initialise(void *seg_list) const;
};

/** [anonymous AmigaOS structure] \ingroup exec_library */
class exec::ResidentArray {
    const Resident *entries[0];
    class BuilderNode;
    void *operator new(size_t);
    void operator delete(void *);
public:
    class BuilderList;
    const Resident *find_name(const char *) const;
    void initialise(Resident::Flags, uint8_t) const;
};

class exec::ResidentArray::BuilderNode : public Node {
public:
    const Resident *resident;
    BuilderNode(uint8_t priority_, const char *name_, const Resident *resident_);
};

class exec::ResidentArray::BuilderList : public ListOf<BuilderNode> {
public:
    // count of longwords needed for the resulting array of Resident *
    int count;
    BuilderList(void) : count(1) {}
    void add(const Resident *resident);
    void search(address_t start, address_t end_);
    ResidentArray *flatten(void);
};

/** a system library \todo fill out fields \ingroup exec_library */
class exec::Library : public Node {
    friend class LibraryList;
    friend class ExecBase;

    class Function;

    enum Flags : uint8_t {
        // LIBF_SUMMING = 1<<0,    //!< library vectors are being checksummed (FIXME: not used?)
            LIBF_CHANGED = 1<<1, //!< library vectors are dirty and need re-checksumming
            LIBF_SUMUSED = 1<<2, //!< set if the library wants checksum protection
            LIBF_DELEXP  = 1<<3, //!< set if the library is to be expunged
            };

    Flags flags;
    uint8_t pad;
    uint16_t neg_size;
    uint16_t pos_size;
    uint16_t version;
    uint16_t revision;
    const char *id;
    uint32_t sum;               // yes, 32 bits, even though the sum is calculated modulo 2**16
    uint16_t open_count;

    void *operator new(size_t, const exec::PackedFunctions *);
    void *operator new(size_t, HeapList *, const exec::PackedFunctions *);

    Function *get_function(int16_t);
public:
    Library(void);

    Library(const char *, uint8_t, uint8_t, const char *,
            Node::Type = Node::NT_LIBRARY);

    static Library *make_library(const PackedFunctions *, const PackedStruct *,
                                 Library *(*)(void), size_t, void *);

    void close_library(void);
    // Forbid() / call library's Close(), Permit()
    void rem_library(void);
    // Forbid(), call library's Expunge(), Permit()
    void sum_library(void);

    void set_function_before(void);
    address_t set_function(int16_t, address_t);
    void set_function_after(void);
};

/** a list of Library; the loaded system libraries \ingroup exec_library */
class exec::LibraryList : private ListOf<Library> {
public:
    LibraryList(void) : ListOf<Library>(Node::NT_LIBRARY) {}

    void add_library(Library *library);

    // openlibrary with default version of zero
    Library *open_library(const char *, unsigned);
    // Forbid(), FindName(), call library's Open(), Permit()
};

#endif
