# Author: Yipeng Sun
# Last Change: Tue Jan 04, 2022 at 04:21 AM +0100

VPATH := include:src:gen
HEADERS := $(shell find ./include -name "*.h")

# Compiler settings
COMPILER	:=	$(shell root-config --cxx)
CXXFLAGS	:=	$(shell root-config --cflags)
LINKFLAGS	:=	$(shell root-config --libs)
ADDCXXFLAGS	:=	-O2 -march=native -mtune=native -Iinclude
ADDLINKFLAGS	:=	-lRooFitCore -lRooFit -lRooStats -lHistFactory

HistFactDstTauDemo: $(HEADERS)
CmdArgDemo: cmd.h

.PHONY: clean fit args

clean:
	@rm -rf ./gen/*

fit: inputs/DemoHistos.root histfact_demo
	@histfact_demo -i $< -o gen 2>&1 | tee ./logs/fit_$$(date +%y_%m_%d_%H%M%S).log

args: cmd_demo
	@cmd_demo --help
	@cmd_demo --int1 233
	@cmd_demo -m both_false --flag1 --flag2=false
	@cmd_demo -f "random_stuff.root"
	@cmd_demo -m all_zero

%: %.cpp flake.nix Makefile
	$(COMPILER) $(CXXFLAGS) $(ADDCXXFLAGS) -o gen/$@ $< $(LINKFLAGS) $(ADDLINKFLAGS)
