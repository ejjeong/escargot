BUILDDIR=./build
HOST=linux

BIN=escargot
SHARED_LIB=libescargot.so
STATIC_LIB=libescargot.a

#######################################################
# Environments
#######################################################

ARCH=#x86,x64
TYPE=none#interpreter
MODE=#debug,release
NPROCS:=1
OS:=$(shell uname -s)
SHELL:=/bin/bash
OUTPUT=
LTO=
ifeq ($(OS),Linux)
  NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
  SHELL:=/bin/bash
endif
ifeq ($(OS),Darwin)
  NPROCS:=$(shell sysctl -n machdep.cpu.thread_count)
  SHELL:=/opt/local/bin/bash
endif

$(info goal... $(MAKECMDGOALS))

ifneq (,$(findstring x86,$(MAKECMDGOALS)))
  ARCH=x86
else ifneq (,$(findstring x64,$(MAKECMDGOALS)))
  ARCH=x64
else ifneq (,$(findstring arm,$(MAKECMDGOALS)))
  ARCH=arm
endif

ifneq (,$(findstring interpreter,$(MAKECMDGOALS)))
  TYPE=interpreter
else ifneq (,$(findstring jit,$(MAKECMDGOALS)))
  ifeq (,$(findstring check-jit,$(MAKECMDGOALS)))
    TYPE=jit
  endif
endif

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
  MODE=debug
else ifneq (,$(findstring release,$(MAKECMDGOALS)))
  MODE=release
endif

ifneq (,$(findstring tizen_wearable_arm,$(MAKECMDGOALS)))
  HOST=tizen_2.3.1_wearable
  VERSION=2.3.1
else ifneq (,$(findstring tizen3_wearable_arm,$(MAKECMDGOALS)))
  HOST=tizen_3.0_wearable
  VERSION=3.0
else ifneq (,$(findstring tizen_mobile_arm,$(MAKECMDGOALS)))
  HOST=tizen_2.3.1_mobile
  VERSION=2.3.1
else ifneq (,$(findstring tizen3_mobile_arm,$(MAKECMDGOALS)))
  HOST=tizen_3.0_mobile
  VERSION=3.0
else ifneq (,$(findstring tizen_wearable_emulator,$(MAKECMDGOALS)))
  HOST=tizen_2.3.1_wearable
  VERSION=2.3.1
  ARCH=i386
else ifneq (,$(findstring tizen3_wearable_emulator,$(MAKECMDGOALS)))
  HOST=tizen_3.0_wearable
  VERSION=3.0
  ARCH=i386
endif

ifneq (,$(findstring tizen,$(HOST)))
  LTO=1
endif

ifneq (,$(findstring shared,$(MAKECMDGOALS)))
  OUTPUT=shared_lib
else ifneq (,$(findstring static,$(MAKECMDGOALS)))
  OUTPUT=static_lib
else
  OUTPUT=bin
endif

OUTDIR=out/$(HOST)/$(ARCH)/$(TYPE)/$(MODE)
ESCARGOT_ROOT=.

$(info host... $(HOST))
$(info arch... $(ARCH))
$(info type... $(TYPE))
$(info mode... $(MODE))
$(info output... $(OUTPUT))
$(info build dir... $(OUTDIR))

#######################################################
# Build flags
#######################################################

include $(BUILDDIR)/Toolchain.mk
include $(BUILDDIR)/Flags.mk

# common flags
CXXFLAGS += $(ESCARGOT_CXXFLAGS_COMMON)
LDFLAGS += $(ESCARGOT_LDFLAGS_COMMON)

# HOST flags
ifeq ($(HOST), linux)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_LINUX)
else ifneq (,$(findstring tizen,$(HOST)))
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_TIZEN)
endif

# ARCH flags
ifeq ($(ARCH), x64)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_X64)
  LDFLAGS += $(ESCARGOT_LDFLAGS_X64)
else ifeq ($(ARCH), x86)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_X86)
  LDFLAGS += $(ESCARGOT_LDFLAGS_X86)
else ifeq ($(ARCH), i386)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_X86)
  LDFLAGS += $(ESCARGOT_LDFLAGS_X86)
else ifeq ($(ARCH), arm)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_ARM)
  LDFLAGS += $(ESCARGOT_LDFLAGS_ARM)
endif

# TYPE flags
ifeq ($(TYPE), interpreter)
  CXXFLAGS+=$(ESCARGOT_CXXFLAGS_INTERPRETER)
