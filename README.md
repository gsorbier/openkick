# Openkick

This is an *incomplete* implementation of
[AmigaOS](https://en.wikipedia.org/wiki/AmigaOS)'s
[exec.library](https://en.wikipedia.org/wiki/Exec_%28Amiga%29) provided for
educational purposes. You will not be able to get it to compile as-is, partly
because it needs a very peculiar toolchain, but mostly because I have had to
redact some of the code.

## Why was this written?

Openkick exists because I took one look at
[AROS](https://en.wikipedia.org/wiki/AROS_Research_Operating_System)'s source
code and didn't like the way it was still in 1980s-style C. AmigaOS is an
object-oriented microkernel and those are all the rage these days, and I felt
it would be instructive to implement it in a more modern language and style to
see if the code would be more readable, faster, and/or smaller than original
AmigaOS and/or AROS.

I feel that it wins on readability. It's got documentation, for a start :)

As for the generated code's size and performance, read on…

## Why was this abandoned?

GCC sucking, basically. It's the only modern compiler that supports m68k, and
its support is awful and getting worse with each new release. Further,
AmigaOS's calling convention places the library base in A6 and parameters in
other registers, whereas GCC wants to use A6 as the frame pointer and pass
parameters on the stack which causes a number of issues:

* Stack accesses require an `nnn(%a7)` operand, which adds an extension word
  (two bytes) per access to encode `nnn`. Registers such as the `%a7` there are
  encoded in the instruction word itself. Not only does this make the code
  larger, but it adds extra memory accesses for the extension word and the data
  itself. (Pedants will no doubt point out that an extension word is not
  required when `nnn` is zero, but that's the return address and parameters
  start at `4(%a7)`.)

* Implementations of library functions must take register parameters. This
  requires either a thunk or a disgusting hack to trick GCC into believing
  parameters are magically already in registers. GCC's "optimiser" keeps
  breaking the hacks, and defeating it also makes the code much less optimal.

* To call a library function requires temporarily moving the frame pointer out
  of the way so that the library base can be placed into A6. Again, more waste.

exec.library has 105 functions, and so adding a few tens of bytes per function
quickly adds up to a several kilobytes, which weighs heavily on a library
that's only about 13kB to start with. Clearly then, if this project were pushed
through to completion, it would not fit in the space available in the ROM and
would be slow.

## Is this based on the recent AmigaOS source leak?

No. I started this way back in 2010 and poked at it occasionally until I pretty
much gave up in 2013, well before the leak. It's written based on Commodore's
published API documentation, AROS source, and disassembling Kickstart as a last
resort if something was still unclear.

The existence of the leak merely prompted me to dig up this ancient code and
publish it.

## So what is there?

* Hardware:
  * Bare metal early startup (Incomplete.)
  * CPU/FPU detection (up to 68020 as per v33.)

* ROMWack:
  * All v33 functions *except* `Debug()` itself, e.g. `RawDoFmt()`, `RawIOInit()`.

* Lists:
  * All v33 functions, e.g. `Enqueue()`.
  * C++ template library for type-safe list handling.

* Memory:
  * RAM tests and arena initialisation.
  * All v33 functions, e.g. `AllocAbs()`, `AvailMem()`.
  * Some v39 extensions such as `MEMF_KICK`.
  * Some extra debugging/test hooks.

* Libraries:
  * ROMTag scanning and automatic library initialisation.
  * All v33 functions, e.g. `SetFunction()`, `OpenLibrary()`.

* Tasks:
  * Stub dummy task to keep UAE debugger sweet.

* Redacted:
  * Custom test.hook to bring up a framebuffer and print diagnostics, since it
    embeds topaz.font.

## Will this ever be restarted and completed?

Maybe, if LLVM ever gains reasonable m68k support. Don't hold your breath.
