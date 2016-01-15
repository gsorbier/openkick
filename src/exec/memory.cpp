// -*- mode: c++ -*-
/**
   Memory handling (implementation)
   \file
*/

/**
   \defgroup exec_memory exec.library memory handling

   \todo write properly

   An Amiga system can contain memory of different performance and visibility to hardware devices,
   and which is scattered all over the memory map. A Heap refers to one such memory zone, and
   contains the performance flags, the start and end address, and the amount free in the zone. The
   type of memory is recorded in the MEMF_* flags.

   HeapList point to a singly-linked list of MemoryChunks. A MemoryChunk is a chunk of free memory.
   Initially, a Heap will point to a single MemoryChunk that covers the whole zone, and if all
   memory in the zone is allocated, this will be nullptr. MemoryChunks are linked to each other in
   ascending order of address so as to allow the deallocator to cheaply determine whether
   MemoryChunks may be merged or not.

   Classic AmigaOS provides global Allocate()/DeAllocate() to allocate and free from a specific
   Heap, AllocMem()/FreeMem() as malloc/free equivalents, AllocEntry()/FreeEntry() as a means of
   allocating from multiple zones atomically (ish), AllocAbs() to obtain memory at a specific
   address, AvailMem() to query free memory, AddMemList() to add a new memory zone, and
   CopyMem()/CopyMemQuick() as memcpy equivalents.

   AllocEntry()/FreeEntry() use a struct MemList, which starts is slightly confusing terminology as
   it's not an AmigaOS List. A MemList is a Node followed by an array of MemEntry, which is a union
   that is essentially a (requirement, size) tuple for allocation requests and an (address, size)
   tuple for the results of that allocation. The Node header exists so that responses may be linked
   together in a Task's tc_mementry (FIXME) field, and they will be automatically freed when a task
   exits.

   One thing to note is that in AmigaOS, allocations do not record their size like malloc/free and
   new/delete do, and the size needs to be provided at deallocation time. This means that
   implementations of new/delete need to squirrel away the space needed for the size *without*
   breaking the ABI.

   \todo split the Heap::Flags into attributes and options so that we can use a Flags type for
   memory attributes.

*/

#include <exec/memory.hpp>
#include <exec/new.hpp>
#include <exec/libc.hpp> // for bzero

using namespace exec;

//! a chunk of unallocated memory within a Heap [AmigaOS struct %MemChunk] \ingroup exec_memory
class exec::Heap::Chunk {
    Chunk *next;               //!< pointer to the next Chunk, or nullptr if this is the last one
    uint32_t size;             //!< size of the Chunk, including this header, in bytes
    char memory[0];            //!< the remainder of the free memory in this Chunk
    // This structure is part of the AmigaOS ABI and may not be extended.

    /** constructor
        \param next_ pointer to next Chunk (or nullptr)
        \param size_ size of this Chunk
    */
    Chunk(Heap::Chunk *next_, size_t size_) : next(next_), size(size_) {};
    friend class Heap;
    friend class HeapList;

    //static Chunk ** find_first(Chunk **, size_t);
};

// Heap::Chunk ** Heap::Chunk::find_first(Heap::Chunk **pchunk, size_t size) {
//     if(!pchunk) return nullptr;
//     if((*pchunk)->size >= size) return pchunk;
//     return find_first(&(*pchunk)->next, size);
// }

// -------------------- Heap --------------------

/** constructor.

    \param size_ the size of the heap, in bytes
    \param attributes_ the attributes of the heap, a Heap::Attributes
    \param priority_ the priority of the heap; slower or more expensive memory should be lower
    \param base_ the base address of the heap
    \param name_ a human-readable name for the heap, e.g. "Chip RAM"
*/
Heap::Heap(size_t size_, Attributes attributes_, uint8_t priority_,
           char *base_, const char *name_)
: Node(Node::NT_MEMORY, priority_, name_),
    attributes(attributes_),
    first(new (base_) Chunk(nullptr, size_)),
    lower(base_),
    upper(base_ + size_),
    free(size_)
{}

