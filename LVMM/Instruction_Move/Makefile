LLVM_PATH=/usr/local/llvm
LIBCLC_PATH=/usr/local/libclc
CXX=clang++
CXXFLAG := $(shell $(LLVM_PATH)/bin/llvm-config --cxxflags)
CXXFLAG := $(CXXFLAG) -g
CXXFLAG := $(CXXFLAG) -I $(shell $(LLVM_PATH)/bin/llvm-config --includedir)
CXXFLAG := $(CXXFLAG) -D LLVM_PATH=$(LLVM_PATH)
CXXFLAG := $(CXXFLAG) -D LIBCLC_PATH=$(LIBCLC_PATH)
CXXFLAG := $(CXXFLAG) -L $(shell $(LLVM_PATH)/bin/llvm-config --libdir)

LDFLAG := $(shell $(LLVM_PATH)/bin/llvm-config --ldflags --libs mcjit x86 irreader --system-libs)

all: compiler

compiler: compiler.cpp
	@$(CXX) $< $(CXXFLAG) -o $@ $(LDFLAG)
	@echo GENERATE INSTRUCTION MOVER

clean:
	rm -vf compiler *.bc *.nvptx.s *.ll
