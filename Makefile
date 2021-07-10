# Author: Yipeng Sun
# Last Change: Sat Jul 10, 2021 at 02:52 AM +0200

VPATH := src:gen

# Compiler settings
COMPILER	:=	$(shell root-config --cxx)
CXXFLAGS	:=	$(shell root-config --cflags)
LINKFLAGS	:=	$(shell root-config --libs)
ADDCXXFLAGS	:=	-O2
ADDLINKFLAGS	:=	-lRooFitCore -lRooFit -lRooStats -lHistFactory -lfmt

HistFactDstTauDemo:
CmdArgDemo:

.PHONY: clean fit args

clean:
	@rm -rf ./gen/*
	@rm -rf ./results

fit: HistFactDstTauDemo
	@HistFactDstTauDemo ./inputs ./gen 2>&1 | tee ./logs/fit_$$(date +%y_%m_%d_%H%M%S).log

args: CmdArgDemo
	@CmdArgDemo --help
	@CmdArgDemo --int1 233

%: %.cpp flake.nix
	$(COMPILER) $(CXXFLAGS) $(ADDCXXFLAGS) -o gen/$@ $< $(LINKFLAGS) $(ADDLINKFLAGS)
