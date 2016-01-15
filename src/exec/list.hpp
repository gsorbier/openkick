// -*- mode: c++ -*-
/**
   List processing (headers)
   \file
*/

#ifndef EXEC_LIST_HPP
#define EXEC_LIST_HPP

#include <exec/types.hpp>

/** a doubly-linked list node [AmigaOS struct %MinNode] \ingroup exec_list */
class exec::MinNode {
    MinNode * next;             //!< pointer to next MinNode
    MinNode * prev;             //!< pointer to previous MinNode
    // This structure is part of the AmigaOS ABI and may not be extended.

    friend class ExecBase;

    friend class MinList;
    friend class List;
    friend class Node;

    //! \returns true if this is the end-of-list marker
    bool iseolm(void) const { return next == nullptr; }
    //! \returns true if this is the start-of-list marker
    bool issolm(void) const { return prev == nullptr; }

public:

    template<typename minnode_t> class iterator;
    template<typename minnode_t> class const_iterator;

    /** empty default constructor: does not initialise MinNode */
    MinNode(void) : next(), prev() {}
    MinNode(const MinNode &) = delete;
    MinNode &operator=(const MinNode &) = delete;

    /** empty dtor */
    //~MinNode() {}

    /** removes this node from the list it's in
        \returns the removed node */
    MinNode *remove(void) {
        prev->next = next;
        next->prev = prev;
        return this;
    }

    /** inserts this node after another node
        \param that the node to insert after
     */
    void insert_after (MinNode * that [[gnu::nonnull]]) {
        prev = that;
        next = that->next;
        next->prev = that->next = this;
    }

    /** inserts this node before another node
        \param that the node to insert before
     */
    void insert_before (MinNode * that [[gnu::nonnull]]) {
        next = that;
        prev = that->prev;
        prev->next = that->prev = this;
    }
};

/** a doubly-linked list node with a priority and name [AmigaOS struct %Node] \ingroup exec_list */
class exec::Node : public MinNode {
public:
    /** the different node types that may be placed into the type field of a Node
     * \ingroup exec_list */
    enum Type : uint8_t {
        NT_TASK = 1,              //!< Node is a Task
        NT_INTERRUPT = 2,         //!< Node is an Interrupt
        NT_DEVICE = 3,            //!< Node is a Library
        NT_PORT = 4,              //!< Node is a Port
        NT_PENDING_MESSAGE = 5,   //!< Node is a pending Message
        // NT_FREEMSG = 6,
        NT_REPLY_MESSAGE = 7,     //!< Node is a reply Message
        NT_RESOURCE = 8,          //!< Node is a Resource
        NT_LIBRARY = 9,           //!< Node is a Library
        NT_MEMORY = 10,           //!< Node is a Heap
        NT_SOFTINT = 11,          //!< Node is a soft Interrupt
        // NT_FONT = 12,
        // NT_PROCESS = 13,
        // NT_SEMAPHORE = 14,
        NT_SIGNAL_SEMAPHORE = 15, //!< Node is a SignalSemaphore
        // NT_BOOTNODE = 16,
        // NT_KICKMEM = 17,
        // NT_GRAPHICS = 18,
        // NT_DEATHMESSAGE = 19,
        NT_UNKNOWN = 0          //!< Node is of unknown or custom type
    };

    // FIXME: these fields probably shouldn't be public
    Type type;                 //!< type of this node (from enum Node::Type)
    int8_t priority;           //!< priority of this node
    const char *name;          //!< name of this node
    // This structure is part of the AmigaOS ABI and may not be extended.


public:
    /** default constructor.
        This constructor does not initialise the type, priority or name fields. */
    Node(void) {};
    /** constructor
        \param type_ type of this node (from enum Node::Type)
        \param priority_ priority of this node (-128 to 127)
        \param name_ name of this node */
    Node(Type type_, int8_t priority_, const char *name_)
        : type(type_), priority(priority_), name(name_)
    {}
};

