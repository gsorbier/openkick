// -*- c++ -*-
/**
   Basic datatypes.
   \file
*/

#ifndef TYPES_HPP
#define TYPES_HPP 1

#ifdef HOSTED_TEST
#include <sys/types.h>
#include <stdint.h>
#define struct_size_assert(NAME, TYPE, SIZE);
#else
typedef signed long int32_t;         //!< 32 bit signed type
typedef unsigned long uint32_t;      //!< 32 bit unsigned type
typedef signed long long int64_t;    //!< 64 bit signed type
typedef unsigned long long uint64_t; //!< 64 bit unsigned type

typedef uint32_t size_t;        //!< sizeof() return type
typedef int32_t ptrdiff_t;      //!< pointer difference type

#endif
typedef unsigned int address_t; //!< a memory address (same size as a pointer)

typedef signed char int8_t;          //!< 8 bit signed type
typedef unsigned char uint8_t;       //!< 8 bit unsigned type
typedef signed short int16_t;        //!< 16 bit signed type
typedef unsigned short uint16_t;     //!< 16 bit unsigned type

// Disable NULL; code should use C++11 nullptr instead.
////! a null pointer
//#define NULL __null

//! the offset of member \a MEMBER in the struct \a TYPE.
#define offsetof(TYPE, MEMBER)  __builtin_offsetof (TYPE, MEMBER)
//#define offsetof(TYPE, MEMBER) reinterpret_cast<size_t>(reinterpret_cast<TYPE *>(0)->MEMBER)

#ifndef HOSTED_TEST
/** a macro for compile-time checking that struct/class \a TYPE is of size \a SIZE (reporting \a
    NAME in the error) */
#define struct_size_assert(NAME, TYPE, SIZE) enum { sizeof__##NAME##__check = 1/(sizeof(TYPE) == (SIZE)) };
#endif

#endif