/** Creates a new Heap based on the given parameters.

    The Heap node itself will be allocated from the heap.

    \param size_ the size of the heap, in bytes
    \param attributes_ the attributes of the heap, a Heap::Flags
    \param priority_ the priority of the heap; slower or more expensive memory should be lower
    \param base_ the base address of the heap
    \param name_ a human-readable name for the heap, e.g. "Chip RAM"
    \returns the new Heap node
*/
Heap *Heap::create(size_t size_, Attributes attributes_, uint8_t priority_,
                   char *base_, const char *name_) {
    // to initialise a heap, we merely do a placement new of a Heap structure at the start of it
    // which manages space from the end of the Heap to the end of the new block
    return new ( base_ ) Heap (
        size_ - sizeof(Heap), attributes_, priority_, base_ + sizeof(Heap), name_
      );
}

/** Allocate memory from this heap.

    This is the underlying implementation for exec.library/Allocate().
    \param size the number of bytes to allocate
    \returns pointer to the new memory, or nullptr if the allocation failed
    \sa deallocate
*/
char *Heap::allocate(size_t size) {
    // essentially, we walk the memory list until we find the first sufficiently-large chunk, then
    // carve what we need out of it and return the address.

    // cheap checks: if we were asked for no bytes, or this chunk doesn't have sufficient free
    // space, we immediately bail
    if(!size || size > this->free) return nullptr;

    size = (size + 7) & ~7;     // round up to the next-largest multiple of 8 bytes

    // The pointer to the location that contains the pointer to the next memchunk. Essentially the
    // previous node in the singly-linked list. We need the address of the pointer and not the
    // pointer itself because we sometimes need to update it.
    Chunk **pchunk = &this->first;
    Chunk **found = nullptr;
    while(*pchunk) {        // While the next pointer is not nullptr, i.e. there are still memchunks
        if((*pchunk)->size >= size) {
            found = pchunk;
            break;
        }
        pchunk = &(*pchunk)->next;
    }

    // search unsuccessful, so fail the allocation
    if(!found)
        return nullptr;

    Chunk *chunk = *found;
    char *cstart = reinterpret_cast<char *>(chunk);
    this->free -= size;

    if(chunk->size == size) {
        // we've found a chunk that's the exact size we need. Result!
        *found = chunk->next;   // unlink Chunk
        return cstart;
    } else {
        // we've found a chunk that's larger than what we need, so it needs splitting. We carve what
        // we require off the bottom and create a new Chunk at the top, updating the pointer to it.
        *pchunk = new (cstart+size) Chunk (chunk->next, chunk->size-size);
        return cstart;
    }
}

/** Allocate memory from this heap, returning the highest address possible.

    \param size the number of bytes to allocate
    \returns pointer to the new memory, or nullptr if the allocation failed
    \sa deallocate
*/
char *Heap::allocate_reverse(size_t size) {
    // essentially, we walk the memory list noting the last sufficiently-large chunk, then carve
    // what we need out of it and return the address.

    // cheap checks: if we were asked for no bytes, or this chunk doesn't have sufficient free
    // space, we immediately bail
    if(!size || size > this->free) return nullptr;

    // round up to the next-largest multiple of 8 bytes
    size = (size + 7) & ~7;

    // The pointer to the location that contains the pointer to the next memchunk. Essentially the
    // previous node in the singly-linked list. We need the address of the pointer and not the
    // pointer itself because we sometimes need to update it.
    Chunk **pchunk = &this->first;
    Chunk **found = nullptr;
    // we iterate over the whole memory list to find the highest address that satisfies the
    // allocation requirements.
    while(*pchunk) {        // While the next pointer is not nullptr, i.e. there are still memchunks
        if((*pchunk)->size >= size)
            found = pchunk;
        pchunk = &(*pchunk)->next;
    }

    // search unsuccessful, so fail the allocation
    if(!found)
        return nullptr;

    Chunk *chunk = *found;
    char *cstart = reinterpret_cast<char *>(chunk);
    this->free -= size;

    if(chunk->size == size) {
        // we've found a chunk that's the exact size we need. Result!
        *found = chunk->next;   // unlink Chunk
        return cstart;
    } else {
        // we've found a chunk that's larger than what we need, so we truncate it by reducing its
        // size, and return a pointer to the no longer free block
        chunk->size -= size;
        return cstart + chunk->size;
    }
}