/** an iterator over linked MinNodes \ingroup exec_list */
template<typename minnode_t> class exec::MinNode::iterator {
    MinNode *ptr;               //!< node the iterator currently points at

public:

    /** constructs an iterator pointing to a given MinNode
        \param p pointer to the MinNode */
    explicit iterator(minnode_t *p [[gnu::nonnull]]) : ptr(p) {}

    /** dereference iterator
        \returns the minnode_t * that the iterator points at */
    minnode_t *operator*(void) const { return static_cast<minnode_t *>(ptr); }
    /** dereference iterator
        \returns the minnode_t * that the iterator points at */
    minnode_t *operator->(void) const { return operator*(); }

    /** advances the iterator to the next node
        \returns an iterator pointing the next node */
    iterator &operator++(void) {
        ptr = ptr->next;
        return *this;
    }

    /** advances the iterator to the next node
        \returns an iterator pointing to the current node */
    iterator operator++(int)  {
        iterator ret(static_cast<minnode_t *>(ptr));
        this->operator++();
        return ret;
    }

    /** test for equality
        \param that the iterator to compare against
        \returns whether the two iterators point to the same node */
    bool operator==(const iterator &that) const { return ptr == that.ptr; }

    /** test for inequality
        \param that the iterator to compare against
        \returns whether the two iterators point to different nodes */
    bool operator!=(const iterator &that) const { return ptr != that.ptr; }
};

/** a const iterator over linked MinNodes \ingroup exec_list */
template<typename minnode_t> class exec::MinNode::const_iterator {
    const MinNode *ptr;        //!< node the const_iterator currently points at
public:

    /** constructs a const_iterator pointing to a given MinNode
        \param p pointer to the MinNode */
    explicit const_iterator(const minnode_t *p [[gnu::nonnull]]) : ptr(p) {}

    /** dereference iterator
        \returns the minnode_t * that the const_iterator points at */
    const minnode_t *operator*(void) const { return static_cast<const minnode_t *>(ptr); }
    /** dereference iterator
        \returns the minnode_t * that the const_iterator points at */
    const minnode_t *operator->(void) const { return operator*(); }

    /** advances the const_iterator to the next node
        \returns a const_iterator pointing the next node */
    const_iterator &operator++(void) {
        ptr = ptr->next;
        return *this;
    }

    /** advances the const_iterator to the next node
        \returns a const_iterator pointing to the current node */
    const_iterator operator++(int) {
        const_iterator ret = static_cast<const minnode_t *>(ptr);
        this->operator++();
        return ret;
    }

    /** test for equality
        \param that the iterator to compare against
        \returns whether the two const_iterators point to the same node */
    bool operator==(const const_iterator &that) const { return ptr == that.ptr; }

    /** test for inequality
        \param that the iterator to compare against
        \returns whether the two const_iterators point to different nodes */
    bool operator!=(const const_iterator &that) const { return ptr != that.ptr; }
};

/** a simple doubly-linked list of MinNode [AmigaOS struct %MinList] \ingroup exec_list */
class exec::MinList {
    MinNode * head;                 //!< start-of-list marker MinNode
    const MinNode * tail;           //!< end-of-list marker MinNode
    MinNode * tail_prev;            //!< prev field of end-of-list marker MinNode

    // This structure is part of the AmigaOS ABI and may not be extended.

    //! disabled copy constructor
    MinList(const MinList &);
    /** disabled copy assignment
        \returns nothing, because this is not implemented */
    MinList &operator=(const MinList &);

    friend class List;
    template<typename minnode_t> friend class MinListOf;
    template<typename node_t> friend class ListOf;

    /** get start-of-list node \returns the address of the start-of-list marker MinNode */
    MinNode *head_node(void) { return reinterpret_cast<MinNode *>(&head); }
    /** get start-of-list node \returns the address of the start-of-list marker MinNode */
    const MinNode *head_node(void) const { return reinterpret_cast<const MinNode *>(&head); }
    /** get end-of-list node \returns the address of the end-of-list marker MinNode */
    MinNode *tail_node(void) { return reinterpret_cast<MinNode *>(&tail); }
    /** get end-of-list node \returns the address of the end-of-list marker MinNode */
    const MinNode *tail_node(void) const { return reinterpret_cast<const MinNode *>(&tail); }

public:

    //! type of an iterator over a MinList
    typedef MinNode::iterator<MinNode> iterator;
    //! type of an iterator over a const MinList
    typedef MinNode::const_iterator<MinNode> const_iterator;

    /** constructs a new empty list */
    MinList(void)
        : head(tail_node()),
          tail(nullptr),
          tail_prev(head_node())
    {}

    //! test for emptiness \returns true if the list is empty
    bool isempty(void) const { return head == tail_node(); }

    /** adds a node to the start of the list
        \param that the node to insert */
    void unshift(MinNode * that [[gnu::nonnull]]) {
        that->next = head;
        that->prev = head_node();
        head->prev = that;
        head = that;
    }

    /** adds a node to the end of the list
        \param that the node to insert */
    void push(MinNode * that [[gnu::nonnull]]) {
        that->prev = tail_prev;
        that->next = tail_node();
        tail_prev->next = that;
        tail_prev = that;
    }

