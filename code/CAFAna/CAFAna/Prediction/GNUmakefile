include SoftRelTools/arch_spec_root.mk

LIB_TYPE    := shared
LIB         := lib$(PACKAGE)Prediction
LIBCXXFILES := $(wildcard *.cxx)
# Need headers only for genie syst enum
override CPPFLAGS += -I$(NUTOOLS_INC) -I$(GENIE_INC)/GENIE/

override CPPFLAGS += -I../FluxFit/DUNEPrismTools/src/flux_tools/ -I../FluxFit/eigen -I../FluxFit/Prob3++.20121225 -l../FluxFit/Prob3++.20121225/libThreeProb_2.10.a
override CPPFLAGS += `root-config --cflags --glibs`

LIBLINK    := -L$(SRT_PRIVATE_CONTEXT)/lib/$(SRT_SUBDIR) -L$(SRT_PUBLIC_CONTEXT)/lib/$(SRT_SUBDIR) -l$(PACKAGE) -l$(PACKAGE)Prediction

include SoftRelTools/standard.mk

override LIBLIBS += $(LOADLIBES)  -L$(SRT_PRIVATE_CONTEXT)/lib/$(SRT_SUBDIR) -L$(SRT_PUBLIC_CONTEXT)/lib/$(SRT_SUBDIR)

# Suppress the "variable tracking size limit exceeded" warning
override CPPFLAGS += -fno-var-tracking-assignments
