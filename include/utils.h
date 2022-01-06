// Author: Phoebe Hamilton, Yipeng Sun
// License: BSD 2-clause
// Last Change: Thu Jan 06, 2022 at 04:54 AM +0100

#ifndef _FIT_DEMO_UTILS_H_
#define _FIT_DEMO_UTILS_H_

#include <string>
#include <vector>

#include <RooStats/HistFactory/Sample.h>
#include <RooStats/HistFactory/Systematics.h>

#include "loader.h"

using std::string;
using std::vector;

void batchAddHistoSys(RooStats::HistFactory::Sample& sample,
                      vector<vector<string>> spec, Config addParams) {
  for (auto s : spec) {
    auto hSys = RooStats::HistFactory::HistoSys();
    hSys.SetName(s[0]);
    hSys.SetHistoLow(addParams.get<TH1*>(s[1]));
    hSys.SetHistoHigh(addParams.get<TH1*>(s[2]));

    sample.AddHistoSys(hSys);
  }
}

#endif
