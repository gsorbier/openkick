/**
   \file
*/
#ifndef EXEC_TODO_HPP
#define EXEC_TODO_HPP

/**
   \defgroup todo stuff that still needs doing
*/


/** (Stub declaration) */
class exec::Device : public Library {
public:
};

/** (Stub declaration) */
class exec::Unit : public Port {
    enum Flags {
        UNITF_ACTIVE = 1,
        UNITF_INTASK = 2
    };
    uint8_t flags;
    uint8_t _pad;
    uint16_t open_count;
};

/** (Stub declaration) */
class exec::DeviceList : private exec::ListOf<exec::Device> {
public:
    DeviceList(void)
        : ListOf<Device>(Node::NT_DEVICE)
    {};
};

/** (Stub declaration) */
class exec::Resource : public Library {
public:
};

/** (Stub declaration) */
class exec::ResourceList : private exec::ListOf<exec::Resource> {
public:
    ResourceList(void)
        : ListOf<Resource>(Node::NT_RESOURCE)
    {};
};

/** (Stub declaration) */
class exec::Task : public Node {

    friend class TaskList;
    enum Flags : uint8_t {
        TF_PROCTIME = 1<<0,
        TF_ETASK    = 1<<3,
        TF_STACKCHK = 1<<4,
        TF_EXCEPT   = 1<<5,     //!< task has a pending exception
        TF_SWITCH   = 1<<6,     //!< switch_fn is valid and should be called on losing CPU
        TF_LAUNCH   = 1<<7      //!< launch_fn is valid and should be called on gaining CPU
    };
    enum State : uint8_t {
        TS_INVALID = 1<<0,
        TS_ADDED   = 1<<1,      //!< task is being added
        TS_RUN     = 1<<2,      //!< task is running
        TS_READY   = 1<<3,      //!< task is runnable
        TS_WAIT    = 1<<4,      //!< task is waiting
        TS_EXCEPT  = 1<<5,
        TS_REMOVED = 1<<6       //!< task is being removed
    };
    enum Signals : uint32_t {
        SIGB_ABORT  = 1<<0,
        SIGB_CHILD  = 1<<1,
        SIGB_BLIT   = 1<<4,
        SIGB_SINGLE = 1<<4,
        SIGB_INTUITION = 1<<5,
        SIGB_NET = 1<<7,
        SIGB_DOS = 1<<8
    };

/// \todo shouldn't be public
public:

    Flags flags;
    State state;
    int8_t interrupt_count;
    int8_t task_count;
    Signals signals_allocated;
    Signals signals_waiting;
    Signals signals_received;
    Signals signals_exception;
    uint16_t traps_allocated;
    uint16_t traps_enabled;
    void *exception_data;
    void *exception_code;
    void *trap_data;
    void *trap_code;
    void *stack_pointer;
    void *stack_bottom;
    void *stack_top; // upper bound + 2
    void (*switch_fn)(void);
    void (*launch_fn)(void);
    MemEntryList mementry; // mementries to be released on task exit
    void *user_data;
public:
    void *operator new(size_t);
    void operator delete(void *);

    Task(const char *name_)
        : Node(Node::NT_TASK, 0, name_)
    {}
};

/** (Stub declaration) */
class exec::TaskList : public exec::ListOf<exec::Task> {
public:
    TaskList(void)
        : ListOf<Task>(Node::NT_TASK)
    {}
    void add(Task *task_) {
        this->enqueue(task_);
    }
};

/** (Stub declaration) */
struct exec::Interrupt : private Node {
    void *data;
    void (*code)(void);
public:
};

/** (Stub declaration) */
class exec::InterruptList : private ListOf<exec::Interrupt> {
public:
    InterruptList(void)
        : ListOf<Interrupt>(Node::NT_INTERRUPT)
    {};
};

/** (Stub declaration) */
struct exec::SoftIntList : private ListOf<exec::Interrupt> { // exec private, supposedly
    uint16_t _padSoftIntList;
public:
    SoftIntList(void)
        : ListOf<Interrupt>(Node::NT_SOFTINT)
    {};
};

/** (Stub declaration) */
struct exec::SemaphoreRequest : private MinNode {
    Task *waiter;
};

/** (Stub declaration) */
struct exec::SignalSemaphore : private Node {
    int16_t nest_count;
    MinListOf<MinNode> wait_queue;
    SemaphoreRequest multiple_link;
    Task *owner;
    int16_t queue_count;
public:
};

/** (Stub declaration) */
class exec::SignalSemaphoreList : private exec::ListOf<exec::SignalSemaphore> {
public:
    SignalSemaphoreList(void)
        : ListOf<SignalSemaphore>(Node::NT_SIGNAL_SEMAPHORE) /// \todo check type
    {};
};

// struct exec::SemaphoreMessage : private Message {
//     SignalSemaphore *semaphore;
// };

/** (Stub declaration) */
struct exec::IntVector {              // exec private, supposedly
    void *data;               // this goes into %a1 when interrupt is called
    void (*code)(void);       // this goes into %a5 when interrupt is called
    Node *node;
};

namespace exec {
    /** (Stub declaration) */
    enum CACRF {};
}

/** (Stub declaration) */
class exec::AVLNode {
    uint32_t reserved[4];
};

/** (Stub declaration) */
class exec::IORequest : public Message {
    enum Command {
        // \todo commands
    };
    Device *device;
    Unit *unit;
    uint16_t command;
    uint8_t flags;
    int8_t error;
};

/** (Stub declaration) */
class exec::IOStdReq : IORequest {
    uint32_t actual;
    uint32_t length;
    char *data;
    uint32_t offset;
};

#endif
