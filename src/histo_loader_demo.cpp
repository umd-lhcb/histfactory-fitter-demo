// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Thu Jan 06, 2022 at 03:03 AM +0100

#include <iostream>
#include <string>

#include <cxxopts.hpp>

#include "cmd.h"
#include "loader.h"

using namespace std;

int main(int argc, char** argv) {
  // Define command line parser
  cxxopts::Options argOpts("histo_loader_demo",
                           "a demo for loading histograms w/ a YAML spec");

  // clang-format off
  argOpts.add_options()
    ("h,help", "print usage")
    ("i,inputFolder", "input folder containing ntuples and a spec.yml.")
    ;
  // clang-format on

  auto parsedArgs = argOpts.parse(argc, argv);

  // Define default values for modes
  auto parsedArgsProxy = ArgProxy(parsedArgs, "default"s);

  if (parsedArgs.count("help")) {
    cout << argOpts.help() << endl;
    exit(0);
  }

  auto histoLoader =
      HistoLoader(parsedArgsProxy.get<string>("inputFolder"), true);
  histoLoader.load();
}