else ifeq ($(TYPE), jit)
  include $(BUILDDIR)/JIT.mk
  CXXFLAGS+=$(ESCARGOT_CXXFLAGS_JIT)
endif

# MODE flags
ifeq ($(MODE), debug)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_DEBUG)
else ifeq ($(MODE), release)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_RELEASE)
endif

# OUTPUT flags
ifeq ($(OUTPUT), bin)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_BIN)
  LDFLAGS += $(ESCARGOT_LDFLAGS_BIN)
else ifeq ($(OUTPUT), shared_lib)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_SHAREDLIB)
  LDFLAGS += $(ESCARGOT_LDFLAGS_SHAREDLIB)
else ifeq ($(OUTPUT), static_lib)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_STATICLIB)
  LDFLAGS += $(ESCARGOT_LDFLAGS_STATICLIB)
endif

CXXFLAGS += $(ESCARGOT_CXXFLAGS_THIRD_PARTY)
LDFLAGS += $(ESCARGOT_LDFLAGS_THIRD_PARTY)

ifeq ($(TC), 1)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_TC)
endif

ifeq ($(LTO), 1)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_LTO)
  LDFLAGS += $(ESCARGOT_LDFLAGS_LTO)
  ifeq ($(OUTPUT), bin)
    LDFLAGS += $(CXXFLAGS)
  endif
endif

#######################################################
# SRCS & OBJS
#######################################################

