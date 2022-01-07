// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Fri Jan 07, 2022 at 04:31 PM +0100

#include <iostream>
#include <string>

#include <cxxopts.hpp>

#include "cmd.h"

using namespace std;

// Declarations
template <typename T>
T SUM(T, T);

int main(int argc, char** argv) {
  // Define command line parser
  cxxopts::Options argOpts("cmd_demo",
                           "a demo for parsing command line options");

  // clang-format off
  argOpts.add_options()
    ("h,help", "print usage")
    ("d,debug", "enable debugging")  // bool by default
    ("flag1", "first bool param")
    ("flag2", "second bool param", cxxopts::value<bool>()
     ->default_value("true"))
    ("int1", "first int param", cxxopts::value<int>()->default_value("42"))
    ("int2", "second int param", cxxopts::value<int>()
     ->default_value("314"))
    ("m,mode", "general mode", cxxopts::value<string>()
     ->default_value("both_true"))
    ("f,filename", "sample filename", cxxopts::value<string>()
     ->default_value("ntp1.root"))
    ;
  // clang-format on

  auto parsedArgs = argOpts.parse(argc, argv);
  auto mode       = parsedArgs["mode"].as<std::string>();

  // Define default values for modes
  auto parsedArgsProxy = ArgProxy(parsedArgs, mode);
  parsedArgsProxy.setDefault("both_true", "flag1", true);
  parsedArgsProxy.setDefault("both_true", "flag2", true);

  parsedArgsProxy.setDefault("both_false", "flag1", false);
  parsedArgsProxy.setDefault("both_false", "flag2", false);

  parsedArgsProxy.setDefault("both_zero", "int1", 0);
  parsedArgsProxy.setDefault("both_zero", "int2", 0);

  // clang-format off
  parsedArgsProxy.setDefault("all_zero", map<string, any>{
    {"flag1", false},
    {"flag2", false},
    {"int1", 0},
    {"int2", 0},
    {"filename", string("zero")}
  });
  // clang-format on

  if (parsedArgs.count("help")) {
    cout << argOpts.help() << endl;
    exit(0);
  }

  auto filename = parsedArgsProxy.get<string>("filename");
  cout << "Sample filename is: " << filename << endl;

  auto flag1Raw = parsedArgs["flag1"].as<bool>();
  auto flag2Raw = parsedArgs["flag2"].as<bool>();
  cout << "First raw flag: " << flag1Raw << ", second raw flag: " << flag2Raw
       << endl;

  auto flag1 = parsedArgsProxy.get<bool>("flag1");
  auto flag2 = parsedArgsProxy.get<bool>("flag2");
  cout << "First flag: " << flag1 << ", second flag: " << flag2 << endl;

  auto int1 = parsedArgsProxy.get<int>("int1");
  auto int2 = parsedArgsProxy.get<int>("int2");
  cout << "First int: " << int1 << ", second int: " << int2 << endl;
  cout << "The sum of the two is: " << SUM(int1, int2) << endl;
}

/////////////
// Helpers //
/////////////

template <typename T>
T SUM(T n1, T n2) {
  return n1 + n2;
}
