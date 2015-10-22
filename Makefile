BIN=escargot

#######################################################
# Environments
#######################################################

HOST=
OPT=
ARCH=x64

NPROCS:=1
OS:=$(shell uname -s)

ifeq ($(OS),Linux)
  NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
endif
ifeq ($(OS),Darwin) # Assume Mac OS X
  NPROCS:=$(shell system_profiler | awk '/Number Of CPUs/{print $4}{next;}')
endif

ifeq ($(HOST),)
	HOST=linux
endif

$(info host... $(HOST))

ifeq ($(HOST), linux)
	CC = gcc
	CXX = g++
	CXXFLAGS = -std=c++11
else
endif

ifeq ($(ARCH), x64)
	CXXFLAGS += -DESCARGOT_64=1
else ifeq ($(ARCH), x86)
	CXXFLAGS += -DESCARGOT_32=1
endif

ifeq ($(MAKECMDGOALS), jit.debug)
	BUILDDIR=out/jit/debug
else ifeq ($(MAKECMDGOALS), jit.release)
	BUILDDIR=out/jit/release
else ifeq ($(MAKECMDGOALS), interpreter.debug)
	BUILDDIR=out/interpreter/debug
else ifeq ($(MAKECMDGOALS), interpreter.release)
	BUILDDIR=out/interpreter/release
else
	BUILDDIR=.
endif

#######################################################
# Global build flags
#######################################################

# common flags
CXXFLAGS += -fno-rtti -fno-math-errno -Isrc/
CXXFLAGS += -fdata-sections -ffunction-sections

LDFLAGS += -lpthread
# -ltcmalloc_minimal
LDFLAGS += -Wl,--gc-sections

# flags for debug/release
CXXFLAGS_DEBUG = -O0 -g3 -frounding-math -fsignaling-nans -fno-omit-frame-pointer -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-invalid-offsetof -Wno-sign-compare -Wno-unused-local-typedefs
CXXFLAGS_RELEASE = -O2 -g3 -DNDEBUG -fomit-frame-pointer -frounding-math -fsignaling-nans

# flags for jit/interpreter
CXXFLAGS_JIT = -DENABLE_ESJIT=1
CXXFLAGS_INTERPRETER =

jit.debug: CXXFLAGS+=$(CXXFLAGS_JIT) $(CXXFLAGS_DEBUG)
jit.release: CXXFLAGS+=$(CXXFLAGS_JIT) $(CXXFLAGS_RELEASE)
interpreter.debug: CXXFLAGS+=$(CXXFLAGS_INTERPRETER) $(CXXFLAGS_DEBUG)
interpreter.release: CXXFLAGS+=$(CXXFLAGS_INTERPRETER) $(CXXFLAGS_RELEASE)

#######################################################
# Third-party build flags
#######################################################

# bdwgc
CXXFLAGS += -Ithird_party/bdwgc/include/
GCLIBS_DEBUG = third_party/bdwgc/out/debug/.libs/libgc.a #third_party/bdwgc/out/debug/.libs/libgccpp.a
GCLIBS_RELEASE = third_party/bdwgc/out/release/.libs/libgc.a #third_party/bdwgc/out/release/.libs/libgccpp.a
jit.debug:           GCLIBS=$(GCLIBS_DEBUG)
jit.release:         GCLIBS=$(GCLIBS_RELEASE)
interpreter.debug:   GCLIBS=$(GCLIBS_DEBUG)
interpreter.release: GCLIBS=$(GCLIBS_RELEASE)

# nanojit
include third_party/nanojit/Build.mk

# netlib
CXXFLAGS += -Ithird_party/netlib/

# v8's fast-dtoa
CXXFLAGS += -Ithird_party/double_conversion/
SRC_DTOA += third_party/double_conversion/fast-dtoa.cpp
SRC_DTOA += third_party/double_conversion/diy-fp.cpp
SRC_DTOA += third_party/double_conversion/cached-powers.cpp
SRC_DTOA += third_party/double_conversion/bignum.cpp
SRC_DTOA += third_party/double_conversion/bignum-dtoa.cpp

# rapidjson
CXXFLAGS += -Ithird_party/rapidjson/include/