    /** removes a node from the start of the list
        \returns the removed node, or nullptr if the list was empty */
    MinNode *shift(void) { return isempty() ? nullptr : head->remove(); }

    /** removes a node from the end of the list
        \returns the removed node, or nullptr if the list was empty */
    MinNode *pop(void) { return isempty() ? nullptr : tail_prev->remove(); }

    /** removes this node from the list it's in
        \param node the node to return
        \returns the removed node (i.e. \a node itself)
     */
    static MinNode *remove(MinNode *node [[gnu::nonnull]]) { return node->remove(); }

    /** inserts a node after another node
        \param existing the existing node that we will insert after
        \param inserted the new node that will be inserted after
     */
    static void insert_after [[gnu::nonnull]] (MinNode *existing, MinNode *inserted) {
        inserted->insert_after(existing);
    }

    /** inserts a node before another node
        \param existing the existing node that we will insert before
        \param inserted the new node that will be inserted before
     */
    static void insert_before [[gnu::nonnull]] (MinNode *existing, MinNode *inserted) {
        inserted->insert_before(existing);
    }

    //! \returns an iterator pointing at the first node
    iterator begin(void) {
        return iterator(head);
    }

    //! \returns an iterator pointing at the end-of-list marker
    iterator end(void) {
        return iterator(reinterpret_cast<MinNode *>(&tail));
    }

    //! \returns a const_iterator pointing at the first node
    const_iterator begin(void) const {
        return const_iterator(head);
    }

    //! \returns a const_iterator pointing at the end-of-list marker
    const_iterator end(void) const {
        return const_iterator(reinterpret_cast<const MinNode *>(&tail));
    }
};

/** a simple doubly-linked list of Node [AmigaOS struct %List] \ingroup exec_list */
class exec::List : public MinList {
    template<typename minnode_t> friend class MinListOf;
public:
    uint8_t type;               //!< list type, a Node::Type
private:
    uint8_t _pad_List;          //!< (structure padding)
    // This structure is part of the AmigaOS ABI and may not be extended.

public:

    //! type of an iterator over a List
    typedef Node::iterator<Node> iterator;
    //! type of an iterator over a const List
    typedef Node::const_iterator<Node> const_iterator;

    /** default constructor.
        \note this does not initialise the type field
    */
    List(void)
        : MinList(), type(), _pad_List()
    {}

    /** constructor
        \param type_ the Node::Type of the elements in the List
    */
    List(uint8_t type_)
        : MinList(), type(type_), _pad_List()
    {}

    //! get start iterator \returns an iterator pointing at the first node
    iterator begin(void) {
        return iterator(static_cast<Node *>(head));
    }

    //! get end iterator \returns an iterator pointing at the end-of-list marker
    iterator end(void) {
        return iterator(static_cast<Node *>(tail_node()));
    }

    //! get start const_iterator \returns a const_iterator pointing at the first node
    const_iterator begin(void) const {
        return const_iterator(static_cast<Node *>(head));
    }

    //! get end const_iterator \returns a const_iterator pointing at the end-of-list marker
    const_iterator end(void) const {
        return const_iterator(static_cast<const Node *>(tail_node()));
    }

    void enqueue [[gnu::nonnull]] (Node *node);
    const Node *find_name(const char *, const Node * = nullptr) const;
    Node *find_name(const char *, Node * = nullptr);
};

/** a simple doubly-linked list of MinNode [AmigaOS struct %MinList] \ingroup exec_list */
template<typename minnode_t> class exec::MinListOf {
    /** the List we're wrapping */
    MinList minlist;

    // This structure is part of the AmigaOS ABI and may not be extended.

    //! disabled copy constructor
    MinListOf(const MinListOf &);
    /** disabled copy assignment
        \returns nothing, because this is not implemented */
    MinListOf &operator=(const MinListOf &);

public:

    //! type of an iterator over a MinList
    typedef MinNode::iterator<minnode_t> iterator;
    //! type of an iterator over a const MinList
    typedef MinNode::const_iterator<minnode_t> const_iterator;

    /** constructs a new empty list */
    MinListOf(void)
        : minlist()
    {}

    //! \returns true if the list is empty
    bool isempty(void) const { return minlist.isempty(); }

    /** adds a node to the start of the list
        \param that the node to insert */
    void unshift(minnode_t * that) __attribute__((nonnull)) { minlist.unshift(that); }

    /** adds a node to the end of the list
        \param that the node to insert */
    void push(minnode_t * that) __attribute__((nonnull)) { minlist.push(that); }

    /** removes a node from the start of the list
        \returns the removed node, or nullptr if the list was empty */
    minnode_t *shift(void) { return minlist.shift(); }

    /** removes a node from the end of the list
        \returns the removed node, or nullptr if the list was empty */
    minnode_t *pop(void) { return minlist.pop(); }

    /** removes this node from the list it's in
        \param node the node to return
        \returns the removed node (i.e. \a node itself)
     */
    static minnode_t *remove(MinNode *node) __attribute__((nonnull)) {
        return MinList::remove(node);
    }

    /** inserts a node after another node
        \param existing the existing node that we will insert after
        \param inserted the new node that will be inserted after
     */
    static void insert_after(MinNode *existing, MinNode *inserted) __attribute__((nonnull)) {
        MinList::insert_after(existing, inserted);
    }

    /** inserts a node before another node
        \param existing the existing node that we will insert before
        \param inserted the new node that will be inserted before
     */
    static void insert_before(MinNode *existing, MinNode *inserted) __attribute__((nonnull)) {
        MinList::insert_before(existing, inserted);
    }

};

