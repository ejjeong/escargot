MAKECMDGOALS=escargot
MODE=
HOST=
ARCH=x64

NPROCS:=1
OS:=$(shell uname -s)

ifeq ($(OS),Linux)
  NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
endif
ifeq ($(OS),Darwin) # Assume Mac OS X
  NPROCS:=$(shell system_profiler | awk '/Number Of CPUs/{print $4}{next;}')
endif

ifeq ($(MODE),)
	MODE=debug
endif

ifeq ($(HOST),)
	HOST=linux
endif

$(info mode... $(MODE))
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
CXXFLAGS += -Ithird_party/slre/
CXXFLAGS += -Ithird_party/mozjs/build/dist/include

#LDFLAGS += -Lthird_party/mozjs/build/dist/lib -lmozjs-24 -lz -ldl -Wl,-rpath,'$$ORIGIN/third_party/mozjs/build/dist/lib/'
LDFLAGS += -lpthread -lz -ldl
LDFLAGS += -Wl,--gc-sections

ifeq ($(ARCH), x64)
	CXXFLAGS += -DESCARGOT_64=1
else ifeq ($(ARCH), x86)
	CXXFLAGS += -DESCARGOT_32=1
endif

ifeq ($(MODE), debug)
	CXXFLAGS += -O0 -g3 -frounding-math -fsignaling-nans -fno-omit-frame-pointer -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-invalid-offsetof
	GCLIBS = third_party/bdwgc/out/debug/.libs/libgc.a #third_party/bdwgc/out/debug/.libs/libgccpp.a
	MOZJSLIBS = third_party/mozjs/build/libjs_static.a
else ifeq ($(MODE), release)
	CXXFLAGS += -O3 -g3 -DNDEBUG -fomit-frame-pointer -frounding-math -fsignaling-nans
	GCLIBS = third_party/bdwgc/out/release/.libs/libgc.a #third_party/bdwgc/out/release/.libs/libgccpp.a
	MOZJSLIBS = third_party/mozjs/build/libjs_static.a
else
	$(error mode error)
endif

THIRD_PARTY_LIBS= $(GCLIBS) $(MOZJSLIBS)

SRC=
SRC += $(foreach dir, ./src , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/ast , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/shell , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/parser , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/vm , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/runtime , $(wildcard $(dir)/*.cpp))
SRC_C = $(foreach dir, ./third_party/slre , $(wildcard $(dir)/*.c))

ifeq ($(HOST), linux)
endif

OBJS :=  $(SRC:%.cpp= %.o)
OBJS +=  $(SRC_C:%.c= %.o)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

$(MAKECMDGOALS): $(OBJS) $(THIRD_PARTY_LIBS)
	$(CXX) -o $(MAKECMDGOALS) $(OBJS) $(THIRD_PARTY_LIBS) $(LDFLAGS)
	cp third_party/mozjs/build/shell/js24 ./mozjs

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $*.cpp -o $*.o
	$(CXX) -MM $(CXXFLAGS) -MT $*.o $*.cpp > $*.d

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	$(CC) -MM $(CFLAGS) -MT $*.o $*.c > $*.d

clean:
	$(shell find ./src/ -name "*.o" -exec rm {} \;)
	$(shell find ./src/ -name "*.d" -exec rm {} \;)

strip: $(MAKECMDGOALS)
	strip $<

run-sunspider:
	cp mozjs test/SunSpider/; \
	cd test/SunSpider/; \
	./sunspider --shell=../../escargot --suite=sunspider-1.0.2

.PHONY: $(MAKECMDGOALS) clean
.DEFAULT_GOAL := escargot
