// -*- mode: c++ -*-
/**
   Memory handling (headers)
   \file
*/

#ifndef EXEC_MEMORY_HPP
#define EXEC_MEMORY_HPP

#include <exec/types.hpp>
#include <exec/list.hpp>

/** an allocatable memory zone [AmigaOS struct %MemHeader] \ingroup exec_memory */
class exec::Heap : public Node {
public:
    class Chunk;

    /// memory attributes \ingroup exec_memory
    enum Attributes : uint16_t {
        MEMF_ANY    =      0,   //!< any kind of memory
        MEMF_PUBLIC =      1,   //!< memory will not be swapped out (unused)
        MEMF_CHIP   =      2,   //!< memory is visible to custom chips
        MEMF_FAST   =      4,   //!< memory is not visible to custom chips
        MEMF_LOCAL  =  0x100,   //!< memory does not vanish on reset (V36+)
        MEMF_DMA24  =  0x200,   //!< memory is visible to Zorro II devices (V36+)
        MEMF_KICK   =  0x400,   //!< memory available in early startup (V39+)
    };

    /// memory requirements; may also include values from Heap::Type \ingroup exec_memory
    enum Options : uint16_t {
        MEMF_NONE         =   0, //!< no options requested
        // AllocMem() options
        MEMF_CLEAR        = 0x1, //!< clear memory before returning
        MEMF_REVERSE      = 0x4, //!< allocate memory from the top of the pool (V36+)
        MEMF_NO_EXPUNGE = 0x800, //!< fail rather than cause GC (V39+)
        // AvailMem() options
        MEMF_LARGEST      = 0x2, //!< return the largest free chunk
        MEMF_TOTAL        = 0x8, //!< return the total memory size
    };

private:
    friend class HeapList;
    const Attributes attributes; //!< memory attributes, values from Heap::Flags
    Chunk *first;                //!< address of first Memchunk in this zone
    const char *lower;           //!< starting address of this zone
    const char *upper;           //!< one-past-end address of this zone
    uint32_t free;               //!< amount of free space in this zone, in bytes
    // This structure is part of the AmigaOS ABI and may not be extended.
public:

    Heap(void);                 // disabled default ctor
    Heap(size_t, Attributes, uint8_t, char *, const char *) __attribute__((nonnull));
public:
    static Heap *create(size_t, Attributes, uint8_t, char *, const char *) __attribute__((nonnull,malloc));
    /**
       Check how much memory is in this heap.
       \return the amount of free memory in this heap
    */
    size_t available(void) const { return free; }
    /**
       Check whether this heap contains a given address.
       \param p the address to check
       \returns true if the heap contains this address, otherwise false
    */
    bool contains(const char *p) const __attribute__((nonnull)) { return lower <= p && p < upper; }
    /**
       Check whether memory from this heap would satisfy the memory requirements.
       \param a the memory requirements to check
       \returns true if the heap satisfies the memory requirements, otherwise false.
    */
    bool provides(const Attributes a) const {
        return ((unsigned)attributes & (unsigned)a) == (unsigned)a;
    }
    char *allocate [[gnu::malloc, gnu::assume_aligned(64)]] (size_t);
    char *allocate_reverse [[gnu::malloc, gnu::assume_aligned(64)]] (size_t);
    char *allocate_at(char *, size_t);
    void deallocate(char *, size_t);
    size_t count_chunks(void) const;
    size_t count_free(void) const;
    bool is_sane(void) const;
};


/** List of Heap; used as the system memory pool \ingroup exec_memory */
class exec::HeapList : private ListOf<exec::Heap> {
    // This structure is part of the AmigaOS ABI and may not be extended.
public:
    HeapList(void);
    HeapList(HeapList *);
    char *allocate [[gnu::malloc]] (
        size_t,
        Heap::Attributes = Heap::MEMF_PUBLIC,
        Heap::Options = Heap::MEMF_NONE
      );
    char *allocate_at(char *, size_t) __attribute__((nonnull));
    void deallocate(char *, size_t) __attribute__((nonnull));
    MemEntryResponse allocate_multiple(const MemEntry *);
    MemEntryResponse allocate_multiple(uint32_t, ...) __attribute__((sentinel));
    void deallocate_multiple(MemEntry *);
    MemEntry *allocate_mementry(size_t);
    void deallocate_mementry(MemEntry *);
    size_t available [[gnu::pure]] (
        Heap::Attributes = Heap::MEMF_ANY,
        Heap::Options = Heap::MEMF_NONE
      ) const;
    Heap::Attributes type [[gnu::nonnull, gnu::pure]] (const char *) const;
    void add [[gnu::nonnull]] (Heap *);
    void add [[gnu::nonnull]] (size_t, Heap::Attributes, uint8_t, char *, const char *);
};

/** input and output of AllocEntry(), ROMTags, and used by Tasks for memory
    autorelease [AmigaOS struct %MemList] \ingroup exec_memory
 */
class exec::MemEntry : public Node {
    // disabled new/delete
    void *operator new(size_t);
    void operator delete(void *);
public:
    uint16_t count;             //!< number of allocations
    struct {
        union {
            char *addr;         //!< address of allocation
            struct {
                Heap::Options options;
                Heap::Attributes attributes;
            };
        };
        uint32_t size;          //!< size of allocation
    } entries[0];               //!< the entries themselves
    // This structure is part of the AmigaOS ABI and may not be extended.
};

/** HeapList::allocate_multiple() response tuple */
class exec::MemEntryResponse {
public:
    size_t failed;              //!< if nonzero, the size of the failed allocation
    MemEntry *mementry;         //!< if not nullptr, the MemEntry * of the allocation
    /** default constructor */
    MemEntryResponse(void) : failed(0), mementry(nullptr) {};
    /** error-reporting constructor
        \param failed_ the size of the allocation that failed */
    MemEntryResponse(size_t failed_) : failed(failed_), mementry(nullptr) {};
    /** success-reporting constructor
        \param mementry_ the MemEntry * for the successful allocation */
    MemEntryResponse(MemEntry *mementry_) : failed(0), mementry(mementry_) {};
};

/** List of MemEntry; used by Tasks for memory autorelease \ingroup exec_memory */
class exec::MemEntryList : private exec::ListOf<MemEntry> {
public:
    MemEntryList(void) : ListOf<MemEntry>(Node::NT_UNKNOWN) {}
};

#endif
