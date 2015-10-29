####################################
# ARCH-dependent settings
####################################
ifeq ($(ARCH), x64)
	TARGET_CPU=x86_64
	CXXFLAGS += -DAVMPLUS_64BIT
	CXXFLAGS += -DAVMPLUS_AMD64
	CXXFLAGS += -DAVMPLUS_X64
	CXXFLAGS += #if defined(_M_AMD64) || defined(_M_X64)
else
	$(error Target architecture is not set properly);
endif

####################################
# target-dependent settings
####################################
jit.debug: CXXFLAGS += -DDEBUG
jit.debug: CXXFLAGS += -D_DEBUG
jit.debug: CXXFLAGS += -DNJ_VERBOSE

####################################
# Other features
####################################
CXXFLAGS += -DESCARGOT
CXXFLAGS += -Ithird_party/nanojit/
CXXFLAGS += -Isrc/jit/
CXXFLAGS += -DFEATURE_NANOJIT

#CXXFLAGS += -DAVMPLUS_VERBOSE
CXXFLAGS += -Wno-error=narrowing

####################################
# Makefile flags
####################################
curdir=third_party/nanojit
include $(curdir)/manifest.mk
SRC_NANOJIT = $(avmplus_CXXSRCS)
SRC_NANOJIT += $(curdir)/EscargotBridge.cpp