/** Allocate memory from this heap at a specific address.

    \param memory the address to allocate at
    \param size the number of bytes to allocate
    \returns pointer to the new memory, or nullptr if the allocation failed
    \sa deallocate
*/
char *Heap::allocate_at(char *memory, size_t size) {
    // This is broadly similar to allocate, except that we allocate at a specific address, or not at
    // all.

    // cheap checks: if we were asked for no bytes, or this chunk doesn't have sufficient free
    // space, or we were asked to allocate nullptr, we immediately bail.
    if(!memory || !size || size > this->free) return nullptr;

    size = (size + 7) & ~7; // round up to the next-largest multiple of 8 bytes

    Chunk **pchunk = &this->first;
    while(*pchunk) { // While the next pointer is not nullptr, i.e. there are still memchunks
        Chunk *chunk = *pchunk;
        char *cstart = reinterpret_cast<char *>(chunk);
        if(cstart <= memory && memory + size <= cstart + chunk->size) {
            // We've found a chunk that completely contains the memory region we were looking to
            // allocate. So we're in luck.
            if(memory + size < cstart + chunk->size) {
                // need to create new chunk between end of allocated block and end of the chunk, and
                // link it in. This temporarily creates an overlap with "chunk" but that's OK as
                // we're about to truncate or unlink that Chunk.
                chunk->next = new (memory + size) Chunk(chunk->next, chunk->size - size);
            }
            if(cstart < memory) {
                // need to truncate chunk
                chunk->size -= size;
            } else {
                // need to unlink chunk
                *pchunk = chunk->next;
            }
            this->free -= size;
            return memory;
        }
        pchunk = &chunk->next;
    };

    // we got to the end without finding the chunk
    return nullptr;
}

/** Release memory from this heap.

    This is the underlying implementation of exec.library/Deallocate().

    \param memory the memory previously allocated
    \param size the number of bytes previously allocated
    \sa allocate, allocate_reverse, allocate_at
*/
void Heap::deallocate(char *memory, size_t size) {
    // cheap checks: if we were asked to free nullptr, or no bytes, we immediately bail
    if(!memory || !size) return;

    /// \todo should reduce memory and increase size if memory is not 8-byte aligned
    size = (size + 7) & ~7;     // round up to the next-largest multiple of 8 bytes

    // update free space
    this->free += size;

    // Deallocation isn't quite the reverse of allocation. In theory we can just insert a Chunk at
    // \c memory and return, but that will leave the memory horribly fragmented. So we actually need
    // to merge this proposed Chunk with the previous or subsequent Chunk that it touches.

    Chunk **pchunk = &this->first; // pointer to pointer to tested Chunk
    Chunk *previous = nullptr;        // previous Chunk, if there is any
    // We want to find the Chunk just ahead of us.
    while(*pchunk) {        // While the next pointer is not nullptr, i.e. there are still memchunks
        Chunk *chunk = *pchunk;
        char *cstart = reinterpret_cast<char *>(chunk);
        // right, we have found the Chunk after us, so we're done.
        if(cstart > memory) break;
        // Update pointers and go for another trip round
        pchunk = &chunk->next;
        previous = chunk;
    }
    // We now have \c previous pointing to the Chunk that needs to be immediately before us, or
    // nullptr if it's the start of the list. \c *pchunk points to the Chunk that needs to be
    // immediately after us, or nullptr if it's the end of the list.
    Chunk *mc_next = *pchunk;

    /// \bug should test for double-free and then oops about it (AN_FreeTwice)

    // If there's a node after us, we check if it's right after us, and merge with it if so.
    if(mc_next) {
        char *cnext = reinterpret_cast<char *>(mc_next);
        if(cnext == memory + size) {
            // blocks are right together, so merge.
            size += mc_next->size;
            mc_next = mc_next->next;
        } else if(cnext < memory + size) {
            /// \bug should panic about overlapping free (AN_MemCorrupt)
            return;
        }
    }

    // Similarly, if there's a node before us, we'll try and merge with that.
    if(previous) {
        // we might be able to merge with the previous chunk, so let's try that.
        char *ptop = reinterpret_cast<char *>(previous) + previous->size;
        if(ptop == memory) {
            // blocks are right together, so merge.
            previous->size += size;
            previous->next = mc_next;
            return;
        } else if(ptop > memory) {
            /// \bug should panic about overlapping free (AN_MemCorrupt)
            return;
        }
    }

    // Now create and link in a new free Chunk
    *pchunk = new (memory) Chunk(mc_next, size);
}

