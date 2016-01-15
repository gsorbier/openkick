// -*- mode: c++ -*-
/**
   Debugger (implementation)
   \file
*/

/**
   \defgroup exec_debugger exec.library debugger

   \todo document properly

   This contains the minimal subset of ROM-Wack to support the exec.library system calls. ROM-Wack
   is a bit-banging serial debugger that doesn't use hardware flow control.

*/

#include <exec/types.hpp>
#include <exec/debugger.hpp>
#include <hw/amiga.hpp>
#include <exec/libc.hpp>

using namespace exec;

//! The debugger's conventional workspace location
static void * const DebuggerBase = reinterpret_cast<void * const>(0x0200);
static amiga::Custom * const custom = reinterpret_cast<amiga::Custom *>(amiga::CustomBase);

void Debugger::init(void) {
    // sets SERPER to 9600 baud
    custom->serper(370);        // 370 for PAL, 374 for NTSC, but they're close enough to not matter
}

void Debugger::putc(char c) {
    if(c == '\n')
        putc('\r');
    // busywait until the transmit buffer is empty (bit 13 of SERDATR is set)
    while(!(custom->serdat() & (1<<13))) {};
    custom->serdat(c);
}

char Debugger::getc(void) {
    // read the port
    uint16_t data = custom->serdat();
    // busywait until the serial receive buffer is full (bit 14 of SERDATR is set)
    while(!(data & (1<<14))) {};
    // clear the serial receive interrupt and return the value
    custom->intreq(1<<11);
    return data & 0xff;
}

int Debugger::try_getc(void) {
    // read the port
    uint16_t data = custom->serdat();
    // poll until the serial receive buffer is full (bit 14 of SERDATR is set)
    if(!(data & (1<<14)))
        return -1;
    // clear the serial receive interrupt and return the value
    custom->intreq(1<<11);
    return data & 0xff;
}

void Formatter::output_repeat(char character, size_t count) {
    size_t bufsize = min(count, size_t(64));
    char buffer[64];
    for(size_t n = 0; n < bufsize; ++n)
        buffer[n] = character;
    while(count) {
        size_t outputsize = min(count, bufsize);
        output(buffer, buffer + outputsize);
        count -= outputsize;
    }
}

