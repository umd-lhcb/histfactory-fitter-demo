# Author: Yipeng Sun
# Last Change: Sat Jul 10, 2021 at 06:17 PM +0200

VPATH := src:gen

# Compiler settings
COMPILER	:=	$(shell root-config --cxx)
CXXFLAGS	:=	$(shell root-config --cflags)
LINKFLAGS	:=	$(shell root-config --libs)
ADDCXXFLAGS	:=	-O2 -march=native -mtune=native -Iinclude
ADDLINKFLAGS	:=	-lRooFitCore -lRooFit -lRooStats -lHistFactory

HistFactDstTauDemo:
CmdArgDemo: include/cmd.h

.PHONY: clean fit args

clean:
	@rm -rf ./gen/*
	@rm -rf ./results

fit: inputs/DemoHistos.root HistFactDstTauDemo
	@HistFactDstTauDemo ./inputs ./gen 2>&1 | tee ./logs/fit_$$(date +%y_%m_%d_%H%M%S).log

args: CmdArgDemo
	@CmdArgDemo --help
	@CmdArgDemo --int1 233
	@CmdArgDemo -m both_false --flag1

%: %.cpp flake.nix Makefile
	$(COMPILER) $(CXXFLAGS) $(ADDCXXFLAGS) -o gen/$@ $< $(LINKFLAGS) $(ADDLINKFLAGS)
