# -*- makefile -*-

EXEC_SRC += \
	src/exec/debugger.cpp \
	src/exec/execbase.cpp \
	src/exec/gcc.asm \
	src/exec/libc.cpp \
	src/exec/library.cpp \
	src/exec/list.cpp \
	src/exec/memory.cpp \
	src/exec/misc.asm \
	src/exec/new.cpp \
	src/exec/probe_cpu.asm \
	src/exec/startup.asm \
	src/exec/types.cpp \

TESTSRC += \
	src/exec/memory.cpp \
	src/exec/list.cpp \

