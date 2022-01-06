# Author: Yipeng Sun
# Last Change: Thu Jan 06, 2022 at 05:23 AM +0100

VPATH := include:src:gen
HEADERS := $(shell find ./include -name "*.h")

# Compiler settings
COMPILER	:=	$(shell root-config --cxx)
CXXFLAGS	:=	$(shell root-config --cflags)
LINKFLAGS	:=	$(shell root-config --libs)
ADDCXXFLAGS	:=	-O2 -march=native -mtune=native -Iinclude
ADDLINKFLAGS	:=	-lRooFitCore -lRooFit -lRooStats -lHistFactory -lyaml-cpp

HistFactDstTauDemo: $(HEADERS)
CmdArgDemo: cmd.h

.PHONY: clean fit args

clean:
	@rm -rf ./gen/*

fit: inputs/* histfact_demo
	histfact_demo -i inputs -o gen 2>&1 | tee ./logs/fit_$$(date +%y_%m_%d_%H%M%S).log

args: cmd_demo
	cmd_demo --help
	cmd_demo --int1 233
	cmd_demo -m both_false --flag1 --flag2=false
	cmd_demo -f "random_stuff.root"
	cmd_demo -m all_zero

load-histo: histo_loader_demo
	histo_loader_demo -i ./inputs

%: %.cpp flake.nix include/*.h
	$(COMPILER) $(CXXFLAGS) $(ADDCXXFLAGS) -o gen/$@ $< $(LINKFLAGS) $(ADDLINKFLAGS)
