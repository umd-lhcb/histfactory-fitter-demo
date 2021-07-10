// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Sat Jul 10, 2021 at 03:14 AM +0200

#include <string>

#include <fmt/core.h>
#include <cxxopts.hpp>

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
    ("flag1", "first bool param", cxxopts::value<bool>()
     ->implicit_value("true"))
    ("flag2", "second bool param", cxxopts::value<bool>()
     ->implicit_value("false"))
    ("int1", "first int param", cxxopts::value<int>()->default_value("42"))
    ("int2", "second int param", cxxopts::value<int>()
     ->default_value("314"))
    ("m,mode", "general mode", cxxopts::value<string>()->default_value("m1"))
    ;
  // clang-format on

  auto parsed_args = argparse.parse(argc, argv);

  if (parsed_args.count("help")) {
    fmt::print("{}\n", argparse.help());
    exit(0);
  }

  auto int1 = parsed_args["int1"].as<int>();
  auto int2 = parsed_args["int2"].as<int>();

  fmt::print("First int param : {}\n", int1);
  fmt::print("Second int param : {}\n", int2);
  fmt::print("The sum of the two is: {}\n", SUM(int1, int2));
}

/////////////
// Helpers //
/////////////

template <typename T>
T SUM(T n1, T n2) {
  return n1 + n2;
}
