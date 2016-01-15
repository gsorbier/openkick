// -*- mode: c++ -*-
/**
   List processing (implementation)
   \file
*/

//! \defgroup exec_list exec.library list primitives

/**
   \ingroup exec_list

   \todo document properly

   Exec comes with Node, MinNode, List and MinList types, which are primitives for a variety of
   list-based datatypes including queues, stacks, and priority queues. Sometimes they are used a bit
   like associative arrays, albeit with O(N) rather than O(log N) performance because of the
   list-based underlying structure.

   A MinNode provides a forward and backward link node. It is intended to be paired with a MinList
   which is a pair of overlaid MinNodes that act as start of list and end of list markers, and not
   actual nodes. This allows list operations which act on arbitrary nodes to not need to treat nodes
   at the ends of the list as special cases.

   A Node is a MinNode with additional type, priority and name fields, and is paired with a List
   which is a MinList with a type. Nodes in a List are supposed to have the same type field, which
   provides for RTTI, but this isn't always done.

   Objects that are to be placed into a List or MinList need to be subclasses of Node or MinNode
   respectively. This is quite unlike C++'s std::list type which does not expose its node objects.

   Classic AmigaOS provides global Insert(), AddHead(), AddTail(), Remove(), RemHead(), RemTail(),
   Enqueue() and FindName() functions for manipulating lists. Applications were expected to provide
   their own functions for everything else, including initialising empty List nodes.

   This reimplementation provides those for compatibility, but also provides methods of the Node,
   MinNode, List and Minlist classes to do this. Those are preferred since they are type-checked,
   perform appropriate object initialisation, and will probably get inlined.

   \todo cleanup doc

*/

#include <exec/list.hpp>
#include <exec/libc.hpp>

using namespace exec;

#if 0
/**
   inserts a node in priority order
   \param node_ the node to insert
*/
void List::enqueue(Node *node_) {
    for(Node *np : *this)
        if(np->priority < node_->priority)
            return node_->insert_before(np);
    return this->push(node_);
}
#endif

/**
   finds a node by name
   \param name the name of the node to find
   \returns the Node found, or nullptr if not found
*/
const Node *List::find_name(const char *name, const Node *node) const {
    const_iterator i = const_iterator(node ? node : reinterpret_cast<const Node *>(&head)),
        e = this->end();
    while(++i != e)
        if(!strcmp(name, i->name))
            return *i;
    return nullptr;
}

/**
   finds a node by name
   \param name the name of the node to find
   \returns the Node found, or nullptr if not found
*/
// \todo should probably be merged with the const version
Node *List::find_name(const char *name, Node *node) {
    iterator i = iterator(node ? node : reinterpret_cast<Node *>(&head)),
        e = this->end();
    while(++i != e)
        if(!strcmp(name, i->name))
            return *i;
    return nullptr;
}