# yarr
CXXFLAGS += -Ithird_party/yarr/
SRC_YARR += third_party/yarr/OSAllocatorPosix.cpp
SRC_YARR += third_party/yarr/PageBlock.cpp
SRC_YARR += third_party/yarr/YarrCanonicalizeUCS2.cpp
SRC_YARR += third_party/yarr/YarrInterpreter.cpp
SRC_YARR += third_party/yarr/YarrPattern.cpp
SRC_YARR += third_party/yarr/YarrSyntaxChecker.cpp

# Common
THIRD_PARTY_LIBS= $(GCLIBS)

#######################################################
# SRCS & OBJS
#######################################################

SRC=
SRC += $(foreach dir, ./src , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/ast , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/bytecode , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/jit , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/parser , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/runtime , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/shell , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/vm , $(wildcard $(dir)/*.cpp))

SRC += $(SRC_YARR)
SRC += $(SRC_ESPRIMA_CPP)
SRC += $(SRC_NANOJIT)
SRC += $(SRC_DTOA)

OBJS := $(SRC:%.cpp= $(BUILDDIR)/%.o)
OBJS += $(SRC_C:%.c= $(BUILDDIR)/%.o)

#######################################################
# Targets
#######################################################

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

jit.debug: $(BUILDDIR)/$(BIN)
	cp -f $< .
jit.release: $(BUILDDIR)/$(BIN)
	cp -f $< .
interpreter.debug: $(BUILDDIR)/$(BIN)
	cp -f $< .
interpreter.release: $(BUILDDIR)/$(BIN)
	cp -f $< .

$(BUILDDIR)/$(BIN): $(OBJS) $(THIRD_PARTY_LIBS)
	$(CXX) -o $@ $(OBJS) $(THIRD_PARTY_LIBS) $(LDFLAGS)

$(BUILDDIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $< -o $@
	$(CXX) -MM $(CXXFLAGS) -MT $@ $< > $(BUILDDIR)/$*.d

$(BUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@
	$(CC) -MM $(CFLAGS) -MT $@ $< > $(BUILDDIR)/$*.d

full:
	BUILDDIR=out/jit/debug make jit.debug -j 32
	ln -sf out/jit/debug/$(BIN) $(BIN).jd
	BUILDDIR=out/jit/release make jit.release -j 32
	ln -sf out/jit/release/$(BIN) $(BIN).jr
	BUILDDIR=out/interpreter/debug make interpreter.debug -j 32
	ln -sf out/interpreter/debug/$(BIN) $(BIN).id
	BUILDDIR=out/interpreter/release make interpreter.release -j 32
	ln -sf out/interpreter/release/$(BIN) $(BIN).ir

clean:
	rm -rf out
	$(shell find ./src/ -name "*.o" -exec rm {} \;)
	$(shell find ./src/ -name "*.d" -exec rm {} \;)
	$(shell find ./third_party/yarr/ -name "*.o" -exec rm {} \;)
	$(shell find ./third_party/yarr/ -name "*.d" -exec rm {} \;)
	$(shell find ./third_party/esprima_cpp/ -name "*.o" -exec rm {} \;)
	$(shell find ./third_party/esprima_cpp/ -name "*.d" -exec rm {} \;)
	$(shell find ./third_party/nanojit/ -name "*.o" -exec rm {} \;)
	$(shell find ./third_party/nanojit/ -name "*.d" -exec rm {} \;)

# Targets : miscellaneous

strip:
	strip $(BIN)

asm:
	objdump -d        $(BIN) | c++filt > $(BIN).asm
	readelf -a --wide $(BIN) | c++filt > $(BIN).elf
	vi -O $(BIN).asm $(BIN).elf

# Targets : tests

check-jit:
	make jit.release -j8
	make run-sunspider
	make interpreter.release -j8
	make run-sunspider
	make jit.debug -j8
	./run-Sunspider-jit.sh -rcf > compiledFunctions.txt
	vimdiff compiledFunctions.txt originalCompiledFunctions.txt

tidy:
	./tools/check-webkit-style `find src/ -name "*.cpp" -o -name "*.h"`> error_report 2>& 1

# Targets : benchmarks

run-sunspider:
	cd test/SunSpider/; \
	./sunspider --shell=../../escargot --suite=sunspider-1.0.2

run-octane:
	cd test/octane/; \
	../../escargot run_escargot_test.js

run-test262:
	cd test/test262/; \
	python tools/packaging/test262.py --command ../../escargot $(OPT)

.PHONY: clean
