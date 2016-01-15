// -*- mode: c++ -*-
/**
   Debugger (headers)
   \file
*/

#ifndef EXEC_DEBUGGER_HPP
#define EXEC_DEBUGGER_HPP

/** system debugger \ingroup exec_debugger */
class exec::Debugger {
    uint32_t key_bindings;
    uint32_t saved_key_bindings;
    uint32_t last_number;
    address_t current_address;
    uint32_t _pad0;
    uint32_t frame_size;
    address_t upper_limit;
    uint16_t input_buffer_size;
    uint8_t redisplay_frame_flag;
    uint8_t alter_mode_flag;
    uint16_t unprocessed_data_flag;
    uint16_t digits_entered;
    uint16_t number_is_param;
    address_t indirection_stack_pointer;
    uint8_t _pad1[38];
    char input_buffer[50];
    char last_typed;
    address_t stack_data_area;
    uint16_t breakpoint_instruction;
    union {
        address_t address;
        uint16_t old_insn;
    } breakpoints[16];
    uint16_t restore_intena;

public:
    static void init(void);
    static void putc(char c);
    static char getc(void);
    static int try_getc(void);

};

class exec::Formatter {
protected:
    virtual void output_repeat(char, size_t);
    virtual void output(const char *, const char *) = 0;
public:
    class Raw;
    const char *format(const char *, const char *);
};

class exec::Formatter::Raw : public Formatter {
    void (*code)(char);
    void *data;
protected:
    void output(const char *, const char *);
public:
    Raw(void(*code_)(char), void *data_)
        : code(code_), data(data_) {}
};

#endif
