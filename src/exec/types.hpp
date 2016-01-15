// -*- mode: c++ -*-
/**
   Exec class forward definitions (headers)
   \file
*/

#ifndef EXEC_TYPES_HPP
#define EXEC_TYPES_HPP

#include <types.hpp>

/*!

  \brief The system executive.

  exec.library contains the core functionality for the OS to run, including memory management, task
  scheduling, and message passing.

*/
namespace exec {
    class AVLNode;
    class CPUFeatures;
    class Device;
    class DeviceList;
    class Debugger;
    class ExecBase;
    class Formatter;
    class Heap;
    class HeapList;
    class IORequest;
    class IOStdReq;
    class IntVector;
    class Interrupt;
    class InterruptList;
    class Library;
    class LibraryList;
    class List;
    template <typename node_t> class ListOf;
    class MemEntry;
    class MemEntryList;
    class MemEntryResponse;
    class Message;
    class MinList;
    template <typename node_t> class MinListOf;
    class MinNode;
    class Node;
    class PackedFunctions;
    class PackedStruct;
    class Port;
    class PortList;
    class Resident;
    class ResidentArray;
    class Resource;
    class ResourceList;
    class Semaphore;
    class SemaphoreMessage;
    class SemaphoreRequest;
    class SignalSemaphore;
    class SignalSemaphoreList;
    class SoftIntList;
    class Task;
    class TaskList;
    class Unit;

    class Buffer;
}

#endif
