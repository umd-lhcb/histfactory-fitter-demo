// Author: Phoebe Hamilton, Yipeng Sun
// License: BSD 2-clause
// Last Change: Thu Jan 06, 2022 at 04:40 PM +0100

#ifndef _FIT_DEMO_UTILS_H_
#define _FIT_DEMO_UTILS_H_

#include <map>
#include <string>
#include <vector>

#include <RooRealVar.h>
#include <RooStats/HistFactory/Sample.h>
#include <RooStats/HistFactory/Systematics.h>
#include <RooStats/ModelConfig.h>

#include "loader.h"

using RooStats::ModelConfig;
using RooStats::HistFactory::Sample;
using std::map;
using std::string;
using std::vector;

///////////////////////
// For fit templates //
///////////////////////

void batchAddHistoSys(Sample &sample, vector<vector<string>> spec,
                      Config addParams) {
  for (auto s : spec) {
    auto hSys = RooStats::HistFactory::HistoSys();
    hSys.SetName(s[0]);
    hSys.SetHistoLow(addParams.get<TH1 *>(s[1]));
    hSys.SetHistoHigh(addParams.get<TH1 *>(s[2]));

    sample.AddHistoSys(hSys);
  }
}

////////////////////////
// For fit parameters //
////////////////////////

typedef map<TString, double>         NuParamKeyVal;
typedef map<TString, vector<double>> NuParamKeyRange;

void setNuisanceParamConst(ModelConfig *mc, vector<TString> params,
                           bool verbose = false) {
  for (const auto &p : params) {
    auto nuParam =
        static_cast<RooRealVar *>(mc->GetNuisanceParameters()->find(p));
    nuParam->setConstant(kTRUE);

    if (verbose) cout << p << " = " << nuParam->getVal() << endl;
  }
}

void setNuisanceParamVal(ModelConfig *mc, NuParamKeyVal keyVal) {
  for (const auto &kv : keyVal) {
    auto nuParam =
        static_cast<RooRealVar *>(mc->GetNuisanceParameters()->find(kv.first));
    nuParam->setVal(kv.second);
    nuParam->setConstant(kTRUE);
  }
}

void setNuisanceParamRange(ModelConfig *mc, NuParamKeyRange keyRange) {
  for (const auto &kv : keyRange) {
    auto nuParam =
        static_cast<RooRealVar *>(mc->GetNuisanceParameters()->find(kv.first));
    nuParam->setRange(kv.second[0], kv.second[1]);
  }
}

#endif
