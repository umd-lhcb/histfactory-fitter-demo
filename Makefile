# Author: Yipeng Sun
# Last Change: Wed Jun 07, 2023 at 12:14 AM +0800

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

.PHONY: all clean fit fit-nobb fit-noshapes args load-histo

all: clean args load-histo fit fit-nobb fit-noshapes

clean:
	@rm -rf ./gen/*

fit: inputs/* histfact_demo
	histfact_demo -i inputs -o gen --useMinos=false 2>&1 | tee ./logs/fit_$$(date +%y_%m_%d_%H%M%S).log

fit-nobb: inputs/* histfact_demo
	histfact_demo -i inputs -o gen --useMinos=false --bbOn3D=false 2>&1 | tee ./logs/fit_$$(date +%y_%m_%d_%H%M%S).log

fit-nobb-load: inputs/* histfact_demo
	histfact_demo -i inputs -o gen \
		--useMinos=false --bbOn3D=false \
		--loadPrefit=./gen/fit_output/saved_result.root \
		2>&1 | tee ./logs/fit_$$(date +%y_%m_%d_%H%M%S).log

fit-noshapes: inputs/* histfact_demo
	histfact_demo -i inputs -o gen --useMinos=false --useMuShapeUncerts=false --useTauShapeUncerts=false --useDststShapeUncerts=false --bbOn3D=false 2>&1 | tee ./logs/fit_$$(date +%y_%m_%d_%H%M%S).log

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
	@cachix push histfactory-basis dev-profile

%: %.cpp flake.nix
	$(COMPILER) $(CXXFLAGS) $(ADDCXXFLAGS) -o gen/$@ $< $(LINKFLAGS) $(ADDLINKFLAGS)
