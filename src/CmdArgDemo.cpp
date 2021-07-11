// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Sun Jul 11, 2021 at 04:20 AM +0200

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
  cxxopts::Options argparse("CmdArgDemo",
                            "a demo for parsing command line options");

  // clang-format off
  argparse.add_options()
    ("h,help", "print usage")
    ("d,debug", "enable debugging")  // bool by default
    ("flag1", "first bool param")
    ("flag2", "second bool param")
    ("int1", "first int param", cxxopts::value<int>()->default_value("42"))
    ("int2", "second int param", cxxopts::value<int>()
     ->default_value("314"))
    ("m,mode", "general mode", cxxopts::value<string>()
     ->default_value("both_true"))
    ("f,filename", "sample filename", cxxopts::value<string>()
     ->default_value("ntp1.root"))
    ;
  // clang-format on

  auto parsed_args = argparse.parse(argc, argv);
  auto mode        = parsed_args["mode"].as<std::string>();

  // Define default values for modes
  auto parsed_args_proxy = ArgProxy<bool>(parsed_args, mode);
  parsed_args_proxy.set_default("both_true", "flag1", true);
  parsed_args_proxy.set_default("both_true", "flag2", true);
  parsed_args_proxy.set_default("both_false", "flag1", false);
  parsed_args_proxy.set_default("both_false", "flag2", false);

  if (parsed_args.count("help")) {
    cout << argparse.help() << endl;
    exit(0);
  }

  auto filename = parsed_args["filename"].as<string>();
  cout << "Sample filename is: " << filename.c_str() << endl;

  auto flag1_raw = parsed_args["flag1"].as<bool>();
  auto flag2_raw = parsed_args["flag2"].as<bool>();
  cout << "First raw flag: " << flag1_raw << ", second raw flag: " << flag2_raw
       << endl;

  auto flag1 = parsed_args_proxy.get("flag1");
  auto flag2 = parsed_args_proxy.get("flag2");
  cout << "First flag: " << flag1 << ", second flag: " << flag2 << endl;

  auto int1 = parsed_args["int1"].as<int>();
  auto int2 = parsed_args["int2"].as<int>();
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
