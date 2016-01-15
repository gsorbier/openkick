# -*- Makefile -*-

SHELL = /bin/bash

# set to "true" or "false" depending on whether assembler output is required
CROSS_GENASM := true

TEST_CXX := mycolorgcc g++

CROSS_PREFIX := /opt/cross-mint/bin/m68k-atari-mint-

CROSS_AS := $(CROSS_PREFIX)as
CROSS_LD := $(CROSS_PREFIX)ld
#CROSS_CXX := mycolorgcc ccache $(CROSS_PREFIX)g++
CROSS_CXX := $(CROSS_PREFIX)g++
CROSS_CXXFILT := c++filt
CROSS_NM := $(CROSS_PREFIX)nm
CROSS_AR := $(CROSS_PREFIX)ar
CROSS_OBJCOPY := $(CROSS_PREFIX)objcopy
CROSS_STRIP := $(CROSS_PREFIX)strip

#ASFLAGS := -32
#CROSS_ARCHFLAGS := -mcpu=68000
#CROSS_ARCHFLAGS := -mcpu=68000 -mtune=68020-60 -O3 -fno-rtti -fno-exceptions -fomit-frame-pointer -fpack-struct=2
#CROSS_ARCHFLAGS := -mcpu=68000 -mtune=68020-40 -Os -fno-rtti -fno-exceptions -fomit-frame-pointer -fpack-struct=2
#CROSS_ARCHFLAGS := -mcpu=68000 -mtune=68020-40 -O3 -fno-rtti -fno-exceptions -fomit-frame-pointer -fpack-struct=2
CROSS_ARCHFLAGS := -mcpu=68020 -mtune=68020-60 -Os -fno-rtti -fno-exceptions -fomit-frame-pointer -fpack-struct=2 -mpcrel -mrtd

# 32 bit code with 16 bit alignment
#TEST_ARCHFLAGS := -m32 -fpack-struct=2
TEST_ARCHFLAGS := -std=gnu++0x

CROSS_CXXFLAGS := --pipe -g -x c++ -std=gnu++0x \
	-fno-use-linker-plugin \
	-fdiagnostics-show-option -Wall -Wextra -Wshadow \
	-Wunreachable-code \
	-ffreestanding -funit-at-a-time -ffunction-sections -fdata-sections -fcheck-new -fno-delete-null-pointer-checks # -fkeep-inline-functions

# CROSS_CXXFLAGS := --pipe -g -x c++ -Os						\
# 	-Wall -W -ansi								\
# 										\
# 	                            -Wundef -Wshadow -Wlarger-than-32768	\
# 	-Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings		\
# 	-Wconversion -Wsign-compare -Waggregate-return -Wno-aggregate-return	\
# 	                       -Wmissing-noreturn -Wpacked -Wpadded		\
# 	-Wredundant-decls                    				\
# 	-Wdisabled-optimization						\
# 										\
# 	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wreorder			\
# 	-Wno-non-template-friend -Wold-style-cast -Woverloaded-virtual		\
# 	-Wsign-promo -Wsynth							\
# 										\
# 	-Wno-long-long								\
# 										\
# 	 -fno-rtti -fno-exceptions -Wunreachable-code -funit-at-a-time		\
# 	 -fomit-frame-pointer -fno-delete-null-pointer-checks			\
# 	 -ffunction-sections -fdata-sections #-fkeep-inline-functions
# 	# -Winline is useful to find what wasn't inlined

# -Wunreachable-code gives bogus warnings under -O3

#DEBUGFLAGS := -O -DDEBUG
#DEBUGFLAGS := -DNDEBUG #-ggdb3 #-DDEBUG
#PROFFLAGS := #-pg
#LIBS := #/usr/lib/gcc/x86_64-linux-gnu/4.3/32/libsupc++.a  /usr/lib/gcc/x86_64-linux-gnu/4.3/32/libgcc_eh.a
CROSS_INCLUDE := -nostdinc -Isrc/
TEST_INCLUDE := -Isrc/ -It/
#BOOTINCLUDE := -nostdinc -Iboot_include # -I/usr/include/g++-3 -I/usr/include/g++-v3
CROSS_LDS := openkick.lds

# directories containing code that needs to be specially linked
#BOOTDIRS := boot/

.PHONY: all clean floppy test

all: openkick.rom

# All the modules that make up this project
#MODULES := $(shell find * -name module.mk | xargs -rn1 dirname)
MODULES := $(shell find src/ t/ -name module.mk | xargs -n1 dirname)

# Code that can be tested as a *hosted* implementation may add to this
TESTSRC :=

EXEC_SRC :=
TEST_SRC :=

# Include all the makefile fragments
include $(patsubst %,%/module.mk,$(MODULES))

SRC := $(EXEC_SRC) $(TEST_SRC)

# Determine the names of the object files
CROSS_ASMOBJ := $(patsubst %.asm,%.o,$(filter %.asm,$(SRC)))
CROSS_CPPOBJ := $(patsubst %.cpp,%.o,$(filter %.cpp,$(SRC)))

#TESTASMOBJ := $(patsubst %.asm,%.to,$(filter %.asm,$(TESTSRC)))
TESTCPPOBJ := $(patsubst %.cpp,%.to,$(filter %.cpp,$(TESTSRC)))
TESTCPPMAINBIN := $(patsubst %.cpp,%.t,$(filter %.cpp,$(TESTMAINSRC)))

# Include the include dependencies
include $(CROSS_CPPOBJ:.o=.d)