/** a simple doubly-linked list of Node [AmigaOS struct %List] \ingroup exec_list */
template<typename node_t> class exec::ListOf {
private:
    /** the List we're wrapping */
    List list;
    // This structure is part of the AmigaOS ABI and may not be extended.

    //! disabled default constructor
public:

    //! type of an iterator over a List
    typedef Node::iterator<node_t> iterator;
    //! type of an iterator over a const List
    typedef Node::const_iterator<node_t> const_iterator;

    /** default constructor.
        \note this does not initialise the type field
     */
    ListOf(void)
        : list()
    {}

    /** constructor
        \param type_ the Node::Type of the elements in the List
    */
    ListOf(uint8_t type_)
        : list(type_)
    {}

    /* same methods as MinListOf... */

    //! \returns true if the list is empty
    bool isempty(void) const { return list.isempty(); }

    /** adds a node to the start of the list
        \param that the node to insert */
    void unshift(node_t * that) __attribute__((nonnull)) { list.unshift(that); }

    /** adds a node to the end of the list
        \param that the node to insert */
    void push(node_t * that)  __attribute__((nonnull)) { list.push(that); }

    /** removes a node from the start of the list
        \returns the removed node, or nullptr if the list was empty */
    node_t *shift(void) { return static_cast<node_t *>(list.shift()); }

    /** removes a node from the end of the list
        \returns the removed node, or nullptr if the list was empty */
    node_t *pop(void) { return static_cast<node_t *>(list.pop()); }

    /** removes this node from the list it's in
        \param node the node to return
        \returns the removed node (i.e. \a node itself)
     */
    static node_t *remove(MinNode *node) __attribute__((nonnull)) { return List::remove(node); }

    /** inserts a node after another node
        \param existing the existing node that we will insert after
        \param inserted the new node that will be inserted after
     */
    static void insert_after(MinNode *existing, MinNode *inserted) __attribute__((nonnull)) {
        List::insert_after(existing, inserted);
    }

    /** inserts a node before another node
        \param existing the existing node that we will insert before
        \param inserted the new node that will be inserted before
     */
    static void insert_before(MinNode *existing, MinNode *inserted) __attribute__((nonnull)) {
        List::insert_before(existing, inserted);
    }

    //! \returns an iterator pointing at the first node
    iterator begin(void) {
        return iterator(static_cast<node_t *>(list.head));
    }

    //! \returns an iterator pointing at the end-of-list marker
    iterator end(void) {
        return iterator(static_cast<node_t *>(list.tail_node()));
    }

    //! \returns a const_iterator pointing at the first node
    const_iterator begin(void) const {
        return const_iterator(static_cast<node_t *>(list.head));
    }

    //! \returns a const_iterator pointing at the end-of-list marker
    const_iterator end(void) const {
        return const_iterator(static_cast<const node_t *>(list.tail_node()));
    }

    /* new ListOf methods... */

    /**
       inserts a node in priority order
       \param node_ the node to insert
    */
    void enqueue(Node *node_) __attribute__((nonnull)) { list.enqueue(node_); }

    /**
       finds a node by name
       \param name the name of the node to find
       \returns the Node found, or nullptr if not found
    */
    const node_t *find_name(const char *name) const __attribute__((nonnull)) {
        return static_cast<node_t *>(list.find_name(name));
    }

    /**
       finds a node by name
       \param name the name of the node to find
       \returns the Node found, or nullptr if not found
    */
    node_t *find_name(const char *name) __attribute__((nonnull)) {
        return static_cast<node_t *>(list.find_name(name));
    }

};

#endif
