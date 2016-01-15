// -*- mode: c++ -*-
/**
   Memory buffers (headers)
   \file
*/

#ifndef EXEC_BUFFER_HPP
#define EXEC_BUFFER_HPP

#include <exec/types.hpp>

struct exec::Buffer {
    char *start, *end;

    Buffer(void) : start(NULL), end(NULL) {}
    Buffer(char *start_, char *end_) : start(start_), end(end_) {}
    size_t size(void) const { return end - start; }
    bool empty(void) const { return end <= start; }
    operator bool(void) const { return !empty(); }
    operator const void *(void) const { return empty() ? NULL : this; }
    Buffer carve_bottom(size_t size_) {
        Buffer ret(start, start + size_);
        start += size_;
        return ret;
    }
    Buffer carve_top(size_t size_) {
        Buffer ret(end - size_, end);
        end -= size_;
        return ret;
    }
};

#endif
