# Author: Yipeng Sun
# Last Change: Sun Jul 04, 2021 at 02:42 AM +0200

export PATH := gen:$(PATH)

VPATH := src:gen

# Compiler settings
COMPILER	:=	$(shell root-config --cxx)
CXXFLAGS	:=	$(shell root-config --cflags)
LINKFLAGS	:=	$(shell root-config --libs)
ADDCXXFLAGS	:=	-O2
ADDLINKFLAGS	:=	-lRooFitCore -lRooFit -lRooStats -lHistFactory

HistFactDstTauDemo:

%: %.cpp
	$(COMPILER) $(CXXFLAGS) $(ADDCXXFLAGS) -o gen/$@ $< $(LINKFLAGS) $(ADDLINKFLAGS)

.PHONY: clean fit

clean:
	@rm -rf ./gen/*
	@rm -rf ./results

fit: HistFactDstTauDemo
	@HistFactDstTauDemo ./inputs ./gen 2>&1 | tee ./gen/fit.log
