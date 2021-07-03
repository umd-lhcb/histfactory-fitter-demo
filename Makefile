# Author: Yipeng Sun
# Last Change: Sun Jul 04, 2021 at 01:12 AM +0200

VPATH := src:gen

# Compiler settings
COMPILER	:=	$(shell root-config --cxx)
CXXFLAGS	:=	$(shell root-config --cflags)
LINKFLAGS	:=	$(shell root-config --libs)
ADDCXXFLAGS	:=	-g -O2
ADDLINKFLAGS	:=	-lTreePlayer -lMinuit -lFoam

HistFactDstTauDemo:

%: %.cpp
	$(COMPILER) $(CXXFLAGS) $(ADDCXXFLAGS) -o gen/$@ $< $(LINKFLAGS) $(ADDLINKFLAGS)
