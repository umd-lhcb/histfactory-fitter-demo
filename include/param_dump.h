// Author: Yipeng Sun
// License: BSD 2-clause
// Last Change: Mon Jan 31, 2022 at 05:06 PM -0500

#ifndef _FIT_DEMO_PARAM_DUMP_H_
#define _FIT_DEMO_PARAM_DUMP_H_

#include <fstream>
#include <string>
#include <vector>

#include <RooFitResult.h>
#include <RooRealVar.h>

#include <yaml-cpp/yaml.h>

using std::ofstream;
using std::string;
using std::vector;

vector<string> completeFitVarNames(vector<string> varNamesShort) {
  vector<string> varNamesFull{};

  for (const auto& n : varNamesShort) {
    varNamesFull.emplace_back("alpha_" + n);
  }

  return varNamesFull;
}

void dumpParams(RooFitResult* result, string outputFile,
                vector<string> varNames, bool useShortNames = true) {
  YAML::Emitter outYml;

  vector<string> varNamesTmp = varNames;
  if (useShortNames) varNamesTmp = completeFitVarNames(varNames);

  outYml << YAML::BeginMap;
  for (const auto& n : varNamesTmp) {
    auto var =
        static_cast<RooRealVar*>(result->floatParsFinal().find(n.c_str()));

    if (var) {
      // fitted value
      outYml << YAML::Key << n + "_fitted";
      outYml << YAML::Value << var->getVal();
      // error
      outYml << YAML::Key << n + "_error";
      outYml << YAML::Value << var->getError();
    }
  }
  outYml << YAML::EndMap;

  ofstream outYmlFile(outputFile);
  outYmlFile << outYml.c_str();
}

#endif
