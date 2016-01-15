// -*- mode: c++ -*-
/**
   Messaging (headers)
   \file
*/

#ifndef EXEC_MESSAGE_HPP
#define EXEC_MESSAGE_HPP

#include <exec/types.hpp>
#include <exec/list.hpp>

//! a message [AmigaOS struct %Message]
class exec::Message : private Node {
    Port *reply_port;          //!< the message's reply port
    uint16_t message_length;   //!< the message length in bytes
    // This structure is part of the AmigaOS ABI and may not be extended.
    friend class Port;
public:
    void send(Port *port);

    void reply(void);
};

//! a message port [AmigaOS struct %MsgPort]
class exec::Port : private Node {
    enum {
        SIGNAL = 0,             //!< signal the task in signal_task on delivery
        SOFTINT = 1,            //!< \todo signal SoftInt
        IGNORE = 2,             //!< do nothing
        CALLSUB = 3             //!< \todo calls a subroutine
    };
    uint8_t flags;
    uint8_t signal_bit;
    void *signal_task;            //!< \todo probably not a void *
    ListOf<Message> message_list; //!< list of queued messages
    // This structure is part of the AmigaOS ABI and may not be extended.

    friend class Message;
public:
    void send(Message *msg);
    Message *getmsg(void);      //!< \todo better name
};

//! a list of message ports
class exec::PortList : private exec::ListOf<exec::Port> {
public:
    PortList(void) : ListOf<Port>(Node::NT_PORT) {};
};

#endif
