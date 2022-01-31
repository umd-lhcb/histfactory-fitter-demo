# Author: Yipeng Sun
# Last Change: Mon Jan 31, 2022 at 05:16 PM -0500

export PATH	:=	gen:$(PATH)

VPATH := include:src:gen
HEADERS := $(shell find ./include -name "*.h")

# Compiler settings
COMPILER	:=	$(shell root-config --cxx)
CXXFLAGS	:=	$(shell root-config --cflags)
LINKFLAGS	:=	$(shell root-config --libs)
ADDCXXFLAGS	:=	-O2 -march=native -mtune=native -Iinclude
ADDLINKFLAGS	:=	-lRooFitCore -lRooFit -lRooStats -lHistFactory -lyaml-cpp

histfact_demo: $(HEADERS)
cmd_demo: cmd.h
histo_loader_demo: loader.h

.PHONY: all clean fit args load-histo

all: clean args load-histo fit

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

cache:
	@echo "Don't forget to export CACHIX_AUTH_TOKEN=<TOKEN> first!"
	@nix develop --profile dev-profile -c echo "dev shell built."
	@cachix push histfactory-fitter-demo dev-profile

%: %.cpp flake.nix
	$(COMPILER) $(CXXFLAGS) $(ADDCXXFLAGS) -o gen/$@ $< $(LINKFLAGS) $(ADDLINKFLAGS)