// TODO: check that we emit a NUL at the end of the string
const char *Formatter::format(const char *pattern, const char *data) {
    // essentially we're running a few intermingled state machines.
    while(char next = *pattern) {
        if(next != '%') {
            // simple case, literal text - just scan for end of string or % marker, then render
            // as-is.
            const char *start = pattern++;
            while(*pattern && *pattern != '%')
                ++pattern;
            output(start, pattern);
        } else {
            ++pattern;   // skip over %
            // parse pattern string, then obtain arguments from data array and output result

            // we now get to parse a %-specifier of the form %[flags][width.limit][length]type

            // (re-)initialise formatting variables
            bool alternate_form = false; // use "alternate form", e.g. "0x" prefix
            bool zero_fill = false;      // fill with '0' characters (otherwise ' ')
            bool left_justified = false; // left-justify output (otherwise right-justify)
            bool grouped = false;        // group digits
            size_t width = 0;            // minimum output width (fill to reach this width)
            size_t limit = ~0;           // truncate at this position (0 for no truncate)
            // integer size to pull off format list
            enum IntSize { INT16 = sizeof(uint16_t), INT32 = sizeof(uint32_t) } intsize = INT16;

            // the scratch is used for formatting numbers. These are formatted right-to-left, so we
            // start with p being scratch_end and working down. Eventually the number is in [p,
            // scratch_end). The largest value it will ever plausibly format is 2**64 in decimal, 20
            // digits, plus one sign digit, plus six commas, or 27 characters in total. No NUL is
            // required on the end, but it's rounded up to 32 anyway, to make sure and to ensure
            // nice alignment.

            const int scratch_size = 32;
            char scratch[scratch_size]; // scratch for number printing
            // start and end of output text
            char *start = scratch + scratch_size;
            char *end = start;

            // process flag characters
            while(true) {
                switch(*pattern) {
                case '#':
                    alternate_form = true;
                    break;
                case '0':
                    zero_fill = true;
                    break;
                case '-':
                    left_justified = true;
                    break;
                case '\'':
                    grouped = true;
                    break;
                default:
                    goto endflag;
                }
                ++pattern;
            }
        endflag:

            while(*pattern >= '0' && *pattern <= '9')
                width = width * 10 + *pattern++ - '0';

            if(width)
                zero_fill = false;

            if(*pattern == '.')
                while(*++pattern >= '0' && *pattern <= '9')
                    limit = limit * 10 + *pattern - '0';

            if(*pattern == 'l') {
                ++pattern;
                intsize = INT32;
            }

            switch(char type = *pattern++) {
            case 'p':
                //pointer type, hardwire pointer-length hex string, with 0x prefix and grouping.
                intsize = IntSize(sizeof(void *));
                type = 'x';
                alternate_form = true;
                zero_fill = true;
                [[clang::fallthrough]]
            case 'c': case 'd': case 'u': case 'x': {
                          bool negative = false; // minus sign to be put in output

                          // for integral types, we grab the next argument as a 16 or 32 bit value
                          // depending on the intsize flag
                          uint32_t value;
                          switch(intsize) {
                          case INT16:
                          default:     // can't happen, treat invalid values as 32 bit
                              value = *reinterpret_cast<const uint16_t *>(data);
                              data += sizeof(uint16_t);
                              break;
                          case INT32:
                              value = *reinterpret_cast<const uint32_t *>(data);
                              data += sizeof(uint32_t);
                              break;
                          }
                          // now re-switch based on the desired output format
                          switch(type) {
                          case 'c':
                              *--start = static_cast<char>(value);
                              break;
                          case 'd':
                              if(static_cast<int32_t>(value) < 0) {
                                  value = -value;
                                  negative = true;
                              }
                              [[clang::fallthrough]]
                          case 'u':
                                  if(value) {
                                      size_t digit = 0;
                                      while(value) {
                                          *--start = static_cast<char>(value % 10 + '0');
                                          value /= 10;
                                          if(grouped && value && !(++digit%3))
                                              *--start = ',';
                                      }
                                  } else {
                                      *--start = '0';
                                  }
                              if(negative)
                                  *--start = '-';
                              break;
                          case 'x':
                              if(value) {
                                  while(value) {
                                      int nybble = value % 16;
                                      nybble = (nybble > 9) ? nybble - 10 + 'a' : nybble + '0';
                                      *--start = static_cast<char>(nybble);
                                      value /= 16;
                                  }
                              } else {
                                  *--start = '0';
                              }
                              if(alternate_form) {
                                  *--start = 'x';
                                  *--start = '0';
                              }
                              break;
                          } // end inside switch()
                          break;
                      }
            case 's': {
                char *string = *reinterpret_cast<char *const*>(data);
                data += sizeof(const char *);
                // don't output the string if a NULL pointer
                if(string) {
                    start = string;
                    end = string + strnlen(string, limit);
                }
                break;
            }
            case 'b': {
                char *bstring = *reinterpret_cast<char *const*>(data);
                data += sizeof(const char *);
                // don't output the string if a NULL pointer
                if(bstring) {
                    uint8_t len = *bstring++;
                    start = bstring;
                    end = start + len;
                }
                break;
            }
            case '\0':
                // if a NUL, we want to step back so that the main loop gets to see it.
                --pattern;
                // fall-thru
            default:
                // just output the format charater (e.g. '%' if we were given "%%")
                start = &type;
                end = start + 1;
                break;
            }
            size_t length = end - start;
            if(length > limit) {
                // truncate output to limit characters
                output(start, start + limit);
            } else if(length >= width) {
                // text is at least as long as desired width so output as-is
                output(start, end);
            } else if(zero_fill) {
                output_repeat('0', width - length);
                output(start, end);
            } else if(left_justified) {
                output(start, end);
                output_repeat(' ', width - length);
            } else {
                output_repeat(' ', width - length);
                output(start, end);
            }

        }
    }
    return data;
}

void Formatter::Raw::output(const char *start, const char *end) {
    register void *in_a3 asm("%a3") = data;
    while(start != end) {
        register char in_d0 asm("%d0") = *start++;
        asm ("jsr (%0)" : : "a"(code), "d"(in_d0), "a"(in_a3) : "%d1", "%a0", "%a1", "%cc" );
    }
}