# calculate the include dependencies
%.d : %.cpp
	@ echo -e "\\033[1m Updating $<'s dependencies\\033[0m"
	@ ( /bin/echo -n $@ `dirname $<`/ && $(CROSS_CXX) $(CROSS_INCLUDE) $(CROSS_CXXFLAGS) $(CROSS_PROFFLAGS) $(CROSS_ARCHFLAGS) $(CROSS_DEBUGFLAGS)  -MM $< ) 	>> $@ || ( rm $@ ; exit 1)

# explicit rules for building
%.o : %.cpp
	@ echo -e "\\033[1m Compiling $< \\033[0m"
	@ if $(CROSS_GENASM) ; then $(CROSS_CXX) $(CROSS_INCLUDE) $(CROSS_CXXFLAGS) $(CROSS_PROFFLAGS) $(CROSS_ARCHFLAGS) $(CROSS_DEBUGFLAGS) -g0 -fverbose-asm -S -o $*.s $< ; fi
	@ $(CROSS_CXX) $(CROSS_INCLUDE) $(CROSS_CXXFLAGS) $(CROSS_PROFFLAGS) $(CROSS_ARCHFLAGS) $(CROSS_DEBUGFLAGS) -c -o $*.o $<

%.o : %.asm
	@ echo -e "\\033[1m Assembling $< \\033[0m"
	@ $(CROSS_AS) $(CROSS_ASFLAGS) -o $@ $<

src/exec.a : $(patsubst %.cpp,%.o,$(EXEC_SRC:.asm=.o))
	@ $(CROSS_LD) -r -o $@ $^

src/test.a : $(TEST_SRC:.cpp=.o)
	@ $(CROSS_LD) -r -o $@ $^

#openkick: $(CROSS_LDS) src/exec.a src/test.a
openkick: $(CROSS_LDS) src/exec.a
	@ echo -e "\\033[1m Linking \\033[0m"
	@ $(CROSS_LD) --sort-section=name --print-gc-sections --gc-sections --script=$(CROSS_LDS) --undefined='exec$$ROMTAG' --undefined='test$$ROMTAG' $^ -o $@
#	@ $(CROSS_LD) --sort-section=name --gc-sections --script=$(CROSS_LDS) --undefined='exec$$ROMTAG' --undefined='test$$ROMTAG' $^ -o $@
	@ $(CROSS_NM) $@ | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | $(CROSS_CXXFILT) | $(CROSS_CXXFILT) | sort -u >$@.map

openkick.rom: openkick
	@ echo -e "\\033[1m Generating ROM image \\033[0m"
	@ $(CROSS_OBJCOPY) --output-format=binary $< /dev/stdout | script/kicksum.pl >$@
#	@ $(CROSS_OBJCOPY) --output-format=binary $< /dev/stdout | ( dd bs=256k count=1 iflag=fullblock conv=sync && cat ROMs/kick13.rom) | script/kicksum.pl >$@

# ======================================================================

%.to : %.cpp
	@ echo -e "\\033[1m Compiling testable $< \\033[0m"
	@ $(TEST_CXX) $(TEST_INCLUDE) $(TEST_CXXFLAGS) $(TEST_ARCHFLAGS) -DHOSTED_TEST -O0 -ggdb --coverage -c -o $*.to $<
#	@ $(TEST_CXX) $(TEST_INCLUDE) $(TEST_CXXFLAGS) $(TEST_ARCHFLAGS) -O3 -c -o $*.to $<

# Note carefully the GCC docs for -fcoverage-arcs. Specifically, if you
# generate an executable directly, the .gcda file strips the path, so we
# need to do a two-stage build-then-link to get the .gcda to be put in the
# same directory as the source file.

%.tto : %.cpp
	@ echo -e "\\033[1m Compiling test $< \\033[0m"
	@ $(TEST_CXX) $(TEST_INCLUDE) $(TEST_CXXFLAGS) $(TEST_ARCHFLAGS) -DHOSTED_TEST -O0 -ggdb --coverage -c -o $@ $<

%.t : %.tto test.a
	@ echo -e "\\033[1m Linking test $@ \\033[0m"
	@ $(TEST_CXX) $(TEST_ARCHFLAGS) -Ikernel/ -DHOSTED_TEST -O0 -ggdb --coverage -o $@ $< test.a

test.a: $(sort $(TESTCPPOBJ))
	@ echo -e "\\033[1m Archiving testables \\033[0m"
	@ ar rcs $@ $(sort $(TESTCPPOBJ))

test: $(TESTCPPMAINBIN) test.a
	@ echo -e "\\033[1m Running test suite \\033[0m"
	@ rm -f *.gcov
	@ lcov --directory t/ --zerocounters
	@ prove -e '' -r t/
	@ for FILE in $$(find * -iname '*.gcno'); do echo $$FILE && gcov -p -o$$FILE $$FILE >/dev/null ; done

# ======================================================================

# reallyclean : here_clean
# 	for i in $(SUBDIRS) ; do $(MAKE) -C $$i reallyclean; done

# distclean : here_clean
# 	for i in $(SUBDIRS) ; do $(MAKE) -C $$i distclean; done

clean:
	find . -name '*~' -print0 | xargs -0 rm -f
	find . -name '*.bak' -print0 | xargs -0 rm -f
	find . -name '*.[osd]' -print0 | xargs -0 rm -f
	find . -name '*.to' -print0 | xargs -0 rm -f
	find . -name '*.gc??' -print0 | xargs -0 rm -f
	rm -f openkick{,.map,.small,.fdd}
	rm -rf html/ genhtml/ t.info
	rm -f test.a t/**/*.t

reallyclean: clean
	rm -rf src/gen

lvo:
	script/genthunk.pl

wc:
	find src/ t/ -type f -iname '*.[ch]pp' -print0 | sort -z | xargs -0 wc
