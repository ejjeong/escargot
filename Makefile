MAKECMDGOALS=escargot
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

CXXFLAGS += -fno-rtti -fno-math-errno -Isrc/
CXXFLAGS += -fdata-sections -ffunction-sections
#add third_party
CXXFLAGS += -Ithird_party/rapidjson/include/
CXXFLAGS += -Ithird_party/bdwgc/include/
CXXFLAGS += -Ithird_party/netlib/

CXXFLAGS += -Ithird_party/yarr/
SRC_YARR += third_party/yarr/OSAllocatorPosix.cpp
SRC_YARR += third_party/yarr/PageBlock.cpp
SRC_YARR += third_party/yarr/YarrCanonicalizeUCS2.cpp
SRC_YARR += third_party/yarr/YarrInterpreter.cpp
SRC_YARR += third_party/yarr/YarrPattern.cpp
SRC_YARR += third_party/yarr/YarrSyntaxChecker.cpp

SRC_ESPRIMA_CPP += $(foreach dir, ./third_party/esprima_cpp , $(wildcard $(dir)/*.cpp))
CXXFLAGS += -Ithird_party/esprima_cpp/

include third_party/nanojit/Build.mk

LDFLAGS += -lpthread 
# -ltcmalloc_minimal
LDFLAGS += -Wl,--gc-sections

ifeq ($(ARCH), x64)
	CXXFLAGS += -DESCARGOT_64=1
else ifeq ($(ARCH), x86)
	CXXFLAGS += -DESCARGOT_32=1
endif

CXXFLAGS_DEBUG = -O0 -g3 -frounding-math -fsignaling-nans -fno-omit-frame-pointer -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-invalid-offsetof -Wno-sign-compare
GCLIBS_DEBUG = third_party/bdwgc/out/debug/.libs/libgc.a #third_party/bdwgc/out/debug/.libs/libgccpp.a
CXXFLAGS_RELEASE = -O3 -g3 -DNDEBUG -fomit-frame-pointer -frounding-math -fsignaling-nans -funroll-loops
GCLIBS_RELEASE = third_party/bdwgc/out/release/.libs/libgc.a #third_party/bdwgc/out/release/.libs/libgccpp.a

interpreter.debug: CXXFLAGS+=$(CXXFLAGS_DEBUG)
interpreter.debug: GCLIBS=$(GCLIBS_DEBUG)

interpreter.release: CXXFLAGS+=$(CXXFLAGS_RELEASE)
interpreter.release: GCLIBS=$(GCLIBS_RELEASE)

THIRD_PARTY_LIBS= $(GCLIBS)

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

ifeq ($(HOST), linux)
endif

OBJS :=  $(SRC:%.cpp= %.o)
OBJS +=  $(SRC_C:%.c= %.o)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

$(MAKECMDGOALS): $(OBJS) $(THIRD_PARTY_LIBS)
	$(CXX) -o $(MAKECMDGOALS) $(OBJS) $(THIRD_PARTY_LIBS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $*.cpp -o $*.o
	$(CXX) -MM $(CXXFLAGS) -MT $*.o $*.cpp > $*.d

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	$(CC) -MM $(CFLAGS) -MT $*.o $*.c > $*.d

clean:
	$(shell find ./src/ -name "*.o" -exec rm {} \;)
	$(shell find ./src/ -name "*.d" -exec rm {} \;)
	$(shell find ./third_party/yarr/ -name "*.o" -exec rm {} \;)
	$(shell find ./third_party/yarr/ -name "*.d" -exec rm {} \;)
	$(shell find ./third_party/esprima_cpp/ -name "*.o" -exec rm {} \;)
	$(shell find ./third_party/esprima_cpp/ -name "*.d" -exec rm {} \;)
	$(shell find ./third_party/nanojit/ -name "*.o" -exec rm {} \;)
	$(shell find ./third_party/nanojit/ -name "*.d" -exec rm {} \;)

interpreter.debug: $(MAKECMDGOALS)

interpreter.release: $(MAKECMDGOALS)

strip: $(MAKECMDGOALS)
	strip $<

run-sunspider:
	cd test/SunSpider/; \
	./sunspider --shell=../../escargot --suite=sunspider-1.0.2

run-test262:
	cd test/test262/; \
	python tools/packaging/test262.py --command ../../escargot $(OPT)

asm: $(MAKECMDGOALS)
	objdump -d        $< | c++filt > $<.asm
	readelf -a --wide $< | c++filt > $<.elf
	vi -O $<.asm $<.elf

.PHONY: $(MAKECMDGOALS) clean
.DEFAULT_GOAL := escargot
