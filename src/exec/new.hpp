// -*- c++ -*-
/**
   Dynamic memory management.
   \file
*/

#ifndef NEW_HPP
//! Include guard
#define NEW_HPP 1

#include <types.hpp>
#include <exec/memory.hpp>

#ifdef HOSTED_TEST
/* HOSTED_TEST is a hosted environment which defines its own new/delete */
#include <new>
#else

//! Allocates memory for an object
void *operator new(size_t); // throw();
//! Allocates memory for an array
void *operator new[](size_t); // throw();
//! Releases the memory occupied by an object
void operator delete(void *) throw();
//! Releases the memory occupied by an array
void operator delete[](void *) throw();

/** Placement new
    \param place where to place the object
    \return the address of the object */
inline void *operator new(size_t, void *place) throw() { return place; }
//! Placement delete
inline void operator delete(void *, void *) throw() {}
/** Placement array new
    \param place where to place the object
    \return the address of the object */
inline void *operator new[](size_t, void *place) throw() { return place; }
//! Placement array delete
inline void operator delete[](void *, void *) throw() {}

#endif /* HOSTED_TEST */

void *operator new(size_t, exec::Heap::Attributes, exec::Heap::Options = exec::Heap::MEMF_NONE);

inline void *operator new(size_t size, exec::Heap::Options options) {
    return operator new(size, exec::Heap::MEMF_ANY, options);
}

inline void *operator new[](size_t size, exec::Heap::Attributes attributes,
                            exec::Heap::Options options = exec::Heap::MEMF_NONE) {
    return operator new(size, attributes, options);
}

inline void *operator new[](size_t size, exec::Heap::Options options) {
    return operator new(size, exec::Heap::MEMF_ANY, options);
}

#endif
