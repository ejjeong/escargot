MAKECMDGOALS=escargot
MODE=
HOST=
ARCH=x64

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

CXXFLAGS += -Isrc/
#add third_party
CXXFLAGS += -Ithird_party/rapidjson/include/
CXXFLAGS += -Ithird_party/bdwgc/include/
LDFLAGS += third_party/bdwgc/.libs/libgc.a -lpthread

ifeq ($(MODE), debug)
	CXXFLAGS += -O0 -g3 -fno-omit-frame-pointer
else ifeq ($(MODE), release)
	CXXFLAGS += -O3 -g0 -DNDEBUG
else
	$(error mode error)
endif

SRC=
SRC += $(foreach dir, ./src , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/shell , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, ./src/parser , $(wildcard $(dir)/*.cpp))

ifeq ($(HOST), linux)
endif

OBJS :=  $(SRC:%.cpp= %.o)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

$(MAKECMDGOALS): $(OBJS)
	$(CXX) -o $(MAKECMDGOALS) $(OBJS) $(OBJS_C) $(LDFLAGS)
	cp third_party/mozjs/prebuilt/$(HOST)/$(ARCH)/mozjs ./mozjs

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $*.cpp -o $*.o
	$(CXX) -MM $(CXXFLAGS) -MT $*.o $*.cpp > $*.d

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	$(CC) -MM $(CFLAGS) -MT $*.o $*.c > $*.d

clean:
	$(shell find ./src/ -name "*.o" -exec rm {} \;)
	$(shell find ./src/ -name "*.d" -exec rm {} \;)

.PHONY: $(MAKECMDGOALS) clean
.DEFAULT_GOAL := escargot