/** Counts the chunks in this heap.

    This is primarily used by the test suite to check that the allocator is working properly. You do
    not normally need to know this.

    \returns the number of chunks in this Heap
*/
size_t Heap::count_chunks(void) const {
    size_t count = 0;
    Chunk *chunk = first;
    while(chunk) {
        ++count;
        chunk = chunk->next;
    };
    return count;
}

/** Counts the free space in this heap.

    This is an O(N) search and is primarily used by the test suite to check that the allocator is
    working properly. You probably want to use Heap::available() which is O(1).

    \returns the number of chunks in this heap
*/
size_t Heap::count_free(void) const {
    size_t count = 0;
    Chunk *chunk = first;
    while(chunk) {
        count += chunk->size;
        chunk = chunk->next;
    };
    return count;
}

/** Checks if this heap is sane.

    This is primarily used by the test suite to check that the allocator is working properly.

    \returns true if the heap is sane, otherwise false
*/
bool Heap::is_sane(void) const {
    Chunk *chunk = first;
    while(chunk) {
        // if the next pointer is to ourself or earlier in memory, we're broken.
        if(chunk->next && chunk->next <= chunk)
            return false;
        chunk = chunk->next;
    }
    return true;
}


// -------------------- HeapList --------------------

/** default constructor */
HeapList::HeapList(void)
    : ListOf<Heap>(Node::NT_MEMORY)
{}

/** moving constructor.

    This moves all of the Heap nodes from the other HeapList into this HeapList.

    \param that the HeapList that we move Heap nodes from. It will become empty.
*/
HeapList::HeapList(HeapList *that)
    : ListOf<Heap>(Node::NT_MEMORY)
{
    // copy from the other list to this list
    while(Heap *heap = that->shift())
        add(heap);
}

/** Allocate memory.
    This is the underlying implementation for exec.library/AllocMem().
    \param size the number of bytes to allocate
    \param requirements the allocation requirements
    \returns pointer to the new memory, or nullptr if the allocation failed
    \sa deallocate
*/
char *HeapList::allocate(size_t size, Heap::Attributes attributes, Heap::Options options) {
    char *mem = nullptr;
    iterator i = begin(), e = end();
    while(!mem && i != e) {
        Heap *heap = *i++;
        // if the memory pool described by the Heap is of the right type, try to allocate the memory
        // from the pool and return it. Otherwise, fall through and try the next Heap.
        if(heap->provides(attributes)) {
            if((unsigned)options & (unsigned)Heap::MEMF_REVERSE) {
                mem = heap->allocate_reverse(size);
            } else {
                mem = heap->allocate(size);
            }
            if(mem) {
                if((unsigned)options & (unsigned)Heap::MEMF_CLEAR)
                    bzero(mem, size);
                return mem;
            }
        }
    }
    return nullptr;
}

/** Allocate memory at a specific address.
    This is the underlying implementation for exec.library/AllocAbs().
    \param address the address to allocate at
    \param size the number of bytes to allocate
    \returns pointer to the new memory, or nullptr if the allocation failed
    \sa deallocate

    \note if you are trying to re-allocate a buffer that has become freed (usually due to a system
    reset), note that the first eight bytes may have been corrupted by a Heap::Node being there to
    mark the space as being free.
*/
char *HeapList::allocate_at(char *address, size_t size) {
    for(iterator i = begin(); i != end(); ++i) {
        Heap *heap = *i;
        if(heap->contains(address))
            return heap->allocate_at(address, size);
    }
    return nullptr;
}

/** Release memory.
    This is the underlying implementation of exec.library/FreeMem().
    \param address the memory previously allocated
    \param size the number of bytes previously allocated
    \sa allocate, allocate_reverse, allocate_at
*/
void HeapList::deallocate(char *address, size_t size) {
    for(iterator i = begin(); i != end(); ++i) {
        Heap *heap = *i;
        if(heap->contains(address))
            return heap->deallocate(address, size);
    }
    /// \bug should oops about bad free address (AN_BadFreeAddr)
}

