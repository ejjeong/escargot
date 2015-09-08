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
# MODE-dependent settings
####################################
ifeq ($(MODE), debug)
	CXXFLAGS += -D_DEBUG
endif

####################################
# Other features
####################################
CXXFLAGS += -DESCARGOT
CXXFLAGS += -Ithird_party/nanojit/
CXXFLAGS += -DFEATURE_NANOJIT

#CXXFLAGS += -DAVMPLUS_VERBOSE

####################################
# Makefile flags
####################################
curdir=third_party/nanojit
include $(curdir)/manifest.mk
SRC_NANOJIT = $(avmplus_CXXSRCS)
