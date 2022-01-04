// Author: Phoebe Hamilton, Yipeng Sun
// License: BSD 2-clause
// Last Change: Tue Jan 04, 2022 at 04:35 AM +0100

#ifndef _FIT_DEMO_CH_DATA_H_
#define _FIT_DEMO_CH_DATA_H_

#include <RooStats/HistFactory/Channel.h>
#include <RooStats/HistFactory/Measurement.h>
#include <RooStats/HistFactory/Sample.h>

#include "cmd.h"
#include "loader.h"

/////////////
// Samples //
/////////////
// Data and data-driven samples

void addData(const char* ntp, RooStats::HistFactory::Channel& chan,
             ArgProxy params, Config addParams) {
  chan.SetData("h_data", ntp);
}

void addMisId(const char* ntp, RooStats::HistFactory::Channel& chan,
              ArgProxy params, Config addParams) {
  RooStats::HistFactory::Sample sample("h_misID", "h_misID", ntp);

  sample.SetNormalizeByTheory(kTRUE);
  sample.AddNormFactor("NmisID", params.get<double>("relLumi"), 1e-6, 1e5);

  chan.AddSample(sample);
}

#endif
