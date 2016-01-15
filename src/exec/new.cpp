#include <types.hpp>
#include <exec/execbase.hpp>
using namespace exec;

/* Note that AmigaOS AllocMem/FreeMem does not maintain the allocation size. Objects that are
   expected to be created/deleted via an external AllocMem()/FreeMem() will need to define operator
   new/delete for that class. */

static void *allocate(size_t size, Heap::Attributes attributes, Heap::Options options) {
    uint32_t *alloc = reinterpret_cast<uint32_t *>(
        execbase->AllocMem(size + 8, attributes + (options << 16))
        );
    if(alloc) {
        alloc[0] = size + 8;
        // perhaps we'll find a use for alloc[1] some day
        return &alloc[2];
    }
    return NULL;
}

static void release(void *mem) {
    uint32_t *alloc = static_cast<uint32_t *>(mem);
    alloc -= 2;
    execbase->FreeMem(reinterpret_cast<char *>(alloc), alloc[0]);
};

void *operator new(size_t size) {
    return allocate(size, Heap::MEMF_PUBLIC, Heap::MEMF_NONE);
}

void *operator new[](size_t size) {
    return allocate(size, Heap::MEMF_PUBLIC, Heap::MEMF_NONE);
}

void operator delete(void *mem) {
    release(mem);
}

void operator delete[](void *mem) {
    release(mem);
}

void *operator new(size_t size, Heap::Attributes attributes, Heap::Options options) {
    return allocate(size, attributes, options);
}