/** Reports the amount of free memory.
    This is the underlying implementation of exec.library/AvailMem().
    \param requirements the requirements describing the memory to check
    \returns the amount of memory matching the description
*/
size_t HeapList::available(Heap::Attributes attributes, Heap::Options options) const {
    size_t size = 0;
    for(const_iterator i = begin(); i != end(); ++i) {
        const Heap *heap = *i;
        if(heap->provides(attributes)) {
            if((unsigned)options & (unsigned)Heap::MEMF_TOTAL) {
                // MEMF_TOTAL doesn't actually seem to be documented
                size += heap->upper - heap->lower;
            } else if((unsigned)options & (unsigned)Heap::MEMF_LARGEST) {
                Heap::Chunk *chunk = heap->first;
                while(chunk) {
                    if(chunk->size > size)
                        size = chunk->size;
                    chunk = chunk->next;
                }
            } else {
                size += heap->free;
            }
        }
    }
    return size;
}

/** Reports memory attributes for a given address.
    This is the underlying implementation of exec.library/TypeOfMem().
    \param address the pointer to check
    \returns the memory's attributes
*/
Heap::Attributes HeapList::type(const char *address) const {
    for(const_iterator i = begin(); i != end(); ++i) {
        const Heap *heap = *i;
        if(heap->contains(address))
            return heap->attributes;
    }
    // classic AmigaOS returns zero.
    return Heap::MEMF_ANY;
}

/** Adds a new Heap to the system.
    \param mh the new Heap to add
*/
void HeapList::add(Heap *mh) {
    this->enqueue(mh);
}

/** Adds a new Heap to the system.
    This is the underlying implementation of exec.library/AddMemList().
    \param size the size of the heap, in bytes
    \param attributes the attributes of the heap, a Heap::Flags
    \param priority the priority of the heap; slower or more expensive memory should be lower
    \param base the base address of the heap
    \param name a human-readable name for the heap, e.g. "Chip RAM"
*/
void HeapList::add(size_t size, Heap::Attributes attributes, uint8_t priority,
                   char *base, const char *name) {
    Heap *heap = Heap::create(size, attributes, priority, base, name);
    this->enqueue(heap);
}

/** Atomic allocation of multiple requests.
    This is the underlying implementation of exec.library/AllocEntry().
    \param request A MemEntryRequest * describing the requests
    \returns a MemEntryResponse describing the allocated memory or failure
    \bug this code is suspected to be broken
*/
MemEntryResponse HeapList::allocate_multiple(const MemEntry *request) {
    // first step, obtain a MemEntry structure \todo generate size from sizeof etc
    size_t me_size = 2 + 4 * request->count;
    MemEntry *me = allocate_mementry(request->count);
    // bail if we couldn't allocate one
    if(!me) return MemEntryResponse(me_size);
    me->count = request->count;

    // now iterate through the list, trying to allocate. "failed" is 0 for
    // success or the size of the failed allocation.
    size_t failed = 0;
    for(size_t i = 0; i < request->count; ++i) {
        char *mem = allocate(
            request->entries[i].size, request->entries[i].attributes, request->entries[1].options
          );
        me->entries[i].addr = mem;
        me->entries[i].size = request->entries[i].size;
        if(!mem)
            failed = request->entries[i].size;
    }

    // return the MemEntry on success
    if(!failed)
        return MemEntryResponse(me);

    // we failed, so release anything we have allocated so far, and return
    // the size of the failed request
    deallocate_multiple(me);
    return MemEntryResponse(failed);
}

/** Atomic deallocation of multiple requests.
    This is the underlying implementation of exec.library/FreeEntry().
    \param me the MemEntry * obtained via allocate_multiple().
    \note \c me is also released and the pointer is invalid after the call.
*/
void HeapList::deallocate_multiple(MemEntry *me) {
    for(size_t i = 0; i < me->count; ++i) {
        deallocate(me->entries[i].addr, me->entries[i].size);
    }
    deallocate_mementry(me);
}

MemEntry *HeapList::allocate_mementry(size_t count) {
    // a bit icky, should probably do sizeof()
    size_t bytes = 2 + count * 4;
    MemEntry * me = reinterpret_cast<MemEntry *>(
        allocate(bytes, Heap::MEMF_PUBLIC, Heap::MEMF_CLEAR)
      );
    if(me)
        me->count = count;
    return me;
}

void HeapList::deallocate_mementry(MemEntry *me) {
    if(me) {
        // a bit icky, should probably do sizeof()
        size_t bytes = 2 + me->count * 4;
        deallocate(reinterpret_cast<char *>(me), bytes);
    }
}