SRC=
SRC += $(foreach dir, src , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/ast , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/bytecode , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/jit , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/parser , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/runtime , $(wildcard $(dir)/*.cpp))
ifeq ($(OUTPUT), bin)
  SRC += $(foreach dir, src/shell , $(wildcard $(dir)/*.cpp))
endif
SRC += $(foreach dir, src/vm , $(wildcard $(dir)/*.cpp))

SRC += $(foreach dir, third_party/yarr, $(wildcard $(dir)/*.cpp))

SRC_CC =
SRC_CC += $(foreach dir, third_party/double_conversion , $(wildcard $(dir)/*.cc))

OBJS := $(SRC:%.cpp= $(OUTDIR)/%.o)
OBJS += $(SRC_CC:%.cc= $(OUTDIR)/%.o)
OBJS += $(SRC_C:%.c= $(OUTDIR)/%.o)

ifeq ($(OUTPUT), bin)
  OBJS_GC=third_party/bdwgc/out/$(HOST)/$(ARCH)/$(MODE).static/.libs/libgc.a
else
  OBJS_GC=third_party/bdwgc/out/$(HOST)/$(ARCH)/$(MODE).shared/.libs/libgc.a
endif
OBJS_THIRD_PARTY = $(OBJS_GC)

#######################################################
# Targets
#######################################################

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

.DEFAULT_GOAL:=x64.interpreter.debug

x86.jit.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x86.jit.release: $(OUTDIR)/$(BIN)
	cp -f $< .
x86.interpreter.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x86.interpreter.release: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.jit.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.jit.release: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.interpreter.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.interpreter.release: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.interpreter.debug.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
x64.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
x64.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
x64.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
#tizen_mobile_arm.jit.debug: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen_mobile_arm.jit.release: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen_mobile_arm.interpreter.debug: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen_mobile_arm.interpreter.release: $(OUTDIR)/$(BIN)
#	cp -f $< .
tizen_mobile_arm.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
#tizen_mobile_arm.interpreter.debug: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen_mobile_arm.interpreter.release: $(OUTDIR)/$(BIN)
#	cp -f $< .
tizen_wearable_arm.interpreter.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_wearable_arm.interpreter.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_wearable_arm.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
tizen_wearable_arm.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen_wearable_arm.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen_wearable_emulator.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
tizen_wearable_emulator.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen_wearable_emulator.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .

##### TIZEN3 #####
#tizen3_mobile_arm.jit.debug: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen3_mobile_arm.jit.release: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen3_mobile_arm.interpreter.debug: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen3_mobile_arm.interpreter.release: $(OUTDIR)/$(BIN)
#	cp -f $< .
tizen3_mobile_arm.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
#tizen3_mobile_arm.interpreter.debug: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen3_mobile_arm.interpreter.release: $(OUTDIR)/$(BIN)
#	cp -f $< .
tizen3_wearable_arm.interpreter.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_wearable_arm.interpreter.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_wearable_arm.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
tizen3_wearable_arm.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen3_wearable_arm.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen3_wearable_emulator.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
tizen3_wearable_emulator.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen3_wearable_emulator.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .

DEPENDENCY_MAKEFILE = Makefile $(BUILDDIR)/Toolchain.mk $(BUILDDIR)/Flags.mk

$(OUTDIR)/$(BIN): $(OBJS) $(OBJS_THIRD_PARTY) $(DEPENDENCY_MAKEFILE)
	@echo "[LINK] $@"
	@$(CXX) -o $@ $(OBJS) $(OBJS_THIRD_PARTY) $(LDFLAGS)

$(OUTDIR)/$(SHARED_LIB): $(OBJS) $(OBJS_THIRD_PARTY) $(DEPENDENCY_MAKEFILE)
	@echo "[LINK] $@"
	$(CXX) -shared -Wl,-soname,$(SHARED_LIB) -o $@ $(OBJS) $(OBJS_THIRD_PARTY) $(LDFLAGS)

$(OUTDIR)/$(STATIC_LIB): $(OBJS) $(DEPENDENCY_MAKEFILE)
	@echo "[LINK] $@"
	$(AR) rc $@ $(ARFLAGS) $(OBJS)

$(OUTDIR)/%.o: %.cpp $(DEPENDENCY_MAKEFILE)
	@echo "[CXX] $@"
	@mkdir -p $(dir $@)
	@$(CXX) -c $(CXXFLAGS) $< -o $@
	@$(CXX) -MM $(CXXFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

$(OUTDIR)/%.o: %.cc $(DEPENDENCY_MAKEFILE)
	@echo "[CXX] $@"
	@mkdir -p $(dir $@)
	@$(CXX) -c $(CXXFLAGS) $< -o $@
	@$(CXX) -MM $(CXXFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

full:
	make x64.jit.debug -j$(NPROCS)
	ln -sf out/x64/jit/debug/$(BIN) $(BIN).x64.jd
	make x64.jit.release -j$(NPROCS)
	ln -sf out/x64/jit/release/$(BIN) $(BIN).x64.jr
	make x64.interpreter.debug -j$(NPROCS)
	ln -sf out/x64/interpreter/debug/$(BIN) $(BIN).x64.id
	make x64.interpreter.release -j$(NPROCS)
	ln -sf out/x64/interpreter/release/$(BIN) $(BIN).x64.ir
	make x86.jit.debug -j$(NPROCS)
	ln -sf out/x86/jit/debug/$(BIN) $(BIN).x86.jd
	make x86.jit.release -j$(NPROCS)
	ln -sf out/x86/jit/release/$(BIN) $(BIN).x86.jr
	make x86.interpreter.debug -j$(NPROCS)
	ln -sf out/x86/interpreter/debug/$(BIN) $(BIN).x86.id
	make x86.interpreter.release -j$(NPROCS)
	ln -sf out/x86/interpreter/release/$(BIN) $(BIN).x86.ir

clean:
	rm -rf out

# Targets : Regression tests

check-jit-64:
	make x64.jit.release -j$(NPROCS)
	make run-sunspider
	make run-octane
	make x64.jit.debug -j$(NPROCS)
	./run-Sunspider.sh -rcf > compiledFunctions.txt
	vimdiff compiledFunctions.txt originalCompiledFunctions.txt
	./run-Sunspider.sh -rof > osrExitedFunctions.txt
	vimdiff osrExitedFunctions.txt originalOSRExitedFunctions.txt

check-jit-32:
	make x86.jit.release -j$(NPROCS)
	make run-sunspider
	make run-octane
	make x86.jit.debug -j$(NPROCS)
	./run-Sunspider.sh -rcf > compiledFunctions.txt
	vimdiff compiledFunctions.txt originalCompiledFunctions.txt
	./run-Sunspider.sh -rof > osrExitedFunctions.txt
	vimdiff osrExitedFunctions.txt originalOSRExitedFunctions.txt

check-jit-arm:
	./setup_measure_for_android.sh build-jit
	#./measure_for_android.sh escargot32.jit time > time.arm32.txt
	adb shell "cd /data/local/tmp ; ./run-Sunspider.sh /data/local/tmp/arm32/escargot/jit/escargot.debug -rcf > compiledFunctions.arm32.txt"
	adb shell "cd /data/local/tmp ; ./run-Sunspider.sh /data/local/tmp/arm32/escargot/jit/escargot.debug -rof > osrExitedFunctions.arm32.txt"
	#adb pull /data/local/tmp/time.arm32.txt .
	adb pull /data/local/tmp/compiledFunctions.arm32.txt .
	adb pull /data/local/tmp/osrExitedFunctions.arm32.txt .

check:
	make x64.interpreter.release -j$(NPROCS)
	make run-sunspider | tee out/sunspider_result
	make run-octane | tee out/octane_result
	make x64.interpreter.debug -j$(NPROCS)
	make run-sunspider
	make check-jit-64
	cat out/sunspider_result
	cat out/octane_result
	./regression_test262
	make tidy

check-lirasm:
	make x64.jit.debug -j8; \
	cd test/lirasm; \
	./testlirc.sh ../../escargot; \
	cd ../..; \
	make x86.jit.debug -j8; \
	cd test/lirasm; \
	./testlirc.sh ../../escargot;

check-lirasm-android:
	adb shell su -e mkdir -p /data/local/tmp/lirasm/tests
	adb push ./android/libs/armeabi-v7a/escargot /data/local/tmp/lirasm/escargot
	adb push test/lirasm/tests /data/local/tmp/lirasm/tests/
	cd test/lirasm/; ./testlirc_android.sh

tidy:
	./tools/check-webkit-style `find src/ -name "*.cpp" -o -name "*.h"`> error_report 2>& 1

# Targets : benchmarks

run-sunspider:
	cd test/SunSpider/; \
	./sunspider --shell=../../escargot --suite=sunspider-1.0.2

run-octane:
	cd test/octane/; \
	../../escargot run.js

run-test262:
	ln -sf excludelist.orig.xml test/test262/test/config/excludelist.xml
	cd test/test262/; \
	python tools/packaging/test262.py --command ../../escargot $(OPT) --full-summary

run-test262-wearable:
	ln -sf excludelist.subset.xml test/test262/test/config/excludelist.xml
	cd test/test262/; \
	python tools/packaging/test262.py --command ../../escargot $(OPT) --summary | sed 's/RELEASE_ASSERT_NOT_REACHED.*//g' | tee test262log.wearable.gen.txt; \
	diff test262log.wearable.orig.txt test262log.wearable.gen.txt

run-spidermonkey:
	cd test/SpiderMonkey; \
	./jstests.py -s --xul-info=x86_64-gcc3:Linux:false ../../escargot --failure-file=mozilla.x64.interpreter.release.escargot.gen.txt -p "$(OPT)"; \
	diff mozilla.x64.interpreter.release.escargot.orig.txt mozilla.x64.interpreter.release.escargot.gen.txt

run-spidermonkey-for-32bit:
	cd test/SpiderMonkey; \
	./jstests.py -s --xul-info=x86-gcc3:Linux:false ../../escargot --failure-file=mozilla.x86.interpreter.release.escargot.gen.txt -p "$(OPT)"; \
	diff mozilla.x86.interpreter.release.escargot.orig.txt mozilla.x64.interpreter.release.escargot.gen.txt

run-jsc-mozilla:
	cd test/JavaScriptCore/mozilla/; \
        perl jsDriver.pl -e escargot -s ../../../escargot

run-jetstream:
	cd test/JetStream-standalone-escargot/JetStream-1.1/; \
        ./run.sh ../../../escargot; \
		python parsingResults.py jetstream-result-raw.res;

run-chakracore:
	cd test/chakracore/; \
	./run.sh ../../escargot $(OPT) | tee chakracorelog.gen.txt; \
	diff chakracorelog.orig.txt chakracorelog.gen.txt

run-v8-test:
	./test/v8/tool/run-tests.py --quickcheck --no-presubmit --no-variants --arch-and-mode=x64.release --escargot --report -p verbose --no-sorting mjsunit | tee test/v8/mjsunit.gen.txt; \
	diff test/v8/mjsunit.orig.txt test/v8/mjsunit.gen.txt

run-v8-test-for-32bit:
	./test/v8/tool/run-tests.py --quickcheck --no-presubmit --no-variants --arch-and-mode=x32.release --escargot --report -p verbose --no-sorting mjsunit | tee test/v8/mjsunit.gen.txt; \
	diff test/v8/mjsunit.orig.txt test/v8/mjsunit.gen.txt


.PHONY: clean
