// -*- mode: c++ -*-
/**
   Exec class size checks
   \file
*/

#include <exec/types.hpp>
#include <exec/list.hpp>
#include <exec/library.hpp>
#include <exec/memory.hpp>
#include <exec/message.hpp>
#include <exec/todo.hpp>
#include <exec/execbase.hpp>

using namespace exec;

// this file contains checks that openkick data structures are size-compatible (and thus hopefully
// binary-compatible) with the AmigaOS versions. It exists so that fields aren't inadvertently added
// or the packing alignment changed.

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// These are listed with the openkick class name (which is size-checked), followed by the AmigaOS
// structure name (or some approximation if it's anonymous) which will be displayed if the test
// fails, then the expected size.

// exec/alerts.h - doesn't define any structures

// exec/avl.h: AVLNode (V45)

struct_size_assert(AVLNode, AVLNode, 16);

// exec/devices.h: Device, Unit

struct_size_assert(Device, Device, sizeof(Library))
struct_size_assert(Unit, Unit, sizeof(Port) + 4)

// exec/errors.h - doesn't define any structures

// exec/exec.h - just pulls in all headers

// exec/execbase.h: ExecBase

struct_size_assert(ExecBase, ExecBase,
    sizeof(Library) + 50 + sizeof(IntVector)*16 + 46 +
    sizeof(List)*8 + sizeof(SoftIntList)*5 + 18 + sizeof(List) + 12
  )

struct_size_assert(anon_HeapList, HeapList, sizeof(List))
struct_size_assert(anon_ResourceList, ResourceList, sizeof(List))
struct_size_assert(anon_DeviceList, DeviceList, sizeof(List))
struct_size_assert(anon_InterruptList, InterruptList, sizeof(List))
struct_size_assert(anon_LibraryList, LibraryList, sizeof(List))
struct_size_assert(anon_PortList, PortList, sizeof(List))
struct_size_assert(anon_TaskList, TaskList, sizeof(List))

// exec/initializers.h - doesn't define any structures

// exec/interrupts.h: Interrupt, IntVector, SoftIntList

struct_size_assert(Interrupt, Interrupt, sizeof(Node) + 8)
struct_size_assert(IntVector, IntVector, 12)
struct_size_assert(SoftIntList, SoftIntList, sizeof(List) + 2)

// exec/io.h: IORequest, IOStdReq

struct_size_assert(IORequest, IORequest, sizeof(Message) + 12)
struct_size_assert(IOStdReq, IOStdReq, sizeof(Message) + 28)

// exec/libraries.h: Library

struct_size_assert(Library, Library, sizeof(Node) + 20);

// exec/Lists.h: List, MinList

struct_size_assert(List, List, 14)
struct_size_assert(template_List, ListOf<Node>, 14)
struct_size_assert(MinList, MinList, 12)
struct_size_assert(template_MinList, MinListOf<MinNode>, 12)

// exec/memory.h: MemChunk, MemHeader, MemEntry, MemList, MemHandlerData

struct_size_assert(MemChunk, Heap::Chunk, 8)
struct_size_assert(MemHeader, Heap, sizeof(Node) + 18)
// struct_size_assert(MemEntry, MemEntry, 8)
// note: MemList is not a MemoryList!
// struct_size_assert(MemList, MemList, 8)
// MemHandlerData is V39+
// struct_size_assert(MemHandlerData, MemHandlerData, 12)

// exec/nodes.h: Node, MinNode

struct_size_assert(Node, Node, 14)
struct_size_assert(MinNode, MinNode, 8)

// exec/ports.h: MsgPort, Message

// struct_size_assert(Port, MsgPort, sizeof(Node) + 6 + sizeof(List))
struct_size_assert(Message, Message, sizeof(Node) + 6)

// exec/Resident.h: Resident

struct_size_assert(Resident, Resident, 26);
struct_size_assert(anon_Resident_AutoInit, Resident::AutoInit, 16);

// exec/semaphores.h: SemaphoreRequest, SignalSemaphore, SemaphoreMessage,
// Semaphore

struct_size_assert(SemaphoreRequest, SemaphoreRequest, sizeof(MinNode) + 4);
struct_size_assert(
    SignalSemaphore, SignalSemaphore, sizeof(Node) + 2 +
    sizeof(MinList) + sizeof(SemaphoreRequest) + 6
  );
// struct_size_assert(SemaphoreMessage, SemaphoreMessage, sizeof(Message) + 4);
// struct_size_assert(Semaphore, Semaphore, sizeof(Port) + 2);

// exec/tasks.h: Task, StackSwapStruct

struct_size_assert(Task, Task, sizeof(Node) + 60 + sizeof(List) + 4);
// struct_size_assert(StackSwapStruct, StackSwapStruct, 12);

// exec/types.h - doesn't define any structures

#endif